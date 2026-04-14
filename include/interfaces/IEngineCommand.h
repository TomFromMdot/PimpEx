#pragma once

#include <any>
#include <memory>
namespace PimpEx::interfaces {

class IEngineCommand {
protected:
public:
  virtual ~IEngineCommand() = default;
  virtual void setup(std::any hook) = 0;
  virtual void execute() = 0;
};
}; // namespace PimpEx::interfaces
