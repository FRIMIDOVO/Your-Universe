#include "universe.h"
#include "analyzer.h"
#include <iostream>     // для std::cout
#include <chrono>       // для std::chrono
#include <cstddef>      // для size_t
#include <nlohmann/json.hpp>

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
            pos.x, pos.y, pos.z,
            vel.x, vel.y, vel.z,
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
            pos.x, pos.y, pos.z,
            vel.x, vel.y, vel.z,
            true
        );
    }

    engine.update_cell_size();
}

void Universe::save_cluster(const std::filesystem::path& save) {
    if (save.empty()) return;

    // Сохранение данных частиц в CSV
    GalaxySaver::save(
        save.string(),
        engine.get_posX(), engine.get_posY(), engine.get_posZ(),
        engine.get_velX(), engine.get_velY(), engine.get_velZ(),
        engine.get_mass(), engine.get_radius(), engine.get_dark()
    );

    // Анализ данных для получения параметров нормализации
    Analyzer::prepare_data(
        engine.get_posX(), engine.get_posY(), engine.get_posZ(),
        engine.get_velX(), engine.get_velY(), engine.get_velZ(),
        engine.get_mass(), engine.get_dark()
    );

    GalaxySaver::save_meta(
        save.string(),
        engine.get_time(),
        Analyzer::posDX, Analyzer::posDY, Analyzer::posDZ,
        Analyzer::velDX, Analyzer::velDY, Analyzer::velDZ,
        Analyzer::rot
    );
}

void Universe::load_cluster(
    const std::filesystem::path& save,
    const bool normal,
    const float dpx, const float dpy, const float dpz,
    const float dvx, const float dvy, const float dvz,
    const float rot[3][3]
) {
    if (save.empty() || !std::filesystem::exists(save) || !std::filesystem::is_directory(save)) return;

    // Загрузка
    std::vector<float> posX, posY, posZ, velX, velY, velZ, mass, radius;
    std::vector<std::uint8_t> is_dark;
    GalaxySaver::load((save / "galaxy.csv").string(), posX, posY, posZ, velX, velY, velZ, mass, radius, is_dark);

    // Метаданные
    float meta_time = 0.0f;
    float meta_pos[3] = {0,0,0};
    float meta_vel[3] = {0,0,0};
    float meta_rot[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    GalaxySaver::load_meta(save.string(), meta_time, meta_pos[0], meta_pos[1], meta_pos[2], meta_vel[0], meta_vel[1], meta_vel[2], meta_rot);
    engine.set_time(meta_time);

    // Матрица пользовательского поворота
    const float identity[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    const float (*R)[3] = rot ? rot : identity;

    // Центр вращения позиций (0 если normal, иначе meta_pos)
    const float cx = normal ? 0.0f : meta_pos[0];
    const float cy = normal ? 0.0f : meta_pos[1];
    const float cz = normal ? 0.0f : meta_pos[2];

    // Цикл
    for (size_t i = 0; i < posX.size(); ++i) {
        float x = posX[i], y = posY[i], z = posZ[i];
        float vx = velX[i], vy = velY[i], vz = velZ[i];

        // --- ПОЗИЦИИ ---
        // Нормализация
        if (normal) {
            float dx = x - meta_pos[0];
            float dy = y - meta_pos[1];
            float dz = z - meta_pos[2];
            x = meta_rot[0][0]*dx + meta_rot[0][1]*dy + meta_rot[0][2]*dz;
            y = meta_rot[1][0]*dx + meta_rot[1][1]*dy + meta_rot[1][2]*dz;
            z = meta_rot[2][0]*dx + meta_rot[2][1]*dy + meta_rot[2][2]*dz;
        }

        // Поворот вокруг центра
        float dx_local = x - cx;
        float dy_local = y - cy;
        float dz_local = z - cz;

        float x_rot = R[0][0]*dx_local + R[0][1]*dy_local + R[0][2]*dz_local;
        float y_rot = R[1][0]*dx_local + R[1][1]*dy_local + R[1][2]*dz_local;
        float z_rot = R[2][0]*dx_local + R[2][1]*dy_local + R[2][2]*dz_local;

        float x_final = x_rot + cx + dpx;
        float y_final = y_rot + cy + dpy;
        float z_final = z_rot + cz + dpz;

        // --- СКОРОСТИ ---
        // Нормализация (если normal)
        if (normal) {
            float dvx_m = vx - meta_vel[0];
            float dvy_m = vy - meta_vel[1];
            float dvz_m = vz - meta_vel[2];
            vx = meta_rot[0][0]*dvx_m + meta_rot[0][1]*dvy_m + meta_rot[0][2]*dvz_m;
            vy = meta_rot[1][0]*dvx_m + meta_rot[1][1]*dvy_m + meta_rot[1][2]*dvz_m;
            vz = meta_rot[2][0]*dvx_m + meta_rot[2][1]*dvy_m + meta_rot[2][2]*dvz_m;
        }

        // Поворот скоростей
        float vx_rot = R[0][0]*vx + R[0][1]*vy + R[0][2]*vz;
        float vy_rot = R[1][0]*vx + R[1][1]*vy + R[1][2]*vz;
        float vz_rot = R[2][0]*vx + R[2][1]*vy + R[2][2]*vz;

        // Добавление пользовательского смещения скоростей
        float vx_final = vx_rot + dvx;
        float vy_final = vy_rot + dvy;
        float vz_final = vz_rot + dvz;

        engine.add_particle(mass[i], radius[i],
                            x_final, y_final, z_final,
                            vx_final, vy_final, vz_final,
                            is_dark[i]);
    }
}

void Universe::density_profile(float* bin_r, float* b_density, float* dm_density) const {
    Analyzer::prepare_data(
        engine.get_posX(), engine.get_posY(), engine.get_posZ(),
        engine.get_velX(), engine.get_velY(), engine.get_velZ(),
        engine.get_mass(), engine.get_dark()
    );

    Analyzer::density_profile(
        bin_r, b_density, dm_density,
        engine.get_posX(), engine.get_posY(), engine.get_posZ(),
        engine.get_mass(), engine.get_dark()
    );
}

void Universe::rotation_curve(float* r_b, float* b_tang_vel, float* r_dm, float* dm_tang_vel) const {
    Analyzer::prepare_data(
        engine.get_posX(), engine.get_posY(), engine.get_posZ(),
        engine.get_velX(), engine.get_velY(), engine.get_velZ(),
        engine.get_mass(), engine.get_dark()
    );

    Analyzer::rotation_curve(
        r_b, b_tang_vel, r_dm, dm_tang_vel,
        engine.get_posX(), engine.get_posY(), engine.get_posZ(),
        engine.get_velX(), engine.get_velY(), engine.get_velZ(),
        engine.get_dark()
    );
}