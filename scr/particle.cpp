#include "particle.h"

Particle::Particle(
    int id,
    Vec3 position,
    Vec3 velocity,
    float mass,
    bool dark_matter,
    bool movable
) : id(id),
    position(position),
    velocity(velocity),
    mass(mass),
    r_(std::cbrt(mass) * 0.2f),
    dark_matter(dark_matter),
    movable(movable) {}

Particle Particle::random_particle(
    const SpaceConfig* space,
    std::optional<Vec3> pos,
    std::optional<Vec3> vel,
    std::optional<float> mass,
    bool dark_matter,
    bool movable
) {
    static int id = 0;
    Vec3 final_pos = pos.has_value() ? pos.value() : random_pos(*space);
    Vec3 final_vel = vel.has_value() ? vel.value() : random_vel();
    float final_mass = mass.has_value() ? mass.value() : random_mass();
    return Particle(id++, final_pos, final_vel, final_mass, dark_matter, movable);
}

Vec3 Particle::random_pos(const SpaceConfig& space) {
    /*
    std::uniform_real_distribution<float> dist(space.min_bound, space.max_bound);
    return Vec3(dist(rng()), dist(rng()), dist(rng()));
    */

    Vec3 center{};
    float R = space.max_bound; // радиус шара
    
    // Равномерный радиус: r = R * u^(1/3)
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    float u = u01(rng());
    float r = R * std::cbrt(u);
    
    // Равномерное направление на сфере
    std::normal_distribution<float> normal(0.0f, 1.0f);
    Vec3 dir(normal(rng()), normal(rng()), normal(rng()));
    float len = dir.norm();
    if (len < 1e-8f) {
        dir = Vec3(1.0f, 0.0f, 0.0f);
        len = 1.0f;
    }
    dir = dir / len;
    
    return center + dir * r;
}

Vec3 Particle::random_vel() {
    std::uniform_real_distribution<float> dist(-20.0f, 20.0f);
    return Vec3(dist(rng()), dist(rng()), dist(rng()));
}

float Particle::random_mass() {
    std::uniform_real_distribution<float> dist(0.0009f, 0.0011f);
    return dist(rng());
}

std::mt19937& Particle::rng() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}