#include <iostream>
#include <vector>
#include <stack>
#include <optional>

#include "token.h"

token::token() : _kind(kind::OBJECT), _value(std::nullopt) {}
token::token(std::shared_ptr<token> other) : _kind(other->_kind), _value(other->_value) {}
token::token(kind kind, const std::optional<std::string_view> value) : _kind(kind), _value(value) {}

const std::vector<std::shared_ptr<token>>& token::get_children() const {
  return _children;
}

void token::set_children(std::vector<std::shared_ptr<token>>&& new_children) {
  _children = std::move(new_children);
}

void token::add_child(std::shared_ptr<token> child) {
  _children.push_back(child);
}

const std::optional<std::string_view>& token::get_value() const {
  return _value;
}

std::pair<std::string_view, std::string_view> token::get_token_delims(std::shared_ptr<token> tkn) {
  switch (tkn->_kind) {
    case kind::KEY: 
    case kind::STRING:
      return {token::double_quotes, token::double_quotes};
    case kind::OBJECT:
      return {token::object_start, token::object_end};
    case kind::ARRAY:
      return {token::array_start, token::array_end};
    default:
      return {"", ""};
  }
}

struct stack_element {
    std::shared_ptr<token> current_token;
    size_t child_index;
    bool has_more_siblings;

    stack_element(std::shared_ptr<token> token) : current_token(token), child_index(0), has_more_siblings(false) {}
};

void token::print_all_children(std::shared_ptr<token> tkn) {
  std::stack<stack_element> token_stack;
  token_stack.push(stack_element(tkn));
  
  size_t level = 0;
  auto in_array = false;
  while (!token_stack.empty()) {
    stack_element& top_element = token_stack.top();
    auto curr_token = top_element.current_token;
    auto curr_kind = curr_token->_kind;
    auto curr_children = curr_token->get_children();

    auto token_value = curr_token->get_value();
    auto [token_start, token_end] = get_token_delims(curr_token);

    if (curr_kind == kind::ARRAY) in_array = true;

    // starting the parent
    if (top_element.child_index == 0) {
      if (curr_kind == kind::KEY) {
        std::cout << std::string(level*2, ' ');
      }

      std::cout << token_start;
      
      if (curr_kind == kind::OBJECT) {
        ++level;
        std::cout << std::endl;
      }
      if (token_value.has_value()) {
        std::cout << token_value.value();
      }
    }
    // one of the children
    if (top_element.child_index < curr_children.size()) {
      auto child_token = curr_children[top_element.child_index];
      bool is_last_child = (top_element.child_index == curr_children.size() - 1);
      ++top_element.child_index;
      token_stack.push(stack_element(child_token));
      token_stack.top().has_more_siblings = !is_last_child;
    }
    // no more children
    else {
      if (curr_kind == kind::OBJECT) {
        --level;
        std::cout << std::endl << std::string(level*2, ' ');
      }
      if (!in_array)
        std::cout << token_end;
      
      if (curr_kind == kind::KEY) {
        std::cout << ": ";
      }
      else if (curr_kind == kind::KEY_VALUE_PAIR || curr_kind == kind::OBJECT || in_array) {
        if (!token_stack.empty() && top_element.has_more_siblings) {
          std::cout << ",";
          if (!in_array) std::cout << std::endl;
        }
        else {
          if (in_array) {
            std::cout << token_end;
            in_array = false;
          }
        }
      }
      token_stack.pop();
    }
  }
}
