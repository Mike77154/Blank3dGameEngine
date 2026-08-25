#include "gk3d.h"

static void gk3d_memzero(void *ptr, unsigned long size)
{
    unsigned char *bytes;
    unsigned long i;

    bytes = (unsigned char *)ptr;
    for (i = 0; i < size; ++i) {
        bytes[i] = 0;
    }
}

static int gk3d_streq(const char *a, const char *b)
{
    if (a == 0 || b == 0) {
        return 0;
    }
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        ++a;
        ++b;
    }
    return *a == *b;
}

gk3d_fix gk3d_fix_mul(gk3d_fix a, gk3d_fix b)
{
    return (a * b) / GK3D_ONE;
}

gk3d_fix gk3d_fix_div(gk3d_fix a, gk3d_fix b)
{
    if (b == 0) {
        return 0;
    }
    return (a * GK3D_ONE) / b;
}

static void gk3d_zero_contact(gk3d_contact_state *contact)
{
    if (contact == 0) {
        return;
    }
    gk3d_memzero(contact, (unsigned long)sizeof(*contact));
    contact->floor_id = GK3D_ID_NONE;
    contact->ceiling_id = GK3D_ID_NONE;
    contact->wall_neg_a_id = GK3D_ID_NONE;
    contact->wall_pos_a_id = GK3D_ID_NONE;
    contact->wall_neg_b_id = GK3D_ID_NONE;
    contact->wall_pos_b_id = GK3D_ID_NONE;
}

void gk3d_world_init(gk3d_world *world)
{
    if (world == 0) {
        return;
    }
    gk3d_memzero(world, (unsigned long)sizeof(*world));
    world->up_axis = GK3D_AXIS_Y;
    world->up_sign = 1;
    world->probe_distance = (gk3d_fix)GK3D_DEFAULT_PROBE_RAW;
    world->solver_step = GK3D_FROM_INT(GK3D_DEFAULT_SOLVER_STEP_PIXELS);
}

void gk3d_world_clear(gk3d_world *world)
{
    gk3d_world_init(world);
}

void gk3d_world_set_up(gk3d_world *world, int axis, int positive_is_up)
{
    if (world == 0) {
        return;
    }
    if (axis < GK3D_AXIS_X || axis > GK3D_AXIS_Z) {
        return;
    }
    world->up_axis = axis;
    world->up_sign = positive_is_up ? 1 : -1;
}

void gk3d_world_set_probe(gk3d_world *world, gk3d_fix distance)
{
    if (world == 0) {
        return;
    }
    world->probe_distance = GK3D_ABS(distance);
}

void gk3d_world_set_solver_step(gk3d_world *world, gk3d_fix step)
{
    if (world == 0) {
        return;
    }
    step = GK3D_ABS(step);
    if (step == 0) {
        step = 1;
    }
    world->solver_step = step;
}

void gk3d_world_set_narrowphase(gk3d_world *world, gk3d_narrowphase_fn fn, void *user)
{
    if (world == 0) {
        return;
    }
    world->narrowphase = fn;
    world->narrowphase_user = user;
}

void gk3d_world_reset_stats(gk3d_world *world)
{
    if (world == 0) {
        return;
    }
    gk3d_memzero(&world->stats, (unsigned long)sizeof(world->stats));
}

int gk3d_world_add_box(gk3d_world *world, int type,
    gk3d_fix x, gk3d_fix y, gk3d_fix z,
    gk3d_fix w, gk3d_fix h, gk3d_fix d,
    unsigned int flags)
{
    int i;
    gk3d_obj *obj;

    if (world == 0 || w <= 0 || h <= 0 || d <= 0) {
        return GK3D_ID_NONE;
    }
    for (i = 0; i < GK3D_MAX_OBJECTS; ++i) {
        if (!world->arena.objects[i].used) {
            obj = &world->arena.objects[i];
            gk3d_memzero(obj, (unsigned long)sizeof(*obj));
            obj->used = 1;
            obj->id = i;
            obj->type = type;
            obj->family = -1;
            obj->layer = 0;
            obj->flags = flags | GK3D_FLAG_ACTIVE | GK3D_FLAG_ENABLED;
            obj->box.x = x;
            obj->box.y = y;
            obj->box.z = z;
            obj->box.w = w;
            obj->box.h = h;
            obj->box.d = d;
            gk3d_zero_contact(&obj->contact);
            ++world->count;
            return i;
        }
    }
    return GK3D_ID_NONE;
}

