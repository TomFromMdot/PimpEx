#pragma once
#include "CLI/CLI.hpp"
#include "interfaces/IEngineCommand.h"
#include <CLI/CLI.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <utility>
namespace PimpEx::cli {

class EngineCommandBuffer {
public:
  enum class Command : uint8_t { Init, Update, Build, Dev };

private:
  std::map<Command, std::shared_ptr<interfaces::IEngineCommand>> _commands;
  std::shared_ptr<CLI::App> _app;

public:
  EngineCommandBuffer(std::shared_ptr<CLI::App> app) : _app(std::move(app)) {}
  void register_command(Command command,
                        std::shared_ptr<interfaces::IEngineCommand> ptr);
  void init_commands();
};

} // namespace PimpEx::cli
