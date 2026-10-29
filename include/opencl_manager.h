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

    // Инициализация: выбор устройства, создание контекста и очереди
    void initialize();

    // Загрузка kernel'а из .cl файла и компиляция программы
    void loadKernel(const std::string& filename);

    // Создание буфера памяти (INPUT, OUTPUT или READ_WRITE)
    cl_mem createBuffer(size_t size, cl_mem_flags flags, void* host_ptr = nullptr);

    // Копирование данных: Host -> Device
    void writeBuffer(cl_mem buffer, const void* ptr, size_t size);

    // Копирование данных: Device -> Host
    void readBuffer(cl_mem buffer, void* ptr, size_t size);

    // Получение глобальных объектов (нужны для запуска ядер)
    cl_context getContext() const { return context; }
    cl_command_queue getQueue() const { return queue; }
    cl_program getProgram() const { return program; }

    // Gravity
    void runGravityForces(cl_mem posX, cl_mem posY, cl_mem posZ, cl_mem mass,
        cl_mem accX, cl_mem accY, cl_mem accZ, int N, float G, float softeningSq);

private:
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;

    // Вспомогательная функция для проверки ошибок
    static void checkError(cl_int err, const std::string& msg);
};