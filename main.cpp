#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctype.h>

const char* red = "\033[31m";
const char* green = "\033[32m";
const char* yellow = "\033[33m";
const char* blue = "\033[34m";
const char* magenta = "\033[35m";
const char* cyan = "\033[36m";
const char* reset = "\033[0m";

enum State {
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
  State current_state = neutral;
  char previous_character;
  char current_character;
  bool inside_array = false;
  int indent_level = 0;
  std::stringstream output{};
};

void object_start(context &ctx) {
  ctx.output << red << "{" << reset;
}

void object_end(context &ctx) {
  ctx.output << "\n" << std::string(ctx.indent_level*2, ' ');
  ctx.output << red << "}" << reset;
}

void array_start(context &ctx) {
  ctx.output << red << "[" << reset;
}

void array_end(context &ctx) {
  ctx.output << red << "]" << reset;
}

void key_start(context &ctx) {
  ctx.output << "\n" << std::string(ctx.indent_level*2, ' ');
  ctx.output << cyan << "\"";
}

void key_end(context &ctx) {
  ctx.output << "\"%s" << reset;
}

void string_start(context &ctx) {
  ctx.output << yellow << "\"";
}

void string_end(context &ctx) {
  ctx.output << "\"%s" << reset;
}

void number_start(context &ctx) {
  ctx.output << magenta << ctx.current_character;
}

void number_end(context &ctx) {
  ctx.output << reset;
}

void literal_start(context &ctx) {
  ctx.output << green << ctx.current_character;
}

void literal_end(context &ctx) {
  ctx.output << ctx.current_character << reset;
}

void colon(context &ctx) {
  ctx.output << ": ";
}

void comma(context &ctx) {
  ctx.output << ctx.current_character;
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
  ctx.output << ctx.current_character;
}

void handle_string(context &ctx) {
  if (ctx.current_character == '"') {
    ctx.current_state = ctx.current_state == inside_string? neutral : after_key;
    ctx.current_state == inside_string ? string_end(ctx) : key_end(ctx);
    return;
  }
  ctx.output << ctx.current_character;
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
  ctx.output << ctx.current_character;
}

void handle_primitives(context &ctx) {
  if (isspace(ctx.current_character))
    return;
  if (isdigit(ctx.current_character)) {
    handle_number(ctx);
    return;
  }
  if (ctx.current_state == inside_number && !is_exponent_char(ctx.current_character)) {
    ctx.current_state = neutral;
    number_end(ctx);
    return;
  }
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
    default:
      ctx.output << ctx.current_character;
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
  std::ifstream in(file_path);
  if(!in.is_open()) {
    exit(1);
  }
  auto ctx = context{};
  char c;
  while(in.get(c) && in.good()) {
    ctx.previous_character = ctx.current_character;
    ctx.current_character = c;
    handle_char_ctx(ctx);
  }
  if(!in.eof() && in.fail()) {
    ctx.output << "error reading " << file_path << std::endl;
  }
  ctx.output << reset << "\n";
  // std::cout << ctx.output.str();
}

int main() {
  std::string file_path("./test-data/movies.json");
  read_char_file(file_path);
}