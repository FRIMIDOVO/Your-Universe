#pragma once
#include "physics.h"

namespace GalaxySaver {
    void save(const std::string& dir, const std::vector<Particle>& particles, const float world_time);
    std::pair<float, std::vector<Particle>> load(const std::string& filename);
}