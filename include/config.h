#pragma once


struct VisualizerConfig {
    // Вкл/выкл рендер
    bool render = true;

    // Дефолтные размеры окна
    int width = 800;
    int height = 800;
};

struct PhysicsConfig {
    PhysicsConfig() { update(); }   // Апдейт при создании, что бы все поля заполнились

    // Вкл/выкл расчёт физики
    bool calculate = true;

    // Дельта время
    float dt = 0.005f;

    // Гравитация
    float softening = 0.1f;
    float softening_sq;
    float G = 1.0;

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