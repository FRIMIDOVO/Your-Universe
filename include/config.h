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
    FIELD(show_hex_cells) \
    FIELD(show_gravity_field) \
    FIELD(gradient_threshold) \
    FIELD(gradient_scale)

#define SPACE_FIELDS \
    FIELD(min_bound) \
    FIELD(max_bound)

#define PHYSICS_FIELDS \
    FIELD(G) \
    FIELD(dt) \
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
    float max_speed_color = 25.0f;
    float arrow_size = 0.2f;
    float arrow_angle = 0.4f;
    bool show_hex_cells = true;
    bool show_gravity_field = true;
    float gradient_threshold = 0.01f;
    float gradient_scale = 0.01f;
};

struct SpaceConfig {
    float min_bound = -100.0f;
    float max_bound = 100.0f;
};

struct PhysicsConfig {
    // Базовые параметры
    float G = 10.0;
    float dt = 0.001f;

    // Упругость
    float K_elasticity = 0.5f;

    // Липкость
    float F_stickiness = 1.0f;
    
    // Трение
    float K_friction = 0.5f;
    float contact_distance = 1.01f;
};