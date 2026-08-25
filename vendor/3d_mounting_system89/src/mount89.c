#include "mount89.h"
#include <string.h>

static mount89_fx mount89_mul_fx_rot(mount89_fx a, mount89_rot b)
{
    mount89_fx q;
    mount89_fx rem;
    q = a / (mount89_fx)MOUNT89_ROT_ONE;
    rem = a % (mount89_fx)MOUNT89_ROT_ONE;
    return q * (mount89_fx)b +
           (rem * (mount89_fx)b) / (mount89_fx)MOUNT89_ROT_ONE;
}

static mount89_rot mount89_mul_rot_sum(mount89_rot a0, mount89_rot a1,
                                        mount89_rot a2, mount89_rot b0,
                                        mount89_rot b1, mount89_rot b2)
{
    long sum;
    sum = (long)a0 * (long)b0;
    sum += (long)a1 * (long)b1;
    sum += (long)a2 * (long)b2;
    sum /= (long)MOUNT89_ROT_ONE;
    if (sum > (long)SHRT_MAX) {
        sum = (long)SHRT_MAX;
    }
    if (sum < (long)SHRT_MIN) {
        sum = (long)SHRT_MIN;
    }
    return (mount89_rot)sum;
}

static int mount89_find_point_slot(const mount89_context *ctx,
                                   int host_id, int point_id)
{
    int i;
    if (ctx == 0) {
        return -1;
    }
    for (i = 0; i < MOUNT89_MAX_POINTS; ++i) {
        if (ctx->points[i].used &&
            ctx->points[i].host_id == host_id &&
            ctx->points[i].point_id == point_id) {
            return i;
        }
    }
    return -1;
}

static int mount89_find_free_point_slot(const mount89_context *ctx)
{
    int i;
    for (i = 0; i < MOUNT89_MAX_POINTS; ++i) {
        if (!ctx->points[i].used) {
            return i;
        }
    }
    return -1;
}

static int mount89_find_link_by_guest(const mount89_context *ctx, int guest_id)
{
    int i;
    if (ctx == 0) {
        return -1;
    }
    for (i = 0; i < MOUNT89_MAX_LINKS; ++i) {
        if (ctx->links[i].used && ctx->links[i].guest_id == guest_id) {
            return i;
        }
    }
    return -1;
}

static int mount89_find_link_by_id(const mount89_context *ctx, int link_id)
{
    int i;
    if (ctx == 0) {
        return -1;
    }
    for (i = 0; i < MOUNT89_MAX_LINKS; ++i) {
        if (ctx->links[i].used && ctx->links[i].link_id == link_id) {
            return i;
        }
    }
    return -1;
}

static int mount89_find_link_by_point_slot(const mount89_context *ctx,
                                           int point_slot)
{
    int i;
    for (i = 0; i < MOUNT89_MAX_LINKS; ++i) {
        if (ctx->links[i].used && ctx->links[i].point_slot == point_slot) {
            return i;
        }
    }
    return -1;
}

static int mount89_find_free_link_slot(const mount89_context *ctx)
{
    int i;
    for (i = 0; i < MOUNT89_MAX_LINKS; ++i) {
        if (!ctx->links[i].used) {
            return i;
        }
    }
    return -1;
}

static int mount89_allocate_link_id(mount89_context *ctx)
{
    int candidate;
    int attempts;
    candidate = ctx->next_link_id;
    if (candidate <= 0) {
        candidate = 1;
    }
    attempts = 0;
    while (attempts <= MOUNT89_MAX_LINKS) {
        if (mount89_find_link_by_id(ctx, candidate) < 0) {
            if (candidate == INT_MAX) {
                ctx->next_link_id = 1;
            } else {
                ctx->next_link_id = candidate + 1;
            }
            return candidate;
        }
        if (candidate == INT_MAX) {
            candidate = 1;
        } else {
            ++candidate;
        }
        ++attempts;
    }
    return -1;
}

