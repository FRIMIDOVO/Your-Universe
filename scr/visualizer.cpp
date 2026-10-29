#include "visualizer.h"
#include <cstring>


void Renderer::init(size_t max_cap) {
    max_particles_ = max_cap;

    particle_shader_ = LoadShader(
        "shaders/particle.vs",
        "shaders/particle.fs"
    );

    packed_data_.resize(max_particles_ * 4);

    mvp_location_ = GetShaderLocation(particle_shader_, "mvp");
    show_dark_location_ = GetShaderLocation(particle_shader_, "showDarkMatter");

    vao_ = rlLoadVertexArray();
    rlEnableVertexArray(vao_);

    // Создаём VBO и выделяем память
    vbo_ = rlLoadVertexBuffer(nullptr, max_particles_ * 4 * sizeof(float), true);

    // Атрибут 0: pos (3 float)
    rlEnableVertexBuffer(vbo_);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 4 * sizeof(float), 0);
    rlEnableVertexAttribute(0);
    
    // Атрибут 1: radius (1 float, смещение 3 * sizeof(float))
    rlSetVertexAttribute(1, 1, RL_FLOAT, false, 4 * sizeof(float), 3 * sizeof(float));
    rlEnableVertexAttribute(1);
    
    rlDisableVertexBuffer();
    rlDisableVertexArray();
}

void Renderer::shutdown() {
    if (particle_shader_.id != 0) {
        UnloadShader(particle_shader_);
        particle_shader_ = {};
    }

    if (vbo_ != 0) {
        rlUnloadVertexBuffer(vbo_);
        vbo_ = 0;
    }

    if (vao_ != 0) {
        rlUnloadVertexArray(vao_);
        vao_ = 0;
    }
}

Renderer::~Renderer() {
    shutdown();
}

/*
template<typename CameraType>
void Renderer::draw(
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<float>& radius,
    const CameraType& camera
)
{
    int N = static_cast<int>(posX.size());
    if (N == 0) return;

    // Упаковка
    for (int i = 0; i < N; ++i) {
        packed_data_[i * 4 + 0] = posX[i];
        packed_data_[i * 4 + 1] = posY[i];
        packed_data_[i * 4 + 2] = posZ[i];
        packed_data_[i * 4 + 3] = radius[i];
    }

    // Заливка в VBO
    rlEnableVertexArray(vao_);
    rlUpdateVertexBuffer(vbo_, packed_data_.data(), N * 4 * sizeof(float), 0);

    // Рендер
    BeginMode3D(camera.get_camera());
    rlEnablePointMode(); // включаем возможность менять размер точек
    rlDrawVertexArray(0, N); // рисуем частицы одной командой
    EndMode3D();

    rlDisableVertexArray();
}

// Явная инстанциация шаблона для двух камер, чтобы линковщик не ругался
template void Renderer::draw<OrbitCamera>(
    const std::vector<float>&, const std::vector<float>&, const std::vector<float>&,
    const std::vector<float>&,
    const OrbitCamera&
);

template void Renderer::draw<FreeCamera>(
    const std::vector<float>&, const std::vector<float>&, const std::vector<float>&,
    const std::vector<float>&,
    const FreeCamera&
);
*/

Visualizer::Visualizer(size_t max_particles) : renderer() {
    InitWindow(1000, 1000, "My Universe");
    SetTargetFPS(60);

    renderer.init(max_particles);
}


Visualizer::~Visualizer() {
    renderer.shutdown();
    CloseWindow();
}

void Visualizer::draw(
    const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
    const std::vector<uint8_t>& is_dark
)
{
    float dt = GetFrameTime();

    // Смена камеры
    if (IsKeyPressed(KEY_TAB)) use_orbit = !use_orbit;

    if (IsKeyPressed(KEY_T)) show_dark_matter = !show_dark_matter;

    // Апдейт активной камеры
    if (use_orbit) {
        orbit_camera.update(dt);
    } else {
        free_camera.update(dt);
    }

    // Начинаем отрисовку
    BeginDrawing();
    ClearBackground(BLACK); // цвет фона

    // Рисуем вид активной камеры
    if (use_orbit) {
        renderer.draw(posX, posY, posZ, is_dark, show_dark_matter, orbit_camera);
    } else {
        renderer.draw(posX, posY, posZ, is_dark, show_dark_matter, free_camera);
    }

    // UI
    DrawFPS(10, 10);

    // Заканчиваем отрисовку
    EndDrawing();
}

