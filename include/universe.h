#pragma once

#include <vector>
#include <utility>      // для std::pair
#include <filesystem>   // для std::filesystem::path
#include <memory>       // для std::unique_ptr

#include "physics.h"
#include "config_loader.h"
#include "opencl_manager.h"
#include "visualizer.h"

class Universe {
public:
    Universe();
    ~Universe() = default;

    void switch_render(bool flag);
    void switch_calculate(bool flag) { physics.calculate = flag; }

    void set_dt(float dt) { physics.dt = dt; }
    void set_theta(float th) { physics.theta = th; }
    void set_elasticity(float el) { physics.K_elasticity = el; }

    const std::vector<float>& get_posX() const { return engine.get_posX(); }
    const std::vector<float>& get_posY() const { return engine.get_posY(); }
    const std::vector<float>& get_posZ() const { return engine.get_posZ(); }
    const std::vector<uint8_t>& get_dark() const { return engine.get_dark(); }

    bool window_should_close() const { return IsWindowReady() && WindowShouldClose(); }

    float get_time() const { return engine.get_time(); }

    // Baryon, Dark
    std::pair<size_t, size_t> get_particle_count() const;

    void engine_reset() { engine.reset(); }
    void engine_step() { if (physics.calculate) engine.step(); }
    void render();

    void spawn_cluster(
        int b_count, float b_mass, float b_radius, float b_spawn_r, float b_omega,
        int dm_count, float dm_mass, float dm_radius, float dm_spawn_r, float dm_omega
    );

    void save_cluster() {
        // Для этого надо переписать GalaxySaver
    }

    void load_cluster(const std::filesystem::path& save = {}) {
        // Для этого надо переписать GalaxySaver
    }

private:
    // Конфиги
    PhysicsConfig physics;
    SpaceConfig space;
    VisualizerConfig viz;

    // Движок
    PhysicsEngine engine;

    // Визуализатор
    std::unique_ptr<Visualizer> visualizer = nullptr;
};