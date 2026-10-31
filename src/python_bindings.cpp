
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
            "calculate_cycle",
            &Universe::calculate_cycle,
            py::arg("frames"),
            py::call_guard<py::gil_scoped_release>()
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

        // NumPy arrays
        .def("get_pos_x", [](const Universe& self) {
            return to_numpy(self.get_posX());
        })

        .def("get_pos_y", [](const Universe& self) {
            return to_numpy(self.get_posY());
        })

        .def("get_pos_z", [](const Universe& self) {
            return to_numpy(self.get_posZ());
        })

        .def("get_dark", [](const Universe& self) {
            return to_numpy(self.get_dark());
        })

        .def("window_should_close", &Universe::window_should_close)

        // Save / Load
        .def("save_cluster", &Universe::save_cluster)

        .def(
            "load_cluster",
            &Universe::load_cluster,
            py::arg("save") = std::filesystem::path{}
        );
}
