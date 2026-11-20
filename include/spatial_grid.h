#pragma once
#include <vector>
#include <cstdint>
#include "opencl_manager.h"

class SpatialGrid {
public:
    explicit SpatialGrid(OpenCLManager& opencl_manager);
    ~SpatialGrid();

    SpatialGrid(const SpatialGrid&) = delete;
    SpatialGrid& operator=(const SpatialGrid&) = delete;

    void set_cell_size(float cs);

    void build(
        const std::vector<float>& posX,
        const std::vector<float>& posY,
        const std::vector<float>& posZ,
        const std::vector<uint8_t>& is_dark
    );

    cl_mem getUniqueKeys()     const { return d_unique_keys_; }
    cl_mem getSortedIndices()  const { return d_sorted_indices_; }
    cl_mem getCellStart()      const { return d_cell_start_; }
    cl_mem getCellCount()      const { return d_cell_count_; }

    int   getNumUniqueCells() const { return num_unique_cells_; }
    int   getOffset()         const { return offset_; }
    float getInvCellSize()    const { return inv_cell_size_; }

private:
    OpenCLManager& cl_manager_;

    cl_mem d_unique_keys_    = nullptr;
    cl_mem d_sorted_indices_ = nullptr;
    cl_mem d_cell_start_     = nullptr;
    cl_mem d_cell_count_     = nullptr;

    // Переиспользуемые CPU-буферы
    std::vector<uint64_t> host_keys_;
    std::vector<uint64_t> host_unique_keys_;
    std::vector<int>      host_start_;
    std::vector<int>      host_count_;

    // Переиспользуемые буферы сортировки
    std::vector<uint32_t> order_;
    std::vector<uint64_t> sorted_keys_;
    std::vector<uint32_t> sorted_idx_;

    int   current_max_particles_ = 0;
    int   num_unique_cells_      = 0;
    int   offset_                = 1 << 20;
    float cell_size_             = 1.0f;
    float inv_cell_size_         = 1.0f;

    void init(int maxParticles);
    void releaseBuffers();
    uint64_t make_key(int cx, int cy, int cz) const;
};