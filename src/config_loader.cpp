#include "config_loader.h"

// === PhysicsConfig ===
void ConfigLoader::set_field(PhysicsConfig& config, const std::string& key, float value) {
    #define FIELD(name) if (key == #name) { config.name = value; return; }
    PHYSICS_FIELDS
    #undef FIELD
}

void ConfigLoader::save_fields(std::ofstream& file, const PhysicsConfig& config) {
    #define FIELD(name) file << #name << " " << config.name << "\n";
    PHYSICS_FIELDS
    #undef FIELD
}

// === SpaceConfig ===
void ConfigLoader::set_field(SpaceConfig& config, const std::string& key, float value) {
    #define FIELD(name) if (key == #name) { config.name = value; return; }
    SPACE_FIELDS
    #undef FIELD
}

void ConfigLoader::save_fields(std::ofstream& file, const SpaceConfig& config) {
    #define FIELD(name) file << #name << " " << config.name << "\n";
    SPACE_FIELDS
    #undef FIELD
}

// === VisualizerConfig ===
void ConfigLoader::set_field(VisualizerConfig& config, const std::string& key, float value) {
    #define FIELD(name) if (key == #name) { config.name = value; return; }
    VISUALIZER_FIELDS
    #undef FIELD
}

void ConfigLoader::save_fields(std::ofstream& file, const VisualizerConfig& config) {
    #define FIELD(name) file << #name << " " << config.name << "\n";
    VISUALIZER_FIELDS
    #undef FIELD
}