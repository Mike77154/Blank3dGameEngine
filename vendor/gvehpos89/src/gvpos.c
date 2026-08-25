#include "gvpos.h"

static size_t gvpos_align_size(size_t n)
{
    size_t a;
    size_t r;
    a = sizeof(void *);
    r = n % a;
    if (r != 0u) {
        n = n + (a - r);
    }
    return n;
}

static GVPos_FP gvpos_abs_fp(GVPos_FP v)
{
    if (v < 0) {
        return -v;
    }
    return v;
}

static GVPos_Bool gvpos_box_distance_ok(GVPos_Vec3 a, GVPos_Vec3 b, GVPos_FP radius)
{
    GVPos_FP dx;
    GVPos_FP dy;
    GVPos_FP dz;
    dx = gvpos_abs_fp(a.x - b.x);
    dy = gvpos_abs_fp(a.y - b.y);
    dz = gvpos_abs_fp(a.z - b.z);
    if (dx > radius) {
        return GVPOS_FALSE;
    }
    if (dy > radius) {
        return GVPOS_FALSE;
    }
    if (dz > radius) {
        return GVPOS_FALSE;
    }
    return GVPOS_TRUE;
}

static void gvpos_push_event(GVPos_Context *ctx, int type, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, int result)
{
    GVPos_Event ev;
    int next_tail;
    if (ctx == 0) {
        return;
    }
    ev.type = type;
    ev.actor_id = actor_id;
    ev.vehicle_id = vehicle_id;
    ev.seat_slot = seat_slot;
    ev.result = result;
    if (ctx->events != 0 && ctx->event_cap > 1) {
        next_tail = ctx->event_tail + 1;
        if (next_tail >= ctx->event_cap) {
            next_tail = 0;
        }
        if (next_tail != ctx->event_head) {
            ctx->events[ctx->event_tail] = ev;
            ctx->event_tail = next_tail;
        }
    }
    if (ctx->cb.on_event != 0) {
        ctx->cb.on_event(ctx, &ev, ctx->user);
    }
}

