#include "particle_data.h"


void ParticleData::add_particle(
    float m,
    float px, float py, float pz,
    float vx, float vy, float vz,
    bool dm
) {
    accX.push_back(0.0f); accY.push_back(0.0f); accZ.push_back(0.0f);
    posX.push_back(px); posY.push_back(py); posZ.push_back(pz);
    velX.push_back(vx); velY.push_back(vy); velZ.push_back(vz);
    mass.push_back(m);
    is_dark.push_back(dm ? 1 : 0);
}

void ParticleData::clear_all() {
    accX.clear(); accY.clear(); accZ.clear();
    posX.clear(); posY.clear(); posZ.clear();
    velX.clear(); velY.clear(); velZ.clear();
    mass.clear();
    is_dark.clear();
}

void ParticleData::clear_dm() {
    const size_t N = posX.size();
    if (N == 0) return;

    size_t write_idx = 0;

    for (size_t i = 0; i < N; ++i) {
        if (is_dark[i]) continue;

        if (i != write_idx) {
            posX[write_idx]  = posX[i];
            posY[write_idx]  = posY[i];
            posZ[write_idx]  = posZ[i];

            velX[write_idx]  = velX[i];
            velY[write_idx]  = velY[i];
            velZ[write_idx]  = velZ[i];

            mass[write_idx]  = mass[i];
            is_dark[write_idx] = is_dark[i];
        }
        write_idx++;
    }

    posX.resize(write_idx);
    posY.resize(write_idx);
    posZ.resize(write_idx);

    velX.resize(write_idx);
    velY.resize(write_idx);
    velZ.resize(write_idx);

    accX.resize(write_idx);
    accY.resize(write_idx);
    accZ.resize(write_idx);

    mass.resize(write_idx);
    is_dark.resize(write_idx);
}