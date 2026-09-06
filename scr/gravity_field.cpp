#include "gravity_field.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <array>

BarycentricWeights Triangle::barycentric_weights(const Vec2& p) const {
    Vec2 v0 = p2 - p0;
    Vec2 v1 = p1 - p0;
    Vec2 v2 = p - p0;

    // Знаменатель (2D векторное произведение v0 и v1, равно удвоенной площади треугольника)
    float denom = v0.x * v1.y - v0.y * v1.x;

    // Защита от вырожденного треугольника (точки на одной прямой)
    if (std::abs(denom) < 1e-6f) {
        return {1.0f, 0.0f, 0.0f};
    }

    float inv_denom = 1.0f / denom;

    // Вычисляем веса через векторные произведения
    float w1 = (v0.x * v2.y - v0.y * v2.x) * inv_denom;
    float w2 = (v2.x * v1.y - v2.y * v1.x) * inv_denom;
    float w0 = 1.0f - w1 - w2;

    return {w0, w1, w2};
}

GravityField::GravityField(const SpaceConfig& space, const PhysicsConfig& physics, float hex_R)
    : hex_R(hex_R), physics(physics), space(space) {
    const float SQRT3 = std::sqrt(3.0f);
    const float PI = 3.14159265359f;
    hex_D = SQRT3 * hex_R;
    inv_hex_D = 1.0f / hex_D;
    inv_hex_D_sqrt3 = 1.0f / (hex_D * SQRT3);

    source_coeff = (12.0f * PI / SQRT3) * physics.G;

    // Находим hex-координаты всех 4 углов мира, чтобы гарантированно покрыть его целиком
    Hex h1 = pixel_to_hex(Vec2(space.min_bound, space.min_bound));
    Hex h2 = pixel_to_hex(Vec2(space.max_bound, space.min_bound));
    Hex h3 = pixel_to_hex(Vec2(space.min_bound, space.max_bound));
    Hex h4 = pixel_to_hex(Vec2(space.max_bound, space.max_bound));

    int min_q = std::min({h1.q, h2.q, h3.q, h4.q}) - 10; // Запас 10 ячеек
    int max_q = std::max({h1.q, h2.q, h3.q, h4.q}) + 10;
    int min_r = std::min({h1.r, h2.r, h3.r, h4.r}) - 10;
    int max_r = std::max({h1.r, h2.r, h3.r, h4.r}) + 10;

    offset_q = min_q;
    offset_r = min_r;
    width = max_q - min_q + 1;
    height = max_r - min_r + 1;

    grid.resize(width * height);

    //Инициализация круговой маски
    active.resize(width * height, false);

    // Центр и радиус
    Vec2 center = Vec2(
        (space.min_bound + space.max_bound) * 0.5f,
        (space.min_bound + space.max_bound) * 0.5f
    );
    float half_size = (space.max_bound - space.min_bound) * 0.5f;
    float diagonal = half_size * std::sqrt(2.0f);
    float radius = diagonal + 25 * hex_D; // Запас ячеек
    float radius_sq = radius * radius;

    // Помечаем ячейки внутри круга
    for (int r = 0; r < height; ++r) {
        for (int q = 0; q < width; ++q) {
            Hex hex(q + offset_q, r + offset_r);
            Vec2 pixel = hex_to_pixel(hex);
            float dx = pixel.x - center.x;
            float dy = pixel.y - center.y;
            float dist_sq = dx * dx + dy * dy;
            active[r * width + q] = (dist_sq <= radius_sq);
        }
    }
}

Hex GravityField::pixel_to_hex(const Vec2& pos) const {
    float q = pos.x * inv_hex_D + pos.y * inv_hex_D_sqrt3;
    float r = -2 * pos.y * inv_hex_D_sqrt3;

    float& x = q;
    float& z = r;
    float y = -x -z;

    int rx = round(x);
    int ry = round(y);
    int rz = round(z);

    float dx = abs(rx -x);
    float dy = abs(ry -y);
    float dz = abs(rz -z);

    if (dx > dy && dx > dz) {
        rx = -ry -rz;
    } else if (dy > dx && dy > dz) {
        ry = -rx -rz;
    } else if (dz > dy && dz > dx) {
        rz = -rx - ry;
    }

    return Hex(rx, rz);
}

