#define MAX_NEIGHBORS 64

inline ulong make_cell_key(int cx, int cy, int cz, int offset) {
    return (((ulong)(cx + offset)) & 0x1FFFFFUL)
         | ((((ulong)(cy + offset)) & 0x1FFFFFUL) << 21)
         | ((((ulong)(cz + offset)) & 0x1FFFFFUL) << 42);
}

__kernel void query_radius(
    __global const float* posX,
    __global const float* posY,
    __global const float* posZ,
    __global const ulong* unique_cell_keys,
    __global const uint*  sorted_indices,
    __global const int*   cell_start,
    __global const int*   cell_count,
    __global int*         out_neighbors, // Теперь транспонированный: [k * N + i]
    __global int*         out_counts,
    const int   N,
    const float R,
    const float inv_cell_size,
    const int   offset,
    const int   num_unique_cells
) {
    int i = get_global_id(0);
    if (i >= N) return;

    // Загружаем позицию один раз в регистры
    float3 pos_i = (float3)(posX[i], posY[i], posZ[i]);

    // floor() для float в OpenCL быстрее приведения через double
    int cx = (int)floor(pos_i.x * inv_cell_size);
    int cy = (int)floor(pos_i.y * inv_cell_size);
    int cz = (int)floor(pos_i.z * inv_cell_size);

    // Если cell_size == R, d_max всегда 1 (константа для развёртки цикла!)
    const int d_max = 1;
    const float R_sq = R * R;
    int count = 0;

    // Предвычисляем X и Y части ключа, чтобы не сдвигать их 27 раз
    for (int dx = -d_max; dx <= d_max; ++dx) {
        ulong key_x = ((ulong)(cx + dx + offset)) & 0x1FFFFFUL;

        for (int dy = -d_max; dy <= d_max; ++dy) {
            ulong key_xy = key_x | ((((ulong)(cy + dy + offset)) & 0x1FFFFFUL) << 21);

            for (int dz = -d_max; dz <= d_max; ++dz) {
                if (count >= MAX_NEIGHBORS) break;

                ulong target_key = key_xy | ((((ulong)(cz + dz + offset)) & 0x1FFFFFUL) << 42);

                // Бинарный поиск без branch-break внутри (меньше дивергенции варпов)
                int lo = 0;
                int hi = num_unique_cells - 1;
                while (lo < hi) {
                    int mid = (lo + hi) >> 1;
                    if (unique_cell_keys[mid] < target_key)
                        lo = mid + 1;
                    else
                        hi = mid;
                }

                if (unique_cell_keys[lo] != target_key) continue;

                int start = cell_start[lo];
                int end   = start + cell_count[lo];

                for (int k = start; k < end; ++k) {
                    int p_idx = sorted_indices[k];
                    if (p_idx == i) continue;

                    float3 r = (float3)(posX[p_idx], posY[p_idx], posZ[p_idx]) - pos_i;
                    if (dot(r, r) <= R_sq) {
                        // Коалесцентная запись! Соседние потоки пишут в соседние адреса памяти
                        out_neighbors[count * N + i] = p_idx;
                        ++count;
                        if (count >= MAX_NEIGHBORS) break;
                    }
                }
            }
        }
    }

    out_counts[i] = count;
}
