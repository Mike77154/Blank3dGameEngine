#include "blank3d_kinverbs.h"
#include <string.h>
#include <stdio.h>

static gk3d_fix b3d_q12_to_gk(g3d_fix v)
{
    return (gk3d_fix)(v / 16L);
}

static g3d_fix b3d_q16_to_q12(mbv89_fixed v)
{
    return (g3d_fix)((long)v / 16L);
}

static void b3d_status(Blank3DKinVerbs *kin, const char *text)
{
    if (!kin) return;
    if (!text) text = "";
    strncpy(kin->status, text, sizeof(kin->status) - 1U);
    kin->status[sizeof(kin->status) - 1U] = '\0';
}

static Blank3DKinBinding *b3d_binding(Blank3DKinVerbs *kin,
                                      unsigned long owner)
{
    int i;
    if (!kin) return 0;
    for (i = 0; i < kin->binding_count; ++i) {
        if (kin->bindings[i].used &&
            (kin->bindings[i].owner == owner ||
             (unsigned long)kin->bindings[i].actor_id == owner))
            return &kin->bindings[i];
    }
    return 0;
}

static const Blank3DKinBinding *b3d_binding_const(const Blank3DKinVerbs *kin,
                                                   unsigned long owner)
{
    int i;
    if (!kin) return 0;
    for (i = 0; i < kin->binding_count; ++i) {
        if (kin->bindings[i].used &&
            (kin->bindings[i].owner == owner ||
             (unsigned long)kin->bindings[i].actor_id == owner))
            return &kin->bindings[i];
    }
    return 0;
}

static void b3d_actor_box(const Blank3DKinBinding *b,
                          const Transform *t, gk3d_aabb *box)
{
    if (!b || !t || !box) return;
    box->x = b3d_q12_to_gk(t->position.x - b->half_x_q12);
    box->y = b3d_q12_to_gk(t->position.y);
    box->z = b3d_q12_to_gk(t->position.z - b->half_z_q12);
    box->w = b3d_q12_to_gk(b->half_x_q12 + b->half_x_q12);
    box->h = b3d_q12_to_gk(b->height_q12);
    box->d = b3d_q12_to_gk(b->half_z_q12 + b->half_z_q12);
}

static int b3d_register_condition_aliases(Blank3DKinVerbs *kin,
                                           gverb89_condition_fn fn)
{
    static const char *names[] = {
        "place_meeting", "placeMeeting",
        "place_free", "placeFree",
        "instance_place", "instancePlace",
        "is_on_floor", "isonfloor", "isOnFloor",
        "is_on_wall", "isonwall", "is_by_wall", "isByWall",
        "is_under_ceiling", "isUnderCeiling",
        "overlap_at_offset", "is_overlapping_at_offset", "isOverlappingAtOffset",
        "can_walk_forward", "canWalkForward",
        "can_walk_backward", "canWalkBackward",
        "can_strafe_left", "canStrafeLeft",
        "can_strafe_right", "canStrafeRight",
        "can_run_forward", "canRunForward",
        "can_run_backward", "canRunBackward"
    };
    unsigned int i;
    for (i = 0U; i < (unsigned int)(sizeof(names) / sizeof(names[0])); ++i)
        if (!gverb89_register_condition(&kin->verbs, names[i], fn, kin)) return 0;
    return 1;
}

static int b3d_gv_condition(void *user, const gverb89_call *call,
                            gverb89_result *out);

void blank3d_kinverbs_init(Blank3DKinVerbs *kin)
{
    if (!kin) return;
    memset(kin, 0, sizeof(*kin));
    gk3d_world_init(&kin->world);
    /* 1/64 world unit: enough for contact semantics without visible gap. */
    gk3d_world_set_probe(&kin->world, (gk3d_fix)4L);
    gverb89_init(&kin->verbs);
    kin->default_probe_q12 = (g3d_fix)(G3D_FIX_ONE / 4L);
    if (!b3d_register_condition_aliases(kin, b3d_gv_condition)) {
        b3d_status(kin, "3DKin verb registry capacity exhausted");
        return;
    }
    kin->initialized = 1;
    b3d_status(kin, "3DKin condition verbs online");
}

