#include "blank3d_gloco.h"
#include "blank3d_gloco_profile_ini.h"

#define B3D_GLOCO_SWEEP_SKIN_Q12 64L

#include <string.h>
#include <stdio.h>
#include <limits.h>

#define B3D_Q16_ONE 65536L
#define B3D_Q12_ONE 4096L
#define B3D_Q10_ONE 1024L

static GLOCO_FX b3d_q12_to_q8(g3d_fix v) { return (GLOCO_FX)((long)v / 16L); }
static g3d_fix b3d_q8_to_q12(GLOCO_FX v) { return (g3d_fix)(v * 16L); }
static GLOCO_FX b3d_q16_to_q8(long v) { return (GLOCO_FX)(v / 256L); }
static ns_fx b3d_q8_to_q10(GLOCO_FX v) { return (ns_fx)(v * 4L); }
static GLOCO_FX b3d_q10_to_q8(ns_fx v) { return (GLOCO_FX)(v / 4L); }

/* Multiply a Q12 displacement by a Q12 fraction without first forming the
   potentially overflowing value*frac product.  This matters on Win32 where
   long is 32-bit.  The fraction is expected in [0, 1]. */
static long b3d_mul_q12_fraction(long value, long fraction)
{
    long q;
    long r;
    long base;
    long tail;
    if (fraction <= 0L) return 0L;
    if (fraction > B3D_Q12_ONE) fraction = B3D_Q12_ONE;
    q = value / B3D_Q12_ONE;
    r = value % B3D_Q12_ONE;
    if (q > 0L && q > LONG_MAX / fraction) base = LONG_MAX;
    else if (q < 0L && q < LONG_MIN / fraction) base = LONG_MIN;
    else base = q * fraction;
    tail = (r * fraction) / B3D_Q12_ONE;
    if (tail > 0L && base > LONG_MAX - tail) return LONG_MAX;
    if (tail < 0L && base < LONG_MIN - tail) return LONG_MIN;
    return base + tail;
}

static void b3d_status(Blank3DGloco *g, const char *s)
{
    unsigned int n;
    if (!g) return;
    if (!s) s = "";
    n = (unsigned int)strlen(s);
    if (n >= sizeof(g->status)) n = sizeof(g->status) - 1U;
    memcpy(g->status, s, n);
    g->status[n] = '\0';
}

static GLOCO_Vec3 b3d_pos_to_gloco(const Vec3 *p)
{
    if (!p) return gloco_v3(0, 0, 0);
    return gloco_v3(b3d_q12_to_q8(p->x), b3d_q12_to_q8(p->y), b3d_q12_to_q8(p->z));
}


static Blank3DGlocoBinding *b3d_binding_by_gloco(Blank3DGloco *g, int gloco_id)
{
    int i;
    if (!g) return 0;
    for (i = 0; i < B3D_GLOCO_MAX_BINDINGS; ++i)
        if (g->bindings[i].used && g->bindings[i].gloco_id == gloco_id)
            return &g->bindings[i];
    return 0;
}

static Blank3DGlocoBinding *b3d_binding_by_actor(Blank3DGloco *g, int actor_id)
{
    int i;
    if (!g) return 0;
    for (i = 0; i < B3D_GLOCO_MAX_BINDINGS; ++i)
        if (g->bindings[i].used && g->bindings[i].actor_id == actor_id)
            return &g->bindings[i];
    return 0;
}

const Blank3DGlocoBinding *blank3d_gloco_binding(const Blank3DGloco *g, int actor_id)
{
    int i;
    if (!g) return 0;
    for (i = 0; i < B3D_GLOCO_MAX_BINDINGS; ++i)
        if (g->bindings[i].used && g->bindings[i].actor_id == actor_id)
            return &g->bindings[i];
    return 0;
}

static int b3d_stats_get(void *user, int gloco_id, GLOCO_FX *out_stamina)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    ns_fx v;
    g = (Blank3DGloco *)user;
    b = b3d_binding_by_gloco(g, gloco_id);
    if (!g || !b || !g->systems || b->stamina_value < 0 || !out_stamina)
        return GLOCO_PROVIDER_FALLBACK;
    if (ns_get_by_id(&g->systems->numbers, b->stamina_value, &v) != NS_OK)
        return GLOCO_PROVIDER_FALLBACK;
    *out_stamina = b3d_q10_to_q8(v);
    return GLOCO_PROVIDER_HANDLED;
}

static int b3d_stats_set(void *user, int gloco_id, GLOCO_FX stamina)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    g = (Blank3DGloco *)user;
    b = b3d_binding_by_gloco(g, gloco_id);
    if (!g || !b || !g->systems || b->stamina_value < 0)
        return GLOCO_PROVIDER_FALLBACK;
    if (ns_set_by_id(&g->systems->numbers, b->stamina_value,
                     b3d_q8_to_q10(stamina)) != NS_OK)
        return GLOCO_PROVIDER_FALLBACK;
    return GLOCO_PROVIDER_HANDLED;
}

