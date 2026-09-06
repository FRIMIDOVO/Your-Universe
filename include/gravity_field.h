#pragma once

#include <vector>
#include <cmath>
#include "vec2.h"
#include "config.h"
#include "hex_grid.h"

struct GravityNode {
    float mass = 0.0f;
    float potential = 0.0f;
};

// Весовые коэффициенты для барицентрической интерполяции
struct BarycentricWeights {
    float w0, w1, w2;
};

// Простой геометрический треугольник
struct Triangle {
    Vec2 p0, p1, p2;

    // Вычисляет барицентрические координаты точки p относительно этого треугольника
    BarycentricWeights barycentric_weights(const Vec2& p) const;
};

class GravityField {
public:
    GravityField(const SpaceConfig& space, const PhysicsConfig& physics, float hex_R = 1.0f);

    void clear();
    void distribute_mass(const Vec2& pos, float mass);  // particle-in-cell (PIC)
    void relax_potentials(int iterations);              // successive over-relaxation
    Vec2 calculate_acceleration(const Vec2& pos) const; // Расчёт ускорения в точке
    float get_potential(const Vec2& pos) const;         // Получить потенциал в точке

    int get_width() const { return width; }
    int get_height() const { return height; }

private:
    int width, height;
    
    float hex_R;
    float hex_D;            // Ширина ячейки
    int offset_q, offset_r; // Смещение, чтобы q и r стали >= 0
    float inv_hex_D;
    float inv_hex_D_sqrt3;

    float source_coeff;     // Физ. коэфициент для релаксации

    const PhysicsConfig& physics;
    const SpaceConfig& space;

    std::vector<GravityNode> grid;
    std::vector<bool> active;       // Активные ячейки


    Hex pixel_to_hex(const Vec2& pos) const;
    Vec2 hex_to_pixel(const Hex& hex) const;
    int get_index(const Hex& hex) const;        // Для получения индекса больше -1

    // Направления 6 соседей для pointy-top гексагонов
    static constexpr int dirQ[6] = {1, 0, -1, -1, 0, 1};
    static constexpr int dirR[6] = {0, 1, 1, 0, -1, -1};
};