#ifndef SICOL_SOLVER_H
#define SICOL_SOLVER_H

/* ============================================================
 * SICOL - Warm-started contact solver
 * ------------------------------------------------------------
 * Linear + angular sequential impulse solver with:
 * - persistent manifolds
 * - lever arms / inverse inertia
 * - islands
 * - sleeping
 * - CCD-assisted integration
 * ============================================================ */

#include "sicol_world.h"

typedef struct {
    fx inv_mass;
    fx velocity[3];
    fx angular_velocity[3];
    fx inv_inertia_local[3];
    fx inv_inertia_tensor_local[3][3];
    fx force[3];
    fx torque[3];
    fx gravity_scale;
    fx restitution;
    fx friction;
    int can_sleep;
    int sleeping;
    unsigned int sleep_frames;
} sicol_solver_body_t;

typedef struct {
    fx dt;
    int velocity_iterations;
    int position_iterations;
    fx baumgarte;
    fx slop;
    int warm_start;
    int enable_friction;
    int enable_angular;
    int enable_islands;
    int enable_sleeping;
    fx sleep_linear_threshold;
    fx sleep_angular_threshold;
    int sleep_frames_threshold;
    int enable_ccd;
    fx ccd_min_speed;
    int ccd_max_impacts;
    fx gravity[3];
    fx linear_damping;
    fx angular_damping;
} sicol_solver_config_t;

typedef struct {
    int pair_count;
    int point_count;
    int joint_count;
    int island_count;
    int sleeping_count;
    int ccd_hits;
    fx total_normal_impulse;
    fx total_tangent_impulse;
    fx total_joint_impulse;
    fx total_angular_impulse;
    fx total_position_correction;
} sicol_solver_stats_t;

void sicol_solver_body_init(sicol_solver_body_t* body, fx inv_mass);
void sicol_solver_body_init_for_shape(sicol_solver_body_t* body, const sicol_shape_t* shape, fx inv_mass);
void sicol_solver_body_make_static(sicol_solver_body_t* body);
void sicol_solver_body_wake(sicol_solver_body_t* body);
void sicol_solver_body_set_inv_inertia_tensor(sicol_solver_body_t* body, fx tensor[3][3]);
void sicol_solver_body_add_force(sicol_solver_body_t* body, const fx force[3]);
void sicol_solver_body_add_torque(sicol_solver_body_t* body, const fx torque[3]);
void sicol_solver_body_clear_forces(sicol_solver_body_t* body);
void sicol_solver_config_default(sicol_solver_config_t* config);

int sicol_world_step_solver(
    sicol_world_t* world,
    sicol_solver_body_t bodies[SICOL_WORLD_MAX],
    const sicol_solver_config_t* config,
    sicol_solver_stats_t* out_stats
);

#endif /* SICOL_SOLVER_H */
