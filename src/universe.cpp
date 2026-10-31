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
    // Подгрузка параметров конфигов из фйлов
    ConfigLoader::load("configs/space_config.txt", space);
    ConfigLoader::load("configs/physics_config.txt", physics);
    ConfigLoader::load("configs/viz_config.txt", viz);

    if (viz.render) visualizer = std::make_unique<Visualizer>(1'000'000);
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

void Universe::calculate_cycle(int frames) {
        // Info
        std::cout << "Particles: " << engine.get_posX().size() << "\n";

        // === Main cycle ===
        int frame = 1;      // номер кадра

        // Счётчики времени
        auto t0 = std::chrono::high_resolution_clock::now();
        auto t1 = std::chrono::high_resolution_clock::now();
        auto t2 = std::chrono::high_resolution_clock::now();
        auto to_ms = [](auto start, auto end) { return std::chrono::duration<double, std::milli>(end - start).count(); };

        while (frame != frames) {
            // Тут думаю вызывать доп методы этого класса для взаимодействия с пользователем
            /*
            */

            // Start time
            t0 = std::chrono::high_resolution_clock::now();
            
            // Physics step
            engine_step();
            t1 = std::chrono::high_resolution_clock::now();

            // Rendering
            render();
            t2 = std::chrono::high_resolution_clock::now();

            // Time cout
            std::cout 
                << "Frame: "  << frame << " | "
                << "Physics: "  << to_ms(t0, t1) << " ms | " 
                << "Render: "   << to_ms(t1, t2) << " ms | "
                << "Total: "    << to_ms(t0, t2) << " ms" 
            << std::endl;

            ++frame;
        }

        // После расчётного цикла
        std::cout << "Calculated " << frame << " frames." << std::endl;
        
        std::cin.get();
        std::cin.get();
    }