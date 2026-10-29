#pragma once
#include "opencl_manager.h"
#include <vector>
#include <stdexcept>

class NBodySolver {
public:
    explicit NBodySolver(OpenCLManager& manager);
    ~NBodySolver();

    // Инициализация/пересоздание буферов под новое количество частиц
    void init(int maxParticles);

    // Основной метод: загрузка данных -> расчет -> выгрузка ускорений
    void solve(const std::vector<float>& posX, 
               const std::vector<float>& posY,
               const std::vector<float>& posZ,
               const std::vector<float>& mass,
               std::vector<float>& outAccX, 
               std::vector<float>& outAccY,
               std::vector<float>& outAccZ,
               float G, float softeningSq);

private:
    OpenCLManager& clManager;
    
    // Буферы памяти GPU
    cl_mem d_posX = nullptr;
    cl_mem d_posY = nullptr;
    cl_mem d_posZ = nullptr;
    cl_mem d_mass = nullptr;
    cl_mem d_accX = nullptr;
    cl_mem d_accY = nullptr;
    cl_mem d_accZ = nullptr;
    
    int currentMaxParticles = 0;

    // Освобождение ресурсов
    void releaseBuffers();
};