void gk3d_world_remove(gk3d_world *world, int id)
{
    if (world == 0 || id < 0 || id >= GK3D_MAX_OBJECTS) {
        return;
    }
    if (world->arena.objects[id].used) {
        gk3d_memzero(&world->arena.objects[id], (unsigned long)sizeof(world->arena.objects[id]));
        if (world->count > 0) {
            --world->count;
        }
    }
}

gk3d_obj *gk3d_world_get(gk3d_world *world, int id)
{
    if (world == 0 || id < 0 || id >= GK3D_MAX_OBJECTS) {
        return 0;
    }
    if (!world->arena.objects[id].used) {
        return 0;
    }
    return &world->arena.objects[id];
}

const gk3d_obj *gk3d_world_get_const(const gk3d_world *world, int id)
{
    if (world == 0 || id < 0 || id >= GK3D_MAX_OBJECTS) {
        return 0;
    }
    if (!world->arena.objects[id].used) {
        return 0;
    }
    return &world->arena.objects[id];
}

void gk3d_obj_set_pos(gk3d_world *world, int id, gk3d_fix x, gk3d_fix y, gk3d_fix z)
{
    gk3d_obj *obj;
    obj = gk3d_world_get(world, id);
    if (obj == 0) {
        return;
    }
    obj->box.x = x;
    obj->box.y = y;
    obj->box.z = z;
}

void gk3d_obj_set_box(gk3d_world *world, int id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z,
    gk3d_fix w, gk3d_fix h, gk3d_fix d)
{
    gk3d_obj *obj;
    obj = gk3d_world_get(world, id);
    if (obj == 0 || w <= 0 || h <= 0 || d <= 0) {
        return;
    }
    obj->box.x = x;
    obj->box.y = y;
    obj->box.z = z;
    obj->box.w = w;
    obj->box.h = h;
    obj->box.d = d;
}

void gk3d_obj_set_flag(gk3d_world *world, int id, unsigned int flag, int enabled)
{
    gk3d_obj *obj;
    obj = gk3d_world_get(world, id);
    if (obj == 0) {
        return;
    }
    if (enabled) {
        obj->flags |= flag;
    } else {
        obj->flags &= ~flag;
    }
}

void gk3d_obj_set_family(gk3d_world *world, int id, int family)
{
    gk3d_obj *obj;
    obj = gk3d_world_get(world, id);
    if (obj != 0) {
        obj->family = family;
    }
}

void gk3d_obj_set_layer(gk3d_world *world, int id, int layer)
{
    gk3d_obj *obj;
    obj = gk3d_world_get(world, id);
    if (obj != 0) {
        obj->layer = layer;
    }
}

void gk3d_obj_set_tags(gk3d_world *world, int id, gk3d_u32 tags)
{
    gk3d_obj *obj;
    obj = gk3d_world_get(world, id);
    if (obj != 0) {
        obj->tags = tags;
    }
}

int gk3d_aabb_valid(gk3d_aabb box)
{
    return box.w > 0 && box.h > 0 && box.d > 0;
}

int gk3d_aabb_overlap(gk3d_aabb a, gk3d_aabb b)
{
    if (!gk3d_aabb_valid(a) || !gk3d_aabb_valid(b)) {
        return 0;
    }
    if (a.x >= b.x + b.w || a.x + a.w <= b.x) {
        return 0;
    }
    if (a.y >= b.y + b.h || a.y + a.h <= b.y) {
        return 0;
    }
    if (a.z >= b.z + b.d || a.z + a.d <= b.z) {
        return 0;
    }
    return 1;
}

