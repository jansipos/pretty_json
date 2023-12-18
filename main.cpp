#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <optional>
#include <vector>
#include <ctype.h>

#include "token.h"

enum parser_state {
  neutral,
  before_key,
  inside_key,
  after_key,
  before_value,
  inside_string,
  inside_number,
  inside_literal,
};

class context {
public:
  std::string_view& source;
  std::stringstream output{};

  std::shared_ptr<token> current_token;
  std::stack<std::shared_ptr<token>> token_stack;

  size_t current_position;
  size_t current_token_start;
  parser_state current_state = neutral;

  char previous_character;
  char current_character;
  bool inside_array = false;
  bool inside_escape = false;
  int indent_level = 0;

  context(std::string_view& source) : source{source} {}
  
  void push(std::shared_ptr<token> t) {
    current_token->add_child(t);
    token_stack.push(current_token);
    current_token = t;
  }
  
  void pop() {
    if (!token_stack.empty()) {
      current_token = token_stack.top();
      token_stack.pop();
    }
  }
};

void object_start(context &ctx) {
  auto t = std::make_shared<token>();
  ctx.push(t);
}

void object_end(context &ctx) {
  ctx.pop();
}

void array_start(context &ctx) {
  auto t = std::make_shared<token>(kind::ARRAY, std::nullopt);
  ctx.push(t);
}

void array_end(context &ctx) {
  ctx.pop();
}

void key_start(context &ctx) {
  auto t = std::make_shared<token>(kind::KEY_VALUE_PAIR, std::nullopt);
  ctx.push(t);

  ctx.current_token_start = ctx.current_position + 1;
}

void key_end(context &ctx) {
  ctx.current_token->add_child(
    std::make_shared<token>(
      kind::KEY,
      ctx.source.substr(ctx.current_token_start, ctx.current_position - ctx.current_token_start)
    )
  );
}

void string_start(context &ctx) {
  ctx.current_token_start = ctx.current_position + 1;
}

void string_end(context &ctx) {
  ctx.current_token->add_child(
    std::make_shared<token>(
      kind::STRING,
      ctx.source.substr(ctx.current_token_start, ctx.current_position - ctx.current_token_start)
    )
  );
  ctx.pop();
}

void number_start(context &ctx) {
  ctx.current_token_start = ctx.current_position;
}

void number_end(context &ctx) {
  ctx.current_token->add_child(
    std::make_shared<token>(
      kind::NUMBER,
      ctx.source.substr(ctx.current_token_start, ctx.current_position - ctx.current_token_start)
    )
  );
  ctx.pop();
}

void literal_start(context &ctx) {
  ctx.current_token_start = ctx.current_position;
}

void literal_end(context &ctx) {
  auto token_val = ctx.source.substr(ctx.current_token_start, ctx.current_position - ctx.current_token_start + 1);
  ctx.current_token->add_child(
    std::make_shared<token>(kind::LITERAL, token_val)
  );
  ctx.pop();
}

void colon(context &ctx) {
}

void comma(context &ctx) {
}

bool is_exponent_char(const char c) {
  return c == 'e' || c == 'E' || c == '+' || c == '-';
}

void handle_array(context &ctx) {
  if (ctx.current_character == '[') {
    ctx.inside_array = true;
    array_start(ctx);
  } else if (ctx.current_character == ']') {
    ctx.inside_array = false;
    array_end(ctx);
  }
}

void handle_object(context &ctx) {
  if (ctx.current_character == '{') {
    ctx.current_state = before_key;
    ++ctx.indent_level;
    object_start(ctx);
  } else if (ctx.current_character == '}') {
    ctx.current_state = neutral;
    --ctx.indent_level;
    object_end(ctx);
  }
}

void handle_number(context &ctx) {
  if (ctx.current_state != inside_number) {
    ctx.current_state = inside_number;
    number_start(ctx);
    return;
  }
}

void handle_string(context &ctx) {
  if (ctx.inside_escape) {
    ctx.inside_escape = false;
    return;
  }
  if (ctx.current_character == '"') {
    ctx.current_state == inside_string ? string_end(ctx) : key_end(ctx);
    ctx.current_state = ctx.current_state == inside_string ? neutral : after_key;
    return;
  }
  if (ctx.current_character == '\\') {
    ctx.inside_escape = true; 
  }
}

void handle_comma(context &ctx) {
  if (ctx.current_state == inside_number) {
    number_end(ctx);
  }
  ctx.current_state = ctx.inside_array ? before_value : before_key;
  comma(ctx);
}

void handle_quotes(context &ctx) {
  if (ctx.current_state == before_key) {
    ctx.current_state = inside_key;
    key_start(ctx);
    return;
  }
  ctx.current_state = inside_string;
  string_start(ctx);
}

void handle_dot(context &ctx) {
  ctx.current_state = inside_number;
}

void handle_primitives(context &ctx) {
  if (isdigit(ctx.current_character)) {
    handle_number(ctx);
    return;
  }
  if (ctx.current_state == inside_number) {
    if (is_exponent_char(ctx.current_character)) {
      return;
    }
    ctx.current_state = neutral;
    number_end(ctx);
    return;
  }
  if (isspace(ctx.current_character))
    return;
  switch(ctx.current_character) {
    case 't': // true
    case 'f': // false
    case 'n': // null
      ctx.current_state = inside_literal;
      literal_start(ctx);
      break;
    case 'e':  // end true, false
      ctx.current_state = neutral;
      literal_end(ctx);
      break;
    case 'l': // end null
      if (ctx.previous_character == 'l') {
        ctx.current_state = neutral;
        literal_end(ctx);
        break;
      }
    default: break;
  }
}

void handle_char_ctx(context &ctx) {
  if (ctx.current_state == inside_string || ctx.current_state == inside_key) {
    handle_string(ctx);
    return;
  }
  switch(ctx.current_character) {
    case '{':
    case '}':
      handle_object(ctx); break;
    case '[':
    case ']':
      handle_array(ctx); break;
    case '"':
      handle_quotes(ctx); break;
    case '-':
      handle_number(ctx); break;
    case ',':
      handle_comma(ctx); break;
    case '.':
      handle_dot(ctx); break;
    case ':':
      colon(ctx); break;
    default:
      handle_primitives(ctx);
  }
}

void read_char_file(const std::string &file_path) {
  std::ifstream input_stream(file_path, std::ios::in);
  input_stream.seekg(0, std::ios::end);
  size_t size = input_stream.tellg();
  std::string str = std::string(size, ' ');
  input_stream.seekg(0);
  input_stream.read(str.data(), size);

  auto source = std::string_view(str);
  auto ctx = context(source);
  auto root_token = std::make_shared<token>(kind::ROOT, std::nullopt);
  ctx.current_token = root_token;
  for (size_t i = 0; i < source.length(); ++i) {
    ctx.current_position = i;
    ctx.previous_character = ctx.current_character;
    ctx.current_character = source[i];
    handle_char_ctx(ctx);
  }
  token::print_all_children(root_token);
  return;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Provide a JSON file to parse" << std::endl;
    return 1;
  }
  std::string file_path(argv[1]);
  read_char_file(file_path);
}