#pragma once
#include "opencl_manager.h"

class SPHSolver {
public:
    SPHSolver(OpenCLManager& manager);
    ~SPHSolver();

    // Основной метод: загрузка данных -> расчет -> выгрузка ускорений
    void solve(
        std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
        const std::vector<float>& mass, const std::vector<uint8_t>& is_dark,
        float h, float rest_density, float K_stiffness, float gamma, float alpha, float cs
    );

private:
    OpenCLManager& opencl_manager;

    // Буферы
    cl_mem d_posX = nullptr, d_posY = nullptr, d_posZ = nullptr;
    cl_mem d_velX = nullptr, d_velY = nullptr, d_velZ = nullptr;
    cl_mem d_mass = nullptr, d_is_dark = nullptr;
    cl_mem d_accX = nullptr, d_accY = nullptr, d_accZ = nullptr;
    cl_mem d_density = nullptr, d_pressure = nullptr; // Внутренние буферы для SPH

    int current_capacity = 0;

    void init_buffers(int N);
    void release_buffers();
};