static int b3d_stats_consume(void *user, int gloco_id, GLOCO_FX amount,
                             GLOCO_FX *out_stamina)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    ns_fx cur;
    ns_fx next;
    g = (Blank3DGloco *)user;
    b = b3d_binding_by_gloco(g, gloco_id);
    if (!g || !b || !g->systems || b->stamina_value < 0)
        return GLOCO_PROVIDER_FALLBACK;
    if (ns_get_by_id(&g->systems->numbers, b->stamina_value, &cur) != NS_OK)
        return GLOCO_PROVIDER_FALLBACK;
    next = cur - b3d_q8_to_q10(amount);
    if (next < 0) next = 0;
    if (ns_set_by_id(&g->systems->numbers, b->stamina_value, next) != NS_OK)
        return GLOCO_PROVIDER_FALLBACK;
    if (out_stamina) *out_stamina = b3d_q10_to_q8(next);
    return GLOCO_PROVIDER_HANDLED;
}

static int b3d_vertical(void *user, int gloco_id, GLOCO_Actor *actor,
                        const GLOCO_Profile *profile, GLOCO_U16 dt_ms)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    (void)profile;
    (void)dt_ms;
    g = (Blank3DGloco *)user;
    b = b3d_binding_by_gloco(g, gloco_id);
    if (!g || !b || !actor || !b->transform) return GLOCO_PROVIDER_FALLBACK;
    actor->pos.y = b3d_q12_to_q8(b->transform->position.y);
    actor->vel.y = 0;
    if (g->vertical_axis && b->vertical_body &&
        blank3d_vertical_axis_grounded(g->vertical_axis, b->vertical_body)) {
        actor->flags |= GLOCO_FLAG_GROUNDED;
        actor->ground_normal = gloco_v3(0, GLOCO_FX_ONE, 0);
    } else {
        actor->flags &= (GLOCO_U16)~GLOCO_FLAG_GROUNDED;
    }
    return GLOCO_PROVIDER_HANDLED;
}

static int b3d_integrate(void *user, int gloco_id, const GLOCO_Actor *actor,
                         GLOCO_U16 dt_ms, const GLOCO_Vec3 *from,
                         const GLOCO_Vec3 *builtin_to, GLOCO_Vec3 *out_to)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    (void)actor;
    (void)dt_ms;
    (void)from;
    g = (Blank3DGloco *)user;
    b = b3d_binding_by_gloco(g, gloco_id);
    if (!b || !b->transform || !out_to || !builtin_to)
        return GLOCO_PROVIDER_FALLBACK;
    *out_to = *builtin_to;
    out_to->y = b3d_q12_to_q8(b->transform->position.y);
    return GLOCO_PROVIDER_HANDLED;
}

static int b3d_physics_move(void *user, int gloco_id, GLOCO_Actor *actor,
                            const GLOCO_Profile *profile, GLOCO_U16 dt_ms,
                            const GLOCO_Vec3 *from, const GLOCO_Vec3 *to,
                            GLOCO_ProbeResult *out)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    GWP89_Vec3 start;
    GWP89_Vec3 delta;
    Blank3DCollisionHit hit;
    long frac;
    long dx;
    long dz;
    (void)dt_ms;
    g = (Blank3DGloco *)user;
    b = b3d_binding_by_gloco(g, gloco_id);
    if (!g || !b || !b->transform || !out) return GLOCO_PROVIDER_FALLBACK;
    out->corrected_pos = *to;
    out->corrected_pos.y = b3d_q12_to_q8(b->transform->position.y);
    out->ground_normal = gloco_v3(0, GLOCO_FX_ONE, 0);
    out->flags = 0;
    if (g->vertical_axis && b->vertical_body &&
        blank3d_vertical_axis_grounded(g->vertical_axis, b->vertical_body))
        out->flags |= GLOCO_FLAG_GROUNDED;
    if (!g->collision || !g->collision->initialized) return GLOCO_PROVIDER_HANDLED;
    start.x = b3d_q8_to_q12(from->x);
    start.y = b->transform->position.y +
              b3d_q8_to_q12(profile->capsule_radius) +
              B3D_GLOCO_SWEEP_SKIN_Q12;
    start.z = b3d_q8_to_q12(from->z);
    delta.x = b3d_q8_to_q12(to->x - from->x);
    delta.y = 0;
    delta.z = b3d_q8_to_q12(to->z - from->z);
    memset(&hit, 0, sizeof(hit));
    if (blank3d_collision_sweep_bullet_mask(g->collision, &start, &delta,
            b3d_q8_to_q12(profile->capsule_radius),
            B3D_COLLISION_LAYER_WORLD, &hit) && hit.hit) {
        /* A horizontal character sweep must not let floor/ceiling contacts
           become walls. Some collision backends return the ground normal as
           -Y instead of +Y, so classify by |normal.y|. */
        {
            long abs_ny;
            abs_ny = (long)hit.normal.y;
            if (abs_ny < 0L) abs_ny = -abs_ny;
            if (abs_ny < (GWP89_FIX_ONE / 2L)) {
                frac = (long)hit.fraction_fx;
                if (frac > 64L) frac -= 64L;
                if (frac < 0L) frac = 0L;
                if (frac > (long)GWP89_FIX_ONE) frac = (long)GWP89_FIX_ONE;
                dx = b3d_mul_q12_fraction((long)delta.x, frac);
                dz = b3d_mul_q12_fraction((long)delta.z, frac);
                out->corrected_pos.x = from->x +
                    b3d_q12_to_q8((g3d_fix)dx);
                out->corrected_pos.z = from->z +
                    b3d_q12_to_q8((g3d_fix)dz);
                out->flags |= GLOCO_FLAG_BLOCKED;
                actor->vel.x = 0;
                actor->vel.z = 0;
            }
        }
    }
    return GLOCO_PROVIDER_HANDLED;
}

