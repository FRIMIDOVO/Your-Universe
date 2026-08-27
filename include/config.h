#pragma once

#define VISUALIZER_FIELDS \
    FIELD(record) \
    FIELD(width) \
    FIELD(height) \
    FIELD(width_texture) \
    FIELD(height_texture) \
    FIELD(show_velocity_vec) \
    FIELD(vector_scale) \
    FIELD(max_speed_color) \
    FIELD(arrow_size) \
    FIELD(arrow_angle) \
    FIELD(show_hex_cells)

#define SPACE_FIELDS \
    FIELD(min_bound) \
    FIELD(max_bound)

#define PHYSICS_FIELDS \
    FIELD(G) \
    FIELD(gravity_degree) \
    FIELD(dt) \
    FIELD(softening) \
    FIELD(K_elasticity) \
    FIELD(F_stickiness) \
    FIELD(K_friction) \
    FIELD(contact_distance)


struct VisualizerConfig {
    bool record = false;
    int width = 800;
    int height = 800;
    int width_texture = 800;
    int height_texture = 800;
    bool show_velocity_vec = true;
    float vector_scale = 0.5f;
    float max_speed_color = 10.0f;
    float arrow_size = 0.2f;
    float arrow_angle = 0.4f;
    bool show_hex_cells = true;
};

struct SpaceConfig {
    float min_bound = -10.0f;
    float max_bound = 10.0f;
};

struct PhysicsConfig {
    // Базовые параметры
    float G = 1.0;
    float gravity_degree = 2.0f;
    float dt = 0.001f;
    float softening = 0.001f;

    // Упругость
    float K_elasticity = 0.5f;

    // Липкость
    float F_stickiness = 1.0f;
    
    // Трение
    float K_friction = 0.5f;
    float contact_distance = 1.01f;
};