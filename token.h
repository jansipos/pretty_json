#pragma once

#include <vector>
#include <string>

enum class kind : uint8_t {
  ROOT,
  LITERAL,
  KEY_VALUE_PAIR,
  KEY,
  NUMBER,
  STRING,
  OBJECT,
  ARRAY
};

class token {
  static constexpr std::string_view litteral_true  = "true";
  static constexpr std::string_view litteral_false = "false";
  static constexpr std::string_view litteral_null  = "null";
  static constexpr std::string_view object_start   = "{";
  static constexpr std::string_view object_end     = "}";
  static constexpr std::string_view array_start    = "[";
  static constexpr std::string_view array_end      = "]";
  static constexpr std::string_view double_quotes  = "\"";
  
  kind _kind;
  std::optional<std::string_view> _value;
  std::vector<std::shared_ptr<token>> _children;
  
  static std::pair<std::string_view, std::string_view> get_token_delims(std::shared_ptr<token> tkn);

public:
  token();
  token(kind kind, const std::optional<std::string_view> value);

  token(token&& other) = default;
  token(std::shared_ptr<token> other);
  token& operator=(token&& other) = default;

  token(const token&) = delete;
  token& operator=(const token&) = delete;

  const std::vector<std::shared_ptr<token>>& get_children() const;

  void set_children(std::vector<std::shared_ptr<token>>&& new_children);

  void add_child(std::shared_ptr<token> child);

  const std::optional<std::string_view>& get_value() const;

  static void print_all_children(std::shared_ptr<token> token);
};