static void b3d_physics_actor_create(void *user, int gloco_id,
                                     const GLOCO_Actor *actor,
                                     const GLOCO_Profile *profile)
{
    Blank3DGloco *g;
    long hx;
    long hy;
    g = (Blank3DGloco *)user;
    if (!g || !g->physics || !g->physics->initialized) return;
    hx = b3d_q8_to_q12(profile->capsule_radius);
    hy = b3d_q8_to_q12(profile->capsule_height) / 2L;
    (void)blank3d_vphysics_create_kinematic_box_q12(g->physics,
        B3D_GLOCO_VPHYS_BASE_ID + (unsigned long)gloco_id,
        b3d_q8_to_q12(actor->pos.x), b3d_q8_to_q12(actor->pos.y),
        b3d_q8_to_q12(actor->pos.z), hx, hy, hx, 0);
}

static void b3d_physics_actor_destroy(void *user, int gloco_id,
                                      const GLOCO_Actor *actor,
                                      const GLOCO_Profile *profile)
{
    Blank3DGloco *g;
    (void)actor;
    (void)profile;
    g = (Blank3DGloco *)user;
    if (g && g->physics)
        (void)blank3d_vphysics_destroy_object(g->physics,
            B3D_GLOCO_VPHYS_BASE_ID + (unsigned long)gloco_id);
}

static void b3d_physics_teleport(void *user, int gloco_id,
                                 const GLOCO_Actor *actor,
                                 const GLOCO_Vec3 *position)
{
    Blank3DGloco *g;
    (void)actor;
    g = (Blank3DGloco *)user;
    if (g && g->physics)
        (void)blank3d_vphysics_set_position_q12(g->physics,
            B3D_GLOCO_VPHYS_BASE_ID + (unsigned long)gloco_id,
            b3d_q8_to_q12(position->x), b3d_q8_to_q12(position->y),
            b3d_q8_to_q12(position->z));
}

static void b3d_physics_post(void *user, int gloco_id, GLOCO_Actor *actor,
                             const GLOCO_Profile *profile,
                             const GLOCO_ProbeResult *result)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    (void)profile;
    g = (Blank3DGloco *)user;
    b = b3d_binding_by_gloco(g, gloco_id);
    if (!g || !b || !b->transform || !actor || !result) return;
    b->transform->position.x = b3d_q8_to_q12(result->corrected_pos.x);
    /* VerticalAxis remains authority for Y. */
    b->transform->position.z = b3d_q8_to_q12(result->corrected_pos.z);
    actor->pos.y = b3d_q12_to_q8(b->transform->position.y);
    if (g->physics && g->physics->initialized)
        (void)blank3d_vphysics_set_position_q12(g->physics,
            B3D_GLOCO_VPHYS_BASE_ID + (unsigned long)gloco_id,
            b->transform->position.x, b->transform->position.y,
            b->transform->position.z);
}


static Blank3DGlocoBinding *b3d_binding_by_owner(Blank3DGloco *g,
                                                  unsigned long owner)
{
    int i;
    if (!g) return 0;
    for (i = 0; i < B3D_GLOCO_MAX_BINDINGS; ++i)
        if (g->bindings[i].used && g->bindings[i].thing_owner == owner)
            return &g->bindings[i];
    return 0;
}

typedef enum B3DGlocoVarKindTag {
    B3D_GV_NONE = 0,
    B3D_GV_Q8,
    B3D_GV_MS,
    B3D_GV_STATE,
    B3D_GV_FLAGS,
    B3D_GV_BOOL,
    B3D_GV_LAST_EVENT,
    B3D_GV_EVENT_VALUE
} B3DGlocoVarKind;

