#include "physics.h"
#include "physics_laws.h"


void PhysicsEngine::step() {
    hex_grid.update_cells(particles);
    Laws::inertia(particles, physics);
    Laws::gravity(particles, physics);
    Laws::elastic(particles, physics, hex_grid);
    Laws::friction(particles, physics, hex_grid);
    Laws::stickiness(particles, physics, hex_grid);
    Laws::bounce(particles, space);
    //Laws::periodic(particles, space);
    time += physics.dt;
}