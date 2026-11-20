#pragma once
#include <cstdint>
#include "opencl_manager.h"

class ParticleBuffers {
public:
    explicit ParticleBuffers(OpenCLManager& clm);
    ~ParticleBuffers();

    ParticleBuffers(const ParticleBuffers&) = delete;
    ParticleBuffers& operator=(const ParticleBuffers&) = delete;

    // Создать/пересоздать буферы под N
    void init(int maxParticles);

    // Загрузка CPU -> GPU (только те данные, что нужны)
    void upload(
        const std::vector<float>& accX, const std::vector<float>& accY, const std::vector<float>& accZ,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
        const std::vector<float>& mass,
        const std::vector<uint8_t>& is_dark
    );

    // Выгрузка GPU -> CPU (только acc)
    void download_acc(
        std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
        int N
    );

    // Геттеры
    cl_mem posX()   const { return d_posX_; }
    cl_mem posY()   const { return d_posY_; }
    cl_mem posZ()   const { return d_posZ_; }
    cl_mem velX()   const { return d_velX_; }
    cl_mem velY()   const { return d_velY_; }
    cl_mem velZ()   const { return d_velZ_; }
    cl_mem mass()   const { return d_mass_; }
    cl_mem isDark() const { return d_is_dark_; }
    cl_mem accX()   const { return d_accX_; }
    cl_mem accY()   const { return d_accY_; }
    cl_mem accZ()   const { return d_accZ_; }

    int capacity() const { return current_max_; }

private:
    OpenCLManager& clm_;

    cl_mem d_posX_    = nullptr;
    cl_mem d_posY_    = nullptr;
    cl_mem d_posZ_    = nullptr;
    cl_mem d_velX_    = nullptr;
    cl_mem d_velY_    = nullptr;
    cl_mem d_velZ_    = nullptr;
    cl_mem d_mass_    = nullptr;
    cl_mem d_is_dark_ = nullptr;
    cl_mem d_accX_    = nullptr;
    cl_mem d_accY_    = nullptr;
    cl_mem d_accZ_    = nullptr;

    int current_max_ = 0;

    void releaseBuffers();
};