static B3DGlocoVarKind b3d_gloco_var_kind(const char *name, int *field)
{
    static const char *q8_names[] = {
        "locomotion.walk_speed", "locomotion.run_speed",
        "locomotion.sprint_speed", "locomotion.aim_speed",
        "locomotion.crouch_speed", "locomotion.acceleration",
        "locomotion.sprint_acceleration", "locomotion.braking",
        "locomotion.hard_braking", "locomotion.ground_friction",
        "locomotion.air_control", "locomotion.side_scale",
        "locomotion.back_scale", "locomotion.aim_side_scale",
        "locomotion.turn_rate", "locomotion.yaw_lag",
        "locomotion.gravity", "locomotion.terminal_fall",
        "locomotion.ground_snap", "locomotion.max_slope_dot",
        "locomotion.capsule_radius", "locomotion.capsule_height",
        "locomotion.evade_speed", "locomotion.evade_friction",
        "locomotion.evade_control", "locomotion.evade_stamina_cost",
        "locomotion.slide_min_speed", "locomotion.slide_friction",
        "locomotion.stamina_max", "locomotion.stamina_sprint_drain",
        "locomotion.stamina_recover", "locomotion.stamina_min_sprint",
        "locomotion.stride_walk", "locomotion.stride_run",
        "locomotion.stride_sprint"
    };
    static const char *ms_names[] = {
        "locomotion.evade_active_ms", "locomotion.evade_recover_ms",
        "locomotion.slide_ms"
    };
    static const char *bool_names[] = {
        "locomotion.grounded", "locomotion.blocked", "locomotion.steep",
        "locomotion.evading", "locomotion.sprinting", "locomotion.aiming",
        "locomotion.sliding"
    };
    int i;
    if (!name) return B3D_GV_NONE;
    for (i = 0; i < (int)(sizeof(q8_names)/sizeof(q8_names[0])); ++i)
        if (strcmp(name, q8_names[i]) == 0) { if (field) *field = i; return B3D_GV_Q8; }
    for (i = 0; i < (int)(sizeof(ms_names)/sizeof(ms_names[0])); ++i)
        if (strcmp(name, ms_names[i]) == 0) { if (field) *field = i; return B3D_GV_MS; }
    for (i = 0; i < (int)(sizeof(bool_names)/sizeof(bool_names[0])); ++i)
        if (strcmp(name, bool_names[i]) == 0) { if (field) *field = i; return B3D_GV_BOOL; }
    if (strcmp(name, "locomotion.state") == 0) return B3D_GV_STATE;
    if (strcmp(name, "locomotion.flags") == 0) return B3D_GV_FLAGS;
    if (strcmp(name, "locomotion.last_event") == 0) return B3D_GV_LAST_EVENT;
    if (strcmp(name, "locomotion.event_value") == 0) return B3D_GV_EVENT_VALUE;
    return B3D_GV_NONE;
}

static GLOCO_FX *b3d_profile_q8_field(GLOCO_Profile *p, int field)
{
    if (!p) return 0;
    switch (field) {
    case 0: return &p->walk_speed; case 1: return &p->run_speed;
    case 2: return &p->sprint_speed; case 3: return &p->aim_speed;
    case 4: return &p->crouch_speed; case 5: return &p->acceleration;
    case 6: return &p->sprint_acceleration; case 7: return &p->braking;
    case 8: return &p->hard_braking; case 9: return &p->ground_friction;
    case 10: return &p->air_control; case 11: return &p->side_scale;
    case 12: return &p->back_scale; case 13: return &p->aim_side_scale;
    case 14: return &p->turn_rate; case 15: return &p->yaw_lag;
    case 16: return &p->gravity; case 17: return &p->terminal_fall;
    case 18: return &p->ground_snap; case 19: return &p->max_slope_dot;
    case 20: return &p->capsule_radius; case 21: return &p->capsule_height;
    case 22: return &p->evade_speed; case 23: return &p->evade_friction;
    case 24: return &p->evade_control; case 25: return &p->evade_stamina_cost;
    case 26: return &p->slide_min_speed; case 27: return &p->slide_friction;
    case 28: return &p->stamina_max; case 29: return &p->stamina_sprint_drain;
    case 30: return &p->stamina_recover; case 31: return &p->stamina_min_sprint;
    case 32: return &p->stride_walk; case 33: return &p->stride_run;
    case 34: return &p->stride_sprint; default: return 0;
    }
}

static GLOCO_U16 *b3d_profile_ms_field(GLOCO_Profile *p, int field)
{
    if (!p) return 0;
    if (field == 0) return &p->evade_active_ms;
    if (field == 1) return &p->evade_recover_ms;
    if (field == 2) return &p->slide_ms;
    return 0;
}

static int b3d_var_claim(void *user, vr89_scope scope, vr89_owner owner,
                         const char *name, vr89_operation operation,
                         const vm89_value *value)
{
    Blank3DGloco *g;
    int field;
    B3DGlocoVarKind kind;
    (void)operation; (void)value;
    g = (Blank3DGloco *)user;
    if (!g || scope == VR89_SCOPE_LOCAL || !b3d_binding_by_owner(g, owner)) return 0;
    kind = b3d_gloco_var_kind(name, &field);
    return kind != B3D_GV_NONE;
}

