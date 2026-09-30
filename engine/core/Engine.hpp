#pragma once

#include <filesystem>
#include <string>

namespace PimpEx {

struct EngineOptions {
  bool watch = true;     // hot-reload: wątek zasobów obserwuje pliki
  bool headless = false; // bez okna (testy, CI, agenci)
  long frames = 0;       // 0 = do zamknięcia okna
  int stateEvery = 30;   // co ile klatek zapisywać .pimpex/state.txt
  bool quiet = false;    // bez wypisywania dziennika na konsolę
  std::string input;     // --headless: zaplanowane klawisze, np. "RIGHT:0-60 SPACE:10"
  std::string screenshot; // zrzut ekranu ostatniej klatki (wymaga okna i --frames)
};

// Pętla gry:
//   wątek zasobów: FileWatcher -> parsowanie -> Mailbox
//   wątek główny:  Mailbox -> Boot (podmiana danych) -> VM -> ruch -> rysowanie
class Engine {
  std::filesystem::path _root;
  EngineOptions _options;

public:
  Engine(std::filesystem::path projectRoot, EngineOptions options)
      : _root(std::move(projectRoot)), _options(options) {}

  // Zwraca kod wyjścia programu.
  int run();
};

} // namespace PimpEx
