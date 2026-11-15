from PyQt5.QtWidgets import QApplication
from bin.universe_py import Universe
from gui.main_window import MainWindow
import threading
import sys
import time


def main():
    universe = Universe()
    gui = MainWindow(); gui.update_configs(universe)

    def unicycle():
        while True:
            gui.process_commands(universe)
            universe.engine_step()
            universe.render()
            gui.graphic_page.update_density(universe)
            gui.graphic_page.update_curve(universe)

            time.sleep(0.001)   # костыль, что бы не лагало


    uni_thr = threading.Thread(target=unicycle, daemon=True)
    uni_thr.start()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    main()
    sys.exit(app.exec_())
