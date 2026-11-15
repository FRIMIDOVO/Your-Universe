#pragma once
#include <vector>
#include "vec3.h"
#include "config.h"
#include "spatial_grid.h"
#include "opencl_manager.h"
#include "nbody_solver.h"
#include "sph_solver.h"
#include "barnes_hut.h"


class PhysicsEngine {
public:
    // Конструктор
    PhysicsEngine(
        PhysicsConfig& physics, 
        SpaceConfig& space
    );

    // Добавление частицы в расчёт
    void add_particle(
        float m,
        float r,
        float px, float py, float pz,
        float vx, float vy, float vz,
        bool dm
    );

    // Очищает все массивы и сбрасывает время
    void reset();

    // Доп управление массивами
    void delete_dm();

    // Шаг симуляции
    void step();

    // Геттеры
    const SpaceConfig& get_space() const { return space; }

    // Инфо
    const float get_time() const { return time; }
    void set_time(float t) { time = t; }

    // Гетеры массивов
    const std::vector<float>& get_posX() const { return posX; }
    const std::vector<float>& get_posY() const { return posY; }
    const std::vector<float>& get_posZ() const { return posZ; }
    const std::vector<float>& get_velX() const { return velX; }
    const std::vector<float>& get_velY() const { return velY; }
    const std::vector<float>& get_velZ() const { return velZ; }
    const std::vector<float>& get_mass() const { return mass; }
    const std::vector<float>& get_radius() const { return radius; }
    const std::vector<uint8_t>& get_dark() const { return is_dark; }

    // Обновление размера хэш сетки
    void update_cell_size();

private:
    // Конфиги
    PhysicsConfig& physics;
    SpaceConfig& space;
    
    OpenCLManager opencl_manager;
    SpatialGrid spatial_grid;
    NBodySolver nbody_solver;
    SPHSolver sph_solver;
    BarnesHutTree bh_tree;

    // Одномерные массивы
    std::vector<float> accX, accY, accZ;
    std::vector<float> posX, posY, posZ;
    std::vector<float> velX, velY, velZ;
    std::vector<float> mass;
    std::vector<float> radius;
    std::vector<uint8_t> is_dark;

    float time = 0.0f;
};