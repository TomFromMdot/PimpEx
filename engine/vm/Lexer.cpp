#include "vm/Lexer.hpp"

#include <cctype>
#include <charconv>

namespace PimpEx::vm {
namespace {

bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Bytes >= 0x80 are UTF-8, so names like "Żaba" are valid words.
bool is_word_char(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalnum(u) || c == '_' || c == '.' || c == '/' || c == '@' || u >= 0x80;
}

} // namespace

LineTokens tokenize_line(std::string_view line) {
  LineTokens result;
  std::size_t i = 0;

  while (i < line.size()) {
    const char c = line[i];

    if (c == ' ' || c == '\t' || c == '\r') {
      ++i;
      continue;
    }
    if (c == '#')
      break;

    if (c == ':') {
      result.tokens.push_back({Token::Kind::Colon, ":"});
      ++i;
      continue;
    }

    if (c == '"') {
      const auto end = line.find('"', i + 1);
      if (end == std::string_view::npos) {
        result.error = "missing closing quote \"";
        return result;
      }
      result.tokens.push_back(
          {Token::Kind::String, std::string(line.substr(i + 1, end - i - 1))});
      i = end + 1;
      continue;
    }

    if (c == '<' || c == '>' || c == '=' || c == '!') {
      std::string op(1, c);
      if (i + 1 < line.size() && line[i + 1] == '=') {
        op += '=';
        ++i;
      }
      ++i;
      if (op == "=" || op == "!") {
        result.error = "unknown operator '" + op + "' (use ==, !=, <, <=, >, >=)";
        return result;
      }
      result.tokens.push_back({Token::Kind::Compare, op});
      continue;
    }

    const bool negativeNumber = c == '-' && i + 1 < line.size() &&
                                (is_digit(line[i + 1]) || line[i + 1] == '.');
    if (is_digit(c) || negativeNumber ||
        (c == '.' && i + 1 < line.size() && is_digit(line[i + 1]))) {
      std::size_t end = i + 1;
      while (end < line.size() && (is_digit(line[end]) || line[end] == '.'))
        ++end;
      const auto text = line.substr(i, end - i);
      double value = 0.0;
      const auto [ptr, ec] =
          std::from_chars(text.data(), text.data() + text.size(), value);
      if (ec != std::errc{} || ptr != text.data() + text.size()) {
        result.error = "invalid number '" + std::string(text) + "'";
        return result;
      }
      result.tokens.push_back({Token::Kind::Number, std::string(text), value});
      i = end;
      continue;
    }

    if (c == '-') {
      result.tokens.push_back({Token::Kind::Minus, "-"});
      ++i;
      continue;
    }

    if (is_word_char(c)) {
      std::size_t end = i;
      while (end < line.size() && is_word_char(line[end]))
        ++end;
      result.tokens.push_back(
          {Token::Kind::Word, std::string(line.substr(i, end - i))});
      i = end;
      continue;
    }

    result.error = std::string("unexpected character '") + c + "'";
    return result;
  }
  return result;
}

} // namespace PimpEx::vm
