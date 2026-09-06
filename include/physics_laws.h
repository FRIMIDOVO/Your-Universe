#pragma once

#include <vector>
#include "particle.h"
#include "hex_grid.h"
#include "config.h"
#include "gravity_field.h"


namespace Laws {
    void inertia(std::vector<Particle>& particles, const PhysicsConfig& physics);
    void gravity(std::vector<Particle>& particles, GravityField& gravity_field, const PhysicsConfig& physics);
    void bounce(std::vector<Particle>& particles, const SpaceConfig& space);
    void periodic(std::vector<Particle>& particles, const SpaceConfig& space);
    void elastic(std::vector<Particle>& particles, const PhysicsConfig& physics, const HexGrid& hex_grid);
    void friction(std::vector<Particle>& particles, const PhysicsConfig& physics, const HexGrid& hex_grid);
    void stickiness(std::vector<Particle>& particles, const PhysicsConfig& physics, const HexGrid& hex_grid);
};