void blank3d_kinverbs_begin_sync(Blank3DKinVerbs *kin)
{
    if (!kin || !kin->initialized) return;
    gk3d_world_clear(&kin->world);
    gk3d_world_set_probe(&kin->world, (gk3d_fix)4L);
    memset(kin->bindings, 0, sizeof(kin->bindings));
    kin->binding_count = 0;
    ++kin->sync_serial;
}

int blank3d_kinverbs_add_world_box(Blank3DKinVerbs *kin,
                                   g3d_fix min_x, g3d_fix min_y, g3d_fix min_z,
                                   g3d_fix size_x, g3d_fix size_y, g3d_fix size_z,
                                   unsigned int flags)
{
    if (!kin || !kin->initialized) return 0;
    return gk3d_world_add_box(&kin->world, B3D_KINVERB_TYPE_WORLD,
        b3d_q12_to_gk(min_x), b3d_q12_to_gk(min_y), b3d_q12_to_gk(min_z),
        b3d_q12_to_gk(size_x), b3d_q12_to_gk(size_y), b3d_q12_to_gk(size_z),
        flags) != GK3D_ID_NONE;
}

int blank3d_kinverbs_sync_actor(Blank3DKinVerbs *kin,
                                unsigned long owner, int actor_id,
                                Transform *transform,
                                g3d_fix half_x_q12, g3d_fix height_q12,
                                g3d_fix half_z_q12,
                                int active)
{
    Blank3DKinBinding *b;
    gk3d_aabb box;
    int id;
    if (!kin || !kin->initialized || !transform || !active) return 0;
    if (kin->binding_count >= B3D_KINVERB_MAX_BINDINGS) return 0;
    b = &kin->bindings[kin->binding_count];
    memset(b, 0, sizeof(*b));
    b->used = 1;
    b->owner = owner;
    b->actor_id = actor_id;
    b->transform = transform;
    b->half_x_q12 = half_x_q12;
    b->height_q12 = height_q12;
    b->half_z_q12 = half_z_q12;
    b3d_actor_box(b, transform, &box);
    id = gk3d_world_add_box(&kin->world, B3D_KINVERB_TYPE_ACTOR,
                            box.x, box.y, box.z, box.w, box.h, box.d,
                            GK3D_FLAG_ACTIVE | GK3D_FLAG_SENSOR);
    if (id == GK3D_ID_NONE) {
        memset(b, 0, sizeof(*b));
        return 0;
    }
    b->gk3d_id = id;
    ++kin->binding_count;
    return 1;
}

void blank3d_kinverbs_end_sync(Blank3DKinVerbs *kin)
{
    if (!kin || !kin->initialized) return;
    gk3d_update_all_contacts(&kin->world);
}

gverb89_registry *blank3d_kinverbs_registry(Blank3DKinVerbs *kin)
{
    return kin ? &kin->verbs : 0;
}

const gverb89_registry *blank3d_kinverbs_registry_const(const Blank3DKinVerbs *kin)
{
    return kin ? &kin->verbs : 0;
}

static int b3d_candidate_free(Blank3DKinVerbs *kin,
                              const Blank3DKinBinding *b,
                              const Transform *candidate)
{
    gk3d_aabb box;
    if (!kin || !b || !candidate) return 1;
    b3d_actor_box(b, candidate, &box);
    return gk3d_place_free(&kin->world, b->gk3d_id,
                           box.x, box.y, box.z);
}

int blank3d_kinverbs_can_game_verb(Blank3DKinVerbs *kin,
                                   unsigned long owner,
                                   mbv89_game_verb verb,
                                   mbv89_fixed amount_q16)
{
    Blank3DKinBinding *b;
    Transform candidate;
    g3d_fix distance;
    if (!kin || !kin->initialized) return 1;
    b = b3d_binding(kin, owner);
    if (!b || !b->transform) return 1;
    candidate = *b->transform;
    distance = b3d_q16_to_q12(amount_q16);
    if (distance < 0) distance = -distance;
    if (distance == 0) distance = kin->default_probe_q12;
    switch (verb) {
    case MBV89_GAME_WALK_FORWARD:
    case MBV89_GAME_RUN_FORWARD:
        transform_move_local_flat(&candidate, 0, 0, distance); break;
    case MBV89_GAME_WALK_BACKWARD:
    case MBV89_GAME_RUN_BACKWARD:
        transform_move_local_flat(&candidate, 0, 0, -distance); break;
    case MBV89_GAME_STRAFE_LEFT:
        transform_move_local_flat(&candidate, -distance, 0, 0); break;
    case MBV89_GAME_STRAFE_RIGHT:
        transform_move_local_flat(&candidate, distance, 0, 0); break;
    default:
        return 1;
    }
    return b3d_candidate_free(kin, b, &candidate);
}

