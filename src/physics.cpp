#include "physics.h"
#include <algorithm>
#include <iostream>


PhysicsEngine::PhysicsEngine(PhysicsConfig& phys)
    : physics(phys)
    , opencl_manager()
    , spatial_grid(opencl_manager)
    , nbody_solver(opencl_manager)
    , sph_solver(opencl_manager)
    , particle_data()
    , particle_buffers(opencl_manager)
{}

void PhysicsEngine::step() {

    const int N = (int)particle_data.posX.size();
    if (N == 0) return;

    const float dt      = physics.dt;
    const float half_dt = dt * 0.5f;

    cl_command_queue queue = opencl_manager.getQueue();

    // === Первый kick + drift (CPU) ===
    for (int i = 0; i < N; ++i) {
        particle_data.velX[i] += particle_data.accX[i] * half_dt;
        particle_data.velY[i] += particle_data.accY[i] * half_dt;
        particle_data.velZ[i] += particle_data.accZ[i] * half_dt;

        particle_data.posX[i] += particle_data.velX[i] * dt;
        particle_data.posY[i] += particle_data.velY[i] * dt;
        particle_data.posZ[i] += particle_data.velZ[i] * dt;
    }
    std::fill(particle_data.accX.begin(), particle_data.accX.end(), 0.0f);
    std::fill(particle_data.accY.begin(), particle_data.accY.end(), 0.0f);
    std::fill(particle_data.accZ.begin(), particle_data.accZ.end(), 0.0f);

    // === Загрузка CPU -> GPU ===
    particle_buffers.upload(
        particle_data.accX, particle_data.accY, particle_data.accZ,
        particle_data.posX, particle_data.posY, particle_data.posZ,
        particle_data.velX, particle_data.velY, particle_data.velZ,
        particle_data.mass, particle_data.is_dark
    );

    // === Гравитация ===
    nbody_solver.solve(
        particle_buffers.posX(), particle_buffers.posY(), particle_buffers.posZ(),
        particle_buffers.mass(),
        particle_buffers.accX(), particle_buffers.accY(), particle_buffers.accZ(),
        N, physics.G, physics.softening_sq
    );

    // === Spatial grid (CPU + GPU) ===
    spatial_grid.build(
        particle_data.posX, particle_data.posY, particle_data.posZ,
        particle_data.is_dark
    );

    // === 5. SPH ===
    sph_solver.solve(
        particle_buffers.posX(), particle_buffers.posY(), particle_buffers.posZ(),
        particle_buffers.velX(), particle_buffers.velY(), particle_buffers.velZ(),
        particle_buffers.mass(), particle_buffers.isDark(),
        particle_buffers.accX(), particle_buffers.accY(), particle_buffers.accZ(),
        spatial_grid.getUniqueKeys(), spatial_grid.getSortedIndices(),
        spatial_grid.getCellStart(), spatial_grid.getCellCount(),
        spatial_grid.getNumUniqueCells(),
        spatial_grid.getOffset(),
        spatial_grid.getInvCellSize(),
        N,
        physics.h, physics.h2, physics.h8,
        physics.rest_density,
        physics.K_stiffness,
        physics.gamma,
        physics.alpha,
        physics.cs
    );

    // === Выгрузка GPU -> CPU ===
    clFinish(queue);    // Обязательно ждём завершения всех ядер
    particle_buffers.download_acc(
        particle_data.accX, particle_data.accY, particle_data.accZ,
        N
    );

    // === Второй kick (CPU) ===
    for (int i = 0; i < N; ++i) {
        particle_data.velX[i] += particle_data.accX[i] * half_dt;
        particle_data.velY[i] += particle_data.accY[i] * half_dt;
        particle_data.velZ[i] += particle_data.accZ[i] * half_dt;
    }

    time += dt;
}