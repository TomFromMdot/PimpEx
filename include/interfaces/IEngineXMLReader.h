#pragma once

#include <complex>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <valarray>
#include <vector>

#include "../Types.h"

namespace PimpEx {
class IEngineXMLReader {
public:
  virtual ~IEngineXMLReader() = default;

  virtual std::optional<
      const std::map<std::string, std::shared_ptr<Types::EngineResource>>>
  get_scene_resources(const std::string &sceneName) = 0;

  virtual std::optional<
      const std::map<std::string, std::shared_ptr<Types::EngineActor>>>
  get_scene_actors(const std::string &sceneName) = 0;

  virtual std::optional<std::vector<Types::EngineActorComponent>>
  get_scene_actor_components(const std::string &sceneName,
                             const std::string &actorName) = 0;
};

} // namespace PimpEx
