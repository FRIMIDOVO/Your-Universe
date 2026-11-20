#pragma once
#include <vector>
#include <cstdint>

struct ParticleData {
    // Ускорения
    std::vector<float> accX, accY, accZ;

    // Положения
    std::vector<float> posX, posY, posZ;

    // Скорости
    std::vector<float> velX, velY, velZ;

    // Массы
    std::vector<float> mass;

    // Тип материи
    std::vector<uint8_t> is_dark;


    // Добавление частицы по всем параметрам
    void add_particle(
        float m,
        float px, float py, float pz,
        float vx, float vy, float vz,
        bool dm
    );

    // Очистка всех частиц
    void clear_all();

    // Очистка всей тёмной материи
    void clear_dm();

};  // ParticleData