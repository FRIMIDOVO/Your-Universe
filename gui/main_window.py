from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtGui import QIcon
from gui.designs.design import Ui_MainWindow
from gui.graphic_page import GraphicPage
from bin.universe_py import Universe
import queue
from scipy.spatial.transform import Rotation as R
import numpy as np


class MainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self):
        super().__init__()
        self.setupUi(self)

        # Страницы графиков
        self.graphic_page = GraphicPage(self, self.graphicsView_density, self.graphicsView_rot_curve)

        self.setWindowIcon(QIcon("gui/designs/galaxy.ico"))

        # Безопасная очередь команд
        self.command_queue = queue.Queue()  # (<name>, args)

        # Configs
        self.checkBox_calculate.toggled.connect(lambda flag: self.command_queue.put(("switch_calculate", flag)))
        self.checkBox_render.toggled.connect(lambda flag: self.command_queue.put(("switch_render", flag)))

        self.horizontalSlider_dt.valueChanged.connect(lambda val: self.command_queue.put(("set_dt", val)))
        self.horizontalSlider_elasticity.valueChanged.connect(lambda val: self.command_queue.put(("set_elasticity", val)))
        self.horizontalSlider_theta.valueChanged.connect(lambda val: self.command_queue.put(("set_theta", val)))

        # SPH Configs
        self.horizontalSlider_h.valueChanged.connect(lambda val: self.command_queue.put(("set_h", val)))
        self.horizontalSlider_stiffness.valueChanged.connect(lambda val: self.command_queue.put(("set_stiffness", val)))
        self.horizontalSlider_cs.valueChanged.connect(lambda val: self.command_queue.put(("set_cs", val)))
        self.horizontalSlider_alpha.valueChanged.connect(lambda val: self.command_queue.put(("set_alpha", val)))
        self.horizontalSlider_gamma.valueChanged.connect(lambda val: self.command_queue.put(("set_gamma", val)))

        # Simulation
        self.pushButton_spawn.clicked.connect(self.pushButton_spawn_clicked)
        self.pushButton_reset.clicked.connect(lambda: self.command_queue.put(("engine_reset", None)))
        self.pushButton_delete_dm.clicked.connect(lambda: self.command_queue.put(("delete_dm", None)))

        # Save / Load
        self.pushButton_load.clicked.connect(self.pushButton_load_clicked)
        self.pushButton_save.clicked.connect(self.pushButton_save_clicked)

        self.show()

    def update_configs(self, u: Universe):
        """Принудительно обновляет все настройки в движке"""
        
        # Собираем актуальные значения из виджетов
        current_dt_val = self.horizontalSlider_dt.value()
        current_el_val = self.horizontalSlider_elasticity.value()
        current_th_val = self.horizontalSlider_theta.value()
        current_calc_flag = self.checkBox_calculate.isChecked()
        current_rend_flag = self.checkBox_render.isChecked()
        
        # SPH
        current_h_val = self.horizontalSlider_h.value()
        current_stiff_val = self.horizontalSlider_stiffness.value()
        current_cs_val = self.horizontalSlider_cs.value()
        current_alpha_val = self.horizontalSlider_alpha.value()
        current_gamma_val = self.horizontalSlider_gamma.value()

        # Кладем их в очередь
        self.command_queue.put(("switch_calculate", current_calc_flag))
        self.command_queue.put(("switch_render", current_rend_flag))
        self.command_queue.put(("set_dt", current_dt_val))
        self.command_queue.put(("set_elasticity", current_el_val))
        self.command_queue.put(("set_theta", current_th_val))
        
        self.command_queue.put(("set_h", current_h_val))
        self.command_queue.put(("set_stiffness", current_stiff_val))
        self.command_queue.put(("set_cs", current_cs_val))
        self.command_queue.put(("set_alpha", current_alpha_val))
        self.command_queue.put(("set_gamma", current_gamma_val))

        # Сразу же обрабатываем очередь
        self.process_commands(u)
                
    def process_commands(self, u: Universe):
        """Обрабатываем всю очередь команд"""
        while not self.command_queue.empty():
            cmd, *args = self.command_queue.get()

            if hasattr(self, cmd) and callable(getattr(self, cmd)):
                getattr(self, cmd)(u, *args)

    def switch_calculate(self, u: Universe, flag):
        u.switch_calculate(flag)

    def switch_render(self, u: Universe, flag):
        u.switch_render(flag)

    def set_dt(self, u: Universe, val):
        exponent = -3.0 + (val - 1) / 999.0 * 3.0
        u.set_dt(10 ** exponent)

    def set_elasticity(self, u: Universe, val):
        u.set_elasticity(val / 100)

    def set_theta(self, u: Universe, val):
        u.set_theta(val / 100)

    def set_h(self, u: Universe, val):
        # Например, от 0.1 до 10.0 (val от 1 до 100)
        h = 0.1 + (val - 1) / 99.0 * 9.9
        u.set_h(h)

    def set_stiffness(self, u: Universe, val):
        u.set_stiffness(val)

    def set_cs(self, u: Universe, val):
        u.set_cs(val)

    def set_alpha(self, u: Universe, val):
        alpha = val / 100.0
        u.set_alpha(alpha)

    def set_gamma(self, u: Universe, val):
        # Например, от 1.0 до 5.0 (val от 100 до 500)
        gamma = val / 100.0
        u.set_gamma(gamma)

    def pushButton_spawn_clicked(self):
        args = (
            self.spinBox_b_count.value(),
            self.doubleSpinBox_b_mass.value(),
            self.doubleSpinBox_b_radius.value(),
            self.spinBox_b_spawn_r.value(),
            self.spinBox_b_omega.value(),
            self.spinBox_dm_count.value(),
            self.doubleSpinBox_dm_mass.value(),
            self.doubleSpinBox_dm_radius.value(),
            self.spinBox_dm_spawn_r.value(),
            self.spinBox_dm_omega.value()
        )
        self.command_queue.put(("spawn_cluster", *args))

    def spawn_cluster(self, u: Universe, *args):
        u.spawn_cluster(*args)

    def engine_reset(self, u: Universe, _):
        u.engine_reset()

    def delete_dm(self, u: Universe, _):
        u.delete_dm()

    def pushButton_load_clicked(self):
        rx = self.doubleSpinBox_rot_x.value()
        ry = self.doubleSpinBox_rot_y.value()
        rz = self.doubleSpinBox_rot_z.value()
        rot = R.from_euler('XYZ', [rx, ry, rz], degrees=True)
        rotation_matrix = rot.as_matrix()
        args = (
            self.lineEdit_save_dir.text(),
            self.checkBox_normal.isChecked(),
            self.doubleSpinBox_dpx.value(),
            self.doubleSpinBox_dpy.value(),
            self.doubleSpinBox_dpz.value(),
            self.doubleSpinBox_dvx.value(),
            self.doubleSpinBox_dvy.value(),
            self.doubleSpinBox_dvz.value(),
            rotation_matrix
        )
        self.command_queue.put(("load_cluster", *args))

    def load_cluster(self, u: Universe, *args):
        u.load_cluster(*args)

    def pushButton_save_clicked(self):
        path = self.lineEdit_save_dir.text()
        self.command_queue.put(("save_cluster", path))

    def save_cluster(self, u: Universe, path):
        u.save_cluster(path)