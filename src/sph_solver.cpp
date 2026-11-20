#include "sph_solver.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

// ------------------------------------------------------------
// Constructor / Destructor
// ------------------------------------------------------------

SPHSolver::SPHSolver(OpenCLManager& manager)
    : opencl_manager(manager) {}

SPHSolver::~SPHSolver() {
    release_buffers();
}

void SPHSolver::release_buffers() {
    auto release = [](cl_mem& buffer) {
        if (buffer) { clReleaseMemObject(buffer); buffer = nullptr; }
    };
    release(d_density);
    release(d_pressure);
    current_capacity = 0;
}

void SPHSolver::init_buffers(int N) {
    if (N <= current_capacity) return;

    clFinish(opencl_manager.getQueue());
    release_buffers();

    const size_t fSize = static_cast<size_t>(N) * sizeof(float);

    d_density  = opencl_manager.createBuffer(fSize, CL_MEM_READ_WRITE);
    d_pressure = opencl_manager.createBuffer(fSize, CL_MEM_READ_WRITE);

    current_capacity = N;
}

// ------------------------------------------------------------
// solve
// ------------------------------------------------------------

void SPHSolver::solve(
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
) {
    if (N <= 0) return;

    if (h <= 0.0f || rest_density <= 0.0f || gamma <= 0.0f) return;

    if (!std::isfinite(h) || !std::isfinite(rest_density) || !std::isfinite(K_stiffness) ||
        !std::isfinite(gamma) || !std::isfinite(alpha) || !std::isfinite(cs)) {
        throw std::runtime_error("SPH: non-finite physics parameter.");
    }

    if (!std::isfinite(h2) || !std::isfinite(h8) || h2 <= 0.0f || h8 <= 0.0f) return;

    constexpr float PI = 3.14159265358979323846f;
    const float four_inv_pi_h8   = 4.0f / (PI * h8);
    const float twfour_inv_pi_h8 = 24.0f / (PI * h8);
    const float k_rd_inv_g       = K_stiffness * rest_density / gamma;

    init_buffers(N);

    cl_command_queue queue    = opencl_manager.getQueue();
    cl_kernel kernel_density  = opencl_manager.getKernel("sph", "calculate_density_sph");
    cl_kernel kernel_pressure = opencl_manager.getKernel("sph", "calculate_pressure_sph");
    cl_kernel kernel_forces   = opencl_manager.getKernel("sph", "calculate_forces_sph");

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

    // --- DENSITY ---
    {
        cl_uint idx = 0;
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_posX);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_posY);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_posZ);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_mass);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_is_dark);
        setArg(kernel_density, idx++, sizeof(cl_mem), &d_density);
        setArg(kernel_density, idx++, sizeof(int),    &N);
        setArg(kernel_density, idx++, sizeof(float),  &h2);
        setArg(kernel_density, idx++, sizeof(float),  &four_inv_pi_h8);
        setArg(kernel_density, idx++, sizeof(cl_mem), &unique_keys);
        setArg(kernel_density, idx++, sizeof(cl_mem), &sorted_indices);
        setArg(kernel_density, idx++, sizeof(cl_mem), &cell_start);
        setArg(kernel_density, idx++, sizeof(cl_mem), &cell_count);
        setArg(kernel_density, idx++, sizeof(int),    &n_uniq);
        setArg(kernel_density, idx++, sizeof(int),    &offset);
        setArg(kernel_density, idx++, sizeof(float),  &inv_cell_size);
    }

    // --- PRESSURE ---
    {
        cl_uint idx = 0;
        setArg(kernel_pressure, idx++, sizeof(cl_mem), &d_density);
        setArg(kernel_pressure, idx++, sizeof(cl_mem), &d_is_dark);
        setArg(kernel_pressure, idx++, sizeof(cl_mem), &d_pressure);
        setArg(kernel_pressure, idx++, sizeof(int),    &N);
        setArg(kernel_pressure, idx++, sizeof(float),  &rest_density);
        setArg(kernel_pressure, idx++, sizeof(float),  &k_rd_inv_g);
        setArg(kernel_pressure, idx++, sizeof(float),  &gamma);
    }

    // --- FORCES ---
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
        setArg(kernel_forces, idx++, sizeof(int),    &N);
        setArg(kernel_forces, idx++, sizeof(float),  &h);
        setArg(kernel_forces, idx++, sizeof(float),  &h2);
        setArg(kernel_forces, idx++, sizeof(float),  &twfour_inv_pi_h8);
        setArg(kernel_forces, idx++, sizeof(float),  &alpha);
        setArg(kernel_forces, idx++, sizeof(float),  &cs);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &unique_keys);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &sorted_indices);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &cell_start);
        setArg(kernel_forces, idx++, sizeof(cl_mem), &cell_count);
        setArg(kernel_forces, idx++, sizeof(int),    &n_uniq);
        setArg(kernel_forces, idx++, sizeof(int),    &offset);
        setArg(kernel_forces, idx++, sizeof(float),  &inv_cell_size);
    }

    constexpr size_t LOCAL_SIZE = 128;
    const size_t global_size = ((static_cast<size_t>(N) + LOCAL_SIZE - 1) / LOCAL_SIZE) * LOCAL_SIZE;
    const size_t local_size  = LOCAL_SIZE;

    cl_int err;

    err = clEnqueueNDRangeKernel(queue, kernel_density, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("SPH: density kernel launch failed: " + std::to_string(err));

    err = clEnqueueNDRangeKernel(queue, kernel_pressure, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("SPH: pressure kernel launch failed: " + std::to_string(err));

    err = clEnqueueNDRangeKernel(queue, kernel_forces, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("SPH: forces kernel launch failed: " + std::to_string(err));

    // clFinish не нужен — PhysicsEngine сделает это перед download_acc()
}