static int b3d_var_get(void *user, vr89_scope scope, vr89_owner owner,
                       const char *name, vm89_value *out)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    GLOCO_Actor *a;
    GLOCO_Profile *p;
    GLOCO_FX *q8;
    GLOCO_U16 *ms;
    int field;
    unsigned int mask;
    B3DGlocoVarKind kind;
    (void)scope;
    g = (Blank3DGloco *)user;
    b = b3d_binding_by_owner(g, owner);
    if (!b || !out) return 0;
    a = gloco_actor_get(&g->context, b->gloco_id);
    if (!a) return 0;
    p = &g->context.profiles[a->profile_id];
    kind = b3d_gloco_var_kind(name, &field);
    if (kind == B3D_GV_Q8) {
        q8 = b3d_profile_q8_field(p, field); if (!q8) return 0;
        vm89_value_fixed_raw(out, (long)(*q8) * 256L); return 1;
    }
    if (kind == B3D_GV_MS) {
        ms = b3d_profile_ms_field(p, field); if (!ms) return 0;
        vm89_value_fixed_raw(out, (long)(*ms) * 65536L); return 1;
    }
    if (kind == B3D_GV_STATE) { vm89_value_fixed_int(out, (long)a->state); return 1; }
    if (kind == B3D_GV_FLAGS) { vm89_value_fixed_int(out, (long)a->flags); return 1; }
    if (kind == B3D_GV_LAST_EVENT) { vm89_value_fixed_int(out, (long)b->last_event); return 1; }
    if (kind == B3D_GV_EVENT_VALUE) { vm89_value_fixed_int(out, (long)b->event_value); return 1; }
    if (kind == B3D_GV_BOOL) {
        mask = field == 0 ? GLOCO_FLAG_GROUNDED :
               field == 1 ? GLOCO_FLAG_BLOCKED :
               field == 2 ? GLOCO_FLAG_STEEP :
               field == 3 ? GLOCO_FLAG_EVADING :
               field == 4 ? GLOCO_FLAG_SPRINTING :
               field == 5 ? GLOCO_FLAG_AIMING : GLOCO_FLAG_SLIDING;
        vm89_value_bool(out, (a->flags & mask) != 0u); return 1;
    }
    return 0;
}

static int b3d_var_set_q16(Blank3DGloco *g, Blank3DGlocoBinding *b,
                           const char *name, long q16)
{
    GLOCO_Actor *a;
    GLOCO_Profile *p;
    GLOCO_FX *q8;
    GLOCO_U16 *ms;
    B3DGlocoVarKind kind;
    int field;
    long whole;
    a = gloco_actor_get(&g->context, b->gloco_id);
    if (!a) return 0;
    p = &g->context.profiles[a->profile_id];
    kind = b3d_gloco_var_kind(name, &field);
    if (kind == B3D_GV_Q8) {
        q8 = b3d_profile_q8_field(p, field); if (!q8) return 0;
        *q8 = (GLOCO_FX)(q16 / 256L);
        if (field == 28 && b->stamina_value >= 0 && g->systems)
            (void)ns_set_bounds_by_id(&g->systems->numbers, b->stamina_value,
                                      0, b3d_q8_to_q10(*q8));
        return 1;
    }
    if (kind == B3D_GV_MS) {
        ms = b3d_profile_ms_field(p, field); if (!ms) return 0;
        whole = q16 / 65536L; if (whole < 0) whole = 0; if (whole > 65535L) whole = 65535L;
        *ms = (GLOCO_U16)whole; return 1;
    }
    return 0; /* state/event fields are read-only */
}

static int b3d_var_set(void *user, vr89_scope scope, vr89_owner owner,
                       const char *name, const vm89_value *value)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    long q16;
    (void)scope;
    g = (Blank3DGloco *)user; b = b3d_binding_by_owner(g, owner);
    if (!b || !value) return 0;
    if (value->type == VM89_VALUE_FIXED) q16 = value->fixed_q16;
    else if (value->type == VM89_VALUE_BOOL) q16 = value->boolean ? 65536L : 0L;
    else return 0;
    return b3d_var_set_q16(g, b, name, q16);
}

static int b3d_var_delta(void *user, vr89_scope scope, vr89_owner owner,
                         const char *name, const vm89_value *value, int subtract)
{
    vm89_value cur;
    long delta;
    long next;
    if (!value || !b3d_var_get(user, scope, owner, name, &cur) ||
        cur.type != VM89_VALUE_FIXED) return 0;
    if (value->type == VM89_VALUE_FIXED) delta = value->fixed_q16;
    else if (value->type == VM89_VALUE_BOOL) delta = value->boolean ? 65536L : 0L;
    else return 0;
    next = subtract ? cur.fixed_q16 - delta : cur.fixed_q16 + delta;
    return b3d_var_set_q16((Blank3DGloco *)user,
        b3d_binding_by_owner((Blank3DGloco *)user, owner), name, next);
}
static int b3d_var_add(void *u, vr89_scope s, vr89_owner o, const char *n, const vm89_value *v)
{ return b3d_var_delta(u,s,o,n,v,0); }
static int b3d_var_sub(void *u, vr89_scope s, vr89_owner o, const char *n, const vm89_value *v)
{ return b3d_var_delta(u,s,o,n,v,1); }

static void b3d_event(void *user, int gloco_id, int event_id, int value)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    g = (Blank3DGloco *)user;
    b = b3d_binding_by_gloco(g, gloco_id);
    if (!g || !b) return;
    ++g->event_count;
    b->last_event = event_id;
    b->event_value = value;
}

