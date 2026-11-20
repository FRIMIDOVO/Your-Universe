#pragma once
#include "opencl_manager.h"

class SPHSolver {
public:
    explicit SPHSolver(OpenCLManager& manager);
    ~SPHSolver();

    SPHSolver(const SPHSolver&) = delete;
    SPHSolver& operator=(const SPHSolver&) = delete;

    // Всё — cl_mem. Никаких ParticleBuffers.
    void solve(
        cl_mem d_posX, cl_mem d_posY, cl_mem d_posZ,
        cl_mem d_velX, cl_mem d_velY, cl_mem d_velZ,
        cl_mem d_mass, cl_mem d_is_dark,
        cl_mem d_accX, cl_mem d_accY, cl_mem d_accZ,
        cl_mem unique_keys, cl_mem sorted_indices,
        cl_mem cell_start, cl_mem cell_count,
        int n_uniq, int offset, float inv_cell_size,
        int N,
        float h, float h2, float h8,
        float rest_density,
        float K_stiffness,
        float gamma,
        float alpha,
        float cs
    );

private:
    OpenCLManager& opencl_manager;

    // Только внутренние буферы SPH
    cl_mem d_density  = nullptr;
    cl_mem d_pressure = nullptr;

    int current_capacity = 0;

    void init_buffers(int N);
    void release_buffers();
};