Vec2 GravityField::hex_to_pixel(const Hex& hex) const {
    float x = hex_D * ((hex.q) + (hex.r) * 0.5f);
    float y = -hex_R * 1.5f * (hex.r);
    return Vec2(x, y);
}

int GravityField::get_index(const Hex& hex) const {
    int array_q = hex.q - offset_q;
    int array_r = hex.r - offset_r;
    if (array_q < 0 || array_q >= width || array_r < 0 || array_r >= height)
        return -1;
    int idx = array_r * width + array_q;
    return active[idx] ? idx : -1;  // Проверяем маску круга
}

void GravityField::clear() {
    for (auto& node : grid) {
        node.mass     = 0.0f;
        node.potential = 0.0f;
    }
}

void GravityField::distribute_mass(const Vec2& pos, float mass) {
    /* Распределение массы методом PIC/CIC через barycentric interpolation */

    Hex host_hex = pixel_to_hex(pos);
    int host_idx = get_index(host_hex);

    if (host_idx == -1) return;

    Vec2 host_center = hex_to_pixel(host_hex);

    // 6 соседей текущего hex
    std::array<Hex, 6> neighbor_hexes;
    std::array<Vec2, 6> neighbor_centers;
    std::array<int, 6> neighbor_indices;

    for (int i = 0; i < 6; ++i) {
        neighbor_hexes[i] = Hex(host_hex.q + dirQ[i], host_hex.r + dirR[i]);
        neighbor_centers[i] = hex_to_pixel(neighbor_hexes[i]);
        neighbor_indices[i] = get_index(neighbor_hexes[i]);
    }

    constexpr float EPSILON = 1e-5f;

    // Ищем треугольник, содержащий pos
    for (int i = 0; i < 6; ++i) {
        int j = (i + 1) % 6;

        // Если одного из соседей нет в grid,
        // этот треугольник использовать нельзя.
        if (neighbor_indices[i] == -1 || neighbor_indices[j] == -1) continue;

        Triangle tri = {
            host_center,
            neighbor_centers[i],
            neighbor_centers[j]
        };

        BarycentricWeights w = tri.barycentric_weights(pos);

        // Проверяем, находится ли точка внутри треугольника
        if (w.w0 < -EPSILON || w.w1 < -EPSILON || w.w2 < -EPSILON) continue;

        // Избавляемся от погрешностей
        w.w0 = std::max(0.0f, w.w0);
        w.w1 = std::max(0.0f, w.w1);
        w.w2 = std::max(0.0f, w.w2);

        float sum_w = w.w0 + w.w1 + w.w2;

        if (sum_w <= EPSILON) continue;

        // Нормализация
        w.w0 /= sum_w;
        w.w1 /= sum_w;
        w.w2 /= sum_w;

        // Распределяем всю массу между тремя центрами.
        grid[host_idx].mass += mass * w.w0;
        grid[neighbor_indices[i]].mass += mass * w.w1;
        grid[neighbor_indices[j]].mass += mass * w.w2;

        // Треугольник найден — заканчиваем.
        return;
    }
    grid[host_idx].mass += mass;
}

void GravityField::relax_potentials(int iterations) {
    /* Расчёт потенциалов в ячейках: решение ур. Пуассона методом релаксации SOR */

    const float omega = 1.9f; // Параметр релаксации SOR
    
    // Граничные ячейки (0 и size-1) НЕ обновляются и остаются равными 0 (условие Дирихле).
    for (int iter = 0; iter < iterations; ++iter) {
        for (int r = 1; r < height - 1; ++r) {
            for (int q = 1; q < width - 1; ++q) {
                int curr_idx = r * width + q;
                Hex current_hex(q + offset_q, r + offset_r);
                
                float sum_neighbors = 0.0f;
                for (int i = 0; i < 6; ++i) {
                    Hex nh(current_hex.q + dirQ[i], current_hex.r + dirR[i]);
                    int n_idx = get_index(nh);
                    if (n_idx != -1) {
                        sum_neighbors += grid[n_idx].potential; // Используем grid для Гаусса-Зейделя (быстрее сходится)
                    }
                }
                
                // Формула Якоби для гексагональной сетки
                float phi_jacobi = (sum_neighbors - source_coeff * grid[curr_idx].mass) / 6.0f;
                
                // SOR шаг релаксации
                float phi_old = grid[curr_idx].potential;
                grid[curr_idx].potential = phi_old + omega * (phi_jacobi - phi_old);
            }
        }
    }
}

