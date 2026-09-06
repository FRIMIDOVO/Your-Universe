#include "visualizer.h"
#include "vec2.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstdint>


Visualizer::Visualizer(const PhysicsEngine& engine, const VisualizerConfig& viz)
    : viz(viz), 
      window(sf::VideoMode(sf::Vector2u(viz.width, viz.height)), "Universe Simulation"),
      engine(engine),
      particles(engine.get_particles()),
      space(engine.get_space()) {
    window.setFramerateLimit(200);
    if (!render_texture.resize(sf::Vector2u(viz.width_texture, viz.height_texture))) {
        std::cerr << "Failed to create render texture!" << std::endl;
    }
    update_view();
}

void Visualizer::update_view() {
    float size = space.max_bound - space.min_bound;
    float center = (space.min_bound + space.max_bound) / 2.0f;

    view.setSize(sf::Vector2f(size, size));
    view.setCenter(sf::Vector2f(center, center));
    render_texture.setView(view);
}

void Visualizer::draw_hex_grid() {
    const HexGrid& hex_grid = engine.get_hex_grid();
    float R = hex_grid.get_hex_radius();
    
    // Получаем ВСЕ активные ячейки (те, в которые записаны частицы)
    std::vector<Hex> active_cells = hex_grid.get_active_cells();
    
    sf::Color line_color(100, 150, 255, 120);  // Полупрозрачный голубой
    
    for (const Hex& hex : active_cells) {
        Vec2 center = hex_grid.hex_to_pixel(hex);
        
        // Вычисляем 6 вершин гекса (pointy-top: 30°, 90°, 150°, 210°, 270°, 330°)
        sf::Vector2f vertices[6];
        for (int i = 0; i < 6; ++i) {
            float angle = 3.14159265f / 6.0f + 3.14159265f / 3.0f * i;
            vertices[i] = sf::Vector2f(
                center.x + R * std::cos(angle),
                center.y + R * std::sin(angle)
            );
        }
        
        // Рисуем 6 линий контура через sf::Lines (толщина ровно 1 пиксель)
        for (int i = 0; i < 6; ++i) {
            sf::Vertex line[2];
            line[0].position = vertices[i];
            line[0].color = line_color;
            line[1].position = vertices[(i + 1) % 6];  // замыкаем контур
            line[1].color = line_color;
            render_texture.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }
}

void Visualizer::draw_gravity_field() {
    const auto& gravity_field = engine.get_gravity_field();
    
    float min_bound = engine.get_space().min_bound;
    float max_bound = engine.get_space().max_bound;
    float world_size = max_bound - min_bound;

    // Размер блока
    const float BLOCK_SIZE = 1.0f; 
    
    int cols = static_cast<int>(std::ceil(world_size / BLOCK_SIZE));
    int rows = static_cast<int>(std::ceil(world_size / BLOCK_SIZE));

    sf::VertexArray va(sf::PrimitiveType::Triangles, cols * rows * 6);
    size_t vertex_idx = 0;

    const float GRADIENT_THRESHOLD = viz.gradient_threshold;
    const float GRADIENT_SCALE = viz.gradient_scale;

    // Цвет поля
    const uint8_t BASE_R = 0;
    const uint8_t BASE_G = 220;
    const uint8_t BASE_B = 255;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            float world_x = min_bound + (x + 0.5f) * BLOCK_SIZE;
            float world_y = min_bound + (y + 0.5f) * BLOCK_SIZE;
            
            // Скип пустых зон для оптимизации
            float phi_center = gravity_field.get_potential(Vec2(world_x, world_y));
            if (std::abs(phi_center) < GRADIENT_THRESHOLD * 0.1f) {
                float x0 = min_bound + x * BLOCK_SIZE;
                float y0 = min_bound + y * BLOCK_SIZE;
                sf::Vector2f p0(x0, y0), p1(x0 + BLOCK_SIZE, y0), p2(x0, y0 + BLOCK_SIZE), p3(x0 + BLOCK_SIZE, y0 + BLOCK_SIZE);
                sf::Color transparent(0, 0, 0, 0);
                va[vertex_idx + 0] = sf::Vertex(p0, transparent);
                va[vertex_idx + 1] = sf::Vertex(p1, transparent);
                va[vertex_idx + 2] = sf::Vertex(p2, transparent);
                va[vertex_idx + 3] = sf::Vertex(p1, transparent);
                va[vertex_idx + 4] = sf::Vertex(p3, transparent);
                va[vertex_idx + 5] = sf::Vertex(p2, transparent);
                vertex_idx += 6;
                continue;
            }
            
            // Считаем градиент только там, где поле реально существует
            float phi_left  = gravity_field.get_potential(Vec2(world_x - BLOCK_SIZE, world_y));
            float phi_right = gravity_field.get_potential(Vec2(world_x + BLOCK_SIZE, world_y));
            float phi_up    = gravity_field.get_potential(Vec2(world_x, world_y - BLOCK_SIZE));
            float phi_down  = gravity_field.get_potential(Vec2(world_x, world_y + BLOCK_SIZE));
            
            float grad_x = (phi_right - phi_left) / (2.0f * BLOCK_SIZE);
            float grad_y = (phi_down - phi_up) / (2.0f * BLOCK_SIZE);
            float grad_magnitude = std::sqrt(grad_x * grad_x + grad_y * grad_y);
            
            // Сырая интенсивность (0.0 ... 1.0)
            float raw_intensity = grad_magnitude * GRADIENT_SCALE;
            raw_intensity = std::clamp(raw_intensity, 0.0f, 1.0f);
            
            // Корень для плавности
            float smooth_factor = std::sqrt(raw_intensity); 
            
            // Слегка приглушаем и яркость цвета к краям для эффекта объёмного свечения
            uint8_t r = static_cast<uint8_t>(BASE_R * smooth_factor);
            uint8_t g = static_cast<uint8_t>(BASE_G * smooth_factor);
            uint8_t b = static_cast<uint8_t>(BASE_B * smooth_factor);
            uint8_t a = static_cast<uint8_t>(255.0f * smooth_factor); 

            sf::Color block_color(r, g, b, a);

            float x0 = min_bound + x * BLOCK_SIZE;
            float y0 = min_bound + y * BLOCK_SIZE;
            sf::Vector2f p0(x0, y0), p1(x0 + BLOCK_SIZE, y0), p2(x0, y0 + BLOCK_SIZE), p3(x0 + BLOCK_SIZE, y0 + BLOCK_SIZE);

            va[vertex_idx + 0] = sf::Vertex(p0, block_color);
            va[vertex_idx + 1] = sf::Vertex(p1, block_color);
            va[vertex_idx + 2] = sf::Vertex(p2, block_color);
            va[vertex_idx + 3] = sf::Vertex(p1, block_color);
            va[vertex_idx + 4] = sf::Vertex(p3, block_color);
            va[vertex_idx + 5] = sf::Vertex(p2, block_color);
            vertex_idx += 6;
        }
    }
    render_texture.draw(va);
}

