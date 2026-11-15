#include "sph_solver.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

// Конструктор / Деструктор

SPHSolver::SPHSolver(OpenCLManager& manager)
    : opencl_manager(manager) {}

SPHSolver::~SPHSolver() {
    release_buffers();
}

// Освобождение буферов

void SPHSolver::release_buffers() {
    auto release = [](cl_mem& buffer) {
        if (buffer) {
            clReleaseMemObject(buffer);
            buffer = nullptr;
        }
    };

    release(d_posX); release(d_posY); release(d_posZ);
    release(d_velX); release(d_velY); release(d_velZ);
    release(d_mass); release(d_is_dark);
    release(d_accX); release(d_accY); release(d_accZ);
    release(d_density); release(d_pressure);

    current_capacity = 0;
}

// Инициализация буферов

void SPHSolver::init_buffers(int N) {
    if (N <= current_capacity) return;

    release_buffers();

    const size_t fSize = static_cast<size_t>(N) * sizeof(float);
    const size_t bSize = static_cast<size_t>(N) * sizeof(uint8_t);

    // Входные данные
    d_posX = opencl_manager.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_posY = opencl_manager.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_posZ = opencl_manager.createBuffer(fSize, CL_MEM_READ_ONLY);

    d_velX = opencl_manager.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_velY = opencl_manager.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_velZ = opencl_manager.createBuffer(fSize, CL_MEM_READ_ONLY);

    d_mass = opencl_manager.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_is_dark = opencl_manager.createBuffer(bSize, CL_MEM_READ_ONLY);

    // Ускорения (READ_WRITE, так как ядра используют +=)
    d_accX = opencl_manager.createBuffer(fSize, CL_MEM_READ_WRITE);
    d_accY = opencl_manager.createBuffer(fSize, CL_MEM_READ_WRITE);
    d_accZ = opencl_manager.createBuffer(fSize, CL_MEM_READ_WRITE);

    // Промежуточные данные SPH
    d_density = opencl_manager.createBuffer(fSize, CL_MEM_READ_WRITE);
    d_pressure = opencl_manager.createBuffer(fSize, CL_MEM_READ_WRITE);

    current_capacity = N;
}

