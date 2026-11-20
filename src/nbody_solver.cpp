#include "nbody_solver.h"
#include <iostream>

NBodySolver::NBodySolver(OpenCLManager& manager) : clManager(manager) {}

void NBodySolver::solve(
    cl_mem d_posX, cl_mem d_posY, cl_mem d_posZ,
    cl_mem d_mass,
    cl_mem d_accX, cl_mem d_accY, cl_mem d_accZ,
    int N,
    float G,
    float softeningSq
) {
    if (N <= 0) return;

    cl_kernel kernel = clManager.getKernel("gravity", "calculate_forces_nbody");
    cl_command_queue queue = clManager.getQueue();

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_posX);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_posY);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_posZ);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &d_mass);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &d_accX);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &d_accY);
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &d_accZ);
    clSetKernelArg(kernel, 7, sizeof(int), &N);
    clSetKernelArg(kernel, 8, sizeof(float), &G);
    clSetKernelArg(kernel, 9, sizeof(float), &softeningSq);

    const size_t LOCAL_SIZE = 128;
    size_t global_size = ((static_cast<size_t>(N) + LOCAL_SIZE - 1) / LOCAL_SIZE) * LOCAL_SIZE;

    cl_int err = clEnqueueNDRangeKernel(
        queue, kernel, 1, nullptr, &global_size, &LOCAL_SIZE, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("OpenCL Error [NBodySolver clEnqueueNDRangeKernel]: " + std::to_string(err));
    }

    // clFinish не нужен — in-order queue гарантирует порядок
    // PhysicsEngine вызовет clFinish перед download_acc()
}