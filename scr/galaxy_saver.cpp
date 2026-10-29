#include "galaxy_saver.h"
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <chrono>
#include <filesystem>

// Возвращает временной штамп для файла
std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* time_info = std::localtime(&time);
    std::stringstream ss;
    ss << std::put_time(time_info, "%Y-%m-%d_%H-%M-%S");
    
    // Миллисекунды
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    ss << '-' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Сохраняет состояния частиц в CSV
void GalaxySaver::save(const std::string& dir, const std::vector<Particle>& particles, const float world_time) {
    std::filesystem::create_directories("saves/" + dir);
    std::string filename = "saves/" + dir + "/galaxy_" + timestamp() + ".csv";
    std::ofstream output(filename);
    
    if (!output.is_open()) return;
    
    // Заголовок CSV
    output << "world_time," << world_time << "\n";
    output << "id,mass,px,py,pz,vx,vy,vz,dark_matter,movable\n";
    
    for (size_t idx = 0; idx < particles.size(); ++idx) {
        const Particle& p = particles[idx];
        Vec3 pos = p.get_position();
        Vec3 vel = p.get_velocity();
        
        output << p.get_id() << ","
               << p.get_mass() << ","
               << pos.x << "," << pos.y << "," << pos.z << ","
               << vel.x << "," << vel.y << "," << vel.z << ","
               << p.is_dark_matter() << ","
               << p.is_movable() << "\n";
    }
    
    std::cout << "Successfully saved " << filename << std::endl;
    output.close();
}

// Выгружает состояния частиц из CSV
std::pair<float, std::vector<Particle>> GalaxySaver::load(const std::string& filename) {
    std::vector<Particle> loaded_particles;
    float world_time = 0.0f;
    
    std::ifstream input(filename);
    if (!input.is_open()) return std::make_pair(world_time, loaded_particles);
    
    std::string line;
    
    // --- Первая строка: world_time,<значение> ---
    if (std::getline(input, line)) {
        auto comma = line.find(',');
        if (comma != std::string::npos) {
            try {
                world_time = std::stof(line.substr(comma + 1));
            } catch (...) {
                world_time = 0.0f;
            }
        }
    }
    
    // Пропускаем строку заголовка столбцов
    std::getline(input, line);
    
    // Чтение данных частиц
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string token;
        
        int id;
        float mass;
        float px, py, pz, vx, vy, vz;
        int dark_matter, movable;
        
        std::getline(ss, token, ','); id = std::stoi(token);
        std::getline(ss, token, ','); mass = std::stof(token);
        std::getline(ss, token, ','); px = std::stof(token);
        std::getline(ss, token, ','); py = std::stof(token);
        std::getline(ss, token, ','); pz = std::stof(token);
        std::getline(ss, token, ','); vx = std::stof(token);
        std::getline(ss, token, ','); vy = std::stof(token);
        std::getline(ss, token, ','); vz = std::stof(token);
        std::getline(ss, token, ','); dark_matter = std::stoi(token);
        std::getline(ss, token, ','); movable = std::stoi(token);
        
        // Создаем 3D векторы
        Vec3 position(px, py, pz);
        Vec3 velocity(vx, vy, vz);
        
        loaded_particles.emplace_back(
            id, position, velocity, mass,
            static_cast<bool>(dark_matter),
            static_cast<bool>(movable)
        );
    }
    
    std::cout << "Successfully loaded " << filename << std::endl;
    input.close();
    
    return std::make_pair(world_time, loaded_particles);
}