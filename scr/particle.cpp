#include "particle.h"


Particle Particle::random_particle(const SpaceConfig& space) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_real_distribution<float> pos_dist(space.min_bound, space.max_bound);
    std::uniform_real_distribution<float> vel_dist(0.0f, 0.0f);
    std::uniform_real_distribution<float> mass_dist(2.0f, 7.0f);

    Vec2 position{pos_dist(gen), pos_dist(gen)};
    Vec2 velocity{vel_dist(gen), vel_dist(gen)};
    float mass = mass_dist(gen);
    int id = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count() % 1000000000);

    return Particle(id, position, velocity, mass);
}