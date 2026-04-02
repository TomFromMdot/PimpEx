#pragma once

#include <cstdint>
#include <string>
#include <vector>
namespace PimpEx {
class IEngineActorAPI {
public:
  virtual ~IEngineActorAPI() = default;
  virtual uint64_t create_empty_actor(const std::string &actorName) = 0;
  virtual void remove_actor(const std::string &actorName) = 0;
  virtual uint64_t get_actor_id(const std::string &actorName) = 0;
};
} // namespace PimpEx
