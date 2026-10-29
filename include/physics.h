#pragma once
#include <vector>
#include "particle.h"
#include "config.h"
#include "spatial_grid.h"
#include "opencl_manager.h"
#include "nbody_solver.h"
#include "barnes_hut.h"


class PhysicsEngine {
public:
    // Конструктор
    PhysicsEngine(
        const PhysicsConfig& physics, 
        const SpaceConfig& space,
        OpenCLManager& clManager
    );

    // Управление частицами
    void add_particle(const Particle& p) { particles.push_back(p); }
    void clear_particles() { particles.clear(); }
    
    // Симуляция
    void step();

    // Геттеры
    const std::vector<Particle>& get_particles() const { return particles; }
    const SpaceConfig& get_space() const { return space; }
    const SpatialGrid& get_spatial_grid() { return spatial_grid; }

    // Инфо
    const float get_time() const { return time; }
    void set_time(float t) { time = t; }

    // Для рендера
    const std::vector<float>& get_posX() const { return posX; }
    const std::vector<float>& get_posY() const { return posY; }
    const std::vector<float>& get_posZ() const { return posZ; }
    const std::vector<float>& get_radius() const { return radius; }
    const std::vector<uint8_t>& get_dark() const { return is_dark; }

    void update_dod();

private:
    PhysicsConfig physics;
    SpaceConfig space;
    
    SpatialGrid spatial_grid;
    NBodySolver nbody_solver;
    BarnesHutTree bh_tree;

    std::vector<Particle> particles;
    // Для будущего dod
    std::vector<float> posX, posY, posZ;
    std::vector<float> velX, velY, velZ;
    std::vector<float> accX, accY, accZ;
    std::vector<float> mass;
    std::vector<float> radius;
    std::vector<uint8_t> is_dark;

    float time = 0.0f;
};