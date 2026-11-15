#include "nbody_solver.h"
#include <iostream>

NBodySolver::NBodySolver(OpenCLManager& manager) : clManager(manager) {}

NBodySolver::~NBodySolver() {
    releaseBuffers();
}

void NBodySolver::releaseBuffers() {
    if (d_posX) clReleaseMemObject(d_posX);
    if (d_posY) clReleaseMemObject(d_posY);
    if (d_posZ) clReleaseMemObject(d_posZ);
    if (d_mass) clReleaseMemObject(d_mass);
    if (d_accX) clReleaseMemObject(d_accX);
    if (d_accY) clReleaseMemObject(d_accY);
    if (d_accZ) clReleaseMemObject(d_accZ);
    
    d_posX = d_posY = d_mass = d_accX = d_accY = nullptr;
    currentMaxParticles = 0;
}

void NBodySolver::init(int maxParticles) {
    if (maxParticles <= currentMaxParticles) return;
    
    releaseBuffers();
    
    size_t bytes = static_cast<size_t>(maxParticles) * sizeof(float) * 5; // 5 буферов по float
    
    d_posX = clManager.createBuffer(bytes, CL_MEM_READ_WRITE);
    d_posY = clManager.createBuffer(bytes, CL_MEM_READ_WRITE);
    d_posZ = clManager.createBuffer(bytes, CL_MEM_READ_WRITE);
    d_mass = clManager.createBuffer(bytes, CL_MEM_READ_ONLY);
    d_accX = clManager.createBuffer(bytes, CL_MEM_WRITE_ONLY);
    d_accY = clManager.createBuffer(bytes, CL_MEM_WRITE_ONLY);
    d_accZ = clManager.createBuffer(bytes, CL_MEM_WRITE_ONLY);
    
    currentMaxParticles = maxParticles;
    
    float mb = static_cast<float>(bytes) / (1024.0f * 1024.0f);
}

void NBodySolver::solve(
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& mass,
    std::vector<float>& outAccX, std::vector<float>& outAccY, std::vector<float>& outAccZ,
    float G, float softeningSq
)
{
    int N = posX.size();
    if (N == 0) return;

    if (N > currentMaxParticles) {
        init(N);
    }

    size_t fSize = N * sizeof(float);

    // Копируем данные на GPU
    clManager.writeBuffer(d_posX, posX.data(), fSize);
    clManager.writeBuffer(d_posY, posY.data(), fSize);
    clManager.writeBuffer(d_posZ, posZ.data(), fSize);
    clManager.writeBuffer(d_mass, mass.data(), fSize);

    // Берем кэшированное ядро и очередь
    cl_kernel kernel = clManager.getGravityKernel();
    cl_command_queue queue = clManager.getQueue();

    // Устанавливаем аргументы (в нужном порядке!)
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

    // Запускаем ядро
    const size_t LOCAL_SIZE = 256;
    size_t global_size = ((static_cast<size_t>(N) + LOCAL_SIZE - 1) / LOCAL_SIZE) * LOCAL_SIZE;
    
    cl_int err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_size, &LOCAL_SIZE, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("OpenCL Error [NBodySolver clEnqueueNDRangeKernel]: " + std::to_string(err));
    }

    clFinish(queue);

    // Читаем результаты
    outAccX.resize(N);
    outAccY.resize(N);
    outAccZ.resize(N);
    
    clManager.readBuffer(d_accX, outAccX.data(), fSize);
    clManager.readBuffer(d_accY, outAccY.data(), fSize);
    clManager.readBuffer(d_accZ, outAccZ.data(), fSize);
}