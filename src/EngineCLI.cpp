#include "../include/EngineCLI.h"

void PimpEx::cli::EngineCommandBuffer::register_command(
    Command command, std::shared_ptr<interfaces::IEngineCommand> ptr) {
  _commands.insert({command, std::move(ptr)});
  std::cout << "commands: " << _commands.size() << "\n";
}

void PimpEx::cli::EngineCommandBuffer::init_commands() {
  for (const auto &[cName, cPtr] : _commands) {
    std::cout << "CLI INIT COMMANDS\n";
    cPtr->setup(_app);
  }
}
