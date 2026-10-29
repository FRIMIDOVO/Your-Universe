#pragma once
#include <cmath>
#include <random>
#include <optional>
#include "vec3.h"
#include "config.h"

class Particle {
private:
    int id;
    Vec3 position;
    Vec3 velocity;
    float mass;
    float r_;
    bool dark_matter;
    bool movable;

    // SPH
    float density_;
    float pressure_;

public:
    Particle(
        int id,
        Vec3 position,
        Vec3 velocity,
        float mass,
        bool dark_matter = false,
        bool movable = true
    );

    static Particle random_particle(
        const SpaceConfig* space = nullptr,
        std::optional<Vec3> pos = std::nullopt,
        std::optional<Vec3> vel = std::nullopt,
        std::optional<float> mass = std::nullopt,
        bool dark_matter = false,
        bool movable = true
    );

    // Геттеры
    int get_id() const { return id; }
    float get_mass() const { return mass; }
    const Vec3& get_position() const { return position; }
    const Vec3& get_velocity() const { return velocity; }

    // Свойства
    float speed() const { return velocity.norm(); }
    float kinetic_energy() const { return 0.5f * mass * speed() * speed(); }
    float radius() const { return r_; }

    // Движение
    void set_position(const Vec3& new_pos) { position = new_pos; }
    void change_position(const Vec3& delta) { position = position + delta; }
    void set_velocity(const Vec3& new_vel) { velocity = new_vel; }
    void add_velocity(const Vec3& delta) { velocity = velocity + delta; }
    void move(const float& dt) { position = position + velocity * dt; }

    // Рандомайзеры
    static Vec3 random_pos(const SpaceConfig& space);
    static Vec3 random_vel();
    static float random_mass();
    static std::mt19937& rng();

    // Доп свойства
    bool is_movable() const { return movable; }
    bool is_dark_matter() const { return dark_matter; }

    // SPH
    void set_density(float new_density) { density_ = new_density; }
    void set_pressure(float new_pressure) { pressure_ = new_pressure; }
    float get_density() const { return density_; }
    float get_pressure() const { return pressure_; }
    float& density() { return density_; }
    float& pressure() { return pressure_; }
};