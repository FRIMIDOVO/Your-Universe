from PyQt5.QtCore import QObject, pyqtSignal, Qt
from PyQt5.QtWidgets import QVBoxLayout
import pyqtgraph as pg
import numpy as np
from bin.universe_py import Universe
import time


class GraphicPage(QObject):
    # Сигналы
    density_data_ready = pyqtSignal(object, object, object)
    curve_data_ready = pyqtSignal(object, object, object, object)

    def __init__(self, parent, graphicsView_density, graphicsView_rot_curve):
        super().__init__()

        self.parent = parent
        self.graphicsView_density = graphicsView_density
        self.graphicsView_rot_curve = graphicsView_rot_curve

        # ГРАФИК ПЛОТНОСТИ
        self.plot_density = pg.PlotWidget(title="Density Profile")
        self.plot_density.setBackground('k')
        self.plot_density.showGrid(x=True, y=True)
        self.plot_density.setLogMode(x=True, y=True)

        layout1 = QVBoxLayout(self.graphicsView_density)
        layout1.setContentsMargins(0, 0, 0, 0)
        layout1.addWidget(self.plot_density)

        # Линии для плотности
        self.line_b = self.plot_density.plot(pen=pg.mkPen('w', width=2), name="Baryons")
        self.line_dm = self.plot_density.plot(pen=pg.mkPen('c', width=2), name="Dark Matter")
        
        # Эталон r^-2
        self.line_ideal = self.plot_density.plot(pen=pg.mkPen('r', width=1, style=Qt.DashLine), name="r^-2")
        x_ideal = np.logspace(0, 2, 50)
        y_ideal = x_ideal ** -2
        self.line_ideal.setData(x_ideal, y_ideal)

        # Массивы для плотности
        NUM_BINS = 100
        self.bin_r = np.zeros(NUM_BINS, dtype=np.float32)
        self.b_density = np.zeros(NUM_BINS, dtype=np.float32)
        self.dm_density = np.zeros(NUM_BINS, dtype=np.float32)

        # ГРАФИК КРИВОЙ ВРАЩЕНИЯ
        self.plot_curve = pg.PlotWidget(title="Rotation Curve")
        self.plot_curve.setBackground('k')
        self.plot_curve.showGrid(x=True, y=True)
        self.plot_curve.setLogMode(x=False, y=False)

        layout2 = QVBoxLayout(self.graphicsView_rot_curve)
        layout2.setContentsMargins(0, 0, 0, 0)
        layout2.addWidget(self.plot_curve)

        # Точечные графики (Scatter)
        self.scatter_b = pg.ScatterPlotItem(size=2, pen=None, brush=pg.mkBrush(255, 255, 255, 100))
        self.scatter_dm = pg.ScatterPlotItem(size=2, pen=None, brush=pg.mkBrush(0, 255, 255, 100))
        self.plot_curve.addItem(self.scatter_b)
        self.plot_curve.addItem(self.scatter_dm)

        # Массивы для скоростей
        self.r_b = np.zeros(1, dtype=np.float32)
        self.v_b = np.zeros(1, dtype=np.float32)
        self.r_dm = np.zeros(1, dtype=np.float32)
        self.v_dm = np.zeros(1, dtype=np.float32)

        # Привязываем сигналы
        self.density_data_ready.connect(self.draw_density)
        self.curve_data_ready.connect(self.draw_curve)

        self.parent.checkBox_draw_curve_dm.clicked.connect(lambda flag: self.switch_draw_curve_dm(flag))

        # Флаги отрисовки
        self.draw_curve_dm = False

        self.last_time_density = time.time()
        self.last_time_curve = time.time()
        self.rest_time = 0.5

        self.density_idx = 4
        self.curve_idx = 5


    def update_density(self, u: Universe):
        if self.parent.tabWidget.currentIndex() != self.density_idx: return
        if time.time() - self.last_time_density > self.rest_time:
            u.density_profile(self.bin_r, self.b_density, self.dm_density)

            self.density_data_ready.emit(
                self.bin_r.copy(),
                self.b_density.copy(),
                self.dm_density.copy()
            )

            self.last_time_density = time.time()

    def update_curve(self, u: Universe):
        if self.parent.tabWidget.currentIndex() != self.curve_idx: return
        if time.time() - self.last_time_curve > self.rest_time:
            n_b, n_dm = u.get_particle_count()

            if self.r_b.size != n_b or self.v_b.size != n_b:
                self.r_b = np.zeros(n_b, dtype=np.float32)
                self.v_b = np.zeros(n_b, dtype=np.float32)

            if self.r_dm.size != n_dm or self.v_dm.size != n_dm:
                self.r_dm = np.zeros(n_dm, dtype=np.float32)
                self.v_dm = np.zeros(n_dm, dtype=np.float32)

            u.rotation_curve(self.r_b, self.v_b, self.r_dm, self.v_dm)

            self.curve_data_ready.emit(
                self.r_b.copy(),
                self.v_b.copy(),
                self.r_dm.copy(),
                self.v_dm.copy()
            )

            self.last_time_curve = time.time()


    def draw_density(self, bin_r, b_density, dm_density):
        self.line_b.setData(bin_r, b_density)
        self.line_dm.setData(bin_r, dm_density)

    def switch_draw_curve_dm(self, flag):
        self.draw_curve_dm = flag

    def draw_curve(self, r_b, v_b, r_dm, v_dm):
        self.scatter_b.setData(r_b, v_b)
        if self.draw_curve_dm:
            self.scatter_dm.setData(r_dm, v_dm)
        else:
            self.scatter_dm.clear()