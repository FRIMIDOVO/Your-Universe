
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define PI 3.14159265358979323846f
#define RHO_EPS 1e-12f


__kernel void calculate_density_sph(
    __global const float* posX,
    __global const float* posY,
    __global const float* posZ,

    __global const float* mass,
    __global const uchar* is_dark,

    __global float* density,

    const int N,
    const float h2,
    const float four_inv_pi_h8
) {
    const int i = get_global_id(0);

    if (i >= N)
        return;

    if (is_dark[i]) {
        density[i] = 0.0f;
        return;
    }

    const float xi = posX[i];
    const float yi = posY[i];
    const float zi = posZ[i];

    float rho = 0.0f;

    for (int j = 0; j < N; ++j) {

        if (is_dark[j] || mass[j] <= 0.0f)
            continue;

        const float dx = posX[j] - xi;
        const float dy = posY[j] - yi;
        const float dz = posZ[j] - zi;

        const float r2 =
            dx * dx +
            dy * dy +
            dz * dz;

        if (r2 < 0.0f || r2 >= h2)
            continue;

        const float d = h2 - r2;

        const float W =
            four_inv_pi_h8 *
            d * d * d;

        rho += mass[j] * W;
    }

    density[i] = rho;
}


__kernel void calculate_pressure_sph(
    __global const float* density,

    __global const uchar* is_dark,

    __global float* pressure,

    const int N,
    const float rest_density,
    const float k_rd_inv_g,
    const float gamma
) {
    const int i = get_global_id(0);

    if (i >= N)
        return;

    if (is_dark[i]) {
        pressure[i] = 0.0f;
        return;
    }

    const float rho = density[i];

    if (rho <= 0.0f) {
        pressure[i] = 0.0f;
        return;
    }

    const float ratio = rho / rest_density;

    const float p =
        k_rd_inv_g *
        (pow(ratio, gamma) - 1.0f);

    pressure[i] = fmax(0.0f, p);
}


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

    const int N,
    const float h,
    const float h2,
    const float twfour_inv_pi_h8,
    const float alpha,
    const float cs
) {
    const int i = get_global_id(0);

    if (i >= N)
        return;

    if (is_dark[i])
        return;

    const float rho1 = density[i];

    if (rho1 <= RHO_EPS)
        return;

    const float xi = posX[i];
    const float yi = posY[i];
    const float zi = posZ[i];

    const float vxi = velX[i];
    const float vyi = velY[i];
    const float vzi = velZ[i];

    const float pi_i = pressure[i];

    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;

    for (int j = 0; j < N; ++j) {

        if (j == i || is_dark[j])
            continue;

        const float rho2 = density[j];

        if (rho2 <= RHO_EPS || mass[j] <= 0.0f)
            continue;

        // ----------------------------------------------------
        // Distance
        // ----------------------------------------------------

        const float dx = xi - posX[j];
        const float dy = yi - posY[j];
        const float dz = zi - posZ[j];

        const float r2 =
            dx * dx +
            dy * dy +
            dz * dz;

        if (r2 <= 0.0f || r2 >= h2)
            continue;

        // ----------------------------------------------------
        // Pressure gradient
        // ----------------------------------------------------

        const float d = h2 - r2;

        const float coeff =
            twfour_inv_pi_h8 *
            d * d;

        const float gx = dx * coeff;
        const float gy = dy * coeff;
        const float gz = dz * coeff;

        // ----------------------------------------------------
        // Relative velocity
        // ----------------------------------------------------

        const float vdx = vxi - velX[j];
        const float vdy = vyi - velY[j];
        const float vdz = vzi - velZ[j];

        const float dot_prod =
            vdx * dx +
            vdy * dy +
            vdz * dz;

        // ----------------------------------------------------
        // Artificial viscosity
        // ----------------------------------------------------

        float PI_12 = 0.0f;

        if (dot_prod < 0.0f) {

            const float mu =
                (h * dot_prod) /
                (r2 + 0.01f * h2);

            const float rho_avg =
                0.5f * (rho1 + rho2);

            if (rho_avg > RHO_EPS) {
                PI_12 =
                    (-alpha * cs * mu) /
                    rho_avg;
            }
        }

        // ----------------------------------------------------
        // SPH force
        // ----------------------------------------------------

        const float sph_factor =
            pi_i / (rho1 * rho1) +
            pressure[j] / (rho2 * rho2) +
            PI_12;

        const float force_mag =
            mass[j] * sph_factor;

        ax += gx * force_mag;
        ay += gy * force_mag;
        az += gz * force_mag;
    }

    // --------------------------------------------------------
    // Accumulate acceleration
    // --------------------------------------------------------

    accX[i] += ax;
    accY[i] += ay;
    accZ[i] += az;
}
