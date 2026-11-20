#include "spatial_grid.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <iostream>
#include <chrono>

#include <parallel/algorithm>

SpatialGrid::SpatialGrid(OpenCLManager& opencl_manager)
    : cl_manager_(opencl_manager) {}

SpatialGrid::~SpatialGrid() {
    releaseBuffers();
}

void SpatialGrid::set_cell_size(float cs) {
    if (cs <= 0.0f) throw std::runtime_error("SpatialGrid: cell_size must be > 0");
    cell_size_ = cs;
    inv_cell_size_ = 1.0f / cs;
}

void SpatialGrid::releaseBuffers() {
    auto rel = [](cl_mem& m) {
        if (m) { clReleaseMemObject(m); m = nullptr; }
    };
    rel(d_unique_keys_);
    rel(d_sorted_indices_);
    rel(d_cell_start_);
    rel(d_cell_count_);
    current_max_particles_ = 0;
}

void SpatialGrid::init(int maxParticles) {
    if (maxParticles <= current_max_particles_) return;

    clFinish(cl_manager_.getQueue());
    releaseBuffers();

    current_max_particles_ = maxParticles;

    host_keys_.resize(maxParticles);
    host_unique_keys_.resize(maxParticles);
    host_start_.resize(maxParticles);
    host_count_.resize(maxParticles);

    order_.resize(maxParticles);
    sorted_keys_.resize(maxParticles);
    sorted_idx_.resize(maxParticles);

    d_unique_keys_    = cl_manager_.createBuffer(maxParticles * sizeof(uint64_t), CL_MEM_READ_WRITE);
    d_sorted_indices_ = cl_manager_.createBuffer(maxParticles * sizeof(uint32_t), CL_MEM_READ_WRITE);
    d_cell_start_     = cl_manager_.createBuffer(maxParticles * sizeof(int),      CL_MEM_READ_WRITE);
    d_cell_count_     = cl_manager_.createBuffer(maxParticles * sizeof(int),      CL_MEM_READ_WRITE);
}

uint64_t SpatialGrid::make_key(int cx, int cy, int cz) const {
    return ((uint64_t)(cx + offset_) & 0x1FFFFF)
         | (((uint64_t)(cy + offset_) & 0x1FFFFF) << 21)
         | (((uint64_t)(cz + offset_) & 0x1FFFFF) << 42);
}

void SpatialGrid::build(
    const std::vector<float>& posX,
    const std::vector<float>& posY,
    const std::vector<float>& posZ,
    const std::vector<uint8_t>& is_dark
) {
    int N = (int)posX.size();
    if (N == 0 || posY.size() != (size_t)N || posZ.size() != (size_t)N
              || is_dark.size() != (size_t)N) {
        num_unique_cells_ = 0;
        return;
    }

    if (N > current_max_particles_) init(N);

    // Ключи
    for (int i = 0; i < N; ++i) {
        if (is_dark[i]) {
            host_keys_[i] = UINT64_MAX;
        } else {
            int cx = (int)std::floor(posX[i] * inv_cell_size_);
            int cy = (int)std::floor(posY[i] * inv_cell_size_);
            int cz = (int)std::floor(posZ[i] * inv_cell_size_);
            host_keys_[i] = make_key(cx, cy, cz);
        }
    }

    // iota + sort
    std::iota(order_.begin(), order_.begin() + N, 0u);

    __gnu_parallel::sort(order_.begin(), order_.begin() + N,
        [&](uint32_t a, uint32_t b) { return host_keys_[a] < host_keys_[b]; });

    // Перестановка
    for (int i = 0; i < N; ++i) {
        sorted_keys_[i] = host_keys_[order_[i]];
        sorted_idx_[i]  = order_[i];
    }

    // Уникальные ключи + start/count
    int unique_count   = 0;
    uint64_t cur_key   = UINT64_MAX;
    int cur_start      = 0;
    int last_valid_idx = N;

    for (int i = 0; i < N; ++i) {
        if (sorted_keys_[i] == UINT64_MAX) {
            last_valid_idx = i;
            break;
        }
        if (sorted_keys_[i] != cur_key) {
            if (cur_key != UINT64_MAX)
                host_count_[unique_count - 1] = i - cur_start;
            cur_key = sorted_keys_[i];
            cur_start = i;
            host_unique_keys_[unique_count] = cur_key;
            host_start_[unique_count] = i;
            ++unique_count;
        }
    }
    if (cur_key != UINT64_MAX)
        host_count_[unique_count - 1] = last_valid_idx - cur_start;

    num_unique_cells_ = unique_count;

    // Загрузка на GPU
    cl_manager_.writeBufferAsync(d_sorted_indices_, sorted_idx_.data(), N * sizeof(uint32_t));

    if (unique_count > 0) {
        cl_manager_.writeBufferAsync(d_unique_keys_, host_unique_keys_.data(), unique_count * sizeof(uint64_t));
        cl_manager_.writeBufferAsync(d_cell_start_,  host_start_.data(),      unique_count * sizeof(int));
        cl_manager_.writeBufferAsync(d_cell_count_,  host_count_.data(),      unique_count * sizeof(int));
    }
}