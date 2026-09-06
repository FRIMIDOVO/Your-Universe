#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include "particle.h"
#include "config.h"
#include "physics.h"


class Visualizer {
public:
    Visualizer(const PhysicsEngine& engine, const VisualizerConfig& viz);
    ~Visualizer() = default; // уточняем дефолт деструктор т.к. используем сторонние ресурсы(окно)
    
    // Запуск, рендер и тп
    void render();

    // Геттеры
    sf::RenderTexture& get_texture() { return render_texture; }

    // Состояние
    bool is_open() const { return window.isOpen(); }

private:
    sf::RenderWindow window;
    sf::RenderTexture render_texture;
    sf::View view;
    
    const PhysicsEngine& engine;
    const std::vector<Particle>& particles;
    const SpaceConfig& space;
    const VisualizerConfig& viz;
    
    void draw_all();
    void draw_velocity_vector(const Particle& p);
    void draw_particle(const Particle& p);
    void draw_hex_grid();
    void draw_gravity_field();
    void handle_events();
    void update_view();
};