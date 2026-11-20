#pragma once
#include <vector>

#include "config.h"

#include "spatial_grid.h"
#include "opencl_manager.h"
#include "nbody_solver.h"
#include "sph_solver.h"

#include "particle_data.h"
#include "particle_buffers.h"


class PhysicsEngine {
public:
    // Конструктор
    PhysicsEngine(PhysicsConfig& physics);

    // Добавление частицы в расчёт
    void add_particle(
        float m,
        float px, float py, float pz,
        float vx, float vy, float vz,
        bool dm
    ) 
    { particle_data.add_particle(
            m,
            px, py, pz,
            vx, vy, vz,
            dm
        );
    }

    // Очистка частиц
    void clear_all() { particle_data.clear_all(); }
    void delete_dm() { particle_data.clear_dm(); }

    // Шаг симуляции
    void step();

    // Время вселенной
    float get_time() const { return time; }
    void set_time(float t) { time = t; }

    // Гетеры информации о частицах
    const std::vector<float>& get_posX() const { return particle_data.posX; }
    const std::vector<float>& get_posY() const { return particle_data.posY; }
    const std::vector<float>& get_posZ() const { return particle_data.posZ; }
    const std::vector<float>& get_velX() const { return particle_data.velX; }
    const std::vector<float>& get_velY() const { return particle_data.velY; }
    const std::vector<float>& get_velZ() const { return particle_data.velZ; }
    const std::vector<float>& get_mass() const { return particle_data.mass; }
    const std::vector<uint8_t>& get_dark() const { return particle_data.is_dark; }

    // Обновление размера хэш сетки
    void update_cell_size() { spatial_grid.set_cell_size(physics.h * 2); }

private:
    // Конфиг с параметрами физики
    PhysicsConfig& physics;
    
    OpenCLManager opencl_manager;
    SpatialGrid spatial_grid;       // хэш сетка
    NBodySolver nbody_solver;       // расчётчик гравитации
    SPHSolver sph_solver;           // расчётчик гидродинамики

    // Всеми частицами владеет только движок
    ParticleData particle_data;
    ParticleBuffers particle_buffers;

    float time = 0.0f;
};