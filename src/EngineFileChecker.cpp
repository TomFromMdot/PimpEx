#include "../include/EngineFileChecker.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <print>
#include <string>
#include <tinyxml2.h>

const std::string PimpEx::IEngineFileChecker::get_file_extension(
    const std::filesystem::path &path) const {
  if (!std::filesystem::exists(path) || std::filesystem::is_directory(path)) {
    std::print("Can't check extension:  {}", path.filename().c_str());
    return std::string();
  }

  const std::string fileName = path.filename();
  auto it = fileName.find_last_of(".");

  if (it == std::string::npos) {
    std::print("Can't find file extension in: {}", path.c_str());
    return std::string();
  }

  return std::string(fileName.begin() + it + 1, fileName.end());
}

bool PimpEx::EngineFileChecker::file_is_xml(
    const std::filesystem::path &path) const {
  // MUSIMY SPRAWDZIĆ CZY W PLIKU JEST TO
  //  <?xml version="1.0" encoding="UTF-8"?>
  using namespace tinyxml2;

  if (!std::filesystem::exists(path) || std::filesystem::is_directory(path))
    return false;

  std::string fileExtension = get_file_extension(path);
  std::transform(fileExtension.begin(), fileExtension.end(),
                 fileExtension.begin(), ::tolower);

  if (fileExtension != "xml") {
    std::print("File name not include extension name: {}", path.c_str());
    return false;
  }
  XMLDocument doc;
  auto result = doc.LoadFile(path.c_str());
  if (doc.Error() != XML_SUCCESS) {
    std::print("XML must contain a header: {}", path.c_str());
    return false;
  }
  return true;
}

bool PimpEx::EngineFileChecker::file_is_scene_actor_xml(
    const std::filesystem::path &path) const {
  using namespace tinyxml2;
  if (!file_is_xml(path))
    return false;
  /*
   *<Scene name="main_scene">
   *  <Actors>
   *    <Actor name="Player">
   *      <Transform x="0" y="0" z="0"/>
   *      <Image href="player_sprite_sheet">
   *        <ImageSlice name="player_slice" startX="0" startY="0" cellSizeX="32"
   * cellSizeY="32"/>
   *      </Image>
   *    </Actor>
   *  </Actors>
   *</Scene>
   *
   * */

  XMLDocument doc;
  auto resut = doc.LoadFile(path.c_str());

  XMLElement *actorElement = doc.FirstChildElement("Actor");
  if (!actorElement) {
    std::println("File not include any actor: {}", path.c_str());
    return false;
  }
  return true;
}