void SPHSolver::solve(
    std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
    const std::vector<float>& mass, const std::vector<uint8_t>& is_dark,
    float h,
    float rest_density,
    float K_stiffness,
    float gamma,
    float alpha,
    float cs
) {
    const size_t n = posX.size();
    if (n == 0) return;

    if (n > static_cast<size_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("SPH: particle count exceeds INT_MAX.");
    }

    const auto checkSize = [n](const auto& v, const char* name) {
        if (v.size() != n) {
            throw std::runtime_error(std::string("SPH: invalid vector size: ") + name);
        }
    };

    checkSize(posY, "posY"); checkSize(posZ, "posZ");
    checkSize(velX, "velX"); checkSize(velY, "velY"); checkSize(velZ, "velZ");
    checkSize(mass, "mass"); checkSize(is_dark, "is_dark");

    if (h <= 0.0f || rest_density <= 0.0f || gamma <= 0.0f) return;

    if (!std::isfinite(h) || !std::isfinite(rest_density) || !std::isfinite(K_stiffness) ||
        !std::isfinite(gamma) || !std::isfinite(alpha) || !std::isfinite(cs)) {
        throw std::runtime_error("SPH: non-finite physics parameter.");
    }

    const int N = static_cast<int>(n);

    const float h2 = h * h;
    const float h4 = h2 * h2;
    const float h8 = h4 * h4;

    if (!std::isfinite(h2) || !std::isfinite(h8) || h2 <= 0.0f || h8 <= 0.0f) return;

    constexpr float PI = 3.14159265358979323846f;
    const float four_inv_pi_h8 = 4.0f / (PI * h8);
    const float twfour_inv_pi_h8 = 24.0f / (PI * h8);
    const float k_rd_inv_g = K_stiffness * rest_density / gamma;

    init_buffers(N);

    const size_t fSize = n * sizeof(float);
    const size_t bSize = n * sizeof(uint8_t);

    // Сохраняем существующее ускорение (например, гравитационное)
    accX.resize(n, 0.0f);
    accY.resize(n, 0.0f);
    accZ.resize(n, 0.0f);

    // Загрузка данных
    opencl_manager.writeBuffer(d_posX, posX.data(), fSize);
    opencl_manager.writeBuffer(d_posY, posY.data(), fSize);
    opencl_manager.writeBuffer(d_posZ, posZ.data(), fSize);

    opencl_manager.writeBuffer(d_velX, velX.data(), fSize);
    opencl_manager.writeBuffer(d_velY, velY.data(), fSize);
    opencl_manager.writeBuffer(d_velZ, velZ.data(), fSize);

    opencl_manager.writeBuffer(d_mass, mass.data(), fSize);
    opencl_manager.writeBuffer(d_is_dark, is_dark.data(), bSize);

    opencl_manager.writeBuffer(d_accX, accX.data(), fSize);
    opencl_manager.writeBuffer(d_accY, accY.data(), fSize);
    opencl_manager.writeBuffer(d_accZ, accZ.data(), fSize);

    cl_command_queue queue = opencl_manager.getQueue();
    cl_kernel kernel_density = opencl_manager.getSPHDensityKernel();
    cl_kernel kernel_pressure = opencl_manager.getSPHPressureKernel();
    cl_kernel kernel_forces = opencl_manager.getSPHForcesKernel();

    if (!queue || !kernel_density || !kernel_pressure || !kernel_forces) {
        throw std::runtime_error("SPH: OpenCL pipeline is not initialized.");
    }

    auto setArg = [](cl_kernel kernel, cl_uint index, size_t size, const void* value) {
        cl_int err = clSetKernelArg(kernel, index, size, value);
        if (err != CL_SUCCESS) {
            throw std::runtime_error(
                "SPH: clSetKernelArg failed at index " + std::to_string(index) +
                ", error: " + std::to_string(err)
            );
        }
    };

    // Ядро 1: ПЛОТНОСТЬ
    {
        cl_uint idx = 0;
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_posX);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_posY);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_posZ);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_mass);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_is_dark);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_density);
        setArg(kernel_density, idx++, sizeof(int), &N);
        setArg(kernel_density, idx++, sizeof(float), &h2);
        setArg(kernel_density, idx++, sizeof(float), &four_inv_pi_h8);
    }

    // Ядро 2: ДАВЛЕНИЕ
    {
        cl_uint idx = 0;
        setArg(kernel_pressure, idx++, sizeof(cl_mem), &d_density);
        setArg(kernel_pressure, idx++, sizeof(cl_mem), &d_is_dark);
        setArg(kernel_pressure, idx++, sizeof(cl_mem), &d_pressure);
        setArg(kernel_pressure, idx++, sizeof(int), &N);
        setArg(kernel_pressure, idx++, sizeof(float), &rest_density);
        setArg(kernel_pressure, idx++, sizeof(float), &k_rd_inv_g);
        setArg(kernel_pressure, idx++, sizeof(float), &gamma);
    }

    // Ядро 3: СИЛЫ
    {
        cl_uint idx = 0;
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_posX);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_posY);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_posZ);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_velX);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_velY);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_velZ);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_mass);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_is_dark);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_accX);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_accY);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_accZ);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_density);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &d_pressure);
        setArg(kernel_forces, idx++, sizeof(int), &N);
        setArg(kernel_forces, idx++, sizeof(float), &h);
        setArg(kernel_forces, idx++, sizeof(float), &h2);
        setArg(kernel_forces, idx++, sizeof(float), &twfour_inv_pi_h8);
        setArg(kernel_forces, idx++, sizeof(float), &alpha);
        setArg(kernel_forces, idx++, sizeof(float), &cs);
    }

    constexpr size_t LOCAL_SIZE = 256;
    const size_t global_size = ((n + LOCAL_SIZE - 1) / LOCAL_SIZE) * LOCAL_SIZE;
    const size_t local_size = LOCAL_SIZE;

    cl_int err;

    err = clEnqueueNDRangeKernel(queue, kernel_density, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("SPH: density kernel launch failed: " + std::to_string(err));
    }

    err = clEnqueueNDRangeKernel(queue, kernel_pressure, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("SPH: pressure kernel launch failed: " + std::to_string(err));
    }

    err = clEnqueueNDRangeKernel(queue, kernel_forces, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("SPH: forces kernel launch failed: " + std::to_string(err));
    }

    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("SPH: pipeline execution failed: " + std::to_string(err));
    }

    opencl_manager.readBuffer(d_accX, accX.data(), fSize);
    opencl_manager.readBuffer(d_accY, accY.data(), fSize);
    opencl_manager.readBuffer(d_accZ, accZ.data(), fSize);
}