Vec2 GravityField::calculate_acceleration(const Vec2& pos) const {
    /* Расчёт ускорения по потенциалам */

    Hex center_hex = pixel_to_hex(pos);
    int center_idx = get_index(center_hex);
    if (center_idx == -1) return Vec2(0.0f, 0.0f);

    float phi_center = grid[center_idx].potential;
    Vec2 total_accel(0.0f, 0.0f);
    Vec2 center_pixel = hex_to_pixel(center_hex);

    // Суммируем вклады от 6 соседей
    for (int i = 0; i < 6; ++i) {
        Hex neighbor_hex(center_hex.q + dirQ[i], center_hex.r + dirR[i]);
        int n_idx = get_index(neighbor_hex);
        
        if (n_idx != -1) {
            float phi_neighbor = grid[n_idx].potential;
            Vec2 neighbor_pixel = hex_to_pixel(neighbor_hex);
            
            // Вектор от центра к соседу
            Vec2 d = neighbor_pixel - center_pixel;
            float len_sq = d.norm_sq();
            
            if (len_sq > 1e-6f) {
                // Вклад в градиент: (phi_neighbor - phi_center) / |d|^2 * d
                float scale = (phi_center - phi_neighbor) / len_sq;

                total_accel.x += d.x * scale;
                total_accel.y += d.y * scale;
            }
        }
    }

    // Ускорение = -∇ф (минус градиент)
    return total_accel;
}

float GravityField::get_potential(const Vec2& pos) const {
    Hex host_hex = pixel_to_hex(pos);

    int host_idx = get_index(host_hex);
    if (host_idx == -1) return 0.0f;

    Vec2 host_center = hex_to_pixel(host_hex);

    // Точка относительно центра host hex
    Vec2 local = pos - host_center;

    // Шесть соседей
    struct Neighbor {
        Hex hex;
        Vec2 center;
        int index;
    };

    std::array<Neighbor, 6> neighbors;

    for (int i = 0; i < 6; ++i) {
        Hex nh(host_hex.q + dirQ[i], host_hex.r + dirR[i]);

        neighbors[i] = {
            nh,
            hex_to_pixel(nh),
            get_index(nh)
        };
    }

    /*
        Hexagon разбивается на 6 треугольников.
        Для каждой позиции внутри hex выбираем один из
        шести треугольников:
        и интерполируем потенциал по barycentric coordinates.
    */

    for (int i = 0; i < 6; ++i) {
        int j = (i + 1) % 6;

        const Neighbor& n0 = neighbors[i];
        const Neighbor& n1 = neighbors[j];

        // Если соседей нет в grid — пропускаем треугольник
        if (n0.index == -1 || n1.index == -1) continue;

        Triangle tri = {
            host_center,
            n0.center,
            n1.center
        };

        BarycentricWeights w = tri.barycentric_weights(pos);

        /*
            Для точки внутри треугольника все barycentric
            координаты должны быть >= 0.

            Небольшой отрицательный вес допускаем из-за
            float-ошибок.
        */
        constexpr float EPSILON = 1e-5f;

        if (w.w0 >= -EPSILON &&
            w.w1 >= -EPSILON &&
            w.w2 >= -EPSILON) {

            // Защита от очень маленьких численных ошибок
            w.w0 = std::max(0.0f, w.w0);
            w.w1 = std::max(0.0f, w.w1);
            w.w2 = std::max(0.0f, w.w2);

            float sum =
                w.w0 + w.w1 + w.w2;

            if (sum <= 1e-6f) {
                return grid[host_idx].potential;
            }

            w.w0 /= sum;
            w.w1 /= sum;
            w.w2 /= sum;

            float phi_center = grid[host_idx].potential;
            float phi_n0 = grid[n0.index].potential;
            float phi_n1 = grid[n1.index].potential;

            return
                w.w0 * phi_center +
                w.w1 * phi_n0 +
                w.w2 * phi_n1;
        }
    }

    /*
        Сюда в нормальной ситуации попадать не должны.

        Но из-за float-погрешности на самой границе hex
        лучше иметь безопасный fallback.
    */
    return grid[host_idx].potential;
}