#pragma once
#include "opencl_manager.h"
#include <stdexcept>

class NBodySolver {
public:
    explicit NBodySolver(OpenCLManager& manager);
    ~NBodySolver() = default;

    void solve(
        cl_mem d_posX, cl_mem d_posY, cl_mem d_posZ,
        cl_mem d_mass,
        cl_mem d_accX, cl_mem d_accY, cl_mem d_accZ,
        int N,
        float G,
        float softeningSq
    );

private:
    OpenCLManager& clManager;
};