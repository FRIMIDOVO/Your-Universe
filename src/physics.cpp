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
     bh_tree(),
     physics(physics),
     space(space)
{
}


void PhysicsEngine::step() {
    // First kick and drift
    static float dt = physics.dt;
    static float half_dt = dt * 0.5f;
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

    //std::cerr << "Building SpatialGrid... ";
    spatial_grid.build(posX, posY, posZ, is_dark);

    // Second kick
    for (size_t i = 0; i < accX.size(); ++i) {
        // Применяем ускорение
        velX[i] += accX[i] * half_dt;
        velY[i] += accY[i] * half_dt;
        velZ[i] += accZ[i] * half_dt;
    }

    //std::cerr << "Computing collisions... ";
    // Коллизии: elastic / SPH
    if (physics.K_elasticity) Laws::collisions(posX, posY, posZ, velX, velY, velZ, mass, radius, is_dark, physics, spatial_grid);
    //else Laws::SPH(accX, accY, accZ, posX, posY, posZ, velX, velY, velZ, mass, is_dark, physics, bh_tree);

    time += physics.dt;
}

void PhysicsEngine::add_particle(
    float m,
    float r,
    Vec3 pos,
    Vec3 vel,
    bool dm
)
{
    accX.push_back(0.0f); accY.push_back(0.0f); accZ.push_back(0.0f);
    posX.push_back(pos.x); posY.push_back(pos.y); posZ.push_back(pos.z);
    velX.push_back(vel.x); velY.push_back(vel.y); velZ.push_back(vel.z);
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