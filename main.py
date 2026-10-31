import bin.universe_py as universe_api

universe = universe_api.Universe()

universe.set_theta(0)

universe.spawn_cluster(
    20000, 1, 0.5, 15, 5,
    20000, 1, 0.5, 20, 5
)

while not universe.window_should_close():
    universe.engine_step()
    universe.render()