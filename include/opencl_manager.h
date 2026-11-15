#pragma once
#define CL_TARGET_OPENCL_VERSION 200

#include <CL/cl.h>
#include <string>
#include <vector>
#include <stdexcept>

class OpenCLManager {
public:
    OpenCLManager();
    ~OpenCLManager();

    OpenCLManager(const OpenCLManager&) = delete;
    OpenCLManager& operator=(const OpenCLManager&) = delete;

    // Инициализация OpenCL
    void initialize();

    // Загрузка одного ядра из файла
    void loadKernel(
        const std::string& filename,
        cl_program& program,
        const std::string& kernel_name,
        cl_kernel& kernel
    );

    // Загрузка трёх SPH-ядер из одной программы
    void loadSPHKernels(const std::string& filename);

    // Создание буфера
    cl_mem createBuffer(
        size_t size,
        cl_mem_flags flags,
        void* host_ptr = nullptr
    );

    // Передача данных
    void writeBuffer(cl_mem buffer, const void* ptr, size_t size);
    void readBuffer(cl_mem buffer, void* ptr, size_t size);

    // OpenCL context / queue
    cl_context getContext() const { return context; }
    cl_command_queue getQueue() const { return queue; }

    // Gravity
    cl_kernel getGravityKernel() const { return kernel_gravity; }

    // SPH
    cl_kernel getSPHDensityKernel() const { return kernel_sph_density; }
    cl_kernel getSPHPressureKernel() const { return kernel_sph_pressure; }
    cl_kernel getSPHForcesKernel() const { return kernel_sph_forces; }

private:
    cl_platform_id platform = nullptr;
    cl_device_id device = nullptr;

    cl_context context = nullptr;
    cl_command_queue queue = nullptr;

    // Gravity
    cl_program program_gravity = nullptr;
    cl_kernel kernel_gravity = nullptr;

    // SPH
    cl_program program_sph = nullptr;

    cl_kernel kernel_sph_density = nullptr;
    cl_kernel kernel_sph_pressure = nullptr;
    cl_kernel kernel_sph_forces = nullptr;

    // Вспомогательные функции
    static void checkError(cl_int err, const std::string& msg);
    std::string readKernelSource(const std::string& filename) const;
    cl_program buildProgram(const std::string& filename);
    cl_kernel createKernel(cl_program program, const std::string& kernel_name);
    void releaseResources();
};
