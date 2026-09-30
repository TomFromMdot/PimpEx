#include "cli/commands/RunCommand.hpp"

#include <iostream>
#include <print>

namespace PimpEx::cli {

CLI::App *RunCommand::setup(CLI::App &app) {
  auto *sub = _dev ? app.add_subcommand("dev", "Uruchamia grę z hot-reloadem: zapisane zmiany "
                                               "w plikach pojawiają się od razu.")
                   : app.add_subcommand("play", "Uruchamia grę (bez obserwowania plików).");
  sub->add_option("project", _project, "Katalog projektu")->capture_default_str();
  sub->add_option("--frames", _options.frames, "Zakończ po tylu klatkach (0 = bez limitu)");
  sub->add_flag("--headless", _options.headless,
                "Bez okna – tylko logika (testy, CI, agenci AI); wymaga --frames");
  sub->add_option("--state-every", _options.stateEvery,
                  "Co ile klatek zapisywać .pimpex/state.txt (0 = tylko na końcu)")
      ->capture_default_str();
  sub->add_option("--input", _options.input,
                  "Z --headless: zaplanowane klawisze, np. \"RIGHT:0-60 SPACE:10\"");
  sub->add_option("--screenshot", _options.screenshot,
                  "Zapisz zrzut ekranu ostatniej klatki do pliku .png (wymaga --frames)");
  sub->add_flag("--quiet", _options.quiet, "Dziennik tylko do .pimpex/runtime.log");
  return sub;
}

int RunCommand::run() {
  if (_options.headless && _options.frames <= 0) {
    std::println(std::cerr, "--headless wymaga --frames N (inaczej gra nigdy się nie skończy).");
    return 1;
  }
  if (!_options.input.empty() && !_options.headless) {
    std::println(std::cerr, "--input działa tylko razem z --headless.");
    return 1;
  }
  if (!_options.screenshot.empty() && (_options.headless || _options.frames <= 0)) {
    std::println(std::cerr, "--screenshot wymaga okna (bez --headless) i --frames N.");
    return 1;
  }
  Engine engine(_project, _options);
  return engine.run();
}

} // namespace PimpEx::cli