static void b3d_reset_input(GLOCO_Input *in)
{
    if (!in) return;
    memset(in, 0, sizeof(*in));
    in->basis_fwd = gloco_v3(0, 0, -GLOCO_FX_ONE);
    in->basis_right = gloco_v3(GLOCO_FX_ONE, 0, 0);
}

static void b3d_publish(Blank3DGloco *g, Blank3DGlocoBinding *b)
{
    const GLOCO_Actor *a;
    unsigned int f;
    if (!g || !b) return;
    a = gloco_actor_get_const(&g->context, b->gloco_id);
    if (!a) return;
    f = a->flags;
    if (b->actor_id == B3D_PLAYER_ACTOR_ID && g->systems) {
        (void)blank3d_systems_set_flag(g->systems, "player.grounded", (f & GLOCO_FLAG_GROUNDED) != 0u);
        (void)blank3d_systems_set_flag(g->systems, "player.sprinting", (f & GLOCO_FLAG_SPRINTING) != 0u);
        (void)blank3d_systems_set_flag(g->systems, "player.evading", (f & GLOCO_FLAG_EVADING) != 0u);
        (void)blank3d_systems_set_flag(g->systems, "player.sliding", (f & GLOCO_FLAG_SLIDING) != 0u);
    }
}

void blank3d_gloco_init(Blank3DGloco *g, Blank3DSystems *systems,
                        Blank3DVariables *variables,
                        Blank3DVerticalAxis *vertical_axis,
                        mbv89_gamlib3d_adapter *movement_fallback)
{
    GLOCO_MovementProvider mp;
    GLOCO_StatsProvider sp;
    int i;
    if (!g) return;
    memset(g, 0, sizeof(*g));
    g->systems = systems;
    g->variables = variables;
    g->vertical_axis = vertical_axis;
    g->movement_fallback = movement_fallback;
    gloco_init(&g->context);
    gloco_set_callbacks(&g->context, 0, b3d_event, g);
    gloco_movement_provider_init(&mp);
    mp.user = g;
    mp.vertical = b3d_vertical;
    mp.integrate = b3d_integrate;
    gloco_set_movement_provider(&g->context, &mp);
    gloco_stats_provider_init(&sp);
    sp.user = g;
    sp.get_stamina = b3d_stats_get;
    sp.set_stamina = b3d_stats_set;
    sp.consume_stamina = b3d_stats_consume;
    gloco_set_stats_provider(&g->context, &sp);
    if (variables) {
        vr89_provider vp;
        memset(&vp, 0, sizeof(vp));
        vp.name = "gloco89";
        vp.priority = 150; /* NumSys stamina stays authoritative at 200. */
        vp.user = g;
        vp.claim = b3d_var_claim;
        vp.get = b3d_var_get;
        vp.set = b3d_var_set;
        vp.add = b3d_var_add;
        vp.sub = b3d_var_sub;
        (void)vr89_add_provider(&variables->runtime, &vp);
    }
    g->stamina_type_id = -1;
    if (systems) {
        g->stamina_type_id = ns_find_type(&systems->numbers, "locomotion.stamina");
        if (g->stamina_type_id < 0)
            g->stamina_type_id = ns_define_type(&systems->numbers,
                "locomotion.stamina", NS_SCOPE_INSTANCE, NS_KIND_FIXED,
                NS_FX_FROM_INT(100), 0, NS_FX_FROM_INT(100),
                NS_OVERFLOW_CLAMP, NS_FLAG_SAVE | NS_FLAG_HUD);
    }
    for (i = 0; i < B3D_GLOCO_MAX_BINDINGS; ++i) g->bindings[i].stamina_value = -1;
    g->initialized = 1;
    b3d_status(g, "GLOCO89 locomotion provider initialized");
}

void blank3d_gloco_attach_physics(Blank3DGloco *g,
                                  Blank3DCollision *collision,
                                  Blank3DVPhysics *physics)
{
    GLOCO_PhysicsProvider pp;
    if (!g) return;
    g->collision = collision;
    g->physics = physics;
    gloco_physics_provider_init(&pp);
    pp.user = g;
    pp.move = b3d_physics_move;
    pp.actor_create = b3d_physics_actor_create;
    pp.actor_destroy = b3d_physics_actor_destroy;
    pp.teleport = b3d_physics_teleport;
    pp.post_move = b3d_physics_post;
    gloco_set_physics_provider(&g->context, &pp);
}

void blank3d_gloco_reset_bindings(Blank3DGloco *g)
{
    int i;
    if (!g) return;
    for (i = 0; i < B3D_GLOCO_MAX_BINDINGS; ++i) {
        if (g->bindings[i].used)
            (void)gloco_actor_destroy(&g->context, g->bindings[i].gloco_id);
        memset(&g->bindings[i], 0, sizeof(g->bindings[i]));
        g->bindings[i].stamina_value = -1;
    }
}