gk3d_aabb gk3d_aabb_at(gk3d_aabb box, gk3d_fix x, gk3d_fix y, gk3d_fix z)
{
    box.x = x;
    box.y = y;
    box.z = z;
    return box;
}

gk3d_aabb gk3d_aabb_offset(gk3d_aabb box, gk3d_fix dx, gk3d_fix dy, gk3d_fix dz)
{
    box.x += dx;
    box.y += dy;
    box.z += dz;
    return box;
}

gk3d_filter gk3d_filter_make(int target)
{
    gk3d_filter filter;
    gk3d_memzero(&filter, (unsigned long)sizeof(filter));
    filter.target = target;
    filter.exact_id = GK3D_ID_NONE;
    filter.family = -1;
    filter.layer = -1;
    return filter;
}

int gk3d_filter_match(const gk3d_obj *obj, int self_id, const gk3d_filter *filter)
{
    if (obj == 0 || filter == 0 || !obj->used || obj->id == self_id) {
        return 0;
    }
    if (!(obj->flags & GK3D_FLAG_ACTIVE) || !(obj->flags & GK3D_FLAG_ENABLED)) {
        return 0;
    }
    if (obj->flags & GK3D_FLAG_DISABLED) {
        return 0;
    }
    if (filter->exact_id != GK3D_ID_NONE && obj->id != filter->exact_id) {
        return 0;
    }
    if (filter->target >= 0 && obj->type != filter->target) {
        return 0;
    }
    if (filter->target == GK3D_TARGET_SOLID && !(obj->flags & GK3D_FLAG_SOLID)) {
        return 0;
    }
    if (filter->target == GK3D_TARGET_TRIGGER && !(obj->flags & GK3D_FLAG_TRIGGER)) {
        return 0;
    }
    if (filter->target == GK3D_TARGET_SENSOR && !(obj->flags & GK3D_FLAG_SENSOR)) {
        return 0;
    }
    if (filter->target == GK3D_TARGET_ACTIVE && !(obj->flags & GK3D_FLAG_ACTIVE)) {
        return 0;
    }
    if (filter->target == GK3D_TARGET_FLOOR &&
        !(obj->flags & (GK3D_FLAG_SOLID | GK3D_FLAG_JUMPTHRU))) {
        return 0;
    }
    if (filter->target < 0 &&
        filter->target != GK3D_TARGET_ANY &&
        filter->target != GK3D_TARGET_SOLID &&
        filter->target != GK3D_TARGET_TRIGGER &&
        filter->target != GK3D_TARGET_SENSOR &&
        filter->target != GK3D_TARGET_ACTIVE &&
        filter->target != GK3D_TARGET_FLOOR) {
        return 0;
    }
    if (filter->family >= 0 && obj->family != filter->family) {
        return 0;
    }
    if (filter->layer >= 0 && obj->layer != filter->layer) {
        return 0;
    }
    if (filter->required_tags != 0 &&
        (obj->tags & filter->required_tags) != filter->required_tags) {
        return 0;
    }
    if (filter->flags_all != 0 &&
        (obj->flags & filter->flags_all) != filter->flags_all) {
        return 0;
    }
    if (filter->flags_any != 0 &&
        (obj->flags & filter->flags_any) == 0) {
        return 0;
    }
    if (filter->flags_none != 0 &&
        (obj->flags & filter->flags_none) != 0) {
        return 0;
    }
    return 1;
}

