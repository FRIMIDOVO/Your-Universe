#define TILE_SIZE 256 // Должен совпадать с local_size в C++ коде

__kernel void calculate_forces_nbody(
    __global const float* pos_x,
    __global const float* pos_y,
    __global const float* pos_z,
    __global const float* mass,
    __global float* acc_x,
    __global float* acc_y,
    __global float* acc_z,
    int N,
    float G,
    float softening_sq)
{
    // Локальная память для тайлинга
    __local float tile_px[TILE_SIZE];
    __local float tile_py[TILE_SIZE];
    __local float tile_pz[TILE_SIZE];
    __local float tile_m[TILE_SIZE];

    int i = get_global_id(0);
    if (i >= N) return;

    float xi = pos_x[i];
    float yi = pos_y[i];
    float zi = pos_z[i];
    
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;

    // Проходим по всем тайлам
    for (int t = 0; t < N; t += TILE_SIZE) {
        // Загружаем блок данных
        int tid = get_local_id(0);
        int idx = t + tid;
        
        if (idx < N) {
            tile_px[tid] = pos_x[idx];
            tile_py[tid] = pos_y[idx];
            tile_pz[tid] = pos_z[idx];
            tile_m[tid] = mass[idx];
        } else {
            // Заполняем нулями, чтобы не читать за границы
            tile_px[tid] = 0.0f;
            tile_py[tid] = 0.0f;
            tile_pz[tid] = 0.0f;
            tile_m[tid] = 0.0f;
        }
        
        // Синхронизация: ждем, пока потоки группы загрузят данные
        barrier(CLK_LOCAL_MEM_FENCE);
        
        // Считаем силы с частицами из текущего тайла
        #pragma unroll 8 // Развертывание цикла для скорости
        for (int j = 0; j < TILE_SIZE; j++) {
            int global_j = t + j;
            if (global_j >= N || global_j == i) continue;
            
            float dx = tile_px[j] - xi;
            float dy = tile_py[j] - yi;
            float dz = tile_pz[j] - zi;
            
            // Расстояние
            float dist_sq = dx*dx + dy*dy + dz*dz + softening_sq;
            float inv_dist = native_rsqrt(dist_sq);
            float f = G * tile_m[j] * inv_dist * inv_dist * inv_dist;
            
            ax += dx * f;
            ay += dy * f;
            az += dz * f;
        }
        
        // Синхронизация перед загрузкой следующего тайла
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    acc_x[i] = ax;
    acc_y[i] = ay;
    acc_z[i] = az;
}