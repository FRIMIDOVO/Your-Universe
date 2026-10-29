#include "physics.h"
#include "physics_laws.h"
#include <iostream>

PhysicsEngine::PhysicsEngine(
        const PhysicsConfig& physics, 
        const SpaceConfig& space,
        OpenCLManager& clManager
) : physics(physics),
    space(space),
    nbody_solver(clManager),
    time(0.0f),
    spatial_grid(0.4f)  // в 2 раза больше среднего радиуса
{
}


void PhysicsEngine::step() {
    std::cerr << "Prepearing data from OOP to DOD... ";
    // Подготавливаем данные
    int N = particles.size();

    for (int i = 0; i < N; ++i) {
        posX[i] = particles[i].get_position().x;
        posY[i] = particles[i].get_position().y;
        posZ[i] = particles[i].get_position().z;
        
        velX[i] = particles[i].get_velocity().x;
        velY[i] = particles[i].get_velocity().y;
        velZ[i] = particles[i].get_velocity().z;
        
        mass[i] = particles[i].get_mass();
        radius[i] = particles[i].radius();
        is_dark[i] = particles[i].is_dark_matter() ? 1 : 0;
    }

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

    std::cerr << "Computing gravity... ";
    // Гравитация: Barnes-hut / clear GPU
    if (physics.theta) {
	std::cerr << "Building Octree... ";
    	// Строим октодерево для гравитации
    	bh_tree.build(posX, posY, posZ, mass, physics);
	Laws::BH_gravity(accX, accY, accZ, posX, posY, posZ, mass, physics, bh_tree);
    } else Laws::GPU_gravity(accX, accY, accZ, posX, posY, posZ, mass, physics, nbody_solver);

    std::cerr << "Building SpatialGrid... ";
    spatial_grid.build(posX, posY, posZ, is_dark);

    // Second kick
    for (size_t i = 0; i < accX.size(); ++i) {
        // Применяем ускорение
        velX[i] += accX[i] * half_dt;
        velY[i] += accY[i] * half_dt;
        velZ[i] += accZ[i] * half_dt;
    }

    std::cerr << "Computing collisions... ";
    // Коллизии: elastic / SPH
    if (physics.K_elasticity) Laws::collisions(posX, posY, posZ, velX, velY, velZ, mass, radius, is_dark, physics, spatial_grid);
    else Laws::SPH(accX, accY, accZ, posX, posY, posZ, velX, velY, velZ, mass, is_dark, physics, bh_tree);

    std::cerr << "Return data to OOP...\n";
    // Свёртываем данные обратно
    #pragma omp parallel for schedule(static, 500)
    for (int i = 0; i < N; ++i) {
        particles[i].set_position(Vec3(posX[i], posY[i], posZ[i]));
        particles[i].set_velocity(Vec3(velX[i], velY[i], velZ[i]));
    }

    time += physics.dt;
}

void PhysicsEngine::update_dod() {
    int N = particles.size();
    posX.resize(N); posY.resize(N); posZ.resize(N);
    velX.resize(N); velY.resize(N); velZ.resize(N);
    accX.resize(N); accY.resize(N); accZ.resize(N);
    mass.resize(N); radius.resize(N); is_dark.resize(N);

    for (int i = 0; i < N; ++i) {
        posX[i] = particles[i].get_position().x;
        posY[i] = particles[i].get_position().y;
        posZ[i] = particles[i].get_position().z;
        
        velX[i] = particles[i].get_velocity().x;
        velY[i] = particles[i].get_velocity().y;
        velZ[i] = particles[i].get_velocity().z;
        
        mass[i] = particles[i].get_mass();
        radius[i] = particles[i].radius();
        is_dark[i] = particles[i].is_dark_matter() ? 1 : 0;
    }
}