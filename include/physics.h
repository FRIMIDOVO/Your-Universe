#pragma once

#include <vector>
#include "particle.h"
#include "config.h"
#include "hex_grid.h"
#include "gravity_field.h"


class PhysicsEngine {
public:
    // Конструктор с конфигами
    PhysicsEngine(
        const PhysicsConfig& physics, 
        const SpaceConfig& space
    ) : physics(physics),
        space(space),
        time(0.0f),
        hex_grid(1.25f, space),
        gravity_field(space, physics, 1.25f) {}

    // Управление частицами
    void add_particle(const Particle& p) { particles.push_back(p); }
    void clear_particles() { particles.clear(); }
    
    // Симуляция
    void step();

    // Геттеры
    const std::vector<Particle>& get_particles() const { return particles; }
    const SpaceConfig& get_space() const { return space; }
    const HexGrid& get_hex_grid() const { return hex_grid; }
    const GravityField& get_gravity_field() const { return gravity_field; }

private:
    PhysicsConfig physics;
    SpaceConfig space;
    std::vector<Particle> particles;
    HexGrid hex_grid;
    GravityField gravity_field;
    float time = 0.0f;
};