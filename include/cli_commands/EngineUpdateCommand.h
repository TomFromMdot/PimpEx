#pragma once
#include "../interfaces/IEngineCommand.h"
namespace PimpEx::cli::commands {
class EngineUpdateCommand : public interfaces::IEngineCommand {
public:
  void setup(std::any hook) override;
  void execute() override;
};
} // namespace PimpEx::cli::commands
