#include "physics.h"
#include "physics_laws.h"
#include <iostream>

PhysicsEngine::PhysicsEngine(
        PhysicsConfig& physics, 
        SpaceConfig& space
)
   : spatial_grid(0.4f),    // there should be 2 radii of medium size
     opencl_manager(),
     nbody_solver(opencl_manager),
     sph_solver(opencl_manager),
     bh_tree(),
     physics(physics),
     space(space)
{
}


void PhysicsEngine::step() {
    // First kick and drift
    float dt = physics.dt;
    float half_dt = dt * 0.5f;
    for (size_t i = 0; i < accX.size(); ++i) {
        // Применяем ускорение
        velX[i] += accX[i] * half_dt;
        velY[i] += accY[i] * half_dt;
        velZ[i] += accZ[i] * half_dt;

        // Применяем скорость
        posX[i] += velX[i] * dt;
        posY[i] += velY[i] * dt;
        posZ[i] += velZ[i] * dt;
    }

    std::fill(accX.begin(), accX.end(), 0.0f);
    std::fill(accY.begin(), accY.end(), 0.0f);
    std::fill(accZ.begin(), accZ.end(), 0.0f);

    //std::cerr << "Computing gravity... ";
    // Гравитация: Barnes-hut / clear GPU
    if (physics.theta) {
	    //std::cerr << "Building Octree... ";
    	// Строим октодерево для гравитации
    	bh_tree.build(posX, posY, posZ, mass, physics);
	    Laws::BH_gravity(accX, accY, accZ, posX, posY, posZ, mass, physics, bh_tree);
    } else Laws::GPU_gravity(accX, accY, accZ, posX, posY, posZ, mass, physics, nbody_solver);

    // Second kick
    for (size_t i = 0; i < accX.size(); ++i) {
        // Применяем ускорение
        velX[i] += accX[i] * half_dt;
        velY[i] += accY[i] * half_dt;
        velZ[i] += accZ[i] * half_dt;
    }

    //std::cerr << "Computing collisions... ";
    // Коллизии: elastic / SPH
    if (physics.K_elasticity) {
        //std::cerr << "Building SpatialGrid... ";
        spatial_grid.build(posX, posY, posZ, is_dark);
        Laws::collisions(posX, posY, posZ, velX, velY, velZ, mass, radius, is_dark, physics, spatial_grid);
    } else {
        Laws::SPH(accX, accY, accZ, posX, posY, posZ, velX, velY, velZ, mass, is_dark, physics, sph_solver);
    }

    time += physics.dt;
}

void PhysicsEngine::add_particle(
    float m,
    float r,
    float px, float py, float pz,
    float vx, float vy, float vz,
    bool dm
)
{
    accX.push_back(0.0f); accY.push_back(0.0f); accZ.push_back(0.0f);
    posX.push_back(px); posY.push_back(py); posZ.push_back(pz);
    velX.push_back(vx); velY.push_back(vy); velZ.push_back(vz);
    mass.push_back(m);
    radius.push_back(r);
    is_dark.push_back(dm ? 1 : 0);
}

void PhysicsEngine::reset() {
    accX.clear(); accY.clear(); accZ.clear();
    posX.clear(); posY.clear(); posZ.clear();
    velX.clear(); velY.clear(); velZ.clear();
    mass.clear();
    radius.clear();
    is_dark.clear();

    time = 0.0f;
}

void PhysicsEngine::delete_dm() {
    const size_t n = posX.size();
    if (n == 0) return;

    size_t write_idx = 0;

    for (size_t i = 0; i < n; ++i) {
        if (is_dark[i]) continue;

        if (i != write_idx) {
            posX[write_idx]  = posX[i];
            posY[write_idx]  = posY[i];
            posZ[write_idx]  = posZ[i];

            velX[write_idx]  = velX[i];
            velY[write_idx]  = velY[i];
            velZ[write_idx]  = velZ[i];

            mass[write_idx]  = mass[i];
            radius[write_idx] = radius[i];
            is_dark[write_idx] = is_dark[i];
        }
        write_idx++;
    }

    posX.resize(write_idx);
    posY.resize(write_idx);
    posZ.resize(write_idx);

    velX.resize(write_idx);
    velY.resize(write_idx);
    velZ.resize(write_idx);

    accX.resize(write_idx);
    accY.resize(write_idx);
    accZ.resize(write_idx);

    mass.resize(write_idx);
    radius.resize(write_idx);
    is_dark.resize(write_idx);
}

void PhysicsEngine::update_cell_size() {
    // Считаем средний радиус по всем частицам
    float r_summ = 0;
    float r_size = radius.size();

    for (size_t i = 0; i < r_size; ++i) {
        if (!is_dark[i]) r_summ += radius[i];
    }

    float average = r_summ / radius.size();

    spatial_grid.set_cell_size(average * 2.0f);
}