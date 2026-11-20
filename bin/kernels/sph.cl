#define RHO_EPS 1e-12f
#define PI_F 3.14159265358979323846f

inline ulong make_cell_key_z(ulong key_xy, int cz, int offset) {
    return key_xy | ((((ulong)(cz + offset)) & 0x1FFFFFUL) << 42);
}

inline int find_cell(
    __global const ulong* unique_keys,
    int n_uniq,
    ulong target
) {
    int lo = 0;
    int hi = n_uniq - 1;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        if (unique_keys[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }
    return (unique_keys[lo] == target) ? lo : -1;
}

// ============================================================================
// 1. DENSITY (Ядро Poly6 для гладкого расчёта плотности)
// ============================================================================
__kernel void calculate_density_sph(
    __global const float* posX,
    __global const float* posY,
    __global const float* posZ,
    __global const float* mass,
    __global const uchar* is_dark,
    __global float* density,
    const int   N,
    const float h2,
    const float four_inv_pi_h8,
    __global const ulong* unique_keys,
    __global const uint*  sorted_indices,
    __global const int*   cell_start,
    __global const int*   cell_count,
    const int   n_uniq,
    const int   offset,
    const float inv_cell_size
) {
    const int i = get_global_id(0);
    if (i >= N) return;

    if (is_dark[i]) {
        density[i] = 0.0f;
        return;
    }

    const float3 pos_i = (float3)(posX[i], posY[i], posZ[i]);

    const int cx = (int)floor(pos_i.x * inv_cell_size);
    const int cy = (int)floor(pos_i.y * inv_cell_size);
    const int cz = (int)floor(pos_i.z * inv_cell_size);

    float rho = 0.0f;

    for (int dx = -1; dx <= 1; ++dx) {
        const ulong key_x = ((ulong)(cx + dx + offset)) & 0x1FFFFFUL;

        for (int dy = -1; dy <= 1; ++dy) {
            const ulong key_xy = key_x | ((((ulong)(cy + dy + offset)) & 0x1FFFFFUL) << 21);

            for (int dz = -1; dz <= 1; ++dz) {
                const ulong key = make_cell_key_z(key_xy, cz + dz, offset);
                const int ci = find_cell(unique_keys, n_uniq, key);
                if (ci < 0) continue;

                const int start = cell_start[ci];
                const int end   = start + cell_count[ci];

                for (int k = start; k < end; ++k) {
                    const int j = sorted_indices[k];
                    const float mj = mass[j];
                    if (is_dark[j] || mj <= 0.0f) continue;

                    const float3 r = (float3)(posX[j], posY[j], posZ[j]) - pos_i;
                    const float r2 = dot(r, r);

                    if (r2 >= h2) continue;
                    const float d = h2 - r2;
                    rho += mj * d * d * d;
                }
            }
        }
    }

    density[i] = rho * four_inv_pi_h8;
}

// ============================================================================
// 2. PRESSURE (Уравнение состояния; пишем сразу P / rho^2)
// ============================================================================
__kernel void calculate_pressure_sph(
    __global const float* density,
    __global const uchar* is_dark,
    __global float* pressure,
    const int   N,
    const float rest_density,
    const float k_rd_inv_g,
    const float gamma
) {
    const int i = get_global_id(0);
    if (i >= N) return;

    const float rho = density[i];
    if (is_dark[i] || rho <= RHO_EPS) {
        pressure[i] = 0.0f;
        return;
    }

    const float ratio = rho * native_recip(rest_density);
    const float p = fmax(0.0f, k_rd_inv_g * (native_exp2(gamma * native_log2(ratio)) - 1.0f));
    pressure[i] = p * native_recip(rho * rho);
}

// ============================================================================
// 3. FORCES (Ядро Spiky Дебруна против слипания частиц + вязкость Монагана)
// ============================================================================
__kernel void calculate_forces_sph(
    __global const float* posX,
    __global const float* posY,
    __global const float* posZ,
    __global const float* velX,
    __global const float* velY,
    __global const float* velZ,
    __global const float* mass,
    __global const uchar* is_dark,
    __global float* accX,
    __global float* accY,
    __global float* accZ,
    __global const float* density,
    __global const float* pressure,
    const int   N,
    const float h,
    const float h2,
    const float twfour_inv_pi_h8, // Оставлено для совместимости сигнатуры с C++
    const float alpha,
    const float cs,
    __global const ulong* unique_keys,
    __global const uint*  sorted_indices,
    __global const int*   cell_start,
    __global const int*   cell_count,
    const int   n_uniq,
    const int   offset,
    const float inv_cell_size
) {
    const int i = get_global_id(0);
    if (i >= N) return;
    if (is_dark[i]) return;

    const float rho1 = density[i];
    if (rho1 <= RHO_EPS) return;

    const float3 pos_i = (float3)(posX[i], posY[i], posZ[i]);
    const float3 vel_i = (float3)(velX[i], velY[i], velZ[i]);
    const float p_rho2_i = pressure[i];

    // Константа градиента ядра Spiky: 45 / (pi * h^6)
    const float spiky_grad = 45.0f * native_recip(PI_F * h2 * h2 * h2);
    const float two_visc_coeff = -2.0f * alpha * cs * h;
    const float eps_h2 = 0.01f * h2;

    float3 acc = (float3)(0.0f);

    const int cx = (int)floor(pos_i.x * inv_cell_size);
    const int cy = (int)floor(pos_i.y * inv_cell_size);
    const int cz = (int)floor(pos_i.z * inv_cell_size);

    for (int dx = -1; dx <= 1; ++dx) {
        const ulong key_x = ((ulong)(cx + dx + offset)) & 0x1FFFFFUL;

        for (int dy = -1; dy <= 1; ++dy) {
            const ulong key_xy = key_x | ((((ulong)(cy + dy + offset)) & 0x1FFFFFUL) << 21);

            for (int dz = -1; dz <= 1; ++dz) {
                const ulong key = make_cell_key_z(key_xy, cz + dz, offset);
                const int ci = find_cell(unique_keys, n_uniq, key);
                if (ci < 0) continue;

                const int start = cell_start[ci];
                const int end   = start + cell_count[ci];

                for (int k = start; k < end; ++k) {
                    const int j = sorted_indices[k];
                    if (j == i || is_dark[j]) continue;

                    const float rho2 = density[j];
                    const float mj = mass[j];
                    if (rho2 <= RHO_EPS || mj <= 0.0f) continue;

                    const float3 r = pos_i - (float3)(posX[j], posY[j], posZ[j]);
                    const float r2 = dot(r, r);

                    // Защита от деления на ноль при r -> 0
                    if (r2 <= 1e-12f || r2 >= h2) continue;

                    const float inv_r = native_rsqrt(r2);
                    const float dist  = r2 * inv_r;
                    const float diff  = h - dist;

                    // Градиент ядра Spiky: максимальное отталкивание при сближении
                    const float grad_mag = spiky_grad * diff * diff * inv_r;

                    const float3 v_diff = vel_i - (float3)(velX[j], velY[j], velZ[j]);
                    const float dot_prod = dot(v_diff, r);

                    float PI_12 = 0.0f;
                    if (dot_prod < 0.0f) {
                        const float denom = (r2 + eps_h2) * (rho1 + rho2);
                        PI_12 = (two_visc_coeff * dot_prod) * native_recip(denom);
                    }

                    const float sph_factor = p_rho2_i + pressure[j] + PI_12;
                    acc += r * (mj * sph_factor * grad_mag);
                }
            }
        }
    }

    accX[i] += acc.x;
    accY[i] += acc.y;
    accZ[i] += acc.z;
}
