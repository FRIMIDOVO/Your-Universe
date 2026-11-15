#pragma once
#include <vector>
#include <cstdint>

namespace Analyzer {
    // Смещение
    inline float posDX, posDY, posDZ;   // центра масс
    inline float velDX, velDY, velDZ;   // скорости

    // Матрица поворота
    inline float rot[3][3] = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };

    // Подготовка данных (расчёт смещений и матрицы)
    void prepare_data(
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
        const std::vector<float>& mass, const std::vector<std::uint8_t>& is_dark
    );

    // Расчёт профиля плотности для барионной и тёмной материй
    void density_profile(
        float* bin_r, float* b_density, float* dm_density,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& mass, const std::vector<std::uint8_t>& is_dark
    );

    // Расчёт распределения тангенсальных скоростей
    void rotation_curve(
        float* r_b, float* b_tang_vel, float* r_dm, float* dm_tang_vel,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
        const std::vector<std::uint8_t>& is_dark
    );

}