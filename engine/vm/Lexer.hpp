#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace PimpEx::vm {

struct Token {
  enum class Kind {
    Word,   // MOVE, Player, Player.x, W ...
    Number, // 10, -2.5
    String, // "tekst w cudzysłowie"
    Colon,  // :
    Minus,  // - przed nazwą pola: -Ball.vx
    Compare // < <= > >= == !=
  };

  Kind kind = Kind::Word;
  std::string text;
  double number = 0.0;
};

struct LineTokens {
  std::vector<Token> tokens;
  std::string error; // pusty, jeśli linia jest poprawna
};

// Dzieli jedną linię pliku .swap na tokeny. Komentarz zaczyna się od `#`.
LineTokens tokenize_line(std::string_view line);

} // namespace PimpEx::vm
