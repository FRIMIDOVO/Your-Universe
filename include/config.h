#pragma once

#define VISUALIZER_FIELDS \
    FIELD(render) \
    FIELD(width) \
    FIELD(height)

#define SPACE_FIELDS \
    FIELD(bounds) \
    FIELD(min_bound) \
    FIELD(max_bound)

#define PHYSICS_FIELDS \
    FIELD(calculate) \
    FIELD(record) \
    FIELD(theta) \
    FIELD(softening) \
    FIELD(G) \
    FIELD(dt) \
    FIELD(K_elasticity) \
    FIELD(K_stiffness) \
    FIELD(h) \
    FIELD(rest_density) \
    FIELD(gamma) \
    FIELD(alpha) \
    FIELD(cs)


struct VisualizerConfig {
    bool render = true;
    int width = 800;
    int height = 800;
};

struct SpaceConfig {
    bool bounds = false;
    float min_bound = -15.0f;
    float max_bound = 15.0f;
};

struct PhysicsConfig {
    PhysicsConfig() { update(); }

    // Базовые параметры
    bool calculate = true;
    bool record = true;
    float theta = 0.0f;
    float softening = 1.0f;
    float softening_sq;
    float G = 1.0;
    float dt = 0.005f;

    // Упругость
    float K_elasticity = 0.5f;

    // SPH
    float K_stiffness = 1.0f;
    float h = 0.1f;
    float rest_density = 0.001;
    float gamma = 1.33f;
    float alpha = 5.0f;
    float cs = 1.0f;

    float h2, h8;

    void update() {
        h2 = h * h;
        h8 = h2 * h2 * h2 * h2;

        softening_sq = softening * softening;
    }
};