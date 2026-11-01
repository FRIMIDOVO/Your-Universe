#include "universe.h"
#include <iostream>     // для std::cout
#include <chrono>       // для std::chrono
#include <cstddef>      // для size_t

#include <random>
#include <cmath>

namespace {
    std::mt19937 rng(std::random_device{}());

    float random_float(float min, float max) {
        return std::uniform_real_distribution<float>(min, max)(rng);
    }

    Vec3 random_position(float radius) {
        // Равномерное распределение по объёму сферы
        float cosTheta = random_float(-1.0f, 1.0f);
        float phi = random_float(0.0f, 2.0f * 3.14159265f);
        float r = radius * std::cbrt(random_float(0.0f, 1.0f));

        float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

        return Vec3(
            r * sinTheta * std::cos(phi),
            r * sinTheta * std::sin(phi),
            r * cosTheta
        );
    }

    Vec3 random_velocity(const Vec3& pos, float omega, float temperature) {
        // Вращение вокруг оси Z
        Vec3 velocity(
            -omega * pos.y,
             omega * pos.x,
             0.0f
        );

        // Случайная составляющая скорости
        velocity.x += random_float(-temperature, temperature);
        velocity.y += random_float(-temperature, temperature);
        velocity.z += random_float(-temperature, temperature);

        return velocity;
    }
}


Universe::Universe() :
    physics(),
    space(),
    viz(),
    engine(physics, space)
{
}

void Universe::switch_render(bool flag) {
    if (flag && visualizer == nullptr) {
        visualizer = std::make_unique<Visualizer>(1'000'000);
    } else if (!flag && visualizer != nullptr) {
        visualizer = nullptr;
    }
    
    viz.render = flag;
}

std::pair<size_t, size_t> Universe::get_particle_count() const {
    std::pair<size_t, size_t> p_count = {0, 0};

    for (uint8_t dm : engine.get_dark()) {
        if (dm) ++p_count.second;
        else    ++p_count.first;
    }

    return p_count;
}

void Universe::render() {
    /* 
    // Пока нет сигнала gui на корректное отображение
    // Закрываем окно, если требуется
    if (WindowShouldClose()) {
        switch_render(false);
    }
    */

    // Рендерим, если есть отрисовщик
    if (viz.render && visualizer != nullptr) visualizer->draw(
        engine.get_posX(), engine.get_posY(), engine.get_posZ(),
        engine.get_dark()
    );
}

void Universe::spawn_cluster(
    int b_count, float b_mass, float b_radius, float b_spawn_r, float b_omega,
    int dm_count, float dm_mass, float dm_radius, float dm_spawn_r, float dm_omega
)
{
    // Барионная материя
    for (int i = 0; i < b_count; ++i) {
        Vec3 pos = random_position(b_spawn_r);
        Vec3 vel = random_velocity(pos, b_omega, b_omega * 0.05f);

        engine.add_particle(
            b_mass,
            b_radius,
            pos,
            vel,
            false
        );
    }

    // Тёмная материя
    for (int i = 0; i < dm_count; ++i) {
        Vec3 pos = random_position(dm_spawn_r);
        Vec3 vel = random_velocity(pos, dm_omega, dm_omega * 0.05f);

        engine.add_particle(
            dm_mass,
            dm_radius,
            pos,
            vel,
            true
        );
    }

    engine.update_cell_size();
}