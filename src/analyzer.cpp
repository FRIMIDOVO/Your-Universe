#include "analyzer.h"
#include <cmath>

namespace Analyzer
{

void prepare_data(
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
    const std::vector<float>& mass, const std::vector<std::uint8_t>& is_dark
)
{
    int N = static_cast<int>(posX.size());
    if (N == 0) return;

    // Итеративный поиск центра (Только барионы)
    float cx = 0, cy = 0, cz = 0;
    float total_m = 0;

    // Начальное приближение — центр масс всех барионов
    for (int i = 0; i < N; ++i) {
        if (is_dark[i]) continue;
        float m = mass[i];
        total_m += m;
        cx += posX[i] * m; cy += posY[i] * m; cz += posZ[i] * m;
    }
    if (total_m > 0) { cx /= total_m; cy /= total_m; cz /= total_m; }
    else return;

    // Итеративное сжатие
    float R = 0.0f;
    // Начальный радиус: расстояние до 50-го перцентиля
    // (Для простоты возьмем расстояние до самой дальней частицы и умножим на 0.5)
    float max_dist_sq = 0.0f;
    for (int i = 0; i < N; ++i) {
        if (is_dark[i]) continue;
        float dx = posX[i] - cx; float dy = posY[i] - cy; float dz = posZ[i] - cz;
        float d_sq = dx*dx + dy*dy + dz*dz;
        if (d_sq > max_dist_sq) max_dist_sq = d_sq;
    }
    R = std::sqrt(max_dist_sq) * 0.5f;
    float r_min = R * 0.02f;

    for (int iter = 0; iter < 80; ++iter) {
        float sum_m = 0;
        float new_cx = 0, new_cy = 0, new_cz = 0;

        for (int i = 0; i < N; ++i) {
            if (is_dark[i]) continue;
            float dx = posX[i] - cx; float dy = posY[i] - cy; float dz = posZ[i] - cz;
            if (dx*dx + dy*dy + dz*dz < R*R) {
                float m = mass[i];
                sum_m += m;
                new_cx += posX[i] * m; new_cy += posY[i] * m; new_cz += posZ[i] * m;
            }
        }
        
        if (sum_m < 10.0f) break; // Если частиц мало, останавливаемся

        new_cx /= sum_m; new_cy /= sum_m; new_cz /= sum_m;

        float shift = std::sqrt((new_cx-cx)*(new_cx-cx) + (new_cy-cy)*(new_cy-cy) + (new_cz-cz)*(new_cz-cz));
        cx = new_cx; cy = new_cy; cz = new_cz;

        if (shift < 1e-6f) break;
        R = std::max(R * 0.6f, r_min);
    }

    posDX = cx; posDY = cy; posDZ = cz;

    // Скорость центра
    // Берем скорость частиц в небольшом радиусе от центра
    float v_cx = 0, v_cy = 0, v_cz = 0, v_sum_m = 0;
    
    // Для начала найдем радиус 30-го перцентиля (приблизительно)
    // Но проще просто взять фиксированный радиус, например 10% от максимального
    float v_radius = std::sqrt(max_dist_sq) * 0.1f; 

    for (int i = 0; i < N; ++i) {
        if (is_dark[i]) continue;
        float dx = posX[i] - posDX; float dy = posY[i] - posDY; float dz = posZ[i] - posDZ;
        if (dx*dx + dy*dy + dz*dz < v_radius * v_radius) {
            float m = mass[i];
            v_sum_m += m;
            v_cx += velX[i] * m; v_cy += velY[i] * m; v_cz += velZ[i] * m;
        }
    }

    if (v_sum_m > 0) {
        velDX = v_cx / v_sum_m; velDY = v_cy / v_sum_m; velDZ = v_cz / v_sum_m;
    } else {
        velDX = 0; velDY = 0; velDZ = 0;
    }

    // Момент импулься (Ось вращения)
    float Lx = 0, Ly = 0, Lz = 0;
    for (int i = 0; i < N; ++i) {
        if (is_dark[i]) continue;

        float m = mass[i];
        float x = posX[i] - posDX;
        float y = posY[i] - posDY;
        float z = posZ[i] - posDZ;
        float vx = velX[i] - velDX;
        float vy = velY[i] - velDY;
        float vz = velZ[i] - velDZ;

        Lx += m * (y * vz - z * vy);
        Ly += m * (z * vx - x * vz);
        Lz += m * (x * vy - y * vx);
    }

    // Нормализация
    float L_len = std::sqrt(Lx*Lx + Ly*Ly + Lz*Lz);
    if (L_len > 1e-6f) {
        Lx /= L_len; Ly /= L_len; Lz /= L_len;
    } else {
        Lx = 0.0f; Ly = 0.0f; Lz = 1.0f;
    }

    // Матрица поворота
    float ux = 0.0f, uy = 0.0f, uz = 0.0f;
    if (std::abs(Lx) < 0.9f) ux = 1.0f;
    else uy = 1.0f;

    float nx = Ly * uz - Lz * uy;
    float ny = Lz * ux - Lx * uz;
    float nz = Lx * uy - Ly * ux;
    float n_len = std::sqrt(nx*nx + ny*ny + nz*nz);
    nx /= n_len; ny /= n_len; nz /= n_len;

    float mx = Ly * nz - Lz * ny;
    float my = Lz * nx - Lx * nz;
    float mz = Lx * ny - Ly * nx;

    rot[0][0] = nx; rot[0][1] = ny; rot[0][2] = nz;
    rot[1][0] = mx; rot[1][1] = my; rot[1][2] = mz;
    rot[2][0] = Lx; rot[2][1] = Ly; rot[2][2] = Lz;
}

void density_profile(
    float* bin_r, float* b_density, float* dm_density,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& mass, const std::vector<std::uint8_t>& is_dark
)
{
    int NUM_BINS = 100;

    int N = static_cast<int>(posX.size());
    if (N == 0 || NUM_BINS <= 0) return;

    // Находим максимальный радиус (с учетом поворота и смещения)
    float max_r_sq = 0.0f;
    for (int i = 0; i < N; ++i) {
        float x = posX[i] - posDX;
        float y = posY[i] - posDY;
        float z = posZ[i] - posDZ;

        float rx = rot[0][0]*x + rot[0][1]*y + rot[0][2]*z;
        float ry = rot[1][0]*x + rot[1][1]*y + rot[1][2]*z;
        float rz = rot[2][0]*x + rot[2][1]*y + rot[2][2]*z;

        float r_sq = rx*rx + ry*ry + rz*rz; 
        if (r_sq > max_r_sq) max_r_sq = r_sq;
    }
    
    float max_r = std::sqrt(max_r_sq);
    if (max_r <= 1e-5f) return;

    // Логарифмический шаг
    float r_min = max_r * 1e-3f; 
    float log_step = std::log(max_r / r_min) / NUM_BINS;

    std::vector<float> mass_b(NUM_BINS, 0.0f);
    std::vector<float> mass_dm(NUM_BINS, 0.0f);

    // Раскладываем массу по бинам
    for (int i = 0; i < N; ++i) {
        float x = posX[i] - posDX;
        float y = posY[i] - posDY;
        float z = posZ[i] - posDZ;

        float rx = rot[0][0]*x + rot[0][1]*y + rot[0][2]*z;
        float ry = rot[1][0]*x + rot[1][1]*y + rot[1][2]*z;
        float rz = rot[2][0]*x + rot[2][1]*y + rot[2][2]*z;

        float r = std::sqrt(rx*rx + ry*ry + rz*rz);
        
        // Если частица ближе r_min, кидаем в первый бин
        if (r < r_min) r = r_min;

        // Вычисляем логарифмический индекс бина
        float idx_f = std::log(r / r_min) / log_step;
        int idx = static_cast<int>(idx_f);

        // Ограничители
        if (idx < 0) idx = 0;
        if (idx >= NUM_BINS) idx = NUM_BINS - 1;

        float frac = idx_f - idx;
        if (idx + 1 >= NUM_BINS) { frac = 0.0f; idx = NUM_BINS - 1; }

        float m = mass[i];
        if (is_dark[i]) {
            mass_dm[idx]     += m * (1.0f - frac);
            mass_dm[idx + 1] += m * frac;
        } else {
            mass_b[idx]     += m * (1.0f - frac);
            mass_b[idx + 1] += m * frac;
        }
    }

    // Переводим массу в плотность (с учетом логарифмических объемов)
    for (int i = 0; i < NUM_BINS; ++i) {
        float r_in = r_min * std::exp(i * log_step);
        float r_out = r_min * std::exp((i + 1) * log_step);
        
        float volume = (4.0f / 3.0f) * 3.14159265f * (r_out*r_out*r_out - r_in*r_in*r_in);
        
        b_density[i] = (volume > 1e-9f) ? mass_b[i] / volume : 0.0f;
        dm_density[i] = (volume > 1e-9f) ? mass_dm[i] / volume : 0.0f;
        
        // Записываем центр бина (среднее геометрическое между r_in и r_out)
        bin_r[i] = std::sqrt(r_in * r_out); 
    }
}

void rotation_curve(
    float* r_b, float* b_tang_vel, float* r_dm, float* dm_tang_vel,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
    const std::vector<std::uint8_t>& is_dark
)
{
    int N = static_cast<int>(posX.size());
    if (N == 0) return;

    int idx_b = 0;   // Счетчик для барионов
    int idx_dm = 0;  // Счетчик для темной материи

    for (int i = 0; i < N; ++i) {
        // 1. Смещаем позицию к центру масс
        float x = posX[i] - posDX;
        float y = posY[i] - posDY;
        float z = posZ[i] - posDZ;

        // 2. Поворачиваем позицию в плоскость XY
        float rx = rot[0][0]*x + rot[0][1]*y + rot[0][2]*z;
        float ry = rot[1][0]*x + rot[1][1]*y + rot[1][2]*z;
        // float rz = rot[2][0]*x + rot[2][1]*y + rot[2][2]*z; // Для 2D радиуса не нужен

        // 3. Считаем радиус в плоскости диска
        float rad = std::sqrt(rx*rx + ry*ry);
        
        // Пропускаем частицы ровно в центре (там скорость не определена)
        if (rad < 1e-5f) continue;

        // 4. Смещаем и поворачиваем скорость
        float vx = velX[i] - velDX;
        float vy = velY[i] - velDY;
        float vz = velZ[i] - velDZ;

        float rvx = rot[0][0]*vx + rot[0][1]*vy + rot[0][2]*vz;
        float rvy = rot[1][0]*vx + rot[1][1]*vy + rot[1][2]*vz;

        // 5. Тангенциальная скорость (проекция на перпендикуляр к радиус-вектору)
        float vt = (rx * rvy - ry * rvx) / rad;

        // 6. Раскладываем по массивам
        if (is_dark[i]) {
            r_dm[idx_dm] = rad;
            dm_tang_vel[idx_dm] = vt;
            idx_dm++;
        } else {
            r_b[idx_b] = rad;
            b_tang_vel[idx_b] = vt;
            idx_b++;
        }
    }
}

} // Analyzer