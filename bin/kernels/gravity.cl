#define TILE_SIZE 128

__kernel void calculate_forces_nbody(
    __global const float* pos_x,       // Координаты частиц по осям X, Y и Z
    __global const float* pos_y,
    __global const float* pos_z,
    __global const float* mass,        // Массы частиц

    __global float* acc_x,             // Результирующие ускорения частиц
    __global float* acc_y,
    __global float* acc_z,

    int N,                             // Общее количество частиц
    float G,                           // Гравитационная постоянная
    float softening_sq                 // Квадрат параметра сглаживания (должен быть > 0)
)
{
    // Векторный тайл в локальной памяти:
    // .xyz — координаты частицы-источника, .w — её масса.
    __local float4 tile[TILE_SIZE];

    int i = get_global_id(0);
    int tid = get_local_id(0);
    bool active = i < N;

    // Координаты текущей частицы в регистрах (в виде float3 для векторных операций)
    float3 pos_i = active ? (float3)(pos_x[i], pos_y[i], pos_z[i]) : (float3)(0.0f);

    // Векторный накопитель ускорения в регистрах
    float3 acc = (float3)(0.0f);

    for (int t = 0; t < N; t += TILE_SIZE)
    {
        int idx = t + tid;

        // Упаковываем скалярные данные из глобальной памяти в один вектор float4.
        // Для несуществующих частиц записываем 0.0f — нулевая масса (.w = 0)
        // автоматически обнулит их гравитационный вклад без проверок if в цикле.
        if (idx < N)
        {
            tile[tid] = (float4)(pos_x[idx], pos_y[idx], pos_z[idx], mass[idx]);
        }
        else
        {
            tile[tid] = (float4)(0.0f);
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        if (active)
        {
            #pragma unroll 8
            for (int j = 0; j < TILE_SIZE; j++)
            {
                float4 p_j = tile[j];

                // Векторная разность координат (обрабатывается за одну инструкцию)
                float3 r = p_j.xyz - pos_i;

                // Скалярное произведение r на себя = dx*dx + dy*dy + dz*dz
                float dist_sq = dot(r, r) + softening_sq;

                float inv_dist = native_rsqrt(dist_sq);
                float inv_dist_cube = inv_dist * inv_dist * inv_dist;

                // p_j.w — это масса частицы j.
                // Если j == i, то r == (0,0,0), и вклад самодействия строго равен 0.
                // Если t + j >= N, то p_j.w == 0, и вклад фиктивной частицы тоже 0.
                float s = p_j.w * inv_dist_cube;

                // Векторное умножение и сложение (FMA — Fused Multiply-Add)
                acc += r * s;
            }
        }

        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // Умножаем на G один раз в конце вместо выполнения этого внутри двойного цикла!
    if (active)
    {
        acc *= G;
        acc_x[i] = acc.x;
        acc_y[i] = acc.y;
        acc_z[i] = acc.z;
    }
}