int blank3d_gloco_bind(Blank3DGloco *g, int actor_id,
                       unsigned long thing_owner, Transform *transform,
                       Blank3DVerticalBody *vertical_body, int preset,
                       const char *profile_ini_path)
{
    int slot;
    int id;
    GLOCO_Profile p;
    GLOCO_Vec3 pos;
    GLOCO_Vec3 facing;
    Vec3 right;
    Vec3 up;
    Vec3 fwd;
    if (!g || !g->initialized || !transform) return 0;
    if (b3d_binding_by_actor(g, actor_id)) return 1;
    slot = -1;
    for (id = 0; id < B3D_GLOCO_MAX_BINDINGS; ++id)
        if (!g->bindings[id].used) { slot = id; break; }
    if (slot < 0 || slot >= GLOCO_MAX_PROFILES) return 0;
    (void)blank3d_gloco_profile_load_ini(&p, profile_ini_path, preset,
                                         g->status, sizeof(g->status));
    if (gloco_set_profile(&g->context, slot, &p) != GLOCO_OK) return 0;
    pos = b3d_pos_to_gloco(&transform->position);
    transform_get_local_axes(transform, &right, &up, &fwd);
    (void)up;
    facing = b3d_pos_to_gloco(&fwd);
    id = gloco_actor_create(&g->context, slot, &pos, &facing);
    if (id < 0) return 0;
    memset(&g->bindings[slot], 0, sizeof(g->bindings[slot]));
    g->bindings[slot].used = 1;
    g->bindings[slot].gloco_id = id;
    g->bindings[slot].actor_id = actor_id;
    g->bindings[slot].thing_owner = thing_owner;
    g->bindings[slot].transform = transform;
    g->bindings[slot].vertical_body = vertical_body;
    g->bindings[slot].stamina_value = -1;
    b3d_reset_input(&g->bindings[slot].input);
    if (g->systems && g->stamina_type_id >= 0) {
        g->bindings[slot].stamina_value = ns_find_value_by_type(&g->systems->numbers,
            (ns_owner)thing_owner, g->stamina_type_id);
        if (g->bindings[slot].stamina_value < 0)
            g->bindings[slot].stamina_value = ns_attach_type(&g->systems->numbers,
                (ns_owner)thing_owner, g->stamina_type_id);
        if (g->bindings[slot].stamina_value >= 0) {
            (void)ns_set_bounds_by_id(&g->systems->numbers,
                g->bindings[slot].stamina_value, 0, b3d_q8_to_q10(p.stamina_max));
            (void)ns_set_by_id(&g->systems->numbers, g->bindings[slot].stamina_value,
                               b3d_q8_to_q10(p.stamina_max));
        }
    }
    return 1;
}

int blank3d_gloco_unbind(Blank3DGloco *g, int actor_id)
{
    Blank3DGlocoBinding *b;
    b = b3d_binding_by_actor(g, actor_id);
    if (!b) return 0;
    (void)gloco_actor_destroy(&g->context, b->gloco_id);
    memset(b, 0, sizeof(*b));
    b->stamina_value = -1;
    return 1;
}

void blank3d_gloco_begin_frame(Blank3DGloco *g, unsigned short dt_ms)
{
    int i;
    if (!g || !g->initialized) return;
    g->frame_ms = dt_ms ? dt_ms : 1U;
    for (i = 0; i < B3D_GLOCO_MAX_BINDINGS; ++i)
        if (g->bindings[i].used) b3d_reset_input(&g->bindings[i].input);
}

void blank3d_gloco_set_suspended(Blank3DGloco *g, int actor_id, int suspended)
{
    Blank3DGlocoBinding *b;
    b = b3d_binding_by_actor(g, actor_id);
    if (b) b->suspended = suspended ? 1 : 0;
}

static void b3d_sync_from_transform(Blank3DGlocoBinding *b, GLOCO_Actor *a)
{
    Vec3 right;
    Vec3 up;
    Vec3 fwd;
    if (!b || !a || !b->transform) return;
    a->pos = b3d_pos_to_gloco(&b->transform->position);
    transform_get_local_axes(b->transform, &right, &up, &fwd);
    (void)up;
    a->facing = gloco_v3_norm_approx(b3d_pos_to_gloco(&fwd));
    b->input.basis_fwd = a->facing;
    b->input.basis_right = gloco_v3_norm_approx(b3d_pos_to_gloco(&right));
}

void blank3d_gloco_tick(Blank3DGloco *g, unsigned short dt_ms)
{
    int i;
    GLOCO_Actor *a;
    if (!g || !g->initialized) return;
    for (i = 0; i < B3D_GLOCO_MAX_BINDINGS; ++i) {
        Blank3DGlocoBinding *b;
        b = &g->bindings[i];
        if (!b->used) continue;
        a = gloco_actor_get(&g->context, b->gloco_id);
        if (!a) continue;
        b3d_sync_from_transform(b, a);
        if (!b->suspended)
            (void)gloco_update_actor(&g->context, b->gloco_id, &b->input,
                                     (GLOCO_U16)dt_ms);
        else {
            a->vel.x = 0; a->vel.y = 0; a->vel.z = 0;
            a->pos = b3d_pos_to_gloco(&b->transform->position);
        }
        b3d_publish(g, b);
        b->suspended = 0;
    }
}