static int mount89_resolve_point_local(mount89_context *ctx, int point_slot,
                                       mount89_transform *out_local)
{
    mount89_point *p;
    if (ctx == 0 || out_local == 0 ||
        point_slot < 0 || point_slot >= MOUNT89_MAX_POINTS) {
        return MOUNT89_ERR_ARGUMENT;
    }
    p = &ctx->points[point_slot];
    if (!p->used) {
        return MOUNT89_ERR_NO_POINT;
    }
    if (p->source_kind == MOUNT89_POINT_STATIC) {
        mount89_transform_copy(out_local, &p->local);
        return MOUNT89_OK;
    }
    if (p->source_kind == MOUNT89_POINT_DYNAMIC) {
        if (ctx->resolve_point == 0) {
            return MOUNT89_ERR_DYNAMIC_POINT;
        }
        if (!ctx->resolve_point(ctx->point_user, p->host_id, p->source_id,
                                out_local)) {
            return MOUNT89_ERR_DYNAMIC_POINT;
        }
        return MOUNT89_OK;
    }
    return MOUNT89_ERR_DYNAMIC_POINT;
}

static int mount89_would_cycle(const mount89_context *ctx,
                               int host_id, int guest_id)
{
    int current;
    int link_slot;
    int guard;
    current = host_id;
    guard = 0;
    while (guard <= MOUNT89_MAX_LINKS) {
        if (current == guest_id) {
            return 1;
        }
        link_slot = mount89_find_link_by_guest(ctx, current);
        if (link_slot < 0) {
            return 0;
        }
        current = ctx->links[link_slot].host_id;
        ++guard;
    }
    return 1;
}

static int mount89_link_depth(const mount89_context *ctx, int link_slot)
{
    int depth;
    int host;
    int parent_slot;
    int guard;
    depth = 0;
    host = ctx->links[link_slot].host_id;
    guard = 0;
    while (guard <= MOUNT89_MAX_LINKS) {
        parent_slot = mount89_find_link_by_guest(ctx, host);
        if (parent_slot < 0) {
            return depth;
        }
        ++depth;
        host = ctx->links[parent_slot].host_id;
        ++guard;
    }
    return -1;
}

static int mount89_apply_link(mount89_context *ctx, int link_slot)
{
    mount89_link *link;
    mount89_transform host_world;
    mount89_transform point_local;
    mount89_transform point_world;
    mount89_transform desired;
    mount89_transform current;
    int rc;
    int i;

    link = &ctx->links[link_slot];
    if (!link->used) {
        return MOUNT89_OK;
    }
    if (ctx->get_world == 0 || ctx->set_world == 0) {
        return MOUNT89_ERR_NO_PROVIDER;
    }
    if (!ctx->get_world(ctx->transform_user, link->host_id, &host_world)) {
        return MOUNT89_ERR_PROVIDER;
    }
    rc = mount89_resolve_point_local(ctx, link->point_slot, &point_local);
    if (rc != MOUNT89_OK) {
        return rc;
    }
    mount89_transform_compose(&host_world, &point_local, &point_world);
    mount89_transform_compose(&point_world, &link->guest_offset, &desired);

    if ((link->inherit_flags & MOUNT89_INHERIT_ALL) != MOUNT89_INHERIT_ALL) {
        if (!ctx->get_world(ctx->transform_user, link->guest_id, &current)) {
            return MOUNT89_ERR_PROVIDER;
        }
        if ((link->inherit_flags & MOUNT89_INHERIT_POSITION) == 0U) {
            desired.p[0] = current.p[0];
            desired.p[1] = current.p[1];
            desired.p[2] = current.p[2];
        }
        if ((link->inherit_flags & MOUNT89_INHERIT_ROTATION) == 0U) {
            for (i = 0; i < 9; ++i) {
                desired.r[i] = current.r[i];
            }
        }
    }

    if (!ctx->set_world(ctx->transform_user, link->guest_id, &desired)) {
        return MOUNT89_ERR_PROVIDER;
    }
    return MOUNT89_OK;
}

