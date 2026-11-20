#include "opencl_manager.h"

#include <fstream>
#include <sstream>
#include <iostream>

void OpenCLManager::checkError(cl_int err, const std::string& msg) {
    if (err != CL_SUCCESS)
        throw std::runtime_error("OpenCL Error [" + msg + "]: " + std::to_string(err));
}

std::string OpenCLManager::readKernelSource(const std::string& filename) const {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("Cannot open kernel file: " + filename);
    std::stringstream ss; ss << file.rdbuf();
    return ss.str();
}

cl_program OpenCLManager::buildProgram(const std::string& filename) {
    const std::string source = readKernelSource(filename);
    const char* src_ptr = source.c_str();

    cl_int err;
    cl_program program = clCreateProgramWithSource(context_, 1, &src_ptr, nullptr, &err);
    checkError(err, "clCreateProgramWithSource");

    err = clBuildProgram(program, 1, &device_, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        size_t log_size = 0;
        clGetProgramBuildInfo(program, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        std::string log(log_size, '\0');
        if (log_size > 0)
            clGetProgramBuildInfo(program, device_, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
        clReleaseProgram(program);
        throw std::runtime_error("[OpenCL] Build failed: " + filename + "\n" + log);
    }
    return program;
}

cl_kernel OpenCLManager::createKernel(cl_program program, const std::string& kernel_name) {
    cl_int err;
    cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), &err);
    checkError(err, "clCreateKernel: " + kernel_name);
    return kernel;
}

OpenCLManager::OpenCLManager() {
    try {
        initialize();

        loadPipeline("gravity", "bin/kernels/gravity.cl",
            { "calculate_forces_nbody" });

        loadPipeline("sph", "bin/kernels/sph.cl",
            { "calculate_density_sph",
              "calculate_pressure_sph",
              "calculate_forces_sph" });

        loadPipeline("spatial_grid", "bin/kernels/spatial_grid.cl",
            { "query_radius" });
    } catch (...) {
        releaseResources();
        throw;
    }
}

OpenCLManager::~OpenCLManager() {
    releaseResources();
}

void OpenCLManager::releaseResources() {
    for (auto& [name, pipe] : pipelines_) {
        for (auto& [kn, kernel] : pipe.kernels)
            if (kernel) clReleaseKernel(kernel);
        if (pipe.program) clReleaseProgram(pipe.program);
    }
    pipelines_.clear();

    if (queue_)   { clReleaseCommandQueue(queue_); queue_   = nullptr; }
    if (context_) { clReleaseContext(context_);     context_ = nullptr; }
}

void OpenCLManager::initialize() {
    cl_int err;

    err = clGetPlatformIDs(1, &platform_, nullptr);
    checkError(err, "clGetPlatformIDs");

    err = clGetDeviceIDs(platform_, CL_DEVICE_TYPE_GPU, 1, &device_, nullptr);
    checkError(err, "clGetDeviceIDs");

    context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err);
    checkError(err, "clCreateContext");

    const cl_queue_properties props[] = { 0 };
    queue_ = clCreateCommandQueueWithProperties(context_, device_, props, &err);
    checkError(err, "clCreateCommandQueueWithProperties");
}

void OpenCLManager::loadPipeline(
    const std::string& name,
    const std::string& filename,
    const std::vector<std::string>& kernel_names
) {
    if (pipelines_.count(name))
        throw std::runtime_error("OpenCL: pipeline '" + name + "' already loaded");

    Pipeline pipe;
    pipe.program = buildProgram(filename);

    try {
        for (const auto& kn : kernel_names)
            pipe.kernels[kn] = createKernel(pipe.program, kn);
    } catch (...) {
        for (auto& [kn, kernel] : pipe.kernels)
            if (kernel) clReleaseKernel(kernel);
        if (pipe.program) clReleaseProgram(pipe.program);
        throw;
    }

    pipelines_[name] = std::move(pipe);
}

cl_kernel OpenCLManager::getKernel(
    const std::string& pipeline,
    const std::string& kernel
) const {
    auto p_it = pipelines_.find(pipeline);
    if (p_it == pipelines_.end())
        throw std::runtime_error("OpenCL: unknown pipeline '" + pipeline + "'");

    auto k_it = p_it->second.kernels.find(kernel);
    if (k_it == p_it->second.kernels.end())
        throw std::runtime_error("OpenCL: unknown kernel '" + kernel + "' in pipeline '" + pipeline + "'");

    return k_it->second;
}

cl_mem OpenCLManager::createBuffer(size_t size, cl_mem_flags flags, void* host_ptr) {
    if (size == 0) throw std::runtime_error("OpenCL: cannot create zero-sized buffer.");
    cl_int err;
    cl_mem buffer = clCreateBuffer(context_, flags, size, host_ptr, &err);
    checkError(err, "clCreateBuffer");
    return buffer;
}

void OpenCLManager::writeBuffer(cl_mem buffer, const void* ptr, size_t size) {
    if (!queue_ || !buffer || !ptr || size == 0)
        throw std::runtime_error("OpenCL: invalid writeBuffer arguments.");
    cl_int err = clEnqueueWriteBuffer(queue_, buffer, CL_TRUE, 0, size, ptr, 0, nullptr, nullptr);
    checkError(err, "clEnqueueWriteBuffer");
}

void OpenCLManager::writeBufferAsync(cl_mem buffer, const void* ptr, size_t size) {
    if (!queue_ || !buffer || !ptr || size == 0)
        throw std::runtime_error("OpenCL: invalid writeBufferAsync arguments.");

    cl_int err = clEnqueueWriteBuffer(
        queue_, buffer, CL_FALSE, 0, size, ptr, 0, nullptr, nullptr);
    checkError(err, "clEnqueueWriteBuffer");
}

void OpenCLManager::readBuffer(cl_mem buffer, void* ptr, size_t size) {
    if (!queue_ || !buffer || !ptr || size == 0)
        throw std::runtime_error("OpenCL: invalid readBuffer arguments.");
    cl_int err = clEnqueueReadBuffer(queue_, buffer, CL_TRUE, 0, size, ptr, 0, nullptr, nullptr);
    checkError(err, "clEnqueueReadBuffer");
}