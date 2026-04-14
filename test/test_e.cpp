#include "../include/EngineXML.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <unistd.h>

constexpr std::string sceneName = "Cave";
constexpr std::string actorName = "Player";

inline void TEST_XML_READER(PimpEx::IEngineXMLReader *reader);
inline void TEST_XML_READER_GUARD(PimpEx::IEngineXMLReader *reader);
inline void
TEST_XML_READER_GET_SCENE_RESOURCE(PimpEx::IEngineXMLReader *reader);
inline void TEST_XML_GET_SCENE_ACTORS(PimpEx::IEngineXMLReader *reader);
inline void TEST_XML_SCENE_ACTOR(PimpEx::Types::EngineActor *actor);
inline void
TEST_XML_GET_SCENE_ACTOR_COMPONENTS(PimpEx::Types::EngineActor *actor);
inline void
TEST_XML_ACTOR_COMPONENTS(const PimpEx::Types::EngineActorComponent &component);
inline void TEST_XML_COMPONENT_PROPERTY(
    const PimpEx::Types::EngineComponentProperty &property);
inline void TEST_XML_COMPONENT_PROCEDURES(const std::string &procedure);

int main(int argc, char *argv[]) {
  std::cout << "start test\n";
  std::unique_ptr<PimpEx::IEngineXMLReader> reader =
      std::make_unique<PimpEx::Utils::EngineXMLReader>("./test.xml");
  TEST_XML_READER(reader.get());
  return 0;
}

void TEST_XML_READER(PimpEx::IEngineXMLReader *reader) {
  TEST_XML_READER_GUARD(reader);
  TEST_XML_READER_GET_SCENE_RESOURCE(reader);
  TEST_XML_GET_SCENE_ACTORS(reader);
}
void TEST_XML_READER_GUARD(PimpEx::IEngineXMLReader *reader) {
  assert(!reader && "Reader is nullptr, test abort.");
}
void TEST_XML_READER_GET_SCENE_RESOURCE(PimpEx::IEngineXMLReader *reader) {
  TEST_XML_READER_GUARD(reader);
  auto resourceMap = reader->get_scene_resources(sceneName);
  assert(resourceMap.has_value() &&
         "\nSomething wrong with get_scene_resources, return nullopt.\n");
  assert((*resourceMap).size() &&
         "\nget_scene_resource return zero elements test have to return more "
         "then 0.\n");
  std::string resType;
  for (const auto &[rName, rPtr] : *resourceMap) {
    resType = rPtr->get_string_type(rPtr->type);
    std::println("Resource name: {}\nPath: {}\nType: {}", rPtr->name,
                 rPtr->path.c_str(), resType);
  }
}
void TEST_XML_GET_SCENE_ACTORS(PimpEx::IEngineXMLReader *reader) {
  TEST_XML_READER_GUARD(reader);
  auto actorsMap = reader->get_scene_actors(sceneName);
  assert(actorsMap.has_value() &&
         "\nSomething wrong with get_scene_actors, return nullopt.\n");
  assert(actorsMap->size() &&
         "\n Actors map is empty no elements. Test looking for sceneName "
         "actors and return 0 elements\n");

  for (const auto &[aName, aPtr] : *actorsMap) {
    TEST_XML_SCENE_ACTOR(aPtr.get());
  }
}

void TEST_XML_GET_SCENE_ACTOR_COMPONENTS(PimpEx::Types::EngineActor *actor) {}

void TEST_XML_SCENE_ACTOR(PimpEx::Types::EngineActor *actor) {
  assert(actor && "\nActor is nullptr !!!!\n");
  assert(actor->id.empty() &&
         "\nActor id can't be empty or something wrong. Test abort.\n");
  assert(actor->name.empty() && "\nActor name can't be empty. Test abort.\n");
  assert(actor->components.empty() &&
         "Actor shoud have one or more components for test. Test abort\n");
  for (const auto &component : actor->components) {
    TEST_XML_ACTOR_COMPONENTS(component);
  }
}

void TEST_XML_ACTOR_COMPONENTS(
    const PimpEx::Types::EngineActorComponent &component) {
  assert(
      !component.name.empty() &&
      "\nComponent name pool is empty. Test component shoud have name attr.\n");
  assert(!component.property.empty() &&
         "\nComponent property is empty. Components allways shoud have more "
         "then one property.\n");
  assert(!component.onStartProcedures.empty() &&
         "\nComponent procedures list is empty. Test component shoud have one "
         "or more procedures.\n");
  std::println("Component name: {}", component.name);
  std::println("====PROPERTY====");
  for (const auto &property : component.property) {
    TEST_XML_COMPONENT_PROPERTY(property);
  }
  std::println("====PROPERTY====");
  for (const auto &onStart : component.onStartProcedures) {
    TEST_XML_COMPONENT_PROCEDURES(onStart);
  }
}

void TEST_XML_COMPONENT_PROPERTY(
    const PimpEx::Types::EngineComponentProperty &property) {
  assert(property.name.empty() && "\nProperty name can't be empty. Abort.\n");
  assert(property.attrs.empty() &&
         "\nProperty can't be without any attributes\n");
  for (const auto &attr : property.attrs) {
    std::println("Attr name: {} value: {}", attr.first, attr.second);
  }
}
void TEST_XML_COMPONENT_PROCEDURES(const std::string &procedure) {
  std::println("Procedure name: {}", procedure);
}