void mount89_transform_identity(mount89_transform *t)
{
    int i;
    if (t == 0) {
        return;
    }
    t->p[0] = 0L;
    t->p[1] = 0L;
    t->p[2] = 0L;
    for (i = 0; i < 9; ++i) {
        t->r[i] = 0;
    }
    t->r[0] = (mount89_rot)MOUNT89_ROT_ONE;
    t->r[4] = (mount89_rot)MOUNT89_ROT_ONE;
    t->r[8] = (mount89_rot)MOUNT89_ROT_ONE;
}

void mount89_transform_copy(mount89_transform *dst,
                            const mount89_transform *src)
{
    int i;
    if (dst == 0 || src == 0) {
        return;
    }
    for (i = 0; i < 3; ++i) {
        dst->p[i] = src->p[i];
    }
    for (i = 0; i < 9; ++i) {
        dst->r[i] = src->r[i];
    }
}

void mount89_transform_compose(const mount89_transform *parent,
                               const mount89_transform *local,
                               mount89_transform *out_world)
{
    mount89_transform tmp;
    int row;
    int col;
    int base;

    if (parent == 0 || local == 0 || out_world == 0) {
        return;
    }

    tmp.p[0] = parent->p[0] +
               mount89_mul_fx_rot(local->p[0], parent->r[0]) +
               mount89_mul_fx_rot(local->p[1], parent->r[1]) +
               mount89_mul_fx_rot(local->p[2], parent->r[2]);
    tmp.p[1] = parent->p[1] +
               mount89_mul_fx_rot(local->p[0], parent->r[3]) +
               mount89_mul_fx_rot(local->p[1], parent->r[4]) +
               mount89_mul_fx_rot(local->p[2], parent->r[5]);
    tmp.p[2] = parent->p[2] +
               mount89_mul_fx_rot(local->p[0], parent->r[6]) +
               mount89_mul_fx_rot(local->p[1], parent->r[7]) +
               mount89_mul_fx_rot(local->p[2], parent->r[8]);

    for (row = 0; row < 3; ++row) {
        for (col = 0; col < 3; ++col) {
            base = row * 3;
            tmp.r[base + col] = mount89_mul_rot_sum(
                parent->r[base + 0], parent->r[base + 1],
                parent->r[base + 2], local->r[col + 0],
                local->r[col + 3], local->r[col + 6]);
        }
    }
    mount89_transform_copy(out_world, &tmp);
}

void mount89_transform_inverse_rigid(const mount89_transform *t,
                                     mount89_transform *out_inverse)
{
    mount89_transform tmp;
    mount89_fx x;
    mount89_fx y;
    mount89_fx z;
    if (t == 0 || out_inverse == 0) {
        return;
    }

    tmp.r[0] = t->r[0];
    tmp.r[1] = t->r[3];
    tmp.r[2] = t->r[6];
    tmp.r[3] = t->r[1];
    tmp.r[4] = t->r[4];
    tmp.r[5] = t->r[7];
    tmp.r[6] = t->r[2];
    tmp.r[7] = t->r[5];
    tmp.r[8] = t->r[8];

    x = mount89_mul_fx_rot(t->p[0], tmp.r[0]) +
        mount89_mul_fx_rot(t->p[1], tmp.r[1]) +
        mount89_mul_fx_rot(t->p[2], tmp.r[2]);
    y = mount89_mul_fx_rot(t->p[0], tmp.r[3]) +
        mount89_mul_fx_rot(t->p[1], tmp.r[4]) +
        mount89_mul_fx_rot(t->p[2], tmp.r[5]);
    z = mount89_mul_fx_rot(t->p[0], tmp.r[6]) +
        mount89_mul_fx_rot(t->p[1], tmp.r[7]) +
        mount89_mul_fx_rot(t->p[2], tmp.r[8]);
    tmp.p[0] = -x;
    tmp.p[1] = -y;
    tmp.p[2] = -z;
    mount89_transform_copy(out_inverse, &tmp);
}