static GLOCO_FX b3d_speed_from_mbv(mbv89_fixed amount, unsigned short dt_ms)
{
    long q16;
    long speed_q16;
    if (dt_ms == 0) dt_ms = 1;
    q16 = amount;
    if (q16 < 0) q16 = -q16;
    speed_q16 = (q16 * 1000L) / (long)dt_ms;
    return b3d_q16_to_q8(speed_q16);
}

int blank3d_gloco_mbv_base_provider(void *provider_user, mbv89_context *ctx,
                                    mbv89_actor *actor, mbv89_base_verb verb,
                                    mbv89_fixed amount)
{
    Blank3DGloco *g;
    Blank3DGlocoBinding *b;
    (void)ctx;
    g = (Blank3DGloco *)provider_user;
    b = b3d_binding_by_actor(g, B3D_PLAYER_ACTOR_ID);
    if (b && actor && actor->user == b->transform) {
        switch (verb) {
        case MBV89_BASE_MOVE_FORWARD: b->input.move_z = 256; break;
        case MBV89_BASE_MOVE_BACKWARD: b->input.move_z = -256; break;
        case MBV89_BASE_MOVE_LEFT: b->input.move_x = -256; break;
        case MBV89_BASE_MOVE_RIGHT: b->input.move_x = 256; break;
        default:
            if (g && g->movement_fallback)
                return mbv89_gamlib3d_base_provider(g->movement_fallback,
                    ctx, actor, verb, amount);
            return MBV89_UNHANDLED;
        }
        b->input.buttons |= GLOCO_INPUT_RUN;
        b->input.speed_override = b3d_speed_from_mbv(amount, g && g->frame_ms ? g->frame_ms : 16U);
        return MBV89_HANDLED;
    }
    if (g && g->movement_fallback)
        return mbv89_gamlib3d_base_provider(g->movement_fallback,
                                            ctx, actor, verb, amount);
    return MBV89_UNHANDLED;
}

int blank3d_gloco_actor_game_verb(Blank3DGloco *g, int actor_id,
                                    mbv89_game_verb verb)
{
    Blank3DGlocoBinding *b;
    b = b3d_binding_by_actor(g, actor_id);
    if (!b) return MBV89_UNHANDLED;
    switch (verb) {
    case MBV89_GAME_WALK_FORWARD: b->input.move_z = 256; b->input.buttons |= GLOCO_INPUT_WALK; break;
    case MBV89_GAME_WALK_BACKWARD: b->input.move_z = -256; b->input.buttons |= GLOCO_INPUT_WALK; break;
    case MBV89_GAME_STRAFE_LEFT: b->input.move_x = -256; b->input.buttons |= GLOCO_INPUT_RUN; break;
    case MBV89_GAME_STRAFE_RIGHT: b->input.move_x = 256; b->input.buttons |= GLOCO_INPUT_RUN; break;
    case MBV89_GAME_RUN_FORWARD: b->input.move_z = 256; b->input.buttons |= GLOCO_INPUT_RUN; break;
    case MBV89_GAME_RUN_BACKWARD: b->input.move_z = -256; b->input.buttons |= GLOCO_INPUT_RUN; break;
    default: return MBV89_UNHANDLED;
    }
    return MBV89_HANDLED;
}

int blank3d_gloco_mbv_game_provider(Blank3DGloco *g, mbv89_game_verb verb)
{
    return blank3d_gloco_actor_game_verb(g, B3D_PLAYER_ACTOR_ID, verb);
}

void blank3d_gloco_install_movement_verbs(Blank3DGloco *g, mbv89_context *ctx)
{
    if (!g || !ctx) return;
    mbv89_set_base_provider(ctx, blank3d_gloco_mbv_base_provider, g);
}

int blank3d_gloco_feed_automotion_position(Blank3DGloco *g, int actor_id,
                                           const Vec3 *current,
                                           const Vec3 *desired,
                                           g3d_fix speed_q12, int run)
{
    Blank3DGlocoBinding *b;
    GLOCO_Vec3 d;
    b = b3d_binding_by_actor(g, actor_id);
    if (!b || !current || !desired) return 0;
    d = gloco_v3(b3d_q12_to_q8(desired->x - current->x), 0,
                 b3d_q12_to_q8(desired->z - current->z));
    if (gloco_v3_len_approx(d) <= 0) return 1;
    d = gloco_v3_norm_approx(d);
    b->input.basis_fwd = d;
    b->input.basis_right = gloco_v3(-d.z, 0, d.x);
    b->input.move_z = 256;
    b->input.buttons |= run ? GLOCO_INPUT_RUN : GLOCO_INPUT_WALK;
    b->input.speed_override = b3d_q12_to_q8(speed_q12);
    return 1;
}

const char *blank3d_gloco_status(const Blank3DGloco *g)
{
    return g ? g->status : "GLOCO89 unavailable";
}