int blank3d_kinverbs_can_named_move(Blank3DKinVerbs *kin,
                                    unsigned long owner,
                                    const char *name,
                                    mbv89_fixed amount_q16)
{
    mbv89_game_verb verb;
    if (!name || !mbv89_game_verb_from_name(name, &verb)) return 1;
    return blank3d_kinverbs_can_game_verb(kin, owner, verb, amount_q16);
}

static int b3d_derived_verb(const char *name, mbv89_game_verb *verb)
{
    if (!name || !verb) return 0;
    if (strcmp(name, "can_walk_forward") == 0 || strcmp(name, "canWalkForward") == 0)
        *verb = MBV89_GAME_WALK_FORWARD;
    else if (strcmp(name, "can_walk_backward") == 0 || strcmp(name, "canWalkBackward") == 0)
        *verb = MBV89_GAME_WALK_BACKWARD;
    else if (strcmp(name, "can_strafe_left") == 0 || strcmp(name, "canStrafeLeft") == 0)
        *verb = MBV89_GAME_STRAFE_LEFT;
    else if (strcmp(name, "can_strafe_right") == 0 || strcmp(name, "canStrafeRight") == 0)
        *verb = MBV89_GAME_STRAFE_RIGHT;
    else if (strcmp(name, "can_run_forward") == 0 || strcmp(name, "canRunForward") == 0)
        *verb = MBV89_GAME_RUN_FORWARD;
    else if (strcmp(name, "can_run_backward") == 0 || strcmp(name, "canRunBackward") == 0)
        *verb = MBV89_GAME_RUN_BACKWARD;
    else return 0;
    return 1;
}

static int b3d_gv_condition(void *user, const gverb89_call *call,
                            gverb89_result *out)
{
    Blank3DKinVerbs *kin;
    const Blank3DKinBinding *b;
    const gk3d_obj *obj;
    gk3d_condition_result r;
    mbv89_game_verb derived;
    gk3d_fix x;
    gk3d_fix y;
    gk3d_fix z;
    int ok;
    if (!user || !call || !out) return GVERB89_ERROR;
    kin = (Blank3DKinVerbs *)user;
    b = b3d_binding_const(kin, call->owner);
    if (!b) return GVERB89_UNHANDLED;
    memset(out, 0, sizeof(*out));
    out->instance_id = GK3D_ID_NONE;
    if (b3d_derived_verb(call->name, &derived)) {
        mbv89_fixed amount;
        amount = call->has_value ? (mbv89_fixed)call->value_q16
                                 : (mbv89_fixed)(kin->default_probe_q12 * 16L);
        out->truth = blank3d_kinverbs_can_game_verb(kin, call->owner,
                                                     derived, amount);
        return GVERB89_HANDLED;
    }
    obj = gk3d_world_get_const(&kin->world, b->gk3d_id);
    if (!obj) return GVERB89_UNHANDLED;
    x = obj->box.x; y = obj->box.y; z = obj->box.z;
    memset(&r, 0, sizeof(r));
    ok = gk3d_check_verb(&kin->world, call->name, b->gk3d_id,
                         GK3D_TARGET_SOLID,
                         x, y, z, 0, 0, 0, &r);
    if (!ok) return GVERB89_UNHANDLED;
    out->truth = r.truth;
    out->instance_id = r.instance_id;
    return GVERB89_HANDLED;
}

int blank3d_kinverbs_query(Blank3DKinVerbs *kin,
                           unsigned long owner, void *subject,
                           const char *name,
                           long value_q16, const char *value_text,
                           int has_value, gverb89_result *out)
{
    gverb89_call call;
    if (!kin || !name) return GVERB89_ERROR;
    memset(&call, 0, sizeof(call));
    call.owner = owner;
    call.subject = subject;
    call.name = name;
    call.value_q16 = value_q16;
    call.value_text = value_text ? value_text : "";
    call.has_value = has_value;
    return gverb89_query(&kin->verbs, &call, out);
}

const char *blank3d_kinverbs_status(const Blank3DKinVerbs *kin)
{
    return kin ? kin->status : "3DKin unavailable";
}
