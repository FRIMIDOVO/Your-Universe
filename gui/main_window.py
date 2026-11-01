from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtGui import QIcon
from gui.designs.design import Ui_MainWindow
from bin.universe_py import Universe
import queue


class MainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self):
        super().__init__()
        self.setupUi(self)

        self.setWindowIcon(QIcon("gui/designs/galaxy.ico"))

        # Безопасная очередь команд
        self.command_queue = queue.Queue()

        # Configs
        self.checkBox_calculate.toggled.connect(lambda: self.command_queue.put("switch_calculate"))
        self.checkBox_render.toggled.connect(lambda: self.command_queue.put("switch_render"))

        self.horizontalSlider_dt.valueChanged.connect(lambda: self.command_queue.put("set_dt"))
        self.horizontalSlider_elasticity.valueChanged.connect(lambda: self.command_queue.put("set_elasticity"))
        self.horizontalSlider_theta.valueChanged.connect(lambda: self.command_queue.put("set_theta"))

        # Simulation
        self.pushButton_spawn.clicked.connect(lambda: self.command_queue.put("spawn_cluster"))
        self.pushButton_reset.clicked.connect(lambda: self.command_queue.put("engine_reset"))

        self.show()

    def update_configs(self, u: Universe):
        """Для обновления всех конфигов"""
        commands = [ "switch_calculate",
                     "switch_render",
                     "set_dt",
                     "set_elasticity",
                     "set_theta" ]
        for cmd in commands:
            # Если метод существует
            if hasattr(self, cmd) and callable(getattr(self, cmd)):
                getattr(self, cmd)(u)   # вызываем

    def process_commands(self, u: Universe):
        """Обрабатываем всю очередь команд"""
        while not self.command_queue.empty():
            cmd= self.command_queue.get()

            # Если метод существует
            if hasattr(self, cmd) and callable(getattr(self, cmd)):
                getattr(self, cmd)(u)    # вызываем

    def switch_calculate(self, u: Universe):
        flag = self.checkBox_calculate.isChecked()
        u.switch_calculate(flag)

    def switch_render(self, u: Universe):
        flag = self.checkBox_render.isChecked()
        u.switch_render(flag)

    def set_dt(self, u: Universe):
        val = self.horizontalSlider_dt.value()
        # Делаем логарифмическую шкалу
        exponent = -3.0 + (val - 1) / 999.0 * 3.0
        dt = 10 ** exponent
        u.set_dt(dt)

    def set_elasticity(self, u: Universe):
        elasticity = self.horizontalSlider_elasticity.value() / 100
        u.set_elasticity(elasticity)

    def set_theta(self, u: Universe):
        theta = self.horizontalSlider_theta.value() / 100
        u.set_theta(theta)

    def spawn_cluster(self, u: Universe):
        u.spawn_cluster(
            # Baryon
            self.spinBox_b_count.value(),
            self.doubleSpinBox_b_mass.value(),
            self.doubleSpinBox_b_radius.value(),
            self.spinBox_b_spawn_r.value(),
            self.spinBox_b_omega.value(),
            # Dark
            self.spinBox_dm_count.value(),
            self.doubleSpinBox_dm_mass.value(),
            self.doubleSpinBox_dm_radius.value(),
            self.spinBox_dm_spawn_r.value(),
            self.spinBox_dm_omega.value(),
        )

    def engine_reset(self, u: Universe): u.engine_reset()