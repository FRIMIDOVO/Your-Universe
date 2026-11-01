#include "galaxy_saver.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace GalaxySaver {

    // Генерирует имя файла с временным штампом
    static std::string make_timestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm* time_info = std::localtime(&time);
        
        std::stringstream ss;
        ss << std::put_time(time_info, "%Y-%m-%d_%H-%M-%S");
        
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        ss << '-' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    void save(
        const std::string& dir,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
        const std::vector<float>& mass,
        const std::vector<uint8_t>& is_dark,
        float world_time
    ) {
        fs::create_directories("saves/" + dir);
        std::string filename = "saves/" + dir + "/galaxy_" + make_timestamp() + ".csv";
        std::ofstream out(filename);
        
        if (!out.is_open()) {
            std::cerr << "Failed to open " << filename << " for writing!\n";
            return;
        }

        // Заголовок (world_time)
        out << "world_time," << world_time << "\n";
        out << "px,py,pz,vx,vy,vz,mass,is_dark\n";

        int N = static_cast<int>(posX.size());
        for (int i = 0; i < N; ++i) {
            out << posX[i] << "," << posY[i] << "," << posZ[i] << ","
                << velX[i] << "," << velY[i] << "," << velZ[i] << ","
                << mass[i] << ","
                << static_cast<int>(is_dark[i]) << "\n";
        }

        std::cout << "Successfully saved " << filename << "\n";
    }

    float load(
        const std::string& filename,
        std::vector<float>& posX, std::vector<float>& posY, std::vector<float>& posZ,
        std::vector<float>& velX, std::vector<float>& velY, std::vector<float>& velZ,
        std::vector<float>& mass,
        std::vector<uint8_t>& is_dark
    ) {
        std::ifstream in(filename);
        if (!in.is_open()) return 0.0f;

        float world_time = 0.0f;
        std::string line;

        // Читаем время
        if (std::getline(in, line)) {
            auto comma = line.find(',');
            if (comma != std::string::npos) {
                try { world_time = std::stof(line.substr(comma + 1)); } 
                catch (...) { world_time = 0.0f; }
            }
        }

        // Пропускаем заголовок
        std::getline(in, line);

        // Очищаем старые данные (на всякий случай)
        posX.clear(); posY.clear(); posZ.clear();
        velX.clear(); velY.clear(); velZ.clear();
        mass.clear(); is_dark.clear();

        // Резервируем память (чтобы не было реаллокаций)
        // (опционально можно посчитать количество строк заранее, но это долго)
        // Поэтому просто читаем.

        while (std::getline(in, line)) {
            if (line.empty()) continue;
            
            std::stringstream ss(line);
            std::string token;
            
            float px, py, pz, vx, vy, vz, m;
            int dm;

            std::getline(ss, token, ','); px = std::stof(token);
            std::getline(ss, token, ','); py = std::stof(token);
            std::getline(ss, token, ','); pz = std::stof(token);
            std::getline(ss, token, ','); vx = std::stof(token);
            std::getline(ss, token, ','); vy = std::stof(token);
            std::getline(ss, token, ','); vz = std::stof(token);
            std::getline(ss, token, ','); m = std::stof(token);
            std::getline(ss, token, ','); dm = std::stoi(token);

            posX.push_back(px);
            posY.push_back(py);
            posZ.push_back(pz);
            velX.push_back(vx);
            velY.push_back(vy);
            velZ.push_back(vz);
            mass.push_back(m);
            is_dark.push_back(static_cast<uint8_t>(dm));
        }

        std::cout << "Successfully loaded " << filename << " (" << posX.size() << " particles)\n";
        return world_time;
    }
}