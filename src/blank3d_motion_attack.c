#include "blank3d_motion_attack.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>

#define B3D_Q16_MAX 2147483647L
#define B3D_Q16_MIN (-2147483647L - 1L)

static long b3d_q12_to_q16(g3d_fix value)
{
    long v;
    v = (long)value;
    if (v > B3D_Q16_MAX / 16L) return B3D_Q16_MAX;
    if (v < B3D_Q16_MIN / 16L) return B3D_Q16_MIN;
    return v * 16L;
}

static g3d_fix b3d_q16_to_q12(long value)
{
    long v;
    v = value / 16L;
    if (v > (long)INT_MAX) v = (long)INT_MAX;
    if (v < (long)INT_MIN) v = (long)INT_MIN;
    return (g3d_fix)v;
}

static int b3d_text_equal(const char *left, const char *right)
{
    unsigned char a;
    unsigned char b;
    if (!left || !right) return 0;
    while (*left && *right) {
        a = (unsigned char)*left;
        b = (unsigned char)*right;
        if (a == '-' || a == ' ') a = '_';
        if (b == '-' || b == ' ') b = '_';
        if (tolower(a) != tolower(b)) return 0;
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

static g3d_fix b3d_abs_fix(g3d_fix value)
{
    return value < 0 ? g3d_fix_neg_sat(value) : value;
}

static g3d_fix b3d_clamp01(g3d_fix value)
{
    if (value < 0) return 0;
    if (value > G3D_FIX_ONE) return G3D_FIX_ONE;
    return value;
}

static gal_vec3 b3d_vec_to_gal(const Vec3 *value)
{
    gal_vec3 result;
    result.x = value ? b3d_q12_to_q16(value->x) : 0;
    result.y = value ? b3d_q12_to_q16(value->y) : 0;
    result.z = value ? b3d_q12_to_q16(value->z) : 0;
    return result;
}

static ggl_vec3 b3d_vec_to_ggl(const Vec3 *value)
{
    ggl_vec3 result;
    result.x = value ? b3d_q12_to_q16(value->x) : 0;
    result.y = value ? b3d_q12_to_q16(value->y) : 0;
    result.z = value ? b3d_q12_to_q16(value->z) : 0;
    return result;
}

static Vec3 b3d_vec_from_gal(const gal_vec3 *value)
{
    if (!value) return gamlib_vec3(0, 0, 0);
    return gamlib_vec3(b3d_q16_to_q12(value->x),
                       b3d_q16_to_q12(value->y),
                       b3d_q16_to_q12(value->z));
}

static Vec3 b3d_vec_from_ggl(const ggl_vec3 *value)
{
    if (!value) return gamlib_vec3(0, 0, 0);
    return gamlib_vec3(b3d_q16_to_q12(value->x),
                       b3d_q16_to_q12(value->y),
                       b3d_q16_to_q12(value->z));
}

static void b3d_motion_target(const Blank3DMotionAttack *attack,
                              const Transform *actor,
                              const Vec3 *contact_target,
                              int air_mode,
                              Vec3 *out_motion_target)
{
    Vec3 delta;
    Vec3 direction;
    Vec3 extension;
    int pierces;
    if (!out_motion_target) return;
    *out_motion_target = contact_target
                       ? *contact_target
                       : gamlib_vec3(0, 0, 0);
    if (!attack || !actor || !contact_target) return;
    pierces = air_mode
            ? attack->air.cfg.intent == GAL_INTENT_PIERCE
            : attack->ground.cfg.contact_policy == GGL_CONTACT_PIERCE;
    if (!pierces || attack->pierce_extension <= 0) return;
    gamlib_vec3_sub(&delta, contact_target, &actor->position);
    if (!air_mode) delta.y = 0;
    if (gamlib_vec3_length(&delta) <= G3D_FIX_EPSILON) return;
    gamlib_vec3_normalize(&direction, &delta);
    gamlib_vec3_scale(&extension, &direction, attack->pierce_extension);
    gamlib_vec3_add(out_motion_target, contact_target, &extension);
}

static int b3d_contact_predicted(const Vec3 *position,
                                 const Vec3 *target,
                                 g3d_fix contact_radius,
                                 g3d_fix step_distance,
                                 int flat_only)
{
    Vec3 delta;
    g3d_fix threshold;
    if (!position || !target) return 0;
    gamlib_vec3_sub(&delta, target, position);
    if (flat_only) delta.y = 0;
    threshold = g3d_fix_add_sat(contact_radius, step_distance);
    return gamlib_vec3_length(&delta) <= threshold;
}

static void b3d_ground_stop_near_target(const Vec3 *position,
                                        const Vec3 *target,
                                        g3d_fix contact_radius,
                                        g3d_fix floor_y,
                                        Vec3 *out_position)
{
    Vec3 delta;
    Vec3 direction;
    Vec3 movement;
    g3d_fix distance;
    g3d_fix advance;

    if (!out_position) return;
    if (!position) {
        *out_position = gamlib_vec3(0, floor_y, 0);
        return;
    }
    *out_position = *position;
    out_position->y = floor_y;
    if (!target) return;

    gamlib_vec3_sub(&delta, target, position);
    delta.y = 0;
    distance = gamlib_vec3_length(&delta);
    if (distance <= contact_radius || distance <= G3D_FIX_EPSILON) return;

    gamlib_vec3_normalize(&direction, &delta);
    advance = g3d_fix_sub_sat(distance, contact_radius);
    gamlib_vec3_scale(&movement, &direction, advance);
    gamlib_vec3_add(out_position, position, &movement);
    out_position->y = floor_y;
}

void blank3d_motion_attack_init(Blank3DMotionAttack *attack)
{
    gal_config air_cfg;
    ggl_config ground_cfg;
    if (!attack) return;
    memset(attack, 0, sizeof(*attack));
    gal_default_config(&air_cfg);
    air_cfg.acceleration = 0;
    air_cfg.arrival_radius = GAL_FX_ONE / 8L;
    air_cfg.max_active_ticks = 240UL;
    air_cfg.recovery_ticks = 6UL;
    air_cfg.target_policy = GAL_TARGET_TRACK_EACH_STEP;
    gal_init(&attack->air, &air_cfg);

    ggl_default_config(&ground_cfg);
    ground_cfg.acceleration = 0;
    ground_cfg.arrival_radius = GGL_FX_ONE / 8L;
    ground_cfg.max_charge_ticks = 360UL;
    ground_cfg.recovery_ticks = 8UL;
    ground_cfg.track_target = 1U;
    ggl_init(&attack->ground, &ground_cfg);

    attack->mode = B3D_MOTION_ATTACK_NONE;
    attack->velocity_per_tick = gamlib_vec3(0, 0, 0);
    attack->speed_per_second = G3D_FIX_FROM_INT(12);
    attack->contact_radius = G3D_FIX_FROM_INT(1);
    attack->pierce_extension = G3D_FIX_FROM_INT(6);
    attack->initialized = 1;
}

void blank3d_motion_attack_init_archetype(Blank3DMotionAttack *attack,
                                           const char *archetype)
{
    g3d_fix steering;
    g3d_fix keep;
    blank3d_motion_attack_init(attack);
    if (!attack || !archetype) return;

    if (strstr(archetype, "air_lunger") != 0 ||
        strstr(archetype, "airlunge") != 0 ||
        strstr(archetype, "midair_lunger") != 0) {
        steering = g3d_fix_div(G3D_FIX_FROM_INT(13),
                               G3D_FIX_FROM_INT(20));
        keep = g3d_fix_div(G3D_FIX_FROM_INT(3),
                           G3D_FIX_FROM_INT(20));
        (void)blank3d_motion_attack_set_air_intent(attack, "claw");
        (void)blank3d_motion_attack_set_air_target_policy(attack, "track");
        blank3d_motion_attack_set_air_steering(attack, steering);
        blank3d_motion_attack_set_air_velocity_keep(attack, keep);
        blank3d_motion_attack_set_contact_radius(attack, G3D_FIX_FROM_INT(1));
    } else if (strstr(archetype, "ground_lancer") != 0 ||
               strstr(archetype, "groundlance") != 0) {
        /* A ground lancer is a committed, floor-locked knight charge.
           Pegasus/skim-hop remain opt-in styles through FPIL actions. */
        (void)blank3d_motion_attack_set_ground_style(attack, "lance");
        (void)blank3d_motion_attack_set_ground_contact(attack, "stop");
        blank3d_motion_attack_set_ground_track(attack, 1);
        blank3d_motion_attack_set_ground_hop_height(attack, 0);
        blank3d_motion_attack_set_contact_radius(attack, G3D_FIX_FROM_INT(1));
    } else if (strstr(archetype, "pegasus") != 0) {
        g3d_fix hop_height;
        hop_height = g3d_fix_div(G3D_FIX_FROM_INT(7),
                                 G3D_FIX_FROM_INT(20));
        (void)blank3d_motion_attack_set_ground_style(attack, "pegasus");
        (void)blank3d_motion_attack_set_ground_contact(attack, "stop");
        blank3d_motion_attack_set_ground_track(attack, 1);
        blank3d_motion_attack_set_ground_hop_height(attack, hop_height);
        blank3d_motion_attack_set_contact_radius(attack, G3D_FIX_FROM_INT(1));
    }
}

void blank3d_motion_attack_reset(Blank3DMotionAttack *attack)
{
    if (!attack) return;
    gal_reset(&attack->air);
    ggl_reset(&attack->ground);
    attack->mode = B3D_MOTION_ATTACK_NONE;
    attack->velocity_per_tick = gamlib_vec3(0, 0, 0);
    attack->trigger_pending = 0;
    attack->cancel_pending = 0;
    attack->impact_latched = 0;
    attack->finished_latched = 0;
    attack->was_contacting = 0;
}

int blank3d_motion_attack_set_air_intent(Blank3DMotionAttack *attack,
                                         const char *intent)
{
    if (!attack || !intent) return 0;
    if (b3d_text_equal(intent, "push"))
        attack->air.cfg.intent = GAL_INTENT_PUSH;
    else if (b3d_text_equal(intent, "ram") ||
             b3d_text_equal(intent, "body"))
        attack->air.cfg.intent = GAL_INTENT_RAM;
    else if (b3d_text_equal(intent, "claw") ||
             b3d_text_equal(intent, "slash"))
        attack->air.cfg.intent = GAL_INTENT_CLAW;
    else if (b3d_text_equal(intent, "pierce") ||
             b3d_text_equal(intent, "through"))
        attack->air.cfg.intent = GAL_INTENT_PIERCE;
    else return 0;
    return 1;
}

int blank3d_motion_attack_set_air_target_policy(Blank3DMotionAttack *attack,
                                                const char *policy)
{
    if (!attack || !policy) return 0;
    if (b3d_text_equal(policy, "snapshot") ||
        b3d_text_equal(policy, "locked"))
        attack->air.cfg.target_policy = GAL_TARGET_SNAPSHOT;
    else if (b3d_text_equal(policy, "track") ||
             b3d_text_equal(policy, "tracking") ||
             b3d_text_equal(policy, "each_step"))
        attack->air.cfg.target_policy = GAL_TARGET_TRACK_EACH_STEP;
    else return 0;
    return 1;
}

void blank3d_motion_attack_set_air_steering(Blank3DMotionAttack *attack,
                                            g3d_fix steering)
{
    if (!attack) return;
    steering = b3d_clamp01(steering);
    attack->air.cfg.steering = b3d_q12_to_q16(steering);
}

void blank3d_motion_attack_set_air_velocity_keep(Blank3DMotionAttack *attack,
                                                 g3d_fix keep)
{
    if (!attack) return;
    keep = b3d_clamp01(keep);
    attack->air.cfg.velocity_keep = b3d_q12_to_q16(keep);
}

void blank3d_motion_attack_set_air_impulse(Blank3DMotionAttack *attack,
                                           g3d_fix impulse)
{
    if (!attack) return;
    attack->air.cfg.impact_impulse = b3d_q12_to_q16(b3d_abs_fix(impulse));
}

void blank3d_motion_attack_set_air_pierce_contacts(
    Blank3DMotionAttack *attack, unsigned int contacts)
{
    if (!attack) return;
    attack->air.cfg.pierce_contacts = contacts;
}

int blank3d_motion_attack_set_ground_style(Blank3DMotionAttack *attack,
                                            const char *style)
{
    if (!attack || !style) return 0;
    if (b3d_text_equal(style, "lance") ||
        b3d_text_equal(style, "charge"))
        attack->ground.cfg.style = GGL_STYLE_LANCE;
    else if (b3d_text_equal(style, "pegasus") ||
             b3d_text_equal(style, "pegasus_fist"))
        attack->ground.cfg.style = GGL_STYLE_PEGASUS;
    else if (b3d_text_equal(style, "ram") ||
             b3d_text_equal(style, "battering_ram"))
        attack->ground.cfg.style = GGL_STYLE_RAM;
    else if (b3d_text_equal(style, "skim_hop") ||
             b3d_text_equal(style, "skimhop") ||
             b3d_text_equal(style, "microhop"))
        attack->ground.cfg.style = GGL_STYLE_SKIM_HOP;
    else return 0;
    return 1;
}

int blank3d_motion_attack_set_ground_contact(Blank3DMotionAttack *attack,
                                              const char *policy)
{
    if (!attack || !policy) return 0;
    if (b3d_text_equal(policy, "stop"))
        attack->ground.cfg.contact_policy = GGL_CONTACT_STOP;
    else if (b3d_text_equal(policy, "bounce") ||
             b3d_text_equal(policy, "rebound"))
        attack->ground.cfg.contact_policy = GGL_CONTACT_BOUNCE;
    else if (b3d_text_equal(policy, "pierce") ||
             b3d_text_equal(policy, "through"))
        attack->ground.cfg.contact_policy = GGL_CONTACT_PIERCE;
    else return 0;
    return 1;
}

void blank3d_motion_attack_set_ground_track(Blank3DMotionAttack *attack,
                                             int enabled)
{
    if (!attack) return;
    attack->ground.cfg.track_target = enabled ? 1U : 0U;
}

void blank3d_motion_attack_set_ground_impulse(Blank3DMotionAttack *attack,
                                               g3d_fix impulse)
{
    if (!attack) return;
    attack->ground.cfg.impact_impulse = b3d_q12_to_q16(b3d_abs_fix(impulse));
}

void blank3d_motion_attack_set_ground_hop_height(
    Blank3DMotionAttack *attack, g3d_fix height)
{
    if (!attack) return;
    attack->ground.cfg.hop_height = b3d_q12_to_q16(b3d_abs_fix(height));
}

void blank3d_motion_attack_set_ground_pierce_contacts(
    Blank3DMotionAttack *attack, unsigned int contacts)
{
    if (!attack) return;
    attack->ground.cfg.pierce_contacts = contacts;
}

void blank3d_motion_attack_set_contact_radius(Blank3DMotionAttack *attack,
                                               g3d_fix radius)
{
    if (!attack) return;
    attack->contact_radius = b3d_abs_fix(radius);
}

void blank3d_motion_attack_set_pierce_extension(Blank3DMotionAttack *attack,
                                                 g3d_fix distance)
{
    if (!attack) return;
    attack->pierce_extension = b3d_abs_fix(distance);
}

int blank3d_motion_attack_request_air(Blank3DMotionAttack *attack,
                                      g3d_fix speed_per_second)
{
    if (!attack || !attack->initialized) return 0;
    if (attack->mode == B3D_MOTION_ATTACK_GROUND_LANCE &&
        blank3d_motion_attack_is_active(attack)) return 0;
    if (attack->mode != B3D_MOTION_ATTACK_AIR_LUNGE ||
        attack->air.state == GAL_STATE_DONE ||
        attack->air.state == GAL_STATE_CANCELLED) {
        gal_reset(&attack->air);
        attack->mode = B3D_MOTION_ATTACK_AIR_LUNGE;
        attack->trigger_pending = 1;
        attack->finished_latched = 0;
        attack->impact_latched = 0;
        attack->was_contacting = 0;
        attack->velocity_per_tick = gamlib_vec3(0, 0, 0);
    }
    attack->speed_per_second = b3d_abs_fix(speed_per_second);
    return 1;
}

int blank3d_motion_attack_request_ground(Blank3DMotionAttack *attack,
                                         g3d_fix speed_per_second,
                                         int grounded)
{
    if (!attack || !attack->initialized || !grounded) return 0;
    if (attack->mode == B3D_MOTION_ATTACK_AIR_LUNGE &&
        blank3d_motion_attack_is_active(attack)) return 0;
    if (attack->mode != B3D_MOTION_ATTACK_GROUND_LANCE ||
        attack->ground.state == GGL_STATE_DONE ||
        attack->ground.state == GGL_STATE_CANCELLED) {
        ggl_reset(&attack->ground);
        attack->mode = B3D_MOTION_ATTACK_GROUND_LANCE;
        attack->trigger_pending = 1;
        attack->finished_latched = 0;
        attack->impact_latched = 0;
        attack->was_contacting = 0;
        attack->velocity_per_tick = gamlib_vec3(0, 0, 0);
    }
    attack->speed_per_second = b3d_abs_fix(speed_per_second);
    return 1;
}

void blank3d_motion_attack_cancel(Blank3DMotionAttack *attack)
{
    if (!attack) return;
    attack->cancel_pending = 1;
}

static int b3d_tick_air(Blank3DMotionAttack *attack,
                        Transform *actor,
                        const Vec3 *contact_target,
                        g3d_fix floor_y,
                        int grounded,
                        g3d_fix dt,
                        Vec3 *out_facing_direction)
{
    gal_input in;
    gal_output out;
    Vec3 motion_target;
    Vec3 movement;
    Vec3 facing;
    g3d_fix step_distance;
    int contact_now;
    int contact_event;

    step_distance = g3d_fix_mul(attack->speed_per_second, dt);
    attack->air.cfg.launch_speed = b3d_q12_to_q16(step_distance);
    attack->air.cfg.max_speed = b3d_q12_to_q16(step_distance);
    attack->air.cfg.acceleration = 0;
    if (attack->air.state == GAL_STATE_ACTIVE)
        attack->air.current_speed = attack->air.cfg.max_speed;

    b3d_motion_target(attack, actor, contact_target, 1, &motion_target);
    memset(&in, 0, sizeof(in));
    in.position = b3d_vec_to_gal(&actor->position);
    in.velocity = b3d_vec_to_gal(&attack->velocity_per_tick);
    in.target_position = b3d_vec_to_gal(&motion_target);
    in.target_valid = contact_target ? 1U : 0U;
    in.airborne = grounded ? 0U : 1U;
    in.trigger = attack->trigger_pending ? 1U : 0U;
    in.cancel = attack->cancel_pending ? 1U : 0U;

    contact_now = b3d_contact_predicted(&actor->position, contact_target,
        attack->contact_radius, step_distance, 0);
    contact_event = contact_now && !attack->was_contacting;
    attack->was_contacting = contact_now;
    in.contact = contact_event ? 1U : 0U;

    gal_step(&attack->air, &in, &out);
    attack->trigger_pending = 0;
    attack->cancel_pending = 0;

    if (out.write_velocity) {
        movement = b3d_vec_from_gal(&out.desired_velocity);
        attack->velocity_per_tick = movement;
        gamlib_vec3_add(&actor->position, &actor->position, &movement);
        if (actor->position.y < floor_y) actor->position.y = floor_y;
    }
    facing = b3d_vec_from_gal(&out.facing_direction);
    if (out_facing_direction) *out_facing_direction = facing;
    if (out.impact_event) attack->impact_latched = 1;
    if (out.finished) attack->finished_latched = 1;
    return 1;
}

static int b3d_tick_ground(Blank3DMotionAttack *attack,
                           Transform *actor,
                           const Vec3 *contact_target,
                           g3d_fix floor_y,
                           g3d_fix dt,
                           Vec3 *out_facing_direction)
{
    ggl_input in;
    ggl_output out;
    Vec3 motion_target;
    Vec3 desired_position;
    Vec3 desired_velocity;
    Vec3 facing;
    g3d_fix step_distance;
    int contact_now;
    int contact_event;

    step_distance = g3d_fix_mul(attack->speed_per_second, dt);
    attack->ground.cfg.speed = b3d_q12_to_q16(step_distance);
    attack->ground.cfg.max_speed = b3d_q12_to_q16(step_distance);
    attack->ground.cfg.acceleration = 0;
    if (attack->ground.state == GGL_STATE_CHARGING)
        attack->ground.current_speed = attack->ground.cfg.max_speed;

    b3d_motion_target(attack, actor, contact_target, 0, &motion_target);
    memset(&in, 0, sizeof(in));
    in.position = b3d_vec_to_ggl(&actor->position);
    in.velocity = b3d_vec_to_ggl(&attack->velocity_per_tick);
    in.target_position = b3d_vec_to_ggl(&motion_target);
    in.target_valid = contact_target ? 1U : 0U;
    in.ground_valid = 1U;
    in.ground_height = b3d_q12_to_q16(floor_y);
    in.ground_normal.x = 0;
    in.ground_normal.y = GGL_FX_ONE;
    in.ground_normal.z = 0;
    in.trigger = attack->trigger_pending ? 1U : 0U;
    in.cancel = attack->cancel_pending ? 1U : 0U;

    contact_now = b3d_contact_predicted(&actor->position, contact_target,
        attack->contact_radius, step_distance, 1);
    contact_event = contact_now && !attack->was_contacting;
    attack->was_contacting = contact_now;
    in.contact = contact_event ? 1U : 0U;

    ggl_step(&attack->ground, &in, &out);
    attack->trigger_pending = 0;
    attack->cancel_pending = 0;

    desired_velocity = b3d_vec_from_ggl(&out.desired_velocity);
    if (attack->ground.cfg.style == GGL_STYLE_LANCE ||
        attack->ground.cfg.style == GGL_STYLE_RAM)
        desired_velocity.y = 0;

    if (out.write_position) {
        desired_position = b3d_vec_from_ggl(&out.desired_position);
        if (attack->ground.cfg.style == GGL_STYLE_LANCE ||
            attack->ground.cfg.style == GGL_STYLE_RAM)
            desired_position.y = floor_y;

        if (out.impact_event &&
            attack->ground.cfg.contact_policy == GGL_CONTACT_STOP) {
            /* End on the contact shell instead of consuming a full final
               frame and overlapping/passing the player. */
            b3d_ground_stop_near_target(&actor->position, contact_target,
                                        attack->contact_radius, floor_y,
                                        &desired_position);
            desired_velocity = gamlib_vec3(0, 0, 0);
        } else if (out.impact_event &&
                   attack->ground.cfg.contact_policy == GGL_CONTACT_BOUNCE) {
            gamlib_vec3_add(&desired_position, &actor->position,
                            &desired_velocity);
        }
        actor->position = desired_position;
        if (actor->position.y < floor_y) actor->position.y = floor_y;
    } else if (out.write_velocity) {
        gamlib_vec3_add(&actor->position, &actor->position,
                        &desired_velocity);
        if (actor->position.y < floor_y) actor->position.y = floor_y;
    }
    attack->velocity_per_tick = desired_velocity;
    facing = b3d_vec_from_ggl(&out.facing_direction);
    if (out_facing_direction) *out_facing_direction = facing;
    if (out.impact_event) attack->impact_latched = 1;
    if (out.finished) attack->finished_latched = 1;
    return 1;
}

int blank3d_motion_attack_tick(Blank3DMotionAttack *attack,
                               Transform *actor,
                               const Vec3 *contact_target,
                               g3d_fix floor_y,
                               int grounded,
                               g3d_fix dt,
                               Vec3 *out_facing_direction)
{
    if (out_facing_direction)
        *out_facing_direction = gamlib_vec3(0, 0, 0);
    if (!attack || !attack->initialized || !actor || dt <= 0) return 0;
    if (attack->mode == B3D_MOTION_ATTACK_AIR_LUNGE)
        return b3d_tick_air(attack, actor, contact_target, floor_y,
                            grounded, dt, out_facing_direction);
    if (attack->mode == B3D_MOTION_ATTACK_GROUND_LANCE)
        return b3d_tick_ground(attack, actor, contact_target, floor_y,
                               dt, out_facing_direction);
    return 0;
}

Blank3DMotionAttackMode blank3d_motion_attack_mode(
    const Blank3DMotionAttack *attack)
{
    return attack ? attack->mode : B3D_MOTION_ATTACK_NONE;
}

int blank3d_motion_attack_is_active(const Blank3DMotionAttack *attack)
{
    if (!attack) return 0;
    if (attack->mode == B3D_MOTION_ATTACK_AIR_LUNGE)
        return attack->air.state == GAL_STATE_ACTIVE ||
               attack->air.state == GAL_STATE_RECOVERY;
    if (attack->mode == B3D_MOTION_ATTACK_GROUND_LANCE)
        return attack->ground.state == GGL_STATE_CHARGING ||
               attack->ground.state == GGL_STATE_RECOVERY;
    return 0;
}

int blank3d_motion_attack_controls_vertical(
    const Blank3DMotionAttack *attack)
{
    if (!attack) return 0;
    if (attack->mode == B3D_MOTION_ATTACK_AIR_LUNGE)
        return attack->air.state == GAL_STATE_ACTIVE;
    if (attack->mode == B3D_MOTION_ATTACK_GROUND_LANCE)
        return attack->ground.state == GGL_STATE_CHARGING;
    return 0;
}

int blank3d_motion_attack_has_impact(const Blank3DMotionAttack *attack)
{
    return attack && attack->impact_latched;
}

void blank3d_motion_attack_clear_impact(Blank3DMotionAttack *attack)
{
    if (!attack) return;
    attack->impact_latched = 0;
}

int blank3d_motion_attack_finished(const Blank3DMotionAttack *attack)
{
    return attack && attack->finished_latched;
}

const char *blank3d_motion_attack_mode_name(
    const Blank3DMotionAttack *attack)
{
    if (!attack) return "none";
    if (attack->mode == B3D_MOTION_ATTACK_AIR_LUNGE)
        return "air_lunge";
    if (attack->mode == B3D_MOTION_ATTACK_GROUND_LANCE)
        return "ground_lance";
    return "none";
}