void OrbitCamera::update(float dt) {
    // Вращение вокруг цели (ПКМ)
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        //DisableCursor();
        Vector2 delta = GetMouseDelta();
        yaw -= delta.x * mouse_sensitivity;
        pitch += delta.y * mouse_sensitivity;
        
        // Ограничиваем наклон, чтобы не переворачивалась
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    } else {
        //EnableCursor();
    }

    // Приближение/Отдаление (Колесико)
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        distance -= wheel * (distance * 0.1f);
        if (distance < 1.0f) distance = 1.0f; // Не даем пролететь сквозь центр
    }

    // Перемещение цели (WASD)
    float speed = distance * 0.5f * dt;

    float yawRad = yaw * DEG2RAD;

    // Направление взгляда по горизонтали
    Vector3 forward = {sinf(yawRad), 0.0f, cosf(yawRad)};

    // Вектор вправо относительно камеры
    Vector3 right = {cosf(yawRad), 0.0f, -sinf(yawRad)};

    if (IsKeyDown(KEY_W)) target = Vector3Add(target, Vector3Scale(forward, -speed));
    if (IsKeyDown(KEY_S)) target = Vector3Subtract(target, Vector3Scale(forward, -speed));
    if (IsKeyDown(KEY_D)) target = Vector3Add(target, Vector3Scale(right, speed));
    if (IsKeyDown(KEY_A)) target = Vector3Subtract(target, Vector3Scale(right, speed));
}

Camera OrbitCamera::get_camera() const {
    // Переводим сферические координаты (yaw, pitch, distance) в декартовы (x, y, z)
    float rad_yaw = yaw * DEG2RAD;
    float rad_pitch = pitch * DEG2RAD;

    Vector3 pos;
    pos.x = target.x + distance * cosf(rad_pitch) * sinf(rad_yaw);
    pos.y = target.y + distance * sinf(rad_pitch);
    pos.z = target.z + distance * cosf(rad_pitch) * cosf(rad_yaw);

    Camera cam;
    cam.position = pos;
    cam.target = target;
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 60.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    return cam;
}

void FreeCamera::update(float dt) {
    // Вращение головы (ПКМ)
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        //DisableCursor();
        Vector2 delta = GetMouseDelta();
        yaw += delta.x * mouse_sensitivity;
        pitch -= delta.y * mouse_sensitivity;
        
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    } else {
        //EnableCursor();
    }

    // Вычисляем вектор направления (куда смотрит камера)
    Vector3 front = {
        cosf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD),
        sinf(pitch * DEG2RAD),
        sinf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD)
    };
    front = Vector3Normalize(front);
    
    // Вектор "вправо" (перпендикулярен направлению взгляда)
    Vector3 right = Vector3Normalize(Vector3CrossProduct(front, {0.0f, 1.0f, 0.0f}));

    // Движение (WASD + Space/Shift)
    float velocity = speed * dt;
    if (IsKeyDown(KEY_W)) position = Vector3Add(position, Vector3Scale(front, velocity));
    if (IsKeyDown(KEY_S)) position = Vector3Subtract(position, Vector3Scale(front, velocity));
    if (IsKeyDown(KEY_A)) position = Vector3Subtract(position, Vector3Scale(right, velocity));
    if (IsKeyDown(KEY_D)) position = Vector3Add(position, Vector3Scale(right, velocity));
    if (IsKeyDown(KEY_SPACE)) position.y += velocity;
    if (IsKeyDown(KEY_LEFT_SHIFT)) position.y -= velocity;

    // Изменение скорости полета (Колесико)
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        speed += wheel * 100.0f;
        if (speed < 10.0f) speed = 10.0f;
    }
}

Camera FreeCamera::get_camera() const {
    // Вычисляем точку, куда смотрит камера
    Vector3 front = {
        cosf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD),
        sinf(pitch * DEG2RAD),
        sinf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD)
    };
    front = Vector3Normalize(front);

    Camera cam;
    cam.position = position;
    cam.target = Vector3Add(position, front); // Смотрим на вектор впереди
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 60.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    return cam;
}