static int gk3d_precise_overlap(gk3d_world *world, int self_id,
    const gk3d_aabb *candidate, const gk3d_obj *other)
{
    const gk3d_obj *self;
    int requires_precise;
    int value;

    self = gk3d_world_get_const(world, self_id);
    requires_precise = 0;
    if (self != 0 && (self->flags & GK3D_FLAG_PRECISE)) {
        requires_precise = 1;
    }
    if (other != 0 && (other->flags & GK3D_FLAG_PRECISE)) {
        requires_precise = 1;
    }
    if (!requires_precise) {
        return 1;
    }
    if (world->narrowphase == 0) {
        ++world->stats.aabb_fallbacks;
        return 1;
    }
    ++world->stats.narrowphase_calls;
    value = world->narrowphase(world, self_id, candidate,
        other->id, world->narrowphase_user);
    if (value == GK3D_NARROWPHASE_UNHANDLED) {
        ++world->stats.aabb_fallbacks;
        return 1;
    }
    return value == GK3D_NARROWPHASE_YES;
}

int gk3d_instance_place_box_filter(gk3d_world *world, int self_id,
    gk3d_aabb candidate, const gk3d_filter *filter)
{
    int i;
    const gk3d_obj *obj;

    if (world == 0 || filter == 0 || !gk3d_aabb_valid(candidate)) {
        return GK3D_ID_NONE;
    }
    ++world->stats.queries;
    for (i = 0; i < GK3D_MAX_OBJECTS; ++i) {
        obj = &world->arena.objects[i];
        if (!gk3d_filter_match(obj, self_id, filter)) {
            continue;
        }
        ++world->stats.broadphase_tests;
        if (!gk3d_aabb_overlap(candidate, obj->box)) {
            continue;
        }
        ++world->stats.broadphase_hits;
        if (gk3d_precise_overlap(world, self_id, &candidate, obj)) {
            return obj->id;
        }
    }
    return GK3D_ID_NONE;
}

int gk3d_place_meeting_filter(gk3d_world *world, int self_id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z, const gk3d_filter *filter)
{
    const gk3d_obj *self;
    gk3d_aabb candidate;

    self = gk3d_world_get_const(world, self_id);
    if (self == 0) {
        return 0;
    }
    candidate = gk3d_aabb_at(self->box, x, y, z);
    return gk3d_instance_place_box_filter(world, self_id, candidate, filter) != GK3D_ID_NONE;
}

int gk3d_place_meeting(gk3d_world *world, int self_id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z, int target)
{
    gk3d_filter filter;
    filter = gk3d_filter_make(target);
    return gk3d_place_meeting_filter(world, self_id, x, y, z, &filter);
}

int gk3d_place_free(gk3d_world *world, int self_id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z)
{
    return !gk3d_place_meeting(world, self_id, x, y, z, GK3D_TARGET_SOLID);
}

int gk3d_instance_place(gk3d_world *world, int self_id,
    gk3d_fix x, gk3d_fix y, gk3d_fix z, int target)
{
    const gk3d_obj *self;
    gk3d_filter filter;
    gk3d_aabb candidate;

    self = gk3d_world_get_const(world, self_id);
    if (self == 0) {
        return GK3D_ID_NONE;
    }
    filter = gk3d_filter_make(target);
    candidate = gk3d_aabb_at(self->box, x, y, z);
    return gk3d_instance_place_box_filter(world, self_id, candidate, &filter);
}

int gk3d_overlap_at_offset(gk3d_world *world, int self_id,
    gk3d_fix dx, gk3d_fix dy, gk3d_fix dz, int target)
{
    const gk3d_obj *self;
    self = gk3d_world_get_const(world, self_id);
    if (self == 0) {
        return 0;
    }
    return gk3d_place_meeting(world, self_id,
        self->box.x + dx, self->box.y + dy, self->box.z + dz, target);
}

int gk3d_instance_at_offset(gk3d_world *world, int self_id,
    gk3d_fix dx, gk3d_fix dy, gk3d_fix dz, int target)
{
    const gk3d_obj *self;
    self = gk3d_world_get_const(world, self_id);
    if (self == 0) {
        return GK3D_ID_NONE;
    }
    return gk3d_instance_place(world, self_id,
        self->box.x + dx, self->box.y + dy, self->box.z + dz, target);
}

