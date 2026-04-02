#include "../include/EngineXML.h"

bool PimpEx::Utils::EngineXMLWriter::save_actor_element(
    const std::filesystem::path &path, void *actor) {}

bool PimpEx::Utils::EngineXMLWriter::save_scene_element(
    const std::filesystem::path &path, void *scene) {}

bool PimpEx::Utils::EngineXMLWriter::save_resource_element(
    const std::filesystem::path &path, void *resource) {}

bool PimpEx::Utils::EngineXMLWriter::save_engine_config_element(
    const std::filesystem::path &path, void *config) {}

bool PimpEx::Utils::EngineXMLWriter::save_engine_config_value_element(
    const std::filesystem::path &path, const std::string &configName,
    void *configValue) {}

bool PimpEx::Utils::EngineXMLWriter::make_actors_bundle(
    const std::filesystem::path &path) {}

bool PimpEx::Utils::EngineXMLWriter::make_scenes_bundle(
    const std::filesystem::path &path) {}

bool PimpEx::Utils::EngineXMLWriter::make_resources_bundle(
    const std::filesystem::path &path) {}

bool PimpEx::Utils::EngineXMLWriter::save_actor_component(
    const std::filesystem::path &path, const std::string &actorName,
    const void *compBuffer) {}

bool PimpEx::Utils::EngineXMLWriter::save_component_procedures(
    const std::filesystem::path &path, const std::string &actorName,
    const void *proceduresBuffer) {}
