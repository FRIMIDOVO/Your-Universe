#include "physics_laws.h"
#include <cmath>
#include <algorithm>
#include "opencl_manager.h"
#include <iostream>
#include <omp.h>


namespace Laws {

void inertia(
    const std::vector<float>& accX, const std::vector<float>& accY, const std::vector<float>& accZ,
    std::vector<float>& posX, std::vector<float>& posY, std::vector<float>& posZ,
    std::vector<float>& velX, std::vector<float>& velY, std::vector<float>& velZ,
    const PhysicsConfig& phys_config
)
{
    for (size_t i = 0; i < accX.size(); ++i) {
        // Применяем ускорение
        const float dt = phys_config.dt;
        velX[i] += accX[i] * dt;
        velY[i] += accY[i] * dt;
        velZ[i] += accZ[i] * dt;

        // Применяем скорость
        posX[i] += velX[i] * dt;
        posY[i] += velY[i] * dt;
        posZ[i] += velZ[i] * dt;
    }
}

// Гравитация на видюхе
void GPU_gravity(
    std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& mass,
    const PhysicsConfig& phys_config, NBodySolver& nbody_solver
)
{
    if (!phys_config.G) return;

    nbody_solver.solve(
        posX, posY, posZ, mass,
        accX, accY, accZ,
        phys_config.G, phys_config.softening_sq
    );
}

void BH_gravity(
    std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& mass,
    const PhysicsConfig& phys_config, BarnesHutTree& bh_tree
)
{
    if (!phys_config.G) return;

    bh_tree.compute_all_accelerations(
        accX, accY, accZ,
        posX, posY, posZ, mass, 
        phys_config
    );
}

// Используем пространственную сетку
void collisions(
    std::vector<float>& posX, std::vector<float>& posY, std::vector<float>& posZ,
    std::vector<float>& velX, std::vector<float>& velY, std::vector<float>& velZ,
    const std::vector<float>& mass, const std::vector<float>& radius, const std::vector<uint8_t>& is_dark,
    const PhysicsConfig& physics, const SpatialGrid& spatial_grid
)
{
    const int N = static_cast<int>(posX.size());
    if (N < 2) return;

    // Коэффициент восстановления.
    const float restitution =
        std::clamp(physics.K_elasticity, 0.0f, 1.0f);

    // Максимальный радиус обычной материи.
    float max_radius = 0.0f;

    #pragma omp parallel for reduction(max:max_radius) schedule(static)
    for (int i = 0; i < N; ++i) {
        if (!is_dark[i] && radius[i] > max_radius)
            max_radius = radius[i];
    }

    if (max_radius <= 0.0f)
        return;

    struct CollisionPair {
        int i, j;
        float nx, ny, nz;
    };

    // Постоянные буферы поиска пар.
    // Сохраняют capacity между вызовами collisions().
    static std::vector<std::vector<CollisionPair>> thread_pairs;
    static std::vector<CollisionPair> pairs;

    const int thread_count = omp_get_max_threads();

    if (thread_pairs.size() < static_cast<size_t>(thread_count))
        thread_pairs.resize(thread_count);

    for (auto& buffer : thread_pairs)
        buffer.clear();

    pairs.clear();

    // ---------------- ПОИСК ПАР ----------------

    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();

        auto& local_pairs = thread_pairs[tid];

        // Переиспользуем буфер соседей в пределах этого потока.
        std::vector<int> neighbors;
        neighbors.reserve(128);

        #pragma omp for schedule(dynamic, 64)
        for (int i = 0; i < N; ++i) {
            if (is_dark[i] || radius[i] <= 0.0f)
                continue;

            const float ri = radius[i];
            const float query_radius = ri + max_radius;

            neighbors.clear();

            spatial_grid.query_radius(
                neighbors,
                posX[i], posY[i], posZ[i],
                query_radius,
                posX, posY, posZ
            );

            for (int j : neighbors) {
                if (j <= i || is_dark[j] || radius[j] <= 0.0f)
                    continue;

                const float dx = posX[j] - posX[i];
                const float dy = posY[j] - posY[i];
                const float dz = posZ[j] - posZ[i];

                const float min_dist = ri + radius[j];
                const float dist_sq =
                    dx * dx + dy * dy + dz * dz;

                if (dist_sq >= min_dist * min_dist)
                    continue;

                float nx, ny, nz;

                if (dist_sq > 1e-12f) {
                    const float inv_dist = 1.0f / std::sqrt(dist_sq);
                    nx = dx * inv_dist;
                    ny = dy * inv_dist;
                    nz = dz * inv_dist;
                } else {
                    // Совпавшие центры: задаём устойчивую нормаль.
                    nx = 1.0f;
                    ny = 0.0f;
                    nz = 0.0f;
                }

                local_pairs.push_back({i, j, nx, ny, nz});
            }
        }
    }