static void gk3d_axis_delta(int axis, gk3d_fix amount,
    gk3d_fix *dx, gk3d_fix *dy, gk3d_fix *dz)
{
    *dx = 0;
    *dy = 0;
    *dz = 0;
    if (axis == GK3D_AXIS_X) {
        *dx = amount;
    } else if (axis == GK3D_AXIS_Y) {
        *dy = amount;
    } else {
        *dz = amount;
    }
}

static int gk3d_probe_axis(gk3d_world *world, int self_id,
    int axis, int sign, int target)
{
    gk3d_fix dx;
    gk3d_fix dy;
    gk3d_fix dz;
    gk3d_fix amount;

    if (world == 0 || sign == 0) {
        return GK3D_ID_NONE;
    }
    amount = world->probe_distance * (sign < 0 ? -1 : 1);
    gk3d_axis_delta(axis, amount, &dx, &dy, &dz);
    return gk3d_instance_at_offset(world, self_id, dx, dy, dz, target);
}

int gk3d_floor_instance(gk3d_world *world, int self_id, int target)
{
    if (world == 0) {
        return GK3D_ID_NONE;
    }
    return gk3d_probe_axis(world, self_id, world->up_axis,
        -world->up_sign, target);
}

int gk3d_ceiling_instance(gk3d_world *world, int self_id, int target)
{
    if (world == 0) {
        return GK3D_ID_NONE;
    }
    return gk3d_probe_axis(world, self_id, world->up_axis,
        world->up_sign, target);
}

int gk3d_is_on_wall_axis(gk3d_world *world, int self_id, int axis, int sign, int target)
{
    if (world == 0 || axis == world->up_axis) {
        return 0;
    }
    return gk3d_probe_axis(world, self_id, axis, sign, target) != GK3D_ID_NONE;
}

int gk3d_wall_instance(gk3d_world *world, int self_id, int target)
{
    int axis;
    int id;

    if (world == 0) {
        return GK3D_ID_NONE;
    }
    for (axis = GK3D_AXIS_X; axis <= GK3D_AXIS_Z; ++axis) {
        if (axis == world->up_axis) {
            continue;
        }
        id = gk3d_probe_axis(world, self_id, axis, -1, target);
        if (id != GK3D_ID_NONE) {
            return id;
        }
        id = gk3d_probe_axis(world, self_id, axis, 1, target);
        if (id != GK3D_ID_NONE) {
            return id;
        }
    }
    return GK3D_ID_NONE;
}

int gk3d_is_on_floor(gk3d_world *world, int self_id)
{
    return gk3d_floor_instance(world, self_id, GK3D_TARGET_FLOOR) != GK3D_ID_NONE;
}

int gk3d_is_under_ceiling(gk3d_world *world, int self_id)
{
    return gk3d_ceiling_instance(world, self_id, GK3D_TARGET_SOLID) != GK3D_ID_NONE;
}

int gk3d_is_on_wall(gk3d_world *world, int self_id)
{
    return gk3d_wall_instance(world, self_id, GK3D_TARGET_SOLID) != GK3D_ID_NONE;
}

