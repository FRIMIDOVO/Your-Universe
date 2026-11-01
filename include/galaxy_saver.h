#pragma once
#include <string>
#include <vector>
#include <utility>
#include <cstdint>

namespace GalaxySaver {

    // Сохраняет состояние мира в CSV
    void save(
        const std::string& dir,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
        const std::vector<float>& mass,
        const std::vector<uint8_t>& is_dark,
        float world_time
    );

    // Загружает состояние мира из CSV
    // Возвращает время и заполняет переданные массивы
    float load(
        const std::string& filename,
        std::vector<float>& posX, std::vector<float>& posY, std::vector<float>& posZ,
        std::vector<float>& velX, std::vector<float>& velY, std::vector<float>& velZ,
        std::vector<float>& mass,
        std::vector<uint8_t>& is_dark
    );
}