#include "../include/Types.h"
#include "../include/Utils.h"
#include <memory>
#include <optional>
#include <print>
#include <string>

namespace PimpEx::Types {
const std::string
EngineResource::get_string_type(EngineResource::Type type) const {
  switch (type) {
  case Type::Audio:
    return std::string("audio");
  case Type::Model:
    return std::string("model");
  case Type::Texture:
    return std::string("texture");
  case Type::Error:
    return std::string("error");
  }
}
const EngineResource::Type
EngineResource::get_type_from_string(const std::string &type) const {
  // LATER REFACTOR WAST OF MY TIME !
  std::string resultString = Utils::string_to_lower(type);
  if (resultString == "audio")
    return Type::Audio;
  if (resultString == "texture")
    return Type::Texture;
  if (resultString == "model")
    return Type::Model;
  return Type::Error;
}

const std::optional<std::weak_ptr<EngineResource>>
EngineScene::get_resouce(const std::string &resourceId) const {
  std::string lowString = Utils::string_to_lower(resourceId);
  auto it = this->resources.find("resourceId");
  if (!it->second) {
    std::println("Can't find resource with id: {}", resourceId);
    return std::nullopt;
  }
  return it->second;
}
} // namespace PimpEx::Types
