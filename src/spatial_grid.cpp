#include "spatial_grid.h"
#include <iostream>

SpatialGrid::SpatialGrid(float cell_size) 
    : cell_size_(cell_size), inv_cell_size_(1.0f / cell_size) 
{
}

void SpatialGrid::set_cell_size(float cs) {
    cell_size_ = cs;
    inv_cell_size_ = 1.0f / cell_size_;
}

// Быстрая хэш-функция
uint32_t SpatialGrid::hash_cell(const Cell& c) const {
    uint32_t hash = (c.x * 73856093) ^ (c.y * 19349663) ^ (c.z * 83492791);
    return hash & mask_;
}

// Перевод координат в индекс ячейки
Cell SpatialGrid::pos_to_cell(float x, float y, float z) const {
    return {
        static_cast<int32_t>(std::floor(x * inv_cell_size_)),
        static_cast<int32_t>(std::floor(y * inv_cell_size_)),
        static_cast<int32_t>(std::floor(z * inv_cell_size_))
    };
}

void SpatialGrid::build(
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<uint8_t>& is_dark
)
{
    int N = static_cast<int>(posX.size());
    if (N == 0) return;

    // Вычисляем размер таблицы (N * 2 с округлением до степени двойки)
    uint32_t target_size = N * 2;
    uint32_t new_capacity = 1;
    while (new_capacity < target_size) new_capacity <<= 1;

    if (new_capacity > capacity_) {
        capacity_ = new_capacity;
        mask_ = capacity_ - 1;
        
        slot_keys_.resize(capacity_);
        slot_begin_.resize(capacity_);
        slot_count_.resize(capacity_);
        indices_.resize(N);
        cursor_.resize(capacity_);
    }

    // Очищаем таблицу
    std::fill(slot_keys_.begin(), slot_keys_.end(), Cell{SENTINEL, SENTINEL, SENTINEL});

    // Считаем количество частиц в каждой ячейке (используя linear probing)
    for (int i = 0; i < N; ++i) {
        // Сразу скипаем тёмную материю
        if (is_dark[i]) continue;

        Cell c = pos_to_cell(posX[i], posY[i], posZ[i]);
        
        uint32_t idx = hash_cell(c);
        while (true) {
            if (slot_keys_[idx].x == SENTINEL) {
                // Нашли пустое место
                slot_keys_[idx] = c;
                slot_begin_[idx] = 0;
                slot_count_[idx] = 1;
                break;
            }
            if (slot_keys_[idx] == c) {
                // Ячейка уже есть — увеличиваем счетчик
                slot_count_[idx]++;
                break;
            }
            idx = (idx + 1) & mask_; // linear probing
        }
    }

    // Вычисляем смещения (куда писать в indices_)
    uint32_t current_offset = 0;
    for (uint32_t i = 0; i < capacity_; ++i) {
        if (slot_keys_[i].x != SENTINEL) {
            slot_begin_[i] = current_offset;
            cursor_[i] = current_offset;
            current_offset += slot_count_[i];
        }
    }

    // Раскладываем индексы частиц по местам
    for (int i = 0; i < N; ++i) {
        // Скипаем тёмную материю
        if (is_dark[i]) continue;

        Cell c = pos_to_cell(posX[i], posY[i], posZ[i]);
        
        uint32_t idx = hash_cell(c);
        while (slot_keys_[idx] != c) {
            idx = (idx + 1) & mask_;
        }
        
        indices_[cursor_[idx]++] = i;
    }
}

void SpatialGrid::query_radius(
    std::vector<int>& out_indices,
    float px, float py, float pz, float R,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ
) const
{
    if (posX.empty() || capacity_ == 0) return;

    // Вычисляем, сколько ячеек задевает наш радиус (d_max)
    int d_max = static_cast<int>(std::ceil(R * inv_cell_size_));
    
    Cell center_cell = pos_to_cell(px, py, pz);
    float R_sq = R * R;

    // Проходим по всем ячейкам в кубе (2 * d_max + 1)^3
    for (int dx = -d_max; dx <= d_max; ++dx) {
        for (int dy = -d_max; dy <= d_max; ++dy) {
            for (int dz = -d_max; dz <= d_max; ++dz) {
                Cell target_cell = { center_cell.x + dx, center_cell.y + dy, center_cell.z + dz };
                
                // Ищем эту ячейку в хэш-таблице
                uint32_t idx = hash_cell(target_cell);
                while (true) {
                    if (slot_keys_[idx].x == SENTINEL) {
                        break; // ячейки нет, выходим
                    }
                    if (slot_keys_[idx] == target_cell) {
                        // Нашли -> Проходим по всем частицам в ней
                        uint32_t begin = slot_begin_[idx];
                        uint32_t count = slot_count_[idx];
                        
                        for (uint32_t k = 0; k < count; ++k) {
                            int p_idx = indices_[begin + k];
                            
                            float ddx = posX[p_idx] - px;
                            float ddy = posY[p_idx] - py;
                            float ddz = posZ[p_idx] - pz;
                            
                            if (ddx*ddx + ddy*ddy + ddz*ddz <= R_sq) {
                                out_indices.push_back(p_idx);
                            }
                        }
                        break;
                    }
                    idx = (idx + 1) & mask_;
                }
            }
        }
    }
}