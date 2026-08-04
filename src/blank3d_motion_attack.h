#ifndef BLANK3D_MOTION_ATTACK_H
#define BLANK3D_MOTION_ATTACK_H

#include "../vendor/gamlib3d/gamlib3d_transform.h"
#include "gairlunge89.h"
#include "ggroundlance89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Blank3DMotionAttackModeTag {
    B3D_MOTION_ATTACK_NONE = 0,
    B3D_MOTION_ATTACK_AIR_LUNGE,
    B3D_MOTION_ATTACK_GROUND_LANCE
} Blank3DMotionAttackMode;

typedef struct Blank3DMotionAttackTag {
    gal_controller air;
    ggl_controller ground;
    Blank3DMotionAttackMode mode;
    Vec3 velocity_per_tick;
    g3d_fix speed_per_second;
    g3d_fix contact_radius;
    g3d_fix pierce_extension;
    int trigger_pending;
    int cancel_pending;
    int impact_latched;
    int finished_latched;
    int was_contacting;
    int initialized;
} Blank3DMotionAttack;

void blank3d_motion_attack_init(Blank3DMotionAttack *attack);
void blank3d_motion_attack_init_archetype(Blank3DMotionAttack *attack,
                                           const char *archetype);
void blank3d_motion_attack_reset(Blank3DMotionAttack *attack);

int blank3d_motion_attack_set_air_intent(Blank3DMotionAttack *attack,
                                         const char *intent);
int blank3d_motion_attack_set_air_target_policy(Blank3DMotionAttack *attack,
                                                const char *policy);
void blank3d_motion_attack_set_air_steering(Blank3DMotionAttack *attack,
                                            g3d_fix steering);
void blank3d_motion_attack_set_air_velocity_keep(Blank3DMotionAttack *attack,
                                                 g3d_fix keep);
void blank3d_motion_attack_set_air_impulse(Blank3DMotionAttack *attack,
                                           g3d_fix impulse);
void blank3d_motion_attack_set_air_pierce_contacts(
    Blank3DMotionAttack *attack, unsigned int contacts);

int blank3d_motion_attack_set_ground_style(Blank3DMotionAttack *attack,
                                            const char *style);
int blank3d_motion_attack_set_ground_contact(Blank3DMotionAttack *attack,
                                              const char *policy);
void blank3d_motion_attack_set_ground_track(Blank3DMotionAttack *attack,
                                             int enabled);
void blank3d_motion_attack_set_ground_impulse(Blank3DMotionAttack *attack,
                                               g3d_fix impulse);
void blank3d_motion_attack_set_ground_hop_height(
    Blank3DMotionAttack *attack, g3d_fix height);
void blank3d_motion_attack_set_ground_pierce_contacts(
    Blank3DMotionAttack *attack, unsigned int contacts);

void blank3d_motion_attack_set_contact_radius(Blank3DMotionAttack *attack,
                                               g3d_fix radius);
void blank3d_motion_attack_set_pierce_extension(Blank3DMotionAttack *attack,
                                                 g3d_fix distance);

int blank3d_motion_attack_request_air(Blank3DMotionAttack *attack,
                                      g3d_fix speed_per_second);
int blank3d_motion_attack_request_ground(Blank3DMotionAttack *attack,
                                         g3d_fix speed_per_second,
                                         int grounded);
void blank3d_motion_attack_cancel(Blank3DMotionAttack *attack);

int blank3d_motion_attack_tick(Blank3DMotionAttack *attack,
                               Transform *actor,
                               const Vec3 *contact_target,
                               g3d_fix floor_y,
                               int grounded,
                               g3d_fix dt,
                               Vec3 *out_facing_direction);

Blank3DMotionAttackMode blank3d_motion_attack_mode(
    const Blank3DMotionAttack *attack);
int blank3d_motion_attack_is_active(const Blank3DMotionAttack *attack);
int blank3d_motion_attack_controls_vertical(
    const Blank3DMotionAttack *attack);
int blank3d_motion_attack_has_impact(const Blank3DMotionAttack *attack);
void blank3d_motion_attack_clear_impact(Blank3DMotionAttack *attack);
int blank3d_motion_attack_finished(const Blank3DMotionAttack *attack);
const char *blank3d_motion_attack_mode_name(
    const Blank3DMotionAttack *attack);

#ifdef __cplusplus
}
#endif

#endif
