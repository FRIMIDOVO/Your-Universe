#include "opencl_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>

void OpenCLManager::checkError(cl_int err, const std::string& msg) {
    if (err != CL_SUCCESS) {
        throw std::runtime_error("OpenCL Error [" + msg + "]: " + std::to_string(err));
    }
}

OpenCLManager::OpenCLManager() : platform(nullptr), device(nullptr),
    context(nullptr), queue(nullptr), program(nullptr)
{
    initialize();
    loadKernel("bin/kernels/gravity.cl");
}

OpenCLManager::~OpenCLManager() {
    if (program) clReleaseProgram(program);
    if (queue) clReleaseCommandQueue(queue);
    if (context) clReleaseContext(context);
}

void OpenCLManager::initialize() {
    cl_int err;
    
    // Выбираем первую доступную платформу
    err = clGetPlatformIDs(1, &platform, nullptr);
    checkError(err, "clGetPlatformIDs");
    
    // Выбираем первое GPU-устройство
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    checkError(err, "clGetDeviceIDs");
    
    // Создаем контекст
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    checkError(err, "clCreateContext");
    
    // Создаем очередь команд
    cl_queue_properties props[] = { 0 };
    queue = clCreateCommandQueueWithProperties(context, device, props, &err);
    checkError(err, "clCreateCommandQueue");
    
    std::cout << "[OpenCL] Initialized successfully on GPU." << std::endl;
}

void OpenCLManager::loadKernel(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) 
        throw std::runtime_error("Cannot open kernel file: " + filename);
    
    std::stringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();
    
    cl_int err;
    const char* srcPtr = source.c_str();
    
    program = clCreateProgramWithSource(context, 1, &srcPtr, nullptr, &err);
    checkError(err, "clCreateProgramWithSource");
    
    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    checkError(err, "clBuildProgram");
    
    std::cout << "[OpenCL] Kernel compiled from: " << filename << std::endl;
}

cl_mem OpenCLManager::createBuffer(size_t size, cl_mem_flags flags, void* host_ptr) {
    cl_int err;
    cl_mem buffer = clCreateBuffer(context, flags, size, host_ptr, &err);
    checkError(err, "clCreateBuffer");
    return buffer;
}

void OpenCLManager::writeBuffer(cl_mem buffer, const void* ptr, size_t size) {
    if (queue == nullptr) {
        throw std::runtime_error("OpenCL command queue is null! GPU initialization failed.");
    }
    
    cl_int err = clEnqueueWriteBuffer(queue, buffer, CL_TRUE, 0, size, ptr, 0, nullptr, nullptr);
    checkError(err, "clEnqueueWriteBuffer");
}

void OpenCLManager::readBuffer(cl_mem buffer, void* ptr, size_t size) {
    if (queue == nullptr) {
        throw std::runtime_error("OpenCL command queue is null! GPU initialization failed.");
    }
    
    cl_int err = clEnqueueReadBuffer(queue, buffer, CL_TRUE, 0, size, ptr, 0, nullptr, nullptr);
    checkError(err, "clEnqueueReadBuffer");
}

void OpenCLManager::runGravityForces(cl_mem posX, cl_mem posY, cl_mem posZ, cl_mem mass,
    cl_mem accX, cl_mem accY, cl_mem accZ,
    int N, float G, float softeningSq) {
    
    if (queue == nullptr) {
        throw std::runtime_error("OpenCL command queue is null! GPU initialization failed.");
    }
    
    cl_int err;
    cl_kernel kernel = clCreateKernel(program, "calculate_forces_nbody", &err);
    checkError(err, "clCreateKernel");
    
    // Установка аргументов
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &posX);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &posY);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &posZ);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &mass);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &accX);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &accY);
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &accZ);
    clSetKernelArg(kernel, 7, sizeof(int), &N);
    clSetKernelArg(kernel, 8, sizeof(float), &G);
    clSetKernelArg(kernel, 9, sizeof(float), &softeningSq);
    
    const size_t LOCAL_SIZE = 256;
    size_t global_size = ((static_cast<size_t>(N) + LOCAL_SIZE - 1) / LOCAL_SIZE) * LOCAL_SIZE;
    
    err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_size, &LOCAL_SIZE, 0, nullptr, nullptr);
    checkError(err, "clEnqueueNDRangeKernel");
    
    clFinish(queue);
    clReleaseKernel(kernel);
}