mount89_fx mount89_fx_from_int(long value)
{
    return value * MOUNT89_FX_ONE;
}

long mount89_fx_to_int(mount89_fx value)
{
    return value / MOUNT89_FX_ONE;
}

void mount89_init(mount89_context *ctx)
{
    if (ctx == 0) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->next_link_id = 1;
}

void mount89_set_transform_provider(mount89_context *ctx,
                                    mount89_get_world_fn get_fn,
                                    mount89_set_world_fn set_fn,
                                    void *user)
{
    if (ctx == 0) {
        return;
    }
    ctx->get_world = get_fn;
    ctx->set_world = set_fn;
    ctx->transform_user = user;
}

void mount89_set_point_provider(mount89_context *ctx,
                                mount89_resolve_point_fn resolve_fn,
                                void *user)
{
    if (ctx == 0) {
        return;
    }
    ctx->resolve_point = resolve_fn;
    ctx->point_user = user;
}

void mount89_set_event_provider(mount89_context *ctx,
                                mount89_event_fn event_fn,
                                void *user)
{
    if (ctx == 0) {
        return;
    }
    ctx->event_fn = event_fn;
    ctx->event_user = user;
}

int mount89_add_static_point(mount89_context *ctx,
                             int host_id, int point_id,
                             unsigned long accept_mask,
                             const mount89_transform *host_local)
{
    int slot;
    if (ctx == 0 || host_local == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    if (mount89_find_point_slot(ctx, host_id, point_id) >= 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    slot = mount89_find_free_point_slot(ctx);
    if (slot < 0) {
        return MOUNT89_ERR_CAPACITY;
    }
    ctx->points[slot].used = 1;
    ctx->points[slot].host_id = host_id;
    ctx->points[slot].point_id = point_id;
    ctx->points[slot].source_kind = MOUNT89_POINT_STATIC;
    ctx->points[slot].source_id = 0;
    ctx->points[slot].accept_mask = accept_mask;
    mount89_transform_copy(&ctx->points[slot].local, host_local);
    return MOUNT89_OK;
}

int mount89_add_dynamic_point(mount89_context *ctx,
                              int host_id, int point_id,
                              int source_id,
                              unsigned long accept_mask)
{
    int slot;
    if (ctx == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    if (mount89_find_point_slot(ctx, host_id, point_id) >= 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    slot = mount89_find_free_point_slot(ctx);
    if (slot < 0) {
        return MOUNT89_ERR_CAPACITY;
    }
    ctx->points[slot].used = 1;
    ctx->points[slot].host_id = host_id;
    ctx->points[slot].point_id = point_id;
    ctx->points[slot].source_kind = MOUNT89_POINT_DYNAMIC;
    ctx->points[slot].source_id = source_id;
    ctx->points[slot].accept_mask = accept_mask;
    mount89_transform_identity(&ctx->points[slot].local);
    return MOUNT89_OK;
}

int mount89_remove_point(mount89_context *ctx, int host_id, int point_id)
{
    int point_slot;
    if (ctx == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    point_slot = mount89_find_point_slot(ctx, host_id, point_id);
    if (point_slot < 0) {
        return MOUNT89_ERR_NO_POINT;
    }
    if (mount89_find_link_by_point_slot(ctx, point_slot) >= 0) {
        return MOUNT89_ERR_POINT_BUSY;
    }
    memset(&ctx->points[point_slot], 0, sizeof(ctx->points[point_slot]));
    return MOUNT89_OK;
}

int mount89_set_point_local(mount89_context *ctx, int host_id, int point_id,
                            const mount89_transform *host_local)
{
    int point_slot;
    if (ctx == 0 || host_local == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    point_slot = mount89_find_point_slot(ctx, host_id, point_id);
    if (point_slot < 0) {
        return MOUNT89_ERR_NO_POINT;
    }
    if (ctx->points[point_slot].source_kind != MOUNT89_POINT_STATIC) {
        return MOUNT89_ERR_DYNAMIC_POINT;
    }
    mount89_transform_copy(&ctx->points[point_slot].local, host_local);
    return MOUNT89_OK;
}

int mount89_can_mount(const mount89_context *ctx,
                      int host_id, int point_id, int guest_id,
                      unsigned long guest_mask)
{
    int point_slot;
    unsigned long accept_mask;
    if (ctx == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    if (host_id == guest_id) {
        return MOUNT89_ERR_SAME_OBJECT;
    }
    point_slot = mount89_find_point_slot(ctx, host_id, point_id);
    if (point_slot < 0) {
        return MOUNT89_ERR_NO_POINT;
    }
    if (mount89_find_link_by_point_slot(ctx, point_slot) >= 0) {
        return MOUNT89_ERR_POINT_BUSY;
    }
    if (mount89_find_link_by_guest(ctx, guest_id) >= 0) {
        return MOUNT89_ERR_GUEST_BUSY;
    }
    accept_mask = ctx->points[point_slot].accept_mask;
    if (accept_mask != MOUNT89_ANY_MASK &&
        (accept_mask & guest_mask) == 0UL) {
        return MOUNT89_ERR_INCOMPATIBLE;
    }
    if (mount89_would_cycle(ctx, host_id, guest_id)) {
        return MOUNT89_ERR_CYCLE;
    }
    return MOUNT89_OK;
}

int mount89_get_point_world(mount89_context *ctx,
                            int host_id, int point_id,
                            mount89_transform *out_world)
{
    int point_slot;
    int rc;
    mount89_transform host_world;
    mount89_transform point_local;
    if (ctx == 0 || out_world == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    if (ctx->get_world == 0) {
        return MOUNT89_ERR_NO_PROVIDER;
    }
    point_slot = mount89_find_point_slot(ctx, host_id, point_id);
    if (point_slot < 0) {
        return MOUNT89_ERR_NO_POINT;
    }
    if (!ctx->get_world(ctx->transform_user, host_id, &host_world)) {
        return MOUNT89_ERR_PROVIDER;
    }
    rc = mount89_resolve_point_local(ctx, point_slot, &point_local);
    if (rc != MOUNT89_OK) {
        return rc;
    }
    mount89_transform_compose(&host_world, &point_local, out_world);
    return MOUNT89_OK;
}

int mount89_mount(mount89_context *ctx,
                  int host_id, int point_id, int guest_id,
                  unsigned long guest_mask,
                  int mount_mode,
                  unsigned int inherit_flags,
                  const mount89_transform *explicit_guest_offset,
                  int *out_link_id)
{
    int rc;
    int point_slot;
    int link_slot;
    mount89_transform point_world;
    mount89_transform point_inverse;
    mount89_transform guest_world;
    mount89_link *link;

    if (ctx == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    if (ctx->get_world == 0 || ctx->set_world == 0) {
        return MOUNT89_ERR_NO_PROVIDER;
    }
    if (mount_mode != MOUNT89_MOUNT_SNAP &&
        mount_mode != MOUNT89_MOUNT_KEEP_WORLD &&
        mount_mode != MOUNT89_MOUNT_OFFSET) {
        return MOUNT89_ERR_ARGUMENT;
    }
    if (mount_mode == MOUNT89_MOUNT_OFFSET && explicit_guest_offset == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }

    rc = mount89_can_mount(ctx, host_id, point_id, guest_id, guest_mask);
    if (rc != MOUNT89_OK) {
        return rc;
    }
    point_slot = mount89_find_point_slot(ctx, host_id, point_id);
    link_slot = mount89_find_free_link_slot(ctx);
    if (link_slot < 0) {
        return MOUNT89_ERR_CAPACITY;
    }
    if (!ctx->get_world(ctx->transform_user, guest_id, &guest_world)) {
        return MOUNT89_ERR_PROVIDER;
    }
    rc = mount89_get_point_world(ctx, host_id, point_id, &point_world);
    if (rc != MOUNT89_OK) {
        return rc;
    }

    link = &ctx->links[link_slot];
    memset(link, 0, sizeof(*link));
    link->used = 1;
    link->link_id = mount89_allocate_link_id(ctx);
    if (link->link_id < 0) {
        memset(link, 0, sizeof(*link));
        return MOUNT89_ERR_CAPACITY;
    }
    link->host_id = host_id;
    link->guest_id = guest_id;
    link->point_slot = point_slot;
    link->inherit_flags = inherit_flags & MOUNT89_INHERIT_ALL;
    mount89_transform_copy(&link->pre_mount_world, &guest_world);

    if (mount_mode == MOUNT89_MOUNT_SNAP) {
        mount89_transform_identity(&link->guest_offset);
    } else if (mount_mode == MOUNT89_MOUNT_KEEP_WORLD) {
        mount89_transform_inverse_rigid(&point_world, &point_inverse);
        mount89_transform_compose(&point_inverse, &guest_world,
                                  &link->guest_offset);
    } else {
        mount89_transform_copy(&link->guest_offset, explicit_guest_offset);
    }

    rc = mount89_apply_link(ctx, link_slot);
    if (rc != MOUNT89_OK) {
        memset(link, 0, sizeof(*link));
        return rc;
    }

    if (out_link_id != 0) {
        *out_link_id = link->link_id;
    }
    if (ctx->event_fn != 0) {
        ctx->event_fn(ctx->event_user, MOUNT89_EVENT_MOUNTED,
                      host_id, point_id, guest_id);
    }
    return MOUNT89_OK;
}

static int mount89_unmount_slot(mount89_context *ctx, int link_slot,
                                int unmount_mode)
{
    mount89_link old_link;
    mount89_point *point;
    if (ctx == 0 || link_slot < 0 || link_slot >= MOUNT89_MAX_LINKS) {
        return MOUNT89_ERR_ARGUMENT;
    }
    if (!ctx->links[link_slot].used) {
        return MOUNT89_ERR_LINK_NOT_FOUND;
    }
    if (unmount_mode != MOUNT89_UNMOUNT_KEEP_WORLD &&
        unmount_mode != MOUNT89_UNMOUNT_RESTORE_WORLD) {
        return MOUNT89_ERR_ARGUMENT;
    }
    mount89_transform_copy(&old_link.pre_mount_world,
                           &ctx->links[link_slot].pre_mount_world);
    old_link.used = ctx->links[link_slot].used;
    old_link.link_id = ctx->links[link_slot].link_id;
    old_link.host_id = ctx->links[link_slot].host_id;
    old_link.guest_id = ctx->links[link_slot].guest_id;
    old_link.point_slot = ctx->links[link_slot].point_slot;
    old_link.inherit_flags = ctx->links[link_slot].inherit_flags;
    mount89_transform_copy(&old_link.guest_offset,
                           &ctx->links[link_slot].guest_offset);

    if (unmount_mode == MOUNT89_UNMOUNT_RESTORE_WORLD) {
        if (ctx->set_world == 0 ||
            !ctx->set_world(ctx->transform_user, old_link.guest_id,
                            &old_link.pre_mount_world)) {
            return MOUNT89_ERR_PROVIDER;
        }
    }

    point = &ctx->points[old_link.point_slot];
    memset(&ctx->links[link_slot], 0, sizeof(ctx->links[link_slot]));
    if (ctx->event_fn != 0) {
        ctx->event_fn(ctx->event_user, MOUNT89_EVENT_UNMOUNTED,
                      old_link.host_id, point->point_id, old_link.guest_id);
    }
    return MOUNT89_OK;
}

int mount89_unmount(mount89_context *ctx, int guest_id, int unmount_mode)
{
    int link_slot;
    if (ctx == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    link_slot = mount89_find_link_by_guest(ctx, guest_id);
    if (link_slot < 0) {
        return MOUNT89_ERR_LINK_NOT_FOUND;
    }
    return mount89_unmount_slot(ctx, link_slot, unmount_mode);
}

int mount89_unmount_link(mount89_context *ctx, int link_id, int unmount_mode)
{
    int link_slot;
    if (ctx == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    link_slot = mount89_find_link_by_id(ctx, link_id);
    if (link_slot < 0) {
        return MOUNT89_ERR_LINK_NOT_FOUND;
    }
    return mount89_unmount_slot(ctx, link_slot, unmount_mode);
}

int mount89_is_mounted(const mount89_context *ctx, int guest_id)
{
    return mount89_find_link_by_guest(ctx, guest_id) >= 0;
}

int mount89_get_host(const mount89_context *ctx, int guest_id,
                     int *out_host_id, int *out_point_id)
{
    int link_slot;
    int point_slot;
    if (ctx == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    link_slot = mount89_find_link_by_guest(ctx, guest_id);
    if (link_slot < 0) {
        return MOUNT89_ERR_LINK_NOT_FOUND;
    }
    point_slot = ctx->links[link_slot].point_slot;
    if (out_host_id != 0) {
        *out_host_id = ctx->links[link_slot].host_id;
    }
    if (out_point_id != 0) {
        *out_point_id = ctx->points[point_slot].point_id;
    }
    return MOUNT89_OK;
}

int mount89_point_is_occupied(const mount89_context *ctx,
                              int host_id, int point_id,
                              int *out_guest_id)
{
    int point_slot;
    int link_slot;
    if (ctx == 0) {
        return 0;
    }
    point_slot = mount89_find_point_slot(ctx, host_id, point_id);
    if (point_slot < 0) {
        return 0;
    }
    link_slot = mount89_find_link_by_point_slot(ctx, point_slot);
    if (link_slot < 0) {
        return 0;
    }
    if (out_guest_id != 0) {
        *out_guest_id = ctx->links[link_slot].guest_id;
    }
    return 1;
}

int mount89_update(mount89_context *ctx)
{
    int depth;
    int i;
    int link_depth;
    int rc;
    if (ctx == 0) {
        return MOUNT89_ERR_ARGUMENT;
    }
    if (ctx->get_world == 0 || ctx->set_world == 0) {
        return MOUNT89_ERR_NO_PROVIDER;
    }
    for (depth = 0; depth < MOUNT89_MAX_LINKS; ++depth) {
        for (i = 0; i < MOUNT89_MAX_LINKS; ++i) {
            if (ctx->links[i].used) {
                link_depth = mount89_link_depth(ctx, i);
                if (link_depth < 0) {
                    return MOUNT89_ERR_CYCLE;
                }
                if (link_depth == depth) {
                    rc = mount89_apply_link(ctx, i);
                    if (rc != MOUNT89_OK) {
                        return rc;
                    }
                }
            }
        }
    }
    return MOUNT89_OK;
}

int mount89_link_count(const mount89_context *ctx)
{
    int i;
    int count;
    if (ctx == 0) {
        return 0;
    }
    count = 0;
    for (i = 0; i < MOUNT89_MAX_LINKS; ++i) {
        if (ctx->links[i].used) {
            ++count;
        }
    }
    return count;
}

int mount89_point_count(const mount89_context *ctx)
{
    int i;
    int count;
    if (ctx == 0) {
        return 0;
    }
    count = 0;
    for (i = 0; i < MOUNT89_MAX_POINTS; ++i) {
        if (ctx->points[i].used) {
            ++count;
        }
    }
    return count;
}