void Visualizer::draw_particle(const Particle& p) {
    Vec2 pos = p.get_position();
    float radius = p.radius();
    if (radius < 0.05f) radius = 0.05f;
    
    sf::CircleShape circle(radius);
    circle.setPosition(sf::Vector2f(pos.x - radius, pos.y - radius));
    
    float mass = p.get_mass();
    float norm_mass = std::min(1.0f, mass / 7.0f);
    
    sf::Color color(
        static_cast<int>(200 * norm_mass + 50),
        static_cast<int>(100 * (1 - norm_mass)),
        static_cast<int>(255 * (1 - norm_mass))
    );
    circle.setFillColor(color);
    circle.setOutlineThickness(0.f);
    render_texture.draw(circle);
}

void Visualizer::draw_velocity_vector(const Particle& p) {
    Vec2 pos = p.get_position();
    Vec2 vel = p.get_velocity();
    float speed = p.speed();

    if (speed < 0.01f) return; 
    
    // ДЛИНА ВЕКТОРА
    Vec2 dir = vel / speed;
    float fixed_length = viz.vector_scale;
    Vec2 end = pos + dir * fixed_length;
    
    // ЦВЕТ
    float t = std::min(speed / viz.max_speed_color, 1.0f);
    
    std::uint8_t r = static_cast<std::uint8_t>(255.0f * t);
    std::uint8_t g = 0;
    std::uint8_t b = static_cast<std::uint8_t>(255.0f * (1.0f - t));
    sf::Color vector_color(r, g, b);
    
    // Основная линия
    sf::Vertex line[2];
    line[0].position = sf::Vector2f(pos.x, pos.y);
    line[0].color = vector_color;
    line[1].position = sf::Vector2f(end.x, end.y);
    line[1].color = vector_color;
    
    render_texture.draw(line, 2, sf::PrimitiveType::Lines);
    
    // Стрелочка на конце вектора
    float angle = std::atan2(vel.y, vel.x);
    
    // РАЗМЕР СТРЕЛКИ
    const float ARROW_SIZE = viz.arrow_size;
    const float ARROW_ANGLE = viz.arrow_angle;
    
    Vec2 arrow1 = end - Vec2(std::cos(angle - ARROW_ANGLE), std::sin(angle - ARROW_ANGLE)) * ARROW_SIZE;
    Vec2 arrow2 = end - Vec2(std::cos(angle + ARROW_ANGLE), std::sin(angle + ARROW_ANGLE)) * ARROW_SIZE;
    
    sf::Vertex arrow[4];
    arrow[0].position = sf::Vector2f(arrow1.x, arrow1.y);
    arrow[0].color = vector_color;
    arrow[1].position = sf::Vector2f(end.x, end.y);
    arrow[1].color = vector_color;
    
    arrow[2].position = sf::Vector2f(end.x, end.y);
    arrow[2].color = vector_color;
    arrow[3].position = sf::Vector2f(arrow2.x, arrow2.y);
    arrow[3].color = vector_color;
    
    render_texture.draw(arrow, 4, sf::PrimitiveType::Lines);
}

void Visualizer::handle_events() {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
    }
}

void Visualizer::draw_all() {
    if (viz.show_gravity_field) draw_gravity_field();
    if (viz.show_hex_cells) draw_hex_grid();
    for (const auto& p : particles) { 
        draw_particle(p);
        if (viz.show_velocity_vec) draw_velocity_vector(p);
    }
}

void Visualizer::render() {
    render_texture.clear(sf::Color(20, 20, 46));
    draw_all();
    render_texture.display();

    window.clear();
    sf::Sprite sprite(render_texture.getTexture());
    
    sf::Vector2u texSize = render_texture.getSize();
    sf::Vector2u winSize = window.getSize();
    float scale = std::min(
        static_cast<float>(winSize.x) / texSize.x,
        static_cast<float>(winSize.y) / texSize.y
    );
    
    sprite.setScale({scale, scale});  // Vector2f
    sprite.setPosition({
        (winSize.x - texSize.x * scale) / 2.0f,
        (winSize.y - texSize.y * scale) / 2.0f
    });
    
    window.draw(sprite);
    window.display();

    handle_events();
}