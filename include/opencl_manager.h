#pragma once
#define CL_TARGET_OPENCL_VERSION 200

#include <CL/cl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

class OpenCLManager {
public:
    OpenCLManager();
    ~OpenCLManager();

    OpenCLManager(const OpenCLManager&) = delete;
    OpenCLManager& operator=(const OpenCLManager&) = delete;

    cl_context       getContext() const { return context_; }
    cl_command_queue getQueue()   const { return queue_; }

    cl_mem createBuffer(size_t size, cl_mem_flags flags, void* host_ptr = nullptr);
    void   writeBuffer(cl_mem buffer, const void* ptr, size_t size);
    void writeBufferAsync(cl_mem buffer, const void* ptr, size_t size);
    void   readBuffer (cl_mem buffer, void* ptr, size_t size);

    cl_kernel getKernel(const std::string& pipeline, const std::string& kernel) const;

private:
    struct Pipeline {
        cl_program program = nullptr;
        std::unordered_map<std::string, cl_kernel> kernels;
    };

    cl_platform_id   platform_ = nullptr;
    cl_device_id     device_   = nullptr;
    cl_context       context_  = nullptr;
    cl_command_queue queue_    = nullptr;

    std::unordered_map<std::string, Pipeline> pipelines_;

    void initialize();
    void loadPipeline(const std::string& name, const std::string& filename,
                      const std::vector<std::string>& kernel_names);

    static void checkError(cl_int err, const std::string& msg);
    std::string readKernelSource(const std::string& filename) const;
    cl_program  buildProgram(const std::string& filename);
    cl_kernel   createKernel(cl_program program, const std::string& kernel_name);
    void        releaseResources();
};