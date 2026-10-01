#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace PimpEx::vm {

struct Token {
  enum class Kind {
    Word,   // MOVE, Player, Player.x, W ...
    Number, // 10, -2.5
    String, // "quoted text"
    Colon,  // :
    Minus,  // before a field: -Ball.vx
    Compare // < <= > >= == !=
  };

  Kind kind = Kind::Word;
  std::string text;
  double number = 0.0;
};

struct LineTokens {
  std::vector<Token> tokens;
  std::string error; // empty when the line is valid
};

// Splits one .swap line into tokens. Comments start with `#`.
LineTokens tokenize_line(std::string_view line);

} // namespace PimpEx::vm