static GVPos_Vec3 gvpos_vec_add(GVPos_Vec3 a, GVPos_Vec3 b)
{
    GVPos_Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

static int gvpos_same_actor_kind_allowed(const GVPos_Actor *a, const GVPos_Vehicle *v, const GVPos_Seat *s)
{
    if ((a->flags & GVPOS_ACTOR_FLAG_PLAYER) != 0) {
        if ((v->flags & GVPOS_VEHICLE_FLAG_NPC_ONLY) != 0) {
            return GVPOS_FALSE;
        }
        if ((s->flags & GVPOS_SEAT_FLAG_ALLOW_PLAYER) == 0) {
            return GVPOS_FALSE;
        }
    }
    if ((a->flags & GVPOS_ACTOR_FLAG_NPC) != 0) {
        if ((v->flags & GVPOS_VEHICLE_FLAG_PLAYER_ONLY) != 0) {
            return GVPOS_FALSE;
        }
        if ((s->flags & GVPOS_SEAT_FLAG_ALLOW_NPC) == 0) {
            return GVPOS_FALSE;
        }
    }
    if ((s->flags & GVPOS_SEAT_FLAG_DRIVER) != 0) {
        if ((a->flags & GVPOS_ACTOR_FLAG_ALLOW_DRIVE) == 0) {
            return GVPOS_FALSE;
        }
    }
    if ((s->flags & GVPOS_SEAT_FLAG_PASSENGER) != 0) {
        if ((a->flags & GVPOS_ACTOR_FLAG_ALLOW_RIDE) == 0 && (s->flags & GVPOS_SEAT_FLAG_DRIVER) == 0) {
            return GVPOS_FALSE;
        }
    }
    return GVPOS_TRUE;
}

void gvpos_default_config(GVPos_Config *cfg)
{
    if (cfg == 0) {
        return;
    }
    cfg->default_enter_ticks = 8;
    cfg->default_exit_ticks = 6;
    cfg->default_cooldown_ticks = 8;
    cfg->default_entry_radius = GVPOS_FROM_INT(2);
    cfg->default_exit_check_radius = GVPOS_FROM_INT(1);
}

void gvpos_zero_input(GVPos_Input *input)
{
    if (input == 0) {
        return;
    }
    input->throttle = 0;
    input->brake = 0;
    input->steer = 0;
    input->handbrake = 0;
    input->interact_pressed = 0;
    input->exit_pressed = 0;
    input->horn_pressed = 0;
}

void gvpos_init(GVPos_Context *ctx, const GVPos_Config *cfg)
{
    GVPos_Config def_cfg;
    if (ctx == 0) {
        return;
    }
    gvpos_default_config(&def_cfg);
    ctx->cfg = def_cfg;
    if (cfg != 0) {
        ctx->cfg = *cfg;
    }
    ctx->cb.can_enter = 0;
    ctx->cb.can_exit = 0;
    ctx->cb.get_actor_pos = 0;
    ctx->cb.get_vehicle_pos = 0;
    ctx->cb.test_exit_clear = 0;
    ctx->cb.on_event = 0;
    ctx->cb.set_actor_hidden = 0;
    ctx->cb.set_actor_attached = 0;
    ctx->cb.apply_vehicle_input = 0;
    ctx->user = 0;
    ctx->actors = 0;
    ctx->actor_cap = 0;
    ctx->actor_count = 0;
    ctx->vehicles = 0;
    ctx->vehicle_cap = 0;
    ctx->vehicle_count = 0;
    ctx->seats = 0;
    ctx->seat_cap = 0;
    ctx->seat_count = 0;
    ctx->events = 0;
    ctx->event_cap = 0;
    ctx->event_head = 0;
    ctx->event_tail = 0;
    ctx->tick = 0u;
}

void gvpos_set_callbacks(GVPos_Context *ctx, const GVPos_Callbacks *cb, void *user)
{
    if (ctx == 0) {
        return;
    }
    if (cb != 0) {
        ctx->cb = *cb;
    } else {
        ctx->cb.can_enter = 0;
        ctx->cb.can_exit = 0;
        ctx->cb.get_actor_pos = 0;
        ctx->cb.get_vehicle_pos = 0;
        ctx->cb.test_exit_clear = 0;
        ctx->cb.on_event = 0;
        ctx->cb.set_actor_hidden = 0;
        ctx->cb.set_actor_attached = 0;
        ctx->cb.apply_vehicle_input = 0;
    }
    ctx->user = user;
}

void gvpos_arena_init(GVPos_Arena *arena, void *memory, size_t bytes)
{
    if (arena == 0) {
        return;
    }
    arena->base = (unsigned char *)memory;
    arena->capacity = bytes;
    arena->used = 0u;
}

void *gvpos_arena_push(GVPos_Arena *arena, size_t bytes)
{
    size_t aligned_used;
    size_t aligned_bytes;
    void *p;
    if (arena == 0 || arena->base == 0) {
        return 0;
    }
    aligned_used = gvpos_align_size(arena->used);
    aligned_bytes = gvpos_align_size(bytes);
    if (aligned_used + aligned_bytes > arena->capacity) {
        return 0;
    }
    p = (void *)(arena->base + aligned_used);
    arena->used = aligned_used + aligned_bytes;
    return p;
}

size_t gvpos_required_bytes(int actor_cap, int vehicle_cap, int seat_cap, int event_cap)
{
    size_t total;
    total = 0u;
    if (actor_cap > 0) {
        total = total + gvpos_align_size(sizeof(GVPos_Actor) * (size_t)actor_cap);
    }
    if (vehicle_cap > 0) {
        total = total + gvpos_align_size(sizeof(GVPos_Vehicle) * (size_t)vehicle_cap);
    }
    if (seat_cap > 0) {
        total = total + gvpos_align_size(sizeof(GVPos_Seat) * (size_t)seat_cap);
    }
    if (event_cap > 0) {
        total = total + gvpos_align_size(sizeof(GVPos_Event) * (size_t)event_cap);
    }
    return total;
}

static void gvpos_clear_storage(GVPos_Context *ctx)
{
    int i;
    if (ctx == 0) {
        return;
    }
    for (i = 0; i < ctx->actor_cap; ++i) {
        ctx->actors[i].id = GVPOS_ID_NONE;
        ctx->actors[i].flags = 0;
        ctx->actors[i].phase = GVPOS_ACTOR_OFF;
        ctx->actors[i].vehicle_id = GVPOS_ID_NONE;
        ctx->actors[i].seat_slot = -1;
        ctx->actors[i].timer_ticks = 0;
        ctx->actors[i].cooldown_ticks = 0;
        gvpos_zero_input(&ctx->actors[i].input);
        ctx->actors[i].cached_pos.x = 0;
        ctx->actors[i].cached_pos.y = 0;
        ctx->actors[i].cached_pos.z = 0;
    }
    for (i = 0; i < ctx->vehicle_cap; ++i) {
        ctx->vehicles[i].id = GVPOS_ID_NONE;
        ctx->vehicles[i].flags = 0;
        ctx->vehicles[i].driver_slot = 0;
        ctx->vehicles[i].driver_actor_id = GVPOS_ID_NONE;
        ctx->vehicles[i].engine_on = 0;
        ctx->vehicles[i].cached_pos.x = 0;
        ctx->vehicles[i].cached_pos.y = 0;
        ctx->vehicles[i].cached_pos.z = 0;
    }
    for (i = 0; i < ctx->seat_cap; ++i) {
        ctx->seats[i].vehicle_id = GVPOS_ID_NONE;
        ctx->seats[i].slot = -1;
        ctx->seats[i].flags = 0;
        ctx->seats[i].entry_radius = 0;
        ctx->seats[i].exit_check_radius = 0;
        ctx->seats[i].local_mount_pos.x = 0;
        ctx->seats[i].local_mount_pos.y = 0;
        ctx->seats[i].local_mount_pos.z = 0;
        ctx->seats[i].local_exit_pos.x = 0;
        ctx->seats[i].local_exit_pos.y = 0;
        ctx->seats[i].local_exit_pos.z = 0;
        ctx->seats[i].occupant_actor_id = GVPOS_ID_NONE;
        ctx->seats[i].reserved_by_actor_id = GVPOS_ID_NONE;
    }
    for (i = 0; i < ctx->event_cap; ++i) {
        ctx->events[i].type = GVPOS_EVENT_NONE;
        ctx->events[i].actor_id = GVPOS_ID_NONE;
        ctx->events[i].vehicle_id = GVPOS_ID_NONE;
        ctx->events[i].seat_slot = -1;
        ctx->events[i].result = GVPOS_OK;
    }
}

int gvpos_bind_arena(GVPos_Context *ctx, GVPos_Arena *arena, int actor_cap, int vehicle_cap, int seat_cap, int event_cap)
{
    GVPos_Actor *actors;
    GVPos_Vehicle *vehicles;
    GVPos_Seat *seats;
    GVPos_Event *events;
    if (ctx == 0 || arena == 0) {
        return GVPOS_ERR_NULL;
    }
    if (actor_cap <= 0 || vehicle_cap <= 0 || seat_cap <= 0 || event_cap <= 0) {
        return GVPOS_ERR_BAD_ARG;
    }
    actors = (GVPos_Actor *)gvpos_arena_push(arena, sizeof(GVPos_Actor) * (size_t)actor_cap);
    vehicles = (GVPos_Vehicle *)gvpos_arena_push(arena, sizeof(GVPos_Vehicle) * (size_t)vehicle_cap);
    seats = (GVPos_Seat *)gvpos_arena_push(arena, sizeof(GVPos_Seat) * (size_t)seat_cap);
    events = (GVPos_Event *)gvpos_arena_push(arena, sizeof(GVPos_Event) * (size_t)event_cap);
    if (actors == 0 || vehicles == 0 || seats == 0 || events == 0) {
        return GVPOS_ERR_BAD_STORAGE;
    }
    return gvpos_bind_storage(ctx, actors, actor_cap, vehicles, vehicle_cap, seats, seat_cap, events, event_cap);
}

int gvpos_bind_storage(GVPos_Context *ctx,
                       GVPos_Actor *actors, int actor_cap,
                       GVPos_Vehicle *vehicles, int vehicle_cap,
                       GVPos_Seat *seats, int seat_cap,
                       GVPos_Event *events, int event_cap)
{
    if (ctx == 0 || actors == 0 || vehicles == 0 || seats == 0 || events == 0) {
        return GVPOS_ERR_NULL;
    }
    if (actor_cap <= 0 || vehicle_cap <= 0 || seat_cap <= 0 || event_cap <= 1) {
        return GVPOS_ERR_BAD_ARG;
    }
    ctx->actors = actors;
    ctx->actor_cap = actor_cap;
    ctx->actor_count = 0;
    ctx->vehicles = vehicles;
    ctx->vehicle_cap = vehicle_cap;
    ctx->vehicle_count = 0;
    ctx->seats = seats;
    ctx->seat_cap = seat_cap;
    ctx->seat_count = 0;
    ctx->events = events;
    ctx->event_cap = event_cap;
    ctx->event_head = 0;
    ctx->event_tail = 0;
    gvpos_clear_storage(ctx);
    return GVPOS_OK;
}

GVPos_Actor *gvpos_find_actor(GVPos_Context *ctx, GVPos_Id actor_id)
{
    int i;
    if (ctx == 0 || ctx->actors == 0) {
        return 0;
    }
    for (i = 0; i < ctx->actor_count; ++i) {
        if (ctx->actors[i].id == actor_id) {
            return &ctx->actors[i];
        }
    }
    return 0;
}

GVPos_Vehicle *gvpos_find_vehicle(GVPos_Context *ctx, GVPos_Id vehicle_id)
{
    int i;
    if (ctx == 0 || ctx->vehicles == 0) {
        return 0;
    }
    for (i = 0; i < ctx->vehicle_count; ++i) {
        if (ctx->vehicles[i].id == vehicle_id) {
            return &ctx->vehicles[i];
        }
    }
    return 0;
}

GVPos_Seat *gvpos_find_seat(GVPos_Context *ctx, GVPos_Id vehicle_id, int seat_slot)
{
    int i;
    if (ctx == 0 || ctx->seats == 0) {
        return 0;
    }
    for (i = 0; i < ctx->seat_count; ++i) {
        if (ctx->seats[i].vehicle_id == vehicle_id && ctx->seats[i].slot == seat_slot) {
            return &ctx->seats[i];
        }
    }
    return 0;
}

int gvpos_add_actor(GVPos_Context *ctx, GVPos_Id actor_id, int flags, GVPos_Vec3 pos)
{
    GVPos_Actor *a;
    if (ctx == 0 || ctx->actors == 0) {
        return GVPOS_ERR_NULL;
    }
    if (actor_id == GVPOS_ID_NONE) {
        return GVPOS_ERR_BAD_ARG;
    }
    if (gvpos_find_actor(ctx, actor_id) != 0) {
        return GVPOS_ERR_DUPLICATE;
    }
    if (ctx->actor_count >= ctx->actor_cap) {
        return GVPOS_ERR_FULL;
    }
    a = &ctx->actors[ctx->actor_count];
    ctx->actor_count = ctx->actor_count + 1;
    a->id = actor_id;
    a->flags = flags;
    a->phase = GVPOS_ACTOR_ON_FOOT;
    a->vehicle_id = GVPOS_ID_NONE;
    a->seat_slot = -1;
    a->timer_ticks = 0;
    a->cooldown_ticks = 0;
    gvpos_zero_input(&a->input);
    a->cached_pos = pos;
    return GVPOS_OK;
}

int gvpos_add_vehicle(GVPos_Context *ctx, GVPos_Id vehicle_id, int flags, int driver_slot, GVPos_Vec3 pos)
{
    GVPos_Vehicle *v;
    if (ctx == 0 || ctx->vehicles == 0) {
        return GVPOS_ERR_NULL;
    }
    if (vehicle_id == GVPOS_ID_NONE) {
        return GVPOS_ERR_BAD_ARG;
    }
    if (gvpos_find_vehicle(ctx, vehicle_id) != 0) {
        return GVPOS_ERR_DUPLICATE;
    }
    if (ctx->vehicle_count >= ctx->vehicle_cap) {
        return GVPOS_ERR_FULL;
    }
    v = &ctx->vehicles[ctx->vehicle_count];
    ctx->vehicle_count = ctx->vehicle_count + 1;
    v->id = vehicle_id;
    v->flags = flags;
    v->driver_slot = driver_slot;
    v->driver_actor_id = GVPOS_ID_NONE;
    v->engine_on = 0;
    v->cached_pos = pos;
    return GVPOS_OK;
}

int gvpos_add_seat(GVPos_Context *ctx, GVPos_Id vehicle_id, int seat_slot, int flags, GVPos_FP entry_radius, GVPos_Vec3 mount_pos, GVPos_Vec3 exit_pos)
{
    GVPos_Seat *s;
    if (ctx == 0 || ctx->seats == 0) {
        return GVPOS_ERR_NULL;
    }
    if (gvpos_find_vehicle(ctx, vehicle_id) == 0) {
        return GVPOS_ERR_NOT_FOUND;
    }
    if (gvpos_find_seat(ctx, vehicle_id, seat_slot) != 0) {
        return GVPOS_ERR_DUPLICATE;
    }
    if (ctx->seat_count >= ctx->seat_cap) {
        return GVPOS_ERR_FULL;
    }
    s = &ctx->seats[ctx->seat_count];
    ctx->seat_count = ctx->seat_count + 1;
    s->vehicle_id = vehicle_id;
    s->slot = seat_slot;
    s->flags = flags;
    if (entry_radius > 0) {
        s->entry_radius = entry_radius;
    } else {
        s->entry_radius = ctx->cfg.default_entry_radius;
    }
    s->exit_check_radius = ctx->cfg.default_exit_check_radius;
    s->local_mount_pos = mount_pos;
    s->local_exit_pos = exit_pos;
    s->occupant_actor_id = GVPOS_ID_NONE;
    s->reserved_by_actor_id = GVPOS_ID_NONE;
    return GVPOS_OK;
}

int gvpos_set_actor_input(GVPos_Context *ctx, GVPos_Id actor_id, const GVPos_Input *input)
{
    GVPos_Actor *a;
    if (ctx == 0 || input == 0) {
        return GVPOS_ERR_NULL;
    }
    a = gvpos_find_actor(ctx, actor_id);
    if (a == 0) {
        return GVPOS_ERR_NOT_FOUND;
    }
    a->input = *input;
    return GVPOS_OK;
}

int gvpos_set_actor_pos(GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Vec3 pos)
{
    GVPos_Actor *a;
    if (ctx == 0) {
        return GVPOS_ERR_NULL;
    }
    a = gvpos_find_actor(ctx, actor_id);
    if (a == 0) {
        return GVPOS_ERR_NOT_FOUND;
    }
    a->cached_pos = pos;
    return GVPOS_OK;
}

int gvpos_set_vehicle_pos(GVPos_Context *ctx, GVPos_Id vehicle_id, GVPos_Vec3 pos)
{
    GVPos_Vehicle *v;
    if (ctx == 0) {
        return GVPOS_ERR_NULL;
    }
    v = gvpos_find_vehicle(ctx, vehicle_id);
    if (v == 0) {
        return GVPOS_ERR_NOT_FOUND;
    }
    v->cached_pos = pos;
    return GVPOS_OK;
}

static int gvpos_read_actor_pos(GVPos_Context *ctx, GVPos_Actor *a, GVPos_Vec3 *out_pos)
{
    if (ctx->cb.get_actor_pos != 0) {
        if (ctx->cb.get_actor_pos(ctx, a->id, out_pos, ctx->user) != GVPOS_FALSE) {
            return GVPOS_OK;
        }
    }
    *out_pos = a->cached_pos;
    return GVPOS_OK;
}

static int gvpos_read_vehicle_pos(GVPos_Context *ctx, GVPos_Vehicle *v, GVPos_Vec3 *out_pos)
{
    if (ctx->cb.get_vehicle_pos != 0) {
        if (ctx->cb.get_vehicle_pos(ctx, v->id, out_pos, ctx->user) != GVPOS_FALSE) {
            return GVPOS_OK;
        }
    }
    *out_pos = v->cached_pos;
    return GVPOS_OK;
}

static int gvpos_validate_mount(GVPos_Context *ctx, GVPos_Actor *a, GVPos_Vehicle *v, GVPos_Seat *s, int req_flags)
{
    GVPos_Vec3 actor_pos;
    GVPos_Vec3 vehicle_pos;
    int forced;
    forced = (req_flags & GVPOS_REQ_FORCE) != 0;
    if (a == 0 || v == 0 || s == 0) {
        return GVPOS_ERR_NOT_FOUND;
    }
    if ((a->flags & GVPOS_ACTOR_FLAG_DISABLED) != 0 && forced == 0) {
        return GVPOS_ERR_BAD_STATE;
    }
    if (a->phase != GVPOS_ACTOR_ON_FOOT && forced == 0) {
        return GVPOS_ERR_BAD_STATE;
    }
    if (a->cooldown_ticks > 0 && forced == 0) {
        return GVPOS_ERR_BAD_STATE;
    }
    if ((v->flags & GVPOS_VEHICLE_FLAG_USABLE) == 0 && forced == 0) {
        return GVPOS_ERR_LOCKED;
    }
    if ((v->flags & GVPOS_VEHICLE_FLAG_LOCKED) != 0 && forced == 0) {
        return GVPOS_ERR_LOCKED;
    }
    if ((s->flags & GVPOS_SEAT_FLAG_LOCKED) != 0 && forced == 0) {
        return GVPOS_ERR_LOCKED;
    }
    if (s->occupant_actor_id != GVPOS_ID_NONE && (req_flags & GVPOS_REQ_ALLOW_SWAP) == 0) {
        return GVPOS_ERR_SEAT_BUSY;
    }
    if (s->reserved_by_actor_id != GVPOS_ID_NONE && s->reserved_by_actor_id != a->id) {
        return GVPOS_ERR_SEAT_BUSY;
    }
    if (gvpos_same_actor_kind_allowed(a, v, s) == GVPOS_FALSE && forced == 0) {
        return GVPOS_ERR_DENIED;
    }
    if ((req_flags & GVPOS_REQ_CHECK_DISTANCE) != 0 && forced == 0) {
        gvpos_read_actor_pos(ctx, a, &actor_pos);
        gvpos_read_vehicle_pos(ctx, v, &vehicle_pos);
        vehicle_pos = gvpos_vec_add(vehicle_pos, s->local_mount_pos);
        if (gvpos_box_distance_ok(actor_pos, vehicle_pos, s->entry_radius) == GVPOS_FALSE) {
            return GVPOS_ERR_TOO_FAR;
        }
    }
    if ((req_flags & GVPOS_REQ_CHECK_CALLBACK) != 0 && forced == 0) {
        if (ctx->cb.can_enter != 0) {
            if (ctx->cb.can_enter(ctx, a->id, v->id, s->slot, ctx->user) == GVPOS_FALSE) {
                return GVPOS_ERR_DENIED;
            }
        }
    }
    return GVPOS_OK;
}

static void gvpos_complete_mount(GVPos_Context *ctx, GVPos_Actor *a, GVPos_Vehicle *v, GVPos_Seat *s)
{
    if (s->occupant_actor_id != GVPOS_ID_NONE && s->occupant_actor_id != a->id) {
        GVPos_Actor *old_actor;
        old_actor = gvpos_find_actor(ctx, s->occupant_actor_id);
        if (old_actor != 0) {
            old_actor->phase = GVPOS_ACTOR_ON_FOOT;
            old_actor->vehicle_id = GVPOS_ID_NONE;
            old_actor->seat_slot = -1;
            old_actor->cooldown_ticks = ctx->cfg.default_cooldown_ticks;
            if (ctx->cb.set_actor_hidden != 0) {
                ctx->cb.set_actor_hidden(ctx, old_actor->id, 0, ctx->user);
            }
            if (ctx->cb.set_actor_attached != 0) {
                ctx->cb.set_actor_attached(ctx, old_actor->id, v->id, s->slot, 0, ctx->user);
            }
        }
    }
    s->occupant_actor_id = a->id;
    s->reserved_by_actor_id = GVPOS_ID_NONE;
    a->phase = GVPOS_ACTOR_IN_VEHICLE;
    a->vehicle_id = v->id;
    a->seat_slot = s->slot;
    a->timer_ticks = 0;
    a->cooldown_ticks = 0;
    a->flags = a->flags | GVPOS_ACTOR_FLAG_HIDDEN;
    if ((s->flags & GVPOS_SEAT_FLAG_DRIVER) != 0 || v->driver_slot == s->slot) {
        v->driver_actor_id = a->id;
        v->engine_on = 1;
        gvpos_push_event(ctx, GVPOS_EVENT_DRIVER_CHANGED, a->id, v->id, s->slot, GVPOS_OK);
    }
    if (ctx->cb.set_actor_hidden != 0) {
        ctx->cb.set_actor_hidden(ctx, a->id, 1, ctx->user);
    }
    if (ctx->cb.set_actor_attached != 0) {
        ctx->cb.set_actor_attached(ctx, a->id, v->id, s->slot, 1, ctx->user);
    }
    gvpos_push_event(ctx, GVPOS_EVENT_MOUNTED, a->id, v->id, s->slot, GVPOS_OK);
}

int gvpos_request_mount(GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int seat_slot, int req_flags)
{
    GVPos_Actor *a;
    GVPos_Vehicle *v;
    GVPos_Seat *s;
    int r;
    if (ctx == 0) {
        return GVPOS_ERR_NULL;
    }
    a = gvpos_find_actor(ctx, actor_id);
    v = gvpos_find_vehicle(ctx, vehicle_id);
    s = gvpos_find_seat(ctx, vehicle_id, seat_slot);
    r = gvpos_validate_mount(ctx, a, v, s, req_flags);
    if (r != GVPOS_OK) {
        gvpos_push_event(ctx, GVPOS_EVENT_DENIED, actor_id, vehicle_id, seat_slot, r);
        return r;
    }
    s->reserved_by_actor_id = actor_id;
    a->phase = GVPOS_ACTOR_ENTERING;
    a->vehicle_id = vehicle_id;
    a->seat_slot = seat_slot;
    if ((s->flags & GVPOS_SEAT_FLAG_NO_ANIM) != 0) {
        a->timer_ticks = 0;
    } else {
        a->timer_ticks = ctx->cfg.default_enter_ticks;
    }
    gvpos_push_event(ctx, GVPOS_EVENT_ENTER_BEGIN, actor_id, vehicle_id, seat_slot, GVPOS_OK);
    if (a->timer_ticks <= 0) {
        gvpos_complete_mount(ctx, a, v, s);
    }
    return GVPOS_OK;
}

int gvpos_request_mount_nearest(GVPos_Context *ctx, GVPos_Id actor_id, GVPos_Id vehicle_id, int req_flags)
{
    int i;
    int r;
    int best_slot;
    GVPos_Actor *a;
    GVPos_Vehicle *v;
    GVPos_Seat *s;
    if (ctx == 0) {
        return GVPOS_ERR_NULL;
    }
    a = gvpos_find_actor(ctx, actor_id);
    v = gvpos_find_vehicle(ctx, vehicle_id);
    if (a == 0 || v == 0) {
        return GVPOS_ERR_NOT_FOUND;
    }
    best_slot = -1;
    for (i = 0; i < ctx->seat_count; ++i) {
        s = &ctx->seats[i];
        if (s->vehicle_id == vehicle_id) {
            r = gvpos_validate_mount(ctx, a, v, s, req_flags);
            if (r == GVPOS_OK) {
                best_slot = s->slot;
                break;
            }
        }
    }
    if (best_slot < 0) {
        gvpos_push_event(ctx, GVPOS_EVENT_DENIED, actor_id, vehicle_id, -1, GVPOS_ERR_DENIED);
        return GVPOS_ERR_DENIED;
    }
    return gvpos_request_mount(ctx, actor_id, vehicle_id, best_slot, req_flags);
}

int gvpos_cancel_enter(GVPos_Context *ctx, GVPos_Id actor_id)
{
    GVPos_Actor *a;
    GVPos_Seat *s;
    if (ctx == 0) {
        return GVPOS_ERR_NULL;
    }
    a = gvpos_find_actor(ctx, actor_id);
    if (a == 0) {
        return GVPOS_ERR_NOT_FOUND;
    }
    if (a->phase != GVPOS_ACTOR_ENTERING) {
        return GVPOS_ERR_BAD_STATE;
    }
    s = gvpos_find_seat(ctx, a->vehicle_id, a->seat_slot);
    if (s != 0 && s->reserved_by_actor_id == actor_id) {
        s->reserved_by_actor_id = GVPOS_ID_NONE;
    }
    gvpos_push_event(ctx, GVPOS_EVENT_RESERVATION_CANCELLED, a->id, a->vehicle_id, a->seat_slot, GVPOS_OK);
    a->phase = GVPOS_ACTOR_ON_FOOT;
    a->vehicle_id = GVPOS_ID_NONE;
    a->seat_slot = -1;
    a->timer_ticks = 0;
    a->cooldown_ticks = ctx->cfg.default_cooldown_ticks;
    return GVPOS_OK;
}

int gvpos_request_exit(GVPos_Context *ctx, GVPos_Id actor_id, int req_flags)
{
    GVPos_Actor *a;
    GVPos_Vehicle *v;
    GVPos_Seat *s;
    GVPos_Vec3 vehicle_pos;
    GVPos_Vec3 exit_pos;
    int forced;
    if (ctx == 0) {
        return GVPOS_ERR_NULL;
    }
    forced = (req_flags & GVPOS_REQ_FORCE) != 0;
    a = gvpos_find_actor(ctx, actor_id);
    if (a == 0) {
        return GVPOS_ERR_NOT_FOUND;
    }
    if (a->phase == GVPOS_ACTOR_ENTERING) {
        return gvpos_cancel_enter(ctx, actor_id);
    }
    if (a->phase != GVPOS_ACTOR_IN_VEHICLE && forced == 0) {
        return GVPOS_ERR_BAD_STATE;
    }
    v = gvpos_find_vehicle(ctx, a->vehicle_id);
    s = gvpos_find_seat(ctx, a->vehicle_id, a->seat_slot);
    if (v == 0 || s == 0) {
        return GVPOS_ERR_NOT_FOUND;
    }
    gvpos_read_vehicle_pos(ctx, v, &vehicle_pos);
    exit_pos = gvpos_vec_add(vehicle_pos, s->local_exit_pos);
    if ((req_flags & GVPOS_REQ_CHECK_CALLBACK) != 0 && forced == 0) {
        if (ctx->cb.can_exit != 0) {
            if (ctx->cb.can_exit(ctx, a->id, v->id, s->slot, ctx->user) == GVPOS_FALSE) {
                gvpos_push_event(ctx, GVPOS_EVENT_DENIED, a->id, v->id, s->slot, GVPOS_ERR_DENIED);
                return GVPOS_ERR_DENIED;
            }
        }
    }
    if ((req_flags & GVPOS_REQ_CHECK_DISTANCE) != 0 && forced == 0) {
        if (ctx->cb.test_exit_clear != 0) {
            if (ctx->cb.test_exit_clear(ctx, a->id, v->id, s->slot, exit_pos, s->exit_check_radius, ctx->user) == GVPOS_FALSE) {
                gvpos_push_event(ctx, GVPOS_EVENT_DENIED, a->id, v->id, s->slot, GVPOS_ERR_BLOCKED_EXIT);
                return GVPOS_ERR_BLOCKED_EXIT;
            }
        }
    }
    s->occupant_actor_id = GVPOS_ID_NONE;
    s->reserved_by_actor_id = GVPOS_ID_NONE;
    if (v->driver_actor_id == a->id) {
        v->driver_actor_id = GVPOS_ID_NONE;
        if ((v->flags & GVPOS_VEHICLE_FLAG_KEEP_ENGINE) == 0) {
            v->engine_on = 0;
        }
        gvpos_push_event(ctx, GVPOS_EVENT_DRIVER_CHANGED, GVPOS_ID_NONE, v->id, s->slot, GVPOS_OK);
    }
    a->phase = GVPOS_ACTOR_EXITING;
    if ((s->flags & GVPOS_SEAT_FLAG_NO_ANIM) != 0) {
        a->timer_ticks = 0;
    } else {
        a->timer_ticks = ctx->cfg.default_exit_ticks;
    }
    a->cached_pos = exit_pos;
    if (ctx->cb.set_actor_attached != 0) {
        ctx->cb.set_actor_attached(ctx, a->id, v->id, s->slot, 0, ctx->user);
    }
    gvpos_push_event(ctx, GVPOS_EVENT_EXIT_BEGIN, a->id, v->id, s->slot, GVPOS_OK);
    if (a->timer_ticks <= 0) {
        a->phase = GVPOS_ACTOR_ON_FOOT;
        a->vehicle_id = GVPOS_ID_NONE;
        a->seat_slot = -1;
        a->cooldown_ticks = ctx->cfg.default_cooldown_ticks;
        a->flags = a->flags & (~GVPOS_ACTOR_FLAG_HIDDEN);
        if (ctx->cb.set_actor_hidden != 0) {
            ctx->cb.set_actor_hidden(ctx, a->id, 0, ctx->user);
        }
        gvpos_push_event(ctx, GVPOS_EVENT_EXITED, a->id, v->id, s->slot, GVPOS_OK);
    }
    return GVPOS_OK;
}

void gvpos_update(GVPos_Context *ctx)
{
    int i;
    GVPos_Actor *a;
    GVPos_Vehicle *v;
    GVPos_Seat *s;
    if (ctx == 0) {
        return;
    }
    ctx->tick = ctx->tick + 1u;
    for (i = 0; i < ctx->actor_count; ++i) {
        a = &ctx->actors[i];
        if (a->cooldown_ticks > 0) {
            a->cooldown_ticks = a->cooldown_ticks - 1;
        }
        if (a->phase == GVPOS_ACTOR_ENTERING) {
            if (a->timer_ticks > 0) {
                a->timer_ticks = a->timer_ticks - 1;
            }
            if (a->timer_ticks <= 0) {
                v = gvpos_find_vehicle(ctx, a->vehicle_id);
                s = gvpos_find_seat(ctx, a->vehicle_id, a->seat_slot);
                if (v != 0 && s != 0 && s->reserved_by_actor_id == a->id) {
                    gvpos_complete_mount(ctx, a, v, s);
                } else {
                    a->phase = GVPOS_ACTOR_ON_FOOT;
                    a->vehicle_id = GVPOS_ID_NONE;
                    a->seat_slot = -1;
                    a->cooldown_ticks = ctx->cfg.default_cooldown_ticks;
                }
            }
        } else if (a->phase == GVPOS_ACTOR_EXITING) {
            if (a->timer_ticks > 0) {
                a->timer_ticks = a->timer_ticks - 1;
            }
            if (a->timer_ticks <= 0) {
                v = gvpos_find_vehicle(ctx, a->vehicle_id);
                s = gvpos_find_seat(ctx, a->vehicle_id, a->seat_slot);
                a->phase = GVPOS_ACTOR_ON_FOOT;
                a->vehicle_id = GVPOS_ID_NONE;
                a->seat_slot = -1;
                a->cooldown_ticks = ctx->cfg.default_cooldown_ticks;
                a->flags = a->flags & (~GVPOS_ACTOR_FLAG_HIDDEN);
                if (ctx->cb.set_actor_hidden != 0) {
                    ctx->cb.set_actor_hidden(ctx, a->id, 0, ctx->user);
                }
                if (v != 0 && s != 0) {
                    gvpos_push_event(ctx, GVPOS_EVENT_EXITED, a->id, v->id, s->slot, GVPOS_OK);
                } else {
                    gvpos_push_event(ctx, GVPOS_EVENT_EXITED, a->id, GVPOS_ID_NONE, -1, GVPOS_OK);
                }
            }
        } else {
            /* no timed transition */
        }
    }
    for (i = 0; i < ctx->vehicle_count; ++i) {
        v = &ctx->vehicles[i];
        if (v->driver_actor_id != GVPOS_ID_NONE && ctx->cb.apply_vehicle_input != 0) {
            a = gvpos_find_actor(ctx, v->driver_actor_id);
            if (a != 0 && a->phase == GVPOS_ACTOR_IN_VEHICLE) {
                ctx->cb.apply_vehicle_input(ctx, v->id, a->id, &a->input, ctx->user);
            }
        }
    }
}

int gvpos_pop_event(GVPos_Context *ctx, GVPos_Event *out_event)
{
    if (ctx == 0 || out_event == 0) {
        return GVPOS_ERR_NULL;
    }
    if (ctx->events == 0 || ctx->event_head == ctx->event_tail) {
        return GVPOS_ERR_NOT_FOUND;
    }
    *out_event = ctx->events[ctx->event_head];
    ctx->event_head = ctx->event_head + 1;
    if (ctx->event_head >= ctx->event_cap) {
        ctx->event_head = 0;
    }
    return GVPOS_OK;
}

GVPos_Bool gvpos_actor_is_mounted(GVPos_Context *ctx, GVPos_Id actor_id)
{
    GVPos_Actor *a;
    a = gvpos_find_actor(ctx, actor_id);
    if (a == 0) {
        return GVPOS_FALSE;
    }
    if (a->phase == GVPOS_ACTOR_IN_VEHICLE) {
        return GVPOS_TRUE;
    }
    return GVPOS_FALSE;
}

GVPos_Id gvpos_vehicle_driver(GVPos_Context *ctx, GVPos_Id vehicle_id)
{
    GVPos_Vehicle *v;
    v = gvpos_find_vehicle(ctx, vehicle_id);
    if (v == 0) {
        return GVPOS_ID_NONE;
    }
    return v->driver_actor_id;
}

const char *gvpos_result_name(int result)
{
    switch (result) {
        case GVPOS_OK: return "OK";
        case GVPOS_ERR_NULL: return "ERR_NULL";
        case GVPOS_ERR_FULL: return "ERR_FULL";
        case GVPOS_ERR_NOT_FOUND: return "ERR_NOT_FOUND";
        case GVPOS_ERR_BAD_STATE: return "ERR_BAD_STATE";
        case GVPOS_ERR_LOCKED: return "ERR_LOCKED";
        case GVPOS_ERR_SEAT_BUSY: return "ERR_SEAT_BUSY";
        case GVPOS_ERR_TOO_FAR: return "ERR_TOO_FAR";
        case GVPOS_ERR_DENIED: return "ERR_DENIED";
        case GVPOS_ERR_BLOCKED_EXIT: return "ERR_BLOCKED_EXIT";
        case GVPOS_ERR_BAD_STORAGE: return "ERR_BAD_STORAGE";
        case GVPOS_ERR_DUPLICATE: return "ERR_DUPLICATE";
        case GVPOS_ERR_BAD_ARG: return "ERR_BAD_ARG";
        default: return "ERR_UNKNOWN";
    }
}

const char *gvpos_event_name(int event_type)
{
    switch (event_type) {
        case GVPOS_EVENT_NONE: return "NONE";
        case GVPOS_EVENT_ENTER_BEGIN: return "ENTER_BEGIN";
        case GVPOS_EVENT_MOUNTED: return "MOUNTED";
        case GVPOS_EVENT_EXIT_BEGIN: return "EXIT_BEGIN";
        case GVPOS_EVENT_EXITED: return "EXITED";
        case GVPOS_EVENT_DENIED: return "DENIED";
        case GVPOS_EVENT_DRIVER_CHANGED: return "DRIVER_CHANGED";
        case GVPOS_EVENT_RESERVATION_CANCELLED: return "RESERVATION_CANCELLED";
        default: return "UNKNOWN";
    }
}
