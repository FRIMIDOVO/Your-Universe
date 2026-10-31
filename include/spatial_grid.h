#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>

// Координаты ячейки в сетке
struct Cell {
    int32_t x = 0, y = 0, z = 0;
    
    bool operator==(const Cell& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const Cell& o) const { return !(*this == o); }
};

class SpatialGrid {
public:
    explicit SpatialGrid(float cell_size);

    void set_cell_size(float cs);

    // Строит сетку по текущим позициям
    void build(
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<uint8_t>& is_dark // для скипа тёмной материи сразу
    );

    // Радиус поиска
    void query_radius(
        std::vector<int>& out_indices,
        float px, float py, float pz, float R,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ
    ) const;

private:
    float cell_size_;
    float inv_cell_size_;

    // Хэш-таблица (открытая адресация)
    std::vector<Cell>     slot_keys_;
    std::vector<uint32_t> slot_begin_;
    std::vector<uint32_t> slot_count_;
    
    // Плоский массив индексов частиц
    std::vector<uint32_t> indices_;
    std::vector<uint32_t> cursor_;

    uint32_t capacity_ = 0;   // Размер таблицы (степень двойки)
    uint32_t mask_ = 0;       // capacity_ - 1
    
    static constexpr int32_t SENTINEL = INT32_MIN;

    uint32_t hash_cell(const Cell& c) const;
    Cell pos_to_cell(float x, float y, float z) const;
};