void gk3d_update_contact(gk3d_world *world, int self_id)
{
    gk3d_obj *obj;
    int horizontal[2];
    int count;
    int axis;
    int id;

    obj = gk3d_world_get(world, self_id);
    if (world == 0 || obj == 0) {
        return;
    }
    gk3d_zero_contact(&obj->contact);
    obj->contact.floor_id = gk3d_floor_instance(world, self_id, GK3D_TARGET_FLOOR);
    obj->contact.ceiling_id = gk3d_ceiling_instance(world, self_id, GK3D_TARGET_SOLID);
    obj->contact.on_floor = obj->contact.floor_id != GK3D_ID_NONE;
    obj->contact.under_ceiling = obj->contact.ceiling_id != GK3D_ID_NONE;

    count = 0;
    for (axis = GK3D_AXIS_X; axis <= GK3D_AXIS_Z; ++axis) {
        if (axis != world->up_axis && count < 2) {
            horizontal[count] = axis;
            ++count;
        }
    }
    if (count == 2) {
        id = gk3d_probe_axis(world, self_id, horizontal[0], -1, GK3D_TARGET_SOLID);
        obj->contact.wall_neg_a_id = id;
        id = gk3d_probe_axis(world, self_id, horizontal[0], 1, GK3D_TARGET_SOLID);
        obj->contact.wall_pos_a_id = id;
        id = gk3d_probe_axis(world, self_id, horizontal[1], -1, GK3D_TARGET_SOLID);
        obj->contact.wall_neg_b_id = id;
        id = gk3d_probe_axis(world, self_id, horizontal[1], 1, GK3D_TARGET_SOLID);
        obj->contact.wall_pos_b_id = id;
    }
    obj->contact.on_wall =
        obj->contact.wall_neg_a_id != GK3D_ID_NONE ||
        obj->contact.wall_pos_a_id != GK3D_ID_NONE ||
        obj->contact.wall_neg_b_id != GK3D_ID_NONE ||
        obj->contact.wall_pos_b_id != GK3D_ID_NONE;

    if (obj->contact.on_floor) {
        if (world->up_axis == GK3D_AXIS_X) {
            obj->contact.nx = world->up_sign * GK3D_ONE;
        } else if (world->up_axis == GK3D_AXIS_Y) {
            obj->contact.ny = world->up_sign * GK3D_ONE;
        } else {
            obj->contact.nz = world->up_sign * GK3D_ONE;
        }
    }
}

void gk3d_update_all_contacts(gk3d_world *world)
{
    int i;
    if (world == 0) {
        return;
    }
    for (i = 0; i < GK3D_MAX_OBJECTS; ++i) {
        if (world->arena.objects[i].used) {
            gk3d_update_contact(world, i);
        }
    }
}

int gk3d_verb_from_name(const char *name)
{
    if (name == 0) {
        return GK3D_VERB_NONE;
    }
    if (gk3d_streq(name, "place_meeting") ||
        gk3d_streq(name, "placeMeeting")) {
        return GK3D_VERB_PLACE_MEETING;
    }
    if (gk3d_streq(name, "place_free") ||
        gk3d_streq(name, "placeFree")) {
        return GK3D_VERB_PLACE_FREE;
    }
    if (gk3d_streq(name, "instance_place") ||
        gk3d_streq(name, "instancePlace")) {
        return GK3D_VERB_INSTANCE_PLACE;
    }
    if (gk3d_streq(name, "is_on_floor") ||
        gk3d_streq(name, "isonfloor") ||
        gk3d_streq(name, "isOnFloor")) {
        return GK3D_VERB_IS_ON_FLOOR;
    }
    if (gk3d_streq(name, "is_on_wall") ||
        gk3d_streq(name, "isonwall") ||
        gk3d_streq(name, "isByWall") ||
        gk3d_streq(name, "is_by_wall")) {
        return GK3D_VERB_IS_ON_WALL;
    }
    if (gk3d_streq(name, "is_under_ceiling") ||
        gk3d_streq(name, "isUnderCeiling")) {
        return GK3D_VERB_IS_UNDER_CEILING;
    }
    if (gk3d_streq(name, "overlap_at_offset") ||
        gk3d_streq(name, "is_overlapping_at_offset") ||
        gk3d_streq(name, "isOverlappingAtOffset")) {
        return GK3D_VERB_OVERLAP_OFFSET;
    }
    return GK3D_VERB_NONE;
}

