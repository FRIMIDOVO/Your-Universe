#pragma once
#include "config.h"
#include <vector>
#include <array>
#include <cstdint>

struct Box {
    float centerX, centerY, centerZ;
    float half_size;

    // Возвращает индекс октанта, в который попадает частица
    inline int get_octant(const float posX, const float posY, const float posZ) const {
        int octant = 0;
        if (posX >= centerX) octant |= 1;
        if (posY >= centerY) octant |= 2;
        if (posZ >= centerZ) octant |= 4;
        return octant;
    }

    // Возвращает бокс для октанта по индексу
    inline Box get_child_box(const int octant) const {
        float new_half = half_size * 0.5f;
        return {
            centerX + ((octant & 1) ? new_half : -new_half),
            centerY + ((octant & 2) ? new_half : -new_half),
            centerZ + ((octant & 4) ? new_half : -new_half),
            new_half
        };
    }

    // Возвращает квадрат минимальтного расстояния от этого куба до заданной точки (px, py, pz)
    inline float sq_dist(float px, float py, float pz) const {
        // Находим ближайшую к точке (px, py, pz) точку, принадлежащую кубу
        float dx = std::max(centerX - half_size - px, std::max(0.0f, px - (centerX + half_size)));
        float dy = std::max(centerY - half_size - py, std::max(0.0f, py - (centerY + half_size)));
        float dz = std::max(centerZ - half_size - pz, std::max(0.0f, pz - (centerZ + half_size)));
        return dx * dx + dy * dy + dz * dz;
    }
};

struct OctreeNode {
    Box box;
    
    // Координаты центра масс группы частиц
    float comX = 0.0f, comY = 0.0f, comZ = 0.0f;
    float total_mass = 0.0f;
    
    // Индекс первого из 8 дочерних узлов в векторе nodes_ (остальные лежат следом: +1, +2... +7)
    int first_child_idx = -1; 

    int first_particle_idx = 0; // Смещение начала группы в глобальном массиве индексов particle_indices_
    int particle_count = 0;     // Количество частиц в данном узле
    
    // Индекс следующего узла в векторе nodes_ для быстрого линейного обхода без рекурсии
    int next = -1;

    // Является ли узел "листом" дерева
    inline bool is_leaf() const { return first_child_idx == -1; }

    // Пустой ли узел (нет частиц внутри)
    inline bool is_empty() const { return total_mass == 0.0f; }
};

// Вспомогательная структура задачи
struct BuildTask {
    int node_index;
    int start;
    int end;
};

class BarnesHutTree {
public:
    // Построение дерева на основе текущих позиций и масс частиц
    void build(
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& mass,
        const PhysicsConfig& phys_config
    );
    
    // Рассчёт результирующих ускорений для всех частиц + запись в accX/Y/Z
    void compute_all_accelerations(
        std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& mass,
        const PhysicsConfig& phys_config
    ) const;

    // Возвращает индексы частиц в интересующем радиусе
    void query_radius(
        std::vector<int>& out_indices,
        float px, float py, float pz, float R,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<uint8_t>& is_dark
    ) const;

    // Очистка дерева и переиспользуемых векторов (без освобождения выделенной памяти)
    void clear();

private:
    // Плоские массивы данных дерева
    std::vector<OctreeNode> nodes_;         // плоский массив всех узлов
    std::vector<int> particle_indices_;     // плоский массив индексов всех частиц
    std::vector<BuildTask> build_stack_;    // плоский стек задач для итеративного построения
    std::vector<int> temp_partition_;       // вектор - буфер сортировки

    const int LEAF_CAPACITY = 8;

    // Перегруппировка индексов в диапазоне
    std::array<int, 8> sort_into_octants(
        int start, int end,
        const Box& parent_box,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ
    );

    // Считает массы и центры масс узлов (линейный проход снизу вверх с конца массива nodes_)
    void compute_mass_distribution(
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& mass
    );
};