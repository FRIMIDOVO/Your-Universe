#include <chrono>
#include <iostream>
#include "physics.h"
#include "visualizer.h"
#include "gif_recorder.h"
#include "frame_saver.h"
#include "config_loader.h"
#include "vec2.h"


void smash(PhysicsEngine& engine);
void populate(PhysicsEngine& engine, SpaceConfig& space, long long N);
void double_system(PhysicsEngine& engine);
void triple_system(PhysicsEngine& engine);
void spawn_rotating_cluster(PhysicsEngine& engine, Vec2 center, int count, float radius, float mass, float rotation_speed);


int main() {
    PhysicsConfig physics;
    SpaceConfig space;
    VisualizerConfig viz;
    
    ConfigLoader::load("space_config.txt", space);
    ConfigLoader::load("physics_config.txt", physics);
    ConfigLoader::load("viz_config.txt", viz);

    PhysicsEngine engine(physics, space);
    Visualizer visualizer(engine, viz);
    //GifRecorder recorder("record.gif", viz);
    std::filesystem::remove_all("frames");

    // ===== Спавн частиц =====

    // spawn_rotating_cluster(engine, Vec2(0,0), 1000, 3.0f, 10.0f, 100.0f);
    populate(engine, space, 5000);

    // engine.add_particle(Particle::random_particle(space)); // Создание партикла с рандомными параметрами
    
    /* engine.add_particle(Particle(
        9999, // id
        Vec2(0,0) , // position
        Vec2(0,0) , // velocity
        5 // mass
    )) */// Партикл с заданными параметрами

    // ===== =====

    std::cout << "Particles: " << engine.get_particles().size() << "\n";

    int count_frames;
    std::cout << "Enter count frames for calculation: ";
    std::cin >> count_frames;
    std::cout << std::endl;

    int frame = 0;
    while (visualizer.is_open() && count_frames != frame) {
        auto t0 = std::chrono::high_resolution_clock::now();
        
        engine.step();
        auto t1 = std::chrono::high_resolution_clock::now();
        
        visualizer.render();
        auto t2 = std::chrono::high_resolution_clock::now();
        
        if (viz.record)
            FrameSaver::save_frame(visualizer.get_texture(), frame);
            
        auto t3 = std::chrono::high_resolution_clock::now();
        
        // Вывод времени каждого этапа
        auto to_ms = [](auto start, auto end) {
            return std::chrono::duration<double, std::milli>(end - start).count();
        };
        
        std::cout << "Frame: " << frame << " | ";
        std::cout << "Physics: " << to_ms(t0, t1) << " ms | "
                << "Render: " << to_ms(t1, t2) << " ms | "
                << "Record: " << to_ms(t2, t3) << " ms | "
                << "Total: " << to_ms(t0, t3) << " ms" << std::endl;

        ++frame;
    }

    ConfigLoader::save("space_config.txt", space);
    ConfigLoader::save("physics_config.txt", physics);
    ConfigLoader::save("viz_config.txt", viz);
    std::cout << "Generated " << frame << " frames.";
    std::cout << "View saved frames `bin/frames/` and encode their with encode_video.bat or encode_gif.bat";
    std::cin.get();
    std::cin.get();

        return 0;
}


// Генерация частиц
void smash(PhysicsEngine& engine) {
    engine.add_particle(Particle(
        1,
        Vec2(-1.0, 0.0),
        Vec2(5.0, 0.2),
        1.0
    ));
    engine.add_particle(Particle(
        2,
        Vec2(1.0, 0.0),
        Vec2(0.0, 0.0),
        5.0
    ));
}

void populate(PhysicsEngine& engine, SpaceConfig& space, long long N) {
    for (long long i = 0; i < N; i++)
        engine.add_particle(Particle::random_particle(space));
}

void double_system(PhysicsEngine& engine) {
    engine.add_particle(Particle(
        1,
        Vec2(-3.0f, 0.0f),
        Vec2(0.0f, -0.35f),
        8.0f
    ));
    engine.add_particle(Particle(
        2,
        Vec2(7.0f, 0.0f),
        Vec2(0.0f, 0.93f),
        3.0f
    ));
}

void triple_system(PhysicsEngine& engine) {
    const float R = 4.0f;
    const float m = 3.0f;
    const float v = 1.15f;
    const float sqrt3 = std::sqrt(3.0f);
    const float h = R * sqrt3 / 2.0f;
    engine.add_particle(Particle(
        1,
        Vec2(R, 0.0f),
        Vec2(0.0f, v),
        m
    ));
    engine.add_particle(Particle(
        2,
        Vec2(-R / 2.0f, h),
        Vec2(-v * sqrt3 / 2.0f, -v / 2.0f),
        m
    ));
    engine.add_particle(Particle(
        3,
        Vec2(-R / 2.0f, -h),
        Vec2(v * sqrt3 / 2.0f, -v / 2.0f),
        m
    ));
}

void spawn_rotating_cluster(PhysicsEngine& engine, Vec2 center, int count, float radius, float mass, float rotation_speed) {
    for (int i = 0; i < count; ++i) {
        // Случайное положение внутри круга
        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
        float r = radius * std::sqrt(static_cast<float>(rand()) / RAND_MAX);  // равномерно по площади
        float x = center.x + r * std::cos(angle);
        float y = center.y + r * std::sin(angle);
        
        // Скорость = вращение вокруг центра + случайный разброс
        float v_rot = rotation_speed * r;
        float vx = -v_rot * std::sin(angle) + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f;
        float vy = v_rot * std::cos(angle) + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f;
        
        float m = mass * (0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f);
        
        engine.add_particle(Particle(
            1000 + i,
            Vec2(x, y),
            Vec2(vx, vy),
            m
        ));
    }
}