int gk3d_check_condition(gk3d_world *world,
    const gk3d_condition *condition, gk3d_condition_result *result)
{
    int id;

    if (result != 0) {
        result->truth = 0;
        result->instance_id = GK3D_ID_NONE;
    }
    if (world == 0 || condition == 0) {
        return 0;
    }
    id = GK3D_ID_NONE;
    if (condition->verb == GK3D_VERB_PLACE_MEETING) {
        id = gk3d_instance_place(world, condition->self_id,
            condition->x, condition->y, condition->z, condition->target);
    } else if (condition->verb == GK3D_VERB_PLACE_FREE) {
        if (gk3d_place_free(world, condition->self_id,
            condition->x, condition->y, condition->z)) {
            if (result != 0) {
                result->truth = 1;
            }
            return 1;
        }
        return 0;
    } else if (condition->verb == GK3D_VERB_INSTANCE_PLACE) {
        id = gk3d_instance_place(world, condition->self_id,
            condition->x, condition->y, condition->z, condition->target);
    } else if (condition->verb == GK3D_VERB_IS_ON_FLOOR) {
        id = gk3d_floor_instance(world, condition->self_id, GK3D_TARGET_FLOOR);
    } else if (condition->verb == GK3D_VERB_IS_ON_WALL) {
        id = gk3d_wall_instance(world, condition->self_id, GK3D_TARGET_SOLID);
    } else if (condition->verb == GK3D_VERB_IS_UNDER_CEILING) {
        id = gk3d_ceiling_instance(world, condition->self_id, GK3D_TARGET_SOLID);
    } else if (condition->verb == GK3D_VERB_OVERLAP_OFFSET) {
        id = gk3d_instance_at_offset(world, condition->self_id,
            condition->dx, condition->dy, condition->dz, condition->target);
    } else {
        return 0;
    }
    if (result != 0) {
        result->instance_id = id;
        result->truth = id != GK3D_ID_NONE;
    }
    return id != GK3D_ID_NONE;
}

int gk3d_check_verb(gk3d_world *world, const char *verb,
    int self_id, int target,
    gk3d_fix x, gk3d_fix y, gk3d_fix z,
    gk3d_fix dx, gk3d_fix dy, gk3d_fix dz,
    gk3d_condition_result *result)
{
    gk3d_condition condition;
    gk3d_memzero(&condition, (unsigned long)sizeof(condition));
    condition.verb = gk3d_verb_from_name(verb);
    condition.self_id = self_id;
    condition.target = target;
    condition.x = x;
    condition.y = y;
    condition.z = z;
    condition.dx = dx;
    condition.dy = dy;
    condition.dz = dz;
    return gk3d_check_condition(world, &condition, result);
}

static int gk3d_candidate_hit(gk3d_world *world, int self_id,
    gk3d_aabb box, int target)
{
    gk3d_filter filter;
    filter = gk3d_filter_make(target);
    return gk3d_instance_place_box_filter(world, self_id, box, &filter);
}

static gk3d_fix gk3d_binary_free_amount(gk3d_world *world, int self_id,
    gk3d_aabb base, int axis, gk3d_fix signed_amount, int target,
    int *hit_id)
{
    gk3d_fix low;
    gk3d_fix high;
    gk3d_fix mid;
    gk3d_fix sign;
    gk3d_fix dx;
    gk3d_fix dy;
    gk3d_fix dz;
    gk3d_aabb candidate;
    int i;
    int id;

    sign = signed_amount < 0 ? -1 : 1;
    low = 0;
    high = GK3D_ABS(signed_amount);
    id = GK3D_ID_NONE;
    for (i = 0; i < GK3D_SOLVER_BINARY_STEPS && high - low > 1; ++i) {
        mid = low + (high - low) / 2;
        gk3d_axis_delta(axis, mid * sign, &dx, &dy, &dz);
        candidate = gk3d_aabb_offset(base, dx, dy, dz);
        id = gk3d_candidate_hit(world, self_id, candidate, target);
        if (id == GK3D_ID_NONE) {
            low = mid;
        } else {
            high = mid;
            if (hit_id != 0) {
                *hit_id = id;
            }
        }
    }
    return low * sign;
}

