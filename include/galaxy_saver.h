#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace GalaxySaver {

    // Сохраняет состояние мира в CSV
    void save(
        const std::string& dir,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
        const std::vector<float>& mass, const std::vector<uint8_t>& is_dark
    );

    // Сохранение в json метаданных (время, смещение, матрица поворота)
    void save_meta(
        const std::string& dir,
        const float& time, 
        const float& dpx, const float& dpy, const float& dpz,
        const float& dvx, const float& dvy, const float& dvz,
        const float rot[3][3]
    );

    // Загружает состояние мира из CSV
    // Возвращает время и заполняет переданные массивы
    void load(
        const std::string& dir,
        std::vector<float>& posX, std::vector<float>& posY, std::vector<float>& posZ,
        std::vector<float>& velX, std::vector<float>& velY, std::vector<float>& velZ,
        std::vector<float>& mass, std::vector<uint8_t>& is_dark
    );
    
    // Загрузка метаданных
    void load_meta(
        const std::string& dir,
        float& time, 
        float& dpx, float& dpy, float& dpz,
        float& dvx, float& dvy, float& dvz,
        float rot[3][3]
    );

}