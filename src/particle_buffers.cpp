#include "particle_buffers.h"
#include <stdexcept>

ParticleBuffers::ParticleBuffers(OpenCLManager& clm)
    : clm_(clm)
{}

ParticleBuffers::~ParticleBuffers() {
    releaseBuffers();
}

void ParticleBuffers::releaseBuffers() {
    auto rel = [](cl_mem& m) {
        if (m) { clReleaseMemObject(m); m = nullptr; }
    };
    rel(d_posX_); rel(d_posY_); rel(d_posZ_);
    rel(d_velX_); rel(d_velY_); rel(d_velZ_);
    rel(d_mass_); rel(d_is_dark_);
    rel(d_accX_); rel(d_accY_); rel(d_accZ_);
    current_max_ = 0;
}

void ParticleBuffers::init(int maxParticles) {
    if (maxParticles <= current_max_) return;

    // Дожидаемся завершения всех команд, чтобы старые буферы точно не использовались
    clFinish(clm_.getQueue());

    releaseBuffers();

    current_max_ = maxParticles;

    const size_t fSize = (size_t)maxParticles * sizeof(float);
    const size_t bSize = (size_t)maxParticles * sizeof(uint8_t);

    d_posX_ = clm_.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_posY_ = clm_.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_posZ_ = clm_.createBuffer(fSize, CL_MEM_READ_ONLY);

    d_velX_ = clm_.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_velY_ = clm_.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_velZ_ = clm_.createBuffer(fSize, CL_MEM_READ_ONLY);

    d_mass_    = clm_.createBuffer(fSize, CL_MEM_READ_ONLY);
    d_is_dark_ = clm_.createBuffer(bSize, CL_MEM_READ_ONLY);

    d_accX_ = clm_.createBuffer(fSize, CL_MEM_READ_WRITE);
    d_accY_ = clm_.createBuffer(fSize, CL_MEM_READ_WRITE);
    d_accZ_ = clm_.createBuffer(fSize, CL_MEM_READ_WRITE);
}

void ParticleBuffers::upload(
    const std::vector<float>& accX, const std::vector<float>& accY, const std::vector<float>& accZ,
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
    const std::vector<float>& mass,
    const std::vector<uint8_t>& is_dark
) {
    const int N = (int)posX.size();
    if (N == 0) return;

    if (N > current_max_) init(N);

    const size_t fSize = (size_t)N * sizeof(float);
    const size_t bSize = (size_t)N * sizeof(uint8_t);

    clm_.writeBufferAsync(d_posX_, posX.data(), fSize);
    clm_.writeBufferAsync(d_posY_, posY.data(), fSize);
    clm_.writeBufferAsync(d_posZ_, posZ.data(), fSize);

    clm_.writeBufferAsync(d_velX_, velX.data(), fSize);
    clm_.writeBufferAsync(d_velY_, velY.data(), fSize);
    clm_.writeBufferAsync(d_velZ_, velZ.data(), fSize);

    clm_.writeBufferAsync(d_mass_,    mass.data(),    fSize);
    clm_.writeBufferAsync(d_is_dark_, is_dark.data(), bSize);

    clm_.writeBufferAsync(d_accX_, accX.data(), fSize);
    clm_.writeBufferAsync(d_accY_, accY.data(), fSize);
    clm_.writeBufferAsync(d_accZ_, accZ.data(), fSize);
}

void ParticleBuffers::download_acc(
    std::vector<float>& accX, std::vector<float>& accY, std::vector<float>& accZ,
    int N
) {
    if (N == 0) return;

    const size_t fSize = (size_t)N * sizeof(float);

    accX.resize(N);
    accY.resize(N);
    accZ.resize(N);

    clm_.readBuffer(d_accX_, accX.data(), fSize);
    clm_.readBuffer(d_accY_, accY.data(), fSize);
    clm_.readBuffer(d_accZ_, accZ.data(), fSize);
}