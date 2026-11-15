#include "opencl_manager.h"

#include <fstream>
#include <sstream>
#include <iostream>


void OpenCLManager::checkError(cl_int err, const std::string& msg) {
    if (err != CL_SUCCESS) {
        throw std::runtime_error(
            "OpenCL Error [" + msg + "]: " +
            std::to_string(err)
        );
    }
}

OpenCLManager::OpenCLManager() {
    try {
        initialize();

        loadKernel(
            "bin/kernels/gravity.cl",
            program_gravity,
            "calculate_forces_nbody",
            kernel_gravity
        );

        loadSPHKernels("bin/kernels/sph.cl");
    }
    catch (...) {
        releaseResources();
        throw;
    }
}

OpenCLManager::~OpenCLManager() {
    releaseResources();
}

void OpenCLManager::releaseResources() {
    if (kernel_gravity) {
        clReleaseKernel(kernel_gravity);
        kernel_gravity = nullptr;
    }

    if (kernel_sph_density) {
        clReleaseKernel(kernel_sph_density);
        kernel_sph_density = nullptr;
    }

    if (kernel_sph_pressure) {
        clReleaseKernel(kernel_sph_pressure);
        kernel_sph_pressure = nullptr;
    }

    if (kernel_sph_forces) {
        clReleaseKernel(kernel_sph_forces);
        kernel_sph_forces = nullptr;
    }

    if (program_gravity) {
        clReleaseProgram(program_gravity);
        program_gravity = nullptr;
    }

    if (program_sph) {
        clReleaseProgram(program_sph);
        program_sph = nullptr;
    }

    if (queue) {
        clReleaseCommandQueue(queue);
        queue = nullptr;
    }

    if (context) {
        clReleaseContext(context);
        context = nullptr;
    }
}

void OpenCLManager::initialize() {
    cl_int err;

    // Получаем платформу
    err = clGetPlatformIDs(1, &platform, nullptr);
    checkError(err, "clGetPlatformIDs");

    // Получаем GPU
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    checkError(err, "clGetDeviceIDs");

    // Создаём context
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    checkError(err, "clCreateContext");

    // Создаём in-order command queue
    const cl_queue_properties props[] = { 0 };

    queue = clCreateCommandQueueWithProperties(context, device, props, &err);
    checkError(err, "clCreateCommandQueueWithProperties");

    std::cout << "[OpenCL] Initialized successfully on GPU" << std::endl;
}

std::string OpenCLManager::readKernelSource(const std::string& filename) const {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open kernel file: " + filename);
    }

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

cl_program OpenCLManager::buildProgram(const std::string& filename) {
    const std::string source = readKernelSource(filename);
    const char* src_ptr = source.c_str();

    cl_int err;

    cl_program program = clCreateProgramWithSource(context, 1, &src_ptr, nullptr, &err);
    checkError(err, "clCreateProgramWithSource");

    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);

    if (err != CL_SUCCESS) {
        // Получаем лог компиляции
        size_t log_size = 0;

        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);

        std::string log(log_size, '\0');

        if (log_size > 0) {
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
        }

        clReleaseProgram(program);

        throw std::runtime_error(
            "[OpenCL] Build failed: " + filename +
            "\n" + log
        );
    }

    std::cout
        << "[OpenCL] Program compiled: "
        << filename
    << std::endl;

    return program;
}

cl_kernel OpenCLManager::createKernel(cl_program program, const std::string& kernel_name) {
    cl_int err;

    cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), &err);

    checkError(err, "clCreateKernel: " + kernel_name);

    std::cout
        << "[OpenCL] Kernel created: "
        << kernel_name
    << std::endl;

    return kernel;
}

void OpenCLManager::loadKernel(
    const std::string& filename,
    cl_program& program,
    const std::string& kernel_name,
    cl_kernel& kernel
){
    program = buildProgram(filename);

    try {
        kernel = createKernel(program, kernel_name);
    }
    catch (...) {
        clReleaseProgram(program);
        program = nullptr;
        throw;
    }
}

void OpenCLManager::loadSPHKernels(
const std::string& filename
) {
    program_sph = buildProgram(filename);

    try {
        kernel_sph_density = createKernel(
            program_sph,
            "calculate_density_sph"
        );

        kernel_sph_pressure = createKernel(
            program_sph,
            "calculate_pressure_sph"
        );

        kernel_sph_forces = createKernel(
            program_sph,
            "calculate_forces_sph"
        );
    }
    catch (...) {
        if (kernel_sph_density) {
            clReleaseKernel(kernel_sph_density);
            kernel_sph_density = nullptr;
        }

        if (kernel_sph_pressure) {
            clReleaseKernel(kernel_sph_pressure);
            kernel_sph_pressure = nullptr;
        }

        if (kernel_sph_forces) {
            clReleaseKernel(kernel_sph_forces);
            kernel_sph_forces = nullptr;
        }

        clReleaseProgram(program_sph);
        program_sph = nullptr;

        throw;
    }

    std::cout
        << "[OpenCL] SPH pipeline initialized successfully."
        << std::endl;
}

// ------------------------------------------------------------
// Buffers
// ------------------------------------------------------------

cl_mem OpenCLManager::createBuffer(
    size_t size,
    cl_mem_flags flags,
    void* host_ptr
) {
    if (size == 0) {
        throw std::runtime_error(
            "OpenCL: cannot create zero-sized buffer."
        );
    }

    cl_int err;

    cl_mem buffer = clCreateBuffer(
        context,
        flags,
        size,
        host_ptr,
        &err
    );

    checkError(err, "clCreateBuffer");

    return buffer;
}

// ------------------------------------------------------------
// Data transfer
// ------------------------------------------------------------

void OpenCLManager::writeBuffer(
    cl_mem buffer,
    const void* ptr,
    size_t size
) {
    if (!queue) {
        throw std::runtime_error(
            "OpenCL command queue is null."
        );
    }

    if (!buffer || !ptr || size == 0) {
        throw std::runtime_error(
            "OpenCL: invalid writeBuffer arguments."
        );
    }

    cl_int err = clEnqueueWriteBuffer(
        queue,
        buffer,
        CL_TRUE,
        0,
        size,
        ptr,
        0,
        nullptr,
        nullptr
    );

    checkError(err, "clEnqueueWriteBuffer");
}

void OpenCLManager::readBuffer(
    cl_mem buffer,
    void* ptr,
    size_t size
) {
    if (!queue) {
        throw std::runtime_error(
            "OpenCL command queue is null."
        );
    }

    if (!buffer || !ptr || size == 0) {
        throw std::runtime_error(
            "OpenCL: invalid readBuffer arguments."
        );
    }

    cl_int err = clEnqueueReadBuffer(
        queue,
        buffer,
        CL_TRUE,
        0,
        size,
        ptr,
        0,
        nullptr,
        nullptr
    );

    checkError(err, "clEnqueueReadBuffer");
}
