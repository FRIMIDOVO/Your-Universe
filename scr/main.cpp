#include <chrono>
#include <iostream>
#include "physics.h"
#include <filesystem>
#include "opencl_manager.h"
#include "config_loader.h"
#include "galaxy_saver.h"
#include "visualizer.h"
#include "vec3.h"
#include <memory>
#include <conio.h>

#define M_PI 3.14159265358979323846

int main() {
    // Иницилизация конфигов
    PhysicsConfig physics;
    SpaceConfig space;
    VisualizerConfig viz;
    
    // Подгрузка параметров конфигов из фйлов
    ConfigLoader::load("configs/space_config.txt", space);
    ConfigLoader::load("configs/physics_config.txt", physics);
    ConfigLoader::load("configs/viz_config.txt", viz);

    // OpenCl для GPU
    OpenCLManager clManager;

    // Сам движок
    PhysicsEngine engine(physics, space, clManager);

    // Визуализатор
    std::unique_ptr<Visualizer> visualizer = nullptr;
    if (viz.render) visualizer = std::make_unique<Visualizer>(500000);


    // ===== Спавн частиц =====

    auto spawn = [&] (int p_count, int dm_p_count, float omega) {

        // ===== Материя =====
        std::vector<Particle> b_particles;

        for (int i = 0; i < p_count; ++i) {
            b_particles.push_back(Particle::random_particle(
                &space,
                std::nullopt,
                Vec3(),         // без температуры
                0.8f,           // mass
                false           // флаг тм (выключен)
            ));
        }

        // Задаём вращение
        for (auto& b : b_particles) {
            Vec3 r = b.get_position();
            Vec3 v_rot(-omega * r.y, omega * r.x, 0.0f);

            b.add_velocity(v_rot);
            engine.add_particle(b);
        }


        // ===== ТМ =====
        std::vector<Particle> dm_particles;

        for (int i = 0; i < dm_p_count; ++i) {
            dm_particles.push_back(Particle::random_particle(
                &space,
                std::nullopt,
                Vec3(),         // без температуры
                1.6f,           // mass
                true            // флаг тм (включен)
            ));
        }

        // Задаём вращение
        for (auto& dm : dm_particles) {
            Vec3 r = dm.get_position();
            Vec3 v_rot(-omega * r.y, omega * r.x, 0.0f);

            dm.add_velocity(v_rot);
            engine.add_particle(dm);
        }

    };

    auto load_or_spawn = [&](int p_count, int dm_p_count, float omega, const std::filesystem::path& save = {}) {
        std::filesystem::path path_to_load;

        // 1. Если передан конкретный путь — грузим его
        if (!save.empty()) {
            if (std::filesystem::exists(save)) {
                path_to_load = save;
            }
        }
        // 2. Иначе — ищем последний сейв в saves/test_simulations/
        else {
            const std::filesystem::path search_dir = "saves/test_simulations/";
            if (std::filesystem::exists(search_dir)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(search_dir)) {
                    if (!entry.is_regular_file()) continue;
                    const auto& p = entry.path();
                    if (p.extension() != ".csv") continue;
                    if (p.filename().string().rfind("galaxy_", 0) != 0) continue;

                    if (path_to_load.empty() || p.string() > path_to_load.string()) {
                        path_to_load = p;
                    }
                }
            }
        }

        // 3. Если нашли — грузим, иначе — спавним
        if (!path_to_load.empty() && std::filesystem::exists(path_to_load)) {
            auto [loaded_time, loaded_particles] = GalaxySaver::load(path_to_load.string());

            engine.clear_particles();
            engine.set_time(loaded_time);
            for (auto& p : loaded_particles) {
                engine.add_particle(p);
            }

        } else {
            engine.clear_particles();
            engine.set_time(0.0f);
            spawn(p_count, dm_p_count, omega);
        }
    };

    auto calculate_cycle = [&] (int frames, std::string save_directory) {
        // Info
        std::cout << "Particles: " << engine.get_particles().size() << "\n";

        if (physics.record) GalaxySaver::save(save_directory, engine.get_particles(), engine.get_time());

        // === Main cycle ===
        int frame = 0;
        int last_frame = 0;

        auto t0 = std::chrono::high_resolution_clock::now();
        auto t1 = std::chrono::high_resolution_clock::now();
        auto t2 = std::chrono::high_resolution_clock::now();
        auto to_ms = [](auto start, auto end) { return std::chrono::duration<double, std::milli>(end - start).count(); };

        auto last_time = std::chrono::steady_clock::now();
        auto curr_time = std::chrono::steady_clock::now();

        while (frames != frame) {

            // Обработка нажатий кнопок
            if (_kbhit()) {
                char ch = _getch();
                if (ch == 27) { // 27 — это ASCII-код клавиши ESC
                    std::cout << "\nFinish program with save progress? (y/n): ";
                    char answ;
                    std::cin >> answ;
                    
                    switch (answ) {
                        case 'y': 
                        case 'Y':   break; // Выходит из switch, код пойдет дальше к break внешнего цикла
                        case 'n':
                        case 'N':
                        default:    continue; // Переходит на следующую итерацию ВНЕШНЕГО цикла
                    }
                    break; // Этот break сработает, только если мы выбрали 'y' и вышли из switch
                } else if (ch == 115) { 
                    GalaxySaver::save(save_directory, engine.get_particles(), engine.get_time());
                } else if (ch == 'p' || ch == 'P') {
                    physics.calculate = !physics.calculate;
                } else if (ch == 'r' || ch == 'R') {
                    viz.render = !viz.render;
                }

            }

            t0 = std::chrono::high_resolution_clock::now();
            
            if (physics.calculate) engine.step();
            t1 = std::chrono::high_resolution_clock::now();

            if (viz.render) visualizer->draw(
                engine.get_posX(), engine.get_posY(), engine.get_posZ(),
                engine.get_dark()
            );
            t2 = std::chrono::high_resolution_clock::now();

            // Вывод времени каждого этапа
            std::cout << "Frame: " << frame << " | "
                    << "Physics: " << to_ms(t0, t1) << " ms | " 
                    << "Render: " << to_ms(t1, t2) << " ms | "
                    << "Total: " << to_ms(t0, t2) << " ms" 
                    << std::endl;
            ++frame;

            curr_time = std::chrono::steady_clock::now();

            if (frame - last_frame > 500 && physics.record) {
                last_frame = frame;
                GalaxySaver::save(save_directory, engine.get_particles(), engine.get_time());
            }
        }

        GalaxySaver::save(save_directory, engine.get_particles(), engine.get_time());

        // Вывод инфы
        std::cout << "Calculated " << frame << " frames." << std::endl;
    };

    /*
    // Для анализа с разными параметрами
    float elastic_range[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    float omega_range[]   = {0.0f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f};

    for (float& elastic : elastic_range) {
        for (float& omega : omega_range) {
            physics.K_elasticity = elastic;
            engine.clear_particles();
            engine.set_time(0.0f);
            spawn(omega);
            std::string save_dir = std::to_string(elastic) + "_" + std::to_string(omega);
            calculate_cycle(5000, save_dir);
        }
    }
    */

    // ====================== 

    int count_frames;
    std::cout << "Enter count frames for calculation(-1 to infinity): ";
    std::cin >> count_frames;
    std::cout << std::endl;

    load_or_spawn(25'000, 75'000, 8.0f, "");
    engine.update_dod();
    calculate_cycle(count_frames, "test_simulations/");

    std::cin.get();
    std::cin.get();

    return 0;
}