static gk3d_fix gk3d_solve_axis(gk3d_world *world, int self_id,
    gk3d_aabb *base, int axis, gk3d_fix wanted, int target,
    int *hit_id, int *steps_used)
{
    gk3d_fix remaining;
    gk3d_fix moved;
    gk3d_fix amount;
    gk3d_fix step;
    gk3d_fix sign;
    gk3d_fix dx;
    gk3d_fix dy;
    gk3d_fix dz;
    gk3d_aabb candidate;
    int id;
    int steps;

    if (wanted == 0) {
        return 0;
    }
    sign = wanted < 0 ? -1 : 1;
    remaining = GK3D_ABS(wanted);
    moved = 0;
    step = world->solver_step;
    if (step <= 0) {
        step = 1;
    }
    steps = 0;
    while (remaining > 0 && steps < GK3D_SOLVER_MAX_STEPS) {
        amount = remaining > step ? step : remaining;
        gk3d_axis_delta(axis, amount * sign, &dx, &dy, &dz);
        candidate = gk3d_aabb_offset(*base, dx, dy, dz);
        id = gk3d_candidate_hit(world, self_id, candidate, target);
        ++steps;
        if (id != GK3D_ID_NONE) {
            amount = gk3d_binary_free_amount(world, self_id, *base,
                axis, amount * sign, target, hit_id);
            gk3d_axis_delta(axis, amount, &dx, &dy, &dz);
            *base = gk3d_aabb_offset(*base, dx, dy, dz);
            moved += GK3D_ABS(amount);
            if (hit_id != 0 && *hit_id == GK3D_ID_NONE) {
                *hit_id = id;
            }
            break;
        }
        *base = candidate;
        moved += amount;
        remaining -= amount;
    }
    if (steps_used != 0) {
        *steps_used += steps;
    }
    return moved * sign;
}

int gk3d_solve_move(gk3d_world *world, int self_id,
    gk3d_fix dx, gk3d_fix dy, gk3d_fix dz,
    int target, gk3d_solve_result *result)
{
    const gk3d_obj *self;
    gk3d_aabb base;

    if (result == 0) {
        return 0;
    }
    gk3d_memzero(result, (unsigned long)sizeof(*result));
    result->hit_x_id = GK3D_ID_NONE;
    result->hit_y_id = GK3D_ID_NONE;
    result->hit_z_id = GK3D_ID_NONE;
    self = gk3d_world_get_const(world, self_id);
    if (world == 0 || self == 0) {
        return 0;
    }
    base = self->box;
    result->allowed_dx = gk3d_solve_axis(world, self_id, &base,
        GK3D_AXIS_X, dx, target, &result->hit_x_id, &result->steps_used);
    result->allowed_dy = gk3d_solve_axis(world, self_id, &base,
        GK3D_AXIS_Y, dy, target, &result->hit_y_id, &result->steps_used);
    result->allowed_dz = gk3d_solve_axis(world, self_id, &base,
        GK3D_AXIS_Z, dz, target, &result->hit_z_id, &result->steps_used);
    result->collided =
        result->hit_x_id != GK3D_ID_NONE ||
        result->hit_y_id != GK3D_ID_NONE ||
        result->hit_z_id != GK3D_ID_NONE;
    if (result->hit_x_id != GK3D_ID_NONE) {
        result->nx = dx > 0 ? -GK3D_ONE : GK3D_ONE;
    }
    if (result->hit_y_id != GK3D_ID_NONE) {
        result->ny = dy > 0 ? -GK3D_ONE : GK3D_ONE;
    }
    if (result->hit_z_id != GK3D_ID_NONE) {
        result->nz = dz > 0 ? -GK3D_ONE : GK3D_ONE;
    }
    return 1;
}

int gk3d_apply_solved_move(gk3d_world *world, int self_id,
    const gk3d_solve_result *result)
{
    gk3d_obj *obj;
    if (world == 0 || result == 0) {
        return 0;
    }
    obj = gk3d_world_get(world, self_id);
    if (obj == 0) {
        return 0;
    }
    obj->box.x += result->allowed_dx;
    obj->box.y += result->allowed_dy;
    obj->box.z += result->allowed_dz;
    return 1;
}
