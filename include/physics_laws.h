#pragma once
#include <vector>
#include "config.h"
#include "nbody_solver.h"
#include "spatial_grid.h"
#include "barnes_hut.h"


namespace Laws {
    // Интегратор
    void inertia(
        const std::vector<float>& accX, const std::vector<float>& accY, const std::vector<float>& accZ,
        std::vector<float>& posX, std::vector<float>& posY, std::vector<float>& posZ,
        std::vector<float>& velX, std::vector<float>& velY, std::vector<float>& velZ,
        const PhysicsConfig& phys_config
    );

    // Гравитация на видюхе
    void GPU_gravity(
        std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& mass,
        const PhysicsConfig& phys_config, NBodySolver& nbody_solver
    );
    
    // Гравитацая аппроксимацией через октодерево (Barnes-Hut)
    void BH_gravity(
        std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& mass,
        const PhysicsConfig& phys_config, BarnesHutTree& bh_tree
    );

    // Механические столкновения
    void collisions(
        std::vector<float>& posX, std::vector<float>& posY, std::vector<float>& posZ,
        std::vector<float>& velX, std::vector<float>& velY, std::vector<float>& velZ,
        const std::vector<float>& mass, const std::vector<float>& radius, const std::vector<uint8_t>& is_dark,
        const PhysicsConfig& physics, const SpatialGrid& spatial_grid
    );

    // Гидродинамические столкновения
    void SPH(
        std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
        const std::vector<float>& mass, const std::vector<uint8_t>& is_dark,
        const PhysicsConfig& physics, const BarnesHutTree& bh_tree
    );
};