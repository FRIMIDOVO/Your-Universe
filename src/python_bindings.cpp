#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include "universe.h"

#include <cstring>
#include <vector>

namespace py = pybind11;

template <typename T>
py::array_t<T> to_numpy(const std::vector<T>& data) {
    py::array_t<T> result(data.size());

    if (!data.empty()) {
        std::memcpy(
            result.mutable_data(),
            data.data(),
            data.size() * sizeof(T)
        );
    }

    return result;
}

PYBIND11_MODULE(universe_py, m) {
    m.doc() = "Your Universe simulation and visualization engine";

    py::class_<Universe>(m, "Universe")
        .def(py::init<>())

        // Simulation
        .def(
            "engine_step",
            &Universe::engine_step,
            py::call_guard<py::gil_scoped_release>()
        )

        .def(
            "engine_reset",
            &Universe::engine_reset
        )

        .def(
            "delete_dm",
            &Universe::delete_dm
        )

        .def(
            "render",
            &Universe::render
        )

        // Configuration
        .def("set_dt", &Universe::set_dt, py::arg("dt"))
        .def("set_theta", &Universe::set_theta, py::arg("theta"))
        .def("set_elasticity", &Universe::set_elasticity, py::arg("elasticity"))

        .def("switch_render", &Universe::switch_render)
        .def("switch_calculate", &Universe::switch_calculate)

        // Particles
        .def(
            "spawn_cluster",
            &Universe::spawn_cluster,
            py::arg("b_count"),
            py::arg("b_mass"),
            py::arg("b_radius"),
            py::arg("b_spawn_r"),
            py::arg("b_omega"),
            py::arg("dm_count"),
            py::arg("dm_mass"),
            py::arg("dm_radius"),
            py::arg("dm_spawn_r"),
            py::arg("dm_omega")
        )

        .def("get_particle_count", &Universe::get_particle_count)

        // State
        .def("get_time", &Universe::get_time)

        .def("window_should_close", &Universe::window_should_close)

        // Save / Load
        .def("save_cluster", 
             &Universe::save_cluster, 
             py::arg("save") = std::filesystem::path{},
             py::call_guard<py::gil_scoped_release>() // Отпускаем GIL на время дисковых операций
        )
        .def("load_cluster", 
            [](Universe& self, 
               const std::filesystem::path& save,
               bool normal,
               float dpx, float dpy, float dpz,
               float dvx, float dvy, float dvz,
               py::object rot_obj) 
            {
                // 1. Инициализируем единичной матрицей по умолчанию
                float c_rot[3][3] = {
                    {1.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f},
                    {0.0f, 0.0f, 1.0f}
                };
                
                // 2. Если передан аргумент rot, парсим его
                if (!rot_obj.is_none()) {
                    // forcecast автоматически преобразует python list/tuple в numpy array
                    py::array_t<float, py::array::c_style | py::array::forcecast> rot(rot_obj);
                    auto buf = rot.request();
                    
                    if (buf.size != 9) {
                        throw std::runtime_error("Matrix 'rot' must contain exactly 9 elements (3x3).");
                    }
                    
                    float* ptr = static_cast<float*>(buf.ptr);
                    for (int i = 0; i < 3; ++i) {
                        for (int j = 0; j < 3; ++j) {
                            c_rot[i][j] = ptr[i * 3 + j];
                        }
                    }
                }
                
                // 3. Отпускаем GIL перед долгими операциями чтения с диска и обработки частиц
                py::gil_scoped_release release;
                self.load_cluster(save, normal, dpx, dpy, dpz, dvx, dvy, dvz, c_rot);
            },
            py::arg("save") = std::filesystem::path{},
            py::arg("normal") = false,
            py::arg("dpx") = 0.0f, py::arg("dpy") = 0.0f, py::arg("dpz") = 0.0f,
            py::arg("dvx") = 0.0f, py::arg("dvy") = 0.0f, py::arg("dvz") = 0.0f,
            py::arg("rot") = py::none()
        )

        .def("density_profile", 
            []( const Universe& self,
                py::array_t<float> bin_r, 
                py::array_t<float> b_density, 
                py::array_t<float> dm_density
            )
            {
                // Получаем указатели на данные из numpy
                float* r_ptr = bin_r.mutable_data();
                float* b_ptr = b_density.mutable_data();
                float* dm_ptr = dm_density.mutable_data();

                self.density_profile(r_ptr, b_ptr, dm_ptr);
            },
            py::arg("bin_r"), py::arg("b_density"), py::arg("dm_density")
        )

        // SPH Configs
        .def("set_stiffness", &Universe::set_stiffness, py::arg("stiff"))
        .def("set_h", &Universe::set_h, py::arg("h"))
        .def("set_rest_density", &Universe::set_rest_density, py::arg("rest_dens"))
        .def("set_gamma", &Universe::set_gamma, py::arg("g"))
        .def("set_alpha", &Universe::set_alpha, py::arg("a"))
        .def("set_cs", &Universe::set_cs, py::arg("cs"))

        .def("rotation_curve", [](const Universe& self, 
                          py::array_t<float> r_b, py::array_t<float> b_tang_vel,
                          py::array_t<float> r_dm, py::array_t<float> dm_tang_vel) 
            {
            // Просто вызываем C++ функцию, передавая указатели на данные
            self.rotation_curve(
                r_b.mutable_data(),
                b_tang_vel.mutable_data(),
                r_dm.mutable_data(),
                dm_tang_vel.mutable_data()
            );
            },
            py::arg("r_b"), py::arg("b_tang_vel"), 
            py::arg("r_dm"), py::arg("dm_tang_vel")
        );
}