    // ---------------- ОБЪЕДИНЕНИЕ БУФЕРОВ ----------------

    size_t total_pairs = 0;

    for (const auto& buffer : thread_pairs)
        total_pairs += buffer.size();

    if (pairs.capacity() < total_pairs)
        pairs.reserve(total_pairs);

    for (const auto& buffer : thread_pairs)
        pairs.insert(pairs.end(), buffer.begin(), buffer.end());

    if (pairs.empty())
        return;

    // ---------------- РЕШЕНИЕ КОНТАКТОВ ----------------

    constexpr int SOLVER_ITERATIONS = 4;
    constexpr float POSITION_CORRECTION = 0.4f;
    constexpr float PENETRATION_SLOP = 0.001f;

    for (int iteration = 0; iteration < SOLVER_ITERATIONS; ++iteration) {
        for (const auto& p : pairs) {
            const int i = p.i;
            const int j = p.j;

            const float mi = mass[i];
            const float mj = mass[j];

            if (mi <= 0.0f || mj <= 0.0f)
                continue;

            const float inv_mi = 1.0f / mi;
            const float inv_mj = 1.0f / mj;
            const float inv_mass_sum = inv_mi + inv_mj;

            // Относительная скорость вдоль нормали.
            const float rvx = velX[i] - velX[j];
            const float rvy = velY[i] - velY[j];
            const float rvz = velZ[i] - velZ[j];

            const float rel_vel_norm =
                rvx * p.nx +
                rvy * p.ny +
                rvz * p.nz;

            // Импульс только при сближении.
            if (rel_vel_norm > 0.0f) {
                const float impulse =
                    (1.0f + restitution) *
                    rel_vel_norm / inv_mass_sum;

                const float impulse_i = impulse * inv_mi;
                const float impulse_j = impulse * inv_mj;

                velX[i] -= p.nx * impulse_i;
                velY[i] -= p.ny * impulse_i;
                velZ[i] -= p.nz * impulse_i;

                velX[j] += p.nx * impulse_j;
                velY[j] += p.ny * impulse_j;
                velZ[j] += p.nz * impulse_j;
            }
        }
    }

    // ---------------- КОРРЕКЦИЯ ПРОНИКНОВЕНИЙ ----------------

    for (const auto& p : pairs) {
        const int i = p.i;
        const int j = p.j;

        const float mi = mass[i];
        const float mj = mass[j];

        if (mi <= 0.0f || mj <= 0.0f)
            continue;

        const float inv_mi = 1.0f / mi;
        const float inv_mj = 1.0f / mj;
        const float inv_mass_sum = inv_mi + inv_mj;

        const float dx = posX[j] - posX[i];
        const float dy = posY[j] - posY[i];
        const float dz = posZ[j] - posZ[i];

        const float dist_sq = dx * dx + dy * dy + dz * dz;
        const float min_dist = radius[i] + radius[j];

        if (dist_sq >= min_dist * min_dist)
            continue;

        const float dist = std::sqrt(dist_sq);

        const float nx = dist > 1e-6f ? dx / dist : p.nx;
        const float ny = dist > 1e-6f ? dy / dist : p.ny;
        const float nz = dist > 1e-6f ? dz / dist : p.nz;

        const float penetration =
            std::max(min_dist - dist - PENETRATION_SLOP, 0.0f);

        const float correction =
            POSITION_CORRECTION * penetration / inv_mass_sum;

        const float ci = correction * inv_mi;
        const float cj = correction * inv_mj;

        posX[i] -= nx * ci;
        posY[i] -= ny * ci;
        posZ[i] -= nz * ci;

        posX[j] += nx * cj;
        posY[j] += ny * cj;
        posZ[j] += nz * cj;
    }
}

// Эластичный газ
void SPH(
    std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
    const std::vector<float>& mass, const std::vector<uint8_t>& is_dark,
    const PhysicsConfig& physics, SPHSolver& sph_solver
)
{
    // Если газа нет — выходим
    if (posX.empty()) return;

    // Передаем данные в солвер (все вычисления произойдут на видеокарте)
    sph_solver.solve(
        accX, accY, accZ,
        posX, posY, posZ,
        velX, velY, velZ,
        mass, is_dark,
        physics.h, 
        physics.rest_density, 
        physics.K_stiffness, 
        physics.gamma, 
        physics.alpha, 
        physics.cs
    );
}

}