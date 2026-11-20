#include "galaxy_saver.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <system_error>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace GalaxySaver {

    namespace {

        // Без radius: px,py,pz,vx,vy,vz,mass,is_dark
        const char* const kColumns = "px,py,pz,vx,vy,vz,mass,is_dark";
        const int kFloatFields = 7;   // было 8
        const char* const kMetaFile = "metadata.json";

        bool read_row(const std::string& line, float* values, int& dark) {
            std::stringstream row(line);
            std::string token;

            try {
                for (int i = 0; i < kFloatFields; ++i) {
                    if (!std::getline(row, token, ',')) return false;
                    values[i] = std::stof(token);
                }
                if (!std::getline(row, token, ',')) return false;
                dark = std::stoi(token);
            } catch (...) {
                return false;
            }
            return true;
        }

    }  // namespace

    // ---------------------------------------------------------------- save

    void save(
        const std::string& dir,
        const std::vector<float>& posX, const std::vector<float>& posY, const std::vector<float>& posZ,
        const std::vector<float>& velX, const std::vector<float>& velY, const std::vector<float>& velZ,
        const std::vector<float>& mass, const std::vector<uint8_t>& is_dark
    ) {
        const std::size_t count = posX.size();

        if (posY.size() != count || posZ.size() != count ||
            velX.size() != count || velY.size() != count || velZ.size() != count ||
            mass.size() != count || is_dark.size() != count) {
            std::cerr << "Save failed: particle arrays have different sizes\n";
            return;
        }

        std::error_code error;
        fs::create_directories(dir, error);
        if (error) {
            std::cerr << "Save failed: cannot create " << dir << " (" << error.message() << ")\n";
            return;
        }

        const std::string filename = (fs::path(dir) / "galaxy.csv").string();
        std::ofstream out(filename);
        if (!out.is_open()) {
            std::cerr << "Save failed: cannot open " << filename << " for writing\n";
            return;
        }

        out << std::setprecision(std::numeric_limits<float>::max_digits10);
        out << kColumns << '\n';

        for (std::size_t i = 0; i < count; ++i) {
            out << posX[i] << ',' << posY[i] << ',' << posZ[i] << ','
                << velX[i] << ',' << velY[i] << ',' << velZ[i] << ','
                << mass[i] << ','
                << static_cast<int>(is_dark[i]) << '\n';
        }

        out.close();
        if (!out) {
            std::cerr << "Save failed: write error in " << filename << "\n";
            return;
        }

        std::cout << "Saved " << filename << " (" << count << " particles)\n";
    }

    void save_meta(
        const std::string& dir,
        const float& time,
        const float& dpx, const float& dpy, const float& dpz,
        const float& dvx, const float& dvy, const float& dvz,
        const float rot[3][3]
    ) {
        std::error_code error;
        fs::create_directories(dir, error);
        if (error) {
            std::cerr << "Save meta failed: cannot create " << dir
                      << " (" << error.message() << ")\n";
            return;
        }

        const float identity[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
        const float (*R)[3] = rot ? rot : identity;

        nlohmann::json meta;
        meta["time"] = time;

        meta["normalization"]["pos_offset"] = { dpx, dpy, dpz };
        meta["normalization"]["vel_offset"] = { dvx, dvy, dvz };

        nlohmann::json rot_json = nlohmann::json::array();
        for (int i = 0; i < 3; ++i) {
            rot_json.push_back({ R[i][0], R[i][1], R[i][2] });
        }
        meta["normalization"]["rotation_matrix"] = rot_json;

        const std::string filename = (fs::path(dir) / kMetaFile).string();
        std::ofstream out(filename);
        if (!out.is_open()) {
            std::cerr << "Save meta failed: cannot open " << filename << " for writing\n";
            return;
        }

        out << meta.dump(4);
        out.close();
        if (!out) {
            std::cerr << "Save meta failed: write error in " << filename << "\n";
            return;
        }

        std::cout << "Saved " << filename << "\n";
    }

    // ---------------------------------------------------------------- load

    void load(
        const std::string& dir,
        std::vector<float>& posX, std::vector<float>& posY, std::vector<float>& posZ,
        std::vector<float>& velX, std::vector<float>& velY, std::vector<float>& velZ,
        std::vector<float>& mass, std::vector<uint8_t>& is_dark
    ) {
        std::ifstream in(dir);
        if (!in.is_open()) {
            std::cerr << "Load failed: cannot open " << dir << "\n";
            return;
        }

        std::string line;
        std::getline(in, line);  // заголовок

        posX.clear(); posY.clear(); posZ.clear();
        velX.clear(); velY.clear(); velZ.clear();
        mass.clear(); is_dark.clear();

        std::size_t skipped = 0;
        while (std::getline(in, line)) {
            if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

            float values[kFloatFields];
            int dark = 0;
            if (!read_row(line, values, dark)) {
                ++skipped;
                continue;
            }

            posX.push_back(values[0]);
            posY.push_back(values[1]);
            posZ.push_back(values[2]);
            velX.push_back(values[3]);
            velY.push_back(values[4]);
            velZ.push_back(values[5]);
            mass.push_back(values[6]);
            is_dark.push_back(static_cast<uint8_t>(dark != 0));
        }

        if (skipped > 0) {
            std::cerr << "Load warning: skipped " << skipped << " broken line(s) in " << dir << "\n";
        }

        std::cout << "Loaded " << dir << " (" << posX.size() << " particles)\n";
    }

    void load_meta(
        const std::string& dir,
        float& time,
        float& dpx, float& dpy, float& dpz,
        float& dvx, float& dvy, float& dvz,
        float rot[3][3]
    ) {
        const fs::path filename = fs::path(dir) / kMetaFile;

        std::ifstream in(filename);
        if (!in.is_open()) {
            std::cerr << "Load meta: cannot open " << filename << "\n";
            return;
        }

        nlohmann::json meta;
        try {
            in >> meta;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "Load meta: parse error in " << filename
                      << ": " << e.what() << "\n";
            return;
        }

        if (meta.contains("time") && meta["time"].is_number()) {
            time = meta["time"].get<float>();
        }

        if (meta.contains("normalization") && meta["normalization"].is_object()) {
            const auto& norm = meta["normalization"];

            if (norm.contains("pos_offset") && norm["pos_offset"].is_array()
                && norm["pos_offset"].size() == 3) {
                dpx = norm["pos_offset"][0].get<float>();
                dpy = norm["pos_offset"][1].get<float>();
                dpz = norm["pos_offset"][2].get<float>();
            }

            if (norm.contains("vel_offset") && norm["vel_offset"].is_array()
                && norm["vel_offset"].size() == 3) {
                dvx = norm["vel_offset"][0].get<float>();
                dvy = norm["vel_offset"][1].get<float>();
                dvz = norm["vel_offset"][2].get<float>();
            }

            if (norm.contains("rotation_matrix") && norm["rotation_matrix"].is_array()
                && norm["rotation_matrix"].size() == 3) {
                for (int i = 0; i < 3; ++i) {
                    const auto& row = norm["rotation_matrix"][i];
                    if (row.is_array() && row.size() == 3) {
                        rot[i][0] = row[0].get<float>();
                        rot[i][1] = row[1].get<float>();
                        rot[i][2] = row[2].get<float>();
                    }
                }
            }
        }

        std::cout << "Loaded " << filename << "\n";
    }

}  // namespace GalaxySaver