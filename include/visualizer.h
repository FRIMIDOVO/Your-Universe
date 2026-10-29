#pragma once
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <vector>
#include <cstdint>

extern "C" {
    __declspec(dllimport) void __stdcall glDrawArrays(
        unsigned int mode, int first, int count
    );

    __declspec(dllimport) void __stdcall glEnable(
        unsigned int cap
    );
}

static constexpr unsigned int GL_POINTS_VALUE = 0x0000;
static constexpr unsigned int GL_PROGRAM_POINT_SIZE_VALUE = 0x8642;


// Орбитальная камера
struct OrbitCamera {
    Vector3 target = {0.0f, 0.0f, 0.0f};
    float distance = 20.0f;
    float yaw = -45.0f;
    float pitch = 0.0f;
    float mouse_sensitivity = 0.15f;

    void update(float dt);
    Camera get_camera() const;
};

// Свободная камера
struct FreeCamera {
    Vector3 position = {0.0f, 15.0f, 15.0f};
    float yaw = -90.0f;
    float pitch = -20.0f;
    float speed = 50.0f;
    float mouse_sensitivity = 0.15f;

    void update(float dt);
    Camera get_camera() const;
};

// Класс отрисовщика
class Renderer {
public:
    Renderer() = default;
    void init(size_t max_cap);
    void shutdown();
    ~Renderer();

    template<typename CameraType>
    void draw(
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<uint8_t>& is_dark, bool show_dark_matter,
        const CameraType& camera
    ) {
        int N = static_cast<int>(posX.size());
        if (N == 0) return;

        for (int i = 0; i < N; ++i) {
            packed_data_[i * 4 + 0] = posX[i];
            packed_data_[i * 4 + 1] = posY[i];
            packed_data_[i * 4 + 2] = posZ[i];
            packed_data_[i * 4 + 3] = is_dark[i] ? 1.0f : 0.0f;
        }

        rlEnableVertexArray(vao_);
        rlUpdateVertexBuffer(vbo_, packed_data_.data(), N * 4 * sizeof(float), 0);

        BeginMode3D(camera.get_camera());

        BeginShaderMode(particle_shader_);

        int show_dark = show_dark_matter ? 1 : 0;
        SetShaderValue(particle_shader_, show_dark_location_, &show_dark, SHADER_UNIFORM_INT);

        Camera cam = camera.get_camera();

        Matrix view = GetCameraMatrix(cam);
        Matrix projection = MatrixPerspective(cam.fovy * DEG2RAD, (double)GetScreenWidth() / GetScreenHeight(), 0.01, 100000.0);

        Matrix mvp = MatrixMultiply(view, projection);
        SetShaderValueMatrix(particle_shader_, mvp_location_, mvp);

        rlEnableVertexArray(vao_);

        glEnable(GL_PROGRAM_POINT_SIZE_VALUE);
        glDrawArrays(GL_POINTS_VALUE, 0, N);
        
        rlDisableVertexArray();

        EndShaderMode();
        EndMode3D();
    }

private:
    size_t max_particles_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    std::vector<float> packed_data_; // X, Y, Z, TYPE

    // Шеёдер
    Shader particle_shader_{};
    int mvp_location_ = -1;
    int show_dark_location_ = -1;
};

// Основной класс, соединяющий отрисовщих и камеры
class Visualizer {
public:
    Visualizer(size_t max_particles);
    ~Visualizer();

    void draw(
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<uint8_t>& is_dark
    );

private:
    // Камеры
    OrbitCamera orbit_camera;
    FreeCamera  free_camera;

    // Отрисовщик
    Renderer renderer;

    bool use_orbit = true;
    bool show_dark_matter = true;
};