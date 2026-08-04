#include "gattach89.h"

static int gatt89_slot_socket(GAtt89_World *w)
{
    int i;
    if (w == 0) return GATTACH89_ERR_NULL;
    for (i = 0; i < GATTACH89_MAX_SOCKETS; ++i) {
        if (!w->sockets[i].used) return i;
    }
    return GATTACH89_ERR_FULL;
}

static int gatt89_slot_attachment(GAtt89_World *w)
{
    int i;
    if (w == 0) return GATTACH89_ERR_NULL;
    for (i = 0; i < GATTACH89_MAX_ATTACHMENTS; ++i) {
        if (!w->attachments[i].used) return i;
    }
    return GATTACH89_ERR_FULL;
}

static int gatt89_slot_part(GAtt89_World *w)
{
    int i;
    if (w == 0) return GATTACH89_ERR_NULL;
    for (i = 0; i < GATTACH89_MAX_PARTS; ++i) {
        if (!w->parts[i].used) return i;
    }
    return GATTACH89_ERR_FULL;
}

static void gatt89_emit(GAtt89_Output *out, const GAtt89_Callbacks *cb, const GAtt89_Event *ev)
{
    if (out != 0) {
        if (out->event_count < GATTACH89_MAX_EVENTS) {
            out->events[out->event_count] = *ev;
            out->event_count += 1;
        } else {
            out->dropped_event_count += 1;
        }
    }
    if (cb != 0 && cb->on_event != 0) {
        cb->on_event(cb->user, ev);
    }
}

static int gatt89_resolve_socket(GAtt89_World *w, const GAtt89_Callbacks *cb, int socket_id, GAtt89_Xform *out_world, GAtt89_Output *out)
{
    GAtt89_Xform base;
    GAtt89_Xform resolved;
    GAtt89_Event ev;
    GAtt89_Socket *s;
    int ok;

    if (w == 0 || cb == 0 || out_world == 0) return GATTACH89_ERR_NULL;
    if (socket_id < 0 || socket_id >= GATTACH89_MAX_SOCKETS) return GATTACH89_ERR_BAD_ARG;
    s = &w->sockets[socket_id];
    if (!s->used) return GATTACH89_ERR_NOT_FOUND;

    base = gatt89_xform_identity();
    ok = GATTACH89_FALSE;

    if (s->kind == GATTACH89_SOCKET_BONE) {
        if (cb->get_bone_xform != 0) {
            ok = cb->get_bone_xform(cb->user, s->owner_entity_id, s->bone_index, &base);
        }
        if (!ok && cb->get_entity_xform != 0) {
            ok = cb->get_entity_xform(cb->user, s->owner_entity_id, &base);
        }
    } else if (s->kind == GATTACH89_SOCKET_SOCKET) {
        if (s->parent_socket_id >= 0 && s->parent_socket_id < GATTACH89_MAX_SOCKETS && s->parent_socket_id != socket_id) {
            ok = (gatt89_resolve_socket(w, cb, s->parent_socket_id, &base, out) == GATTACH89_OK);
        }
    } else {
        if (cb->get_entity_xform != 0) {
            ok = cb->get_entity_xform(cb->user, s->owner_entity_id, &base);
        }
    }

    if (!ok) {
        ev.kind = GATTACH89_EVENT_MISSING_BASE;
        ev.owner_entity_id = s->owner_entity_id;
        ev.child_entity_id = GATTACH89_INVALID_ID;
        ev.socket_id = socket_id;
        ev.attachment_id = GATTACH89_INVALID_ID;
        ev.part_id = GATTACH89_INVALID_ID;
        ev.xform = gatt89_xform_identity();
        gatt89_emit(out, cb, &ev);
        return GATTACH89_ERR_NOT_FOUND;
    }

    resolved = gatt89_xform_compose(&base, &s->local);
    s->world = resolved;
    *out_world = resolved;

    ev.kind = GATTACH89_EVENT_SOCKET_RESOLVED;
    ev.owner_entity_id = s->owner_entity_id;
    ev.child_entity_id = GATTACH89_INVALID_ID;
    ev.socket_id = socket_id;
    ev.attachment_id = GATTACH89_INVALID_ID;
    ev.part_id = GATTACH89_INVALID_ID;
    ev.xform = resolved;
    gatt89_emit(out, cb, &ev);

    return GATTACH89_OK;
}

gatt_fix gatt89_from_int(int v)
{
    return (gatt_fix)(v << GATTACH89_FIX_SHIFT);
}

int gatt89_to_int(gatt_fix v)
{
    if (v >= 0) return (int)((v + GATTACH89_FIX_HALF) >> GATTACH89_FIX_SHIFT);
    return (int)(-(((-v) + GATTACH89_FIX_HALF) >> GATTACH89_FIX_SHIFT));
}

gatt_fix gatt89_mul(gatt_fix a, gatt_fix b)
{
    /* C89 32-bit friendly fixed multiply. Precision is intentionally traded for portability. */
    return (gatt_fix)((a >> 6) * (b >> 6));
}

gatt_fix gatt89_div(gatt_fix a, gatt_fix b)
{
    if (b == 0) return 0;
    return (gatt_fix)((a << 6) / (b >> 6));
}

GAtt89_Vec3 gatt89_vec3(gatt_fix x, gatt_fix y, gatt_fix z)
{
    GAtt89_Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

GAtt89_Quat gatt89_quat_identity(void)
{
    GAtt89_Quat q;
    q.x = 0;
    q.y = 0;
    q.z = 0;
    q.w = GATTACH89_FIX_ONE;
    return q;
}

GAtt89_Xform gatt89_xform_identity(void)
{
    GAtt89_Xform x;
    x.pos.x = 0;
    x.pos.y = 0;
    x.pos.z = 0;
    x.rot = gatt89_quat_identity();
    x.scale.x = GATTACH89_FIX_ONE;
    x.scale.y = GATTACH89_FIX_ONE;
    x.scale.z = GATTACH89_FIX_ONE;
    return x;
}

GAtt89_Vec3 gatt89_vec3_add(GAtt89_Vec3 a, GAtt89_Vec3 b)
{
    GAtt89_Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

GAtt89_Vec3 gatt89_vec3_sub(GAtt89_Vec3 a, GAtt89_Vec3 b)
{
    GAtt89_Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

GAtt89_Vec3 gatt89_vec3_mul_components(GAtt89_Vec3 a, GAtt89_Vec3 b)
{
    GAtt89_Vec3 r;
    r.x = gatt89_mul(a.x, b.x);
    r.y = gatt89_mul(a.y, b.y);
    r.z = gatt89_mul(a.z, b.z);
    return r;
}

static GAtt89_Vec3 gatt89_vec3_cross(GAtt89_Vec3 a, GAtt89_Vec3 b)
{
    GAtt89_Vec3 r;
    r.x = gatt89_mul(a.y, b.z) - gatt89_mul(a.z, b.y);
    r.y = gatt89_mul(a.z, b.x) - gatt89_mul(a.x, b.z);
    r.z = gatt89_mul(a.x, b.y) - gatt89_mul(a.y, b.x);
    return r;
}

GAtt89_Quat gatt89_quat_mul(GAtt89_Quat a, GAtt89_Quat b)
{
    GAtt89_Quat r;
    r.w = gatt89_mul(a.w, b.w) - gatt89_mul(a.x, b.x) - gatt89_mul(a.y, b.y) - gatt89_mul(a.z, b.z);
    r.x = gatt89_mul(a.w, b.x) + gatt89_mul(a.x, b.w) + gatt89_mul(a.y, b.z) - gatt89_mul(a.z, b.y);
    r.y = gatt89_mul(a.w, b.y) - gatt89_mul(a.x, b.z) + gatt89_mul(a.y, b.w) + gatt89_mul(a.z, b.x);
    r.z = gatt89_mul(a.w, b.z) + gatt89_mul(a.x, b.y) - gatt89_mul(a.y, b.x) + gatt89_mul(a.z, b.w);
    return r;
}

GAtt89_Vec3 gatt89_quat_rotate_vec3(GAtt89_Quat q, GAtt89_Vec3 v)
{
    GAtt89_Vec3 qv;
    GAtt89_Vec3 t;
    GAtt89_Vec3 c;
    GAtt89_Vec3 r;

    qv.x = q.x;
    qv.y = q.y;
    qv.z = q.z;

    t = gatt89_vec3_cross(qv, v);
    t.x += t.x;
    t.y += t.y;
    t.z += t.z;

    c = gatt89_vec3_cross(qv, t);

    r.x = v.x + gatt89_mul(q.w, t.x) + c.x;
    r.y = v.y + gatt89_mul(q.w, t.y) + c.y;
    r.z = v.z + gatt89_mul(q.w, t.z) + c.z;
    return r;
}

GAtt89_Xform gatt89_xform_compose(const GAtt89_Xform *parent, const GAtt89_Xform *local)
{
    GAtt89_Xform r;
    GAtt89_Vec3 scaled;
    GAtt89_Vec3 rotated;

    if (parent == 0 && local == 0) return gatt89_xform_identity();
    if (parent == 0) return *local;
    if (local == 0) return *parent;

    scaled = gatt89_vec3_mul_components(local->pos, parent->scale);
    rotated = gatt89_quat_rotate_vec3(parent->rot, scaled);

    r.pos = gatt89_vec3_add(parent->pos, rotated);
    r.rot = gatt89_quat_mul(parent->rot, local->rot);
    r.scale = gatt89_vec3_mul_components(parent->scale, local->scale);
    return r;
}

void gatt89_name_copy(char dst[GATTACH89_NAME_LEN], const char *src)
{
    int i;
    if (dst == 0) return;
    for (i = 0; i < GATTACH89_NAME_LEN; ++i) dst[i] = '\0';
    if (src == 0) return;
    for (i = 0; i < GATTACH89_NAME_LEN - 1; ++i) {
        dst[i] = src[i];
        if (src[i] == '\0') break;
    }
    dst[GATTACH89_NAME_LEN - 1] = '\0';
}

int gatt89_name_eq(const char *a, const char *b)
{
    int i;
    if (a == 0 || b == 0) return GATTACH89_FALSE;
    for (i = 0; i < GATTACH89_NAME_LEN; ++i) {
        if (a[i] != b[i]) return GATTACH89_FALSE;
        if (a[i] == '\0') return GATTACH89_TRUE;
    }
    return GATTACH89_TRUE;
}

void gatt89_world_init(GAtt89_World *w)
{
    int i;
    if (w == 0) return;
    for (i = 0; i < GATTACH89_MAX_SOCKETS; ++i) {
        w->sockets[i].used = GATTACH89_FALSE;
        w->sockets[i].owner_entity_id = GATTACH89_INVALID_ID;
        w->sockets[i].kind = GATTACH89_SOCKET_ENTITY;
        w->sockets[i].bone_index = GATTACH89_INVALID_ID;
        w->sockets[i].parent_socket_id = GATTACH89_INVALID_ID;
        gatt89_name_copy(w->sockets[i].name, "");
        w->sockets[i].local = gatt89_xform_identity();
        w->sockets[i].world = gatt89_xform_identity();
    }
    for (i = 0; i < GATTACH89_MAX_ATTACHMENTS; ++i) {
        w->attachments[i].used = GATTACH89_FALSE;
        w->attachments[i].parent_entity_id = GATTACH89_INVALID_ID;
        w->attachments[i].child_entity_id = GATTACH89_INVALID_ID;
        w->attachments[i].socket_id = GATTACH89_INVALID_ID;
        w->attachments[i].kind = GATTACH89_ATTACH_PROP;
        w->attachments[i].flags = 0u;
        gatt89_name_copy(w->attachments[i].name, "");
        w->attachments[i].local = gatt89_xform_identity();
        w->attachments[i].world = gatt89_xform_identity();
    }
    for (i = 0; i < GATTACH89_MAX_PARTS; ++i) {
        w->parts[i].used = GATTACH89_FALSE;
        w->parts[i].owner_entity_id = GATTACH89_INVALID_ID;
        w->parts[i].flags = 0u;
        gatt89_name_copy(w->parts[i].name, "");
    }
}

void gatt89_output_init(GAtt89_Output *out)
{
    int i;
    if (out == 0) return;
    out->event_count = 0;
    out->dropped_event_count = 0;
    for (i = 0; i < GATTACH89_MAX_EVENTS; ++i) {
        out->events[i].kind = 0;
        out->events[i].owner_entity_id = GATTACH89_INVALID_ID;
        out->events[i].child_entity_id = GATTACH89_INVALID_ID;
        out->events[i].socket_id = GATTACH89_INVALID_ID;
        out->events[i].attachment_id = GATTACH89_INVALID_ID;
        out->events[i].part_id = GATTACH89_INVALID_ID;
        out->events[i].xform = gatt89_xform_identity();
    }
}

int gatt89_socket_add(GAtt89_World *w, int owner_entity_id, const char *name, int kind, int bone_index, int parent_socket_id, const GAtt89_Xform *local)
{
    int id;
    if (w == 0 || name == 0) return GATTACH89_ERR_NULL;
    id = gatt89_slot_socket(w);
    if (id < 0) return id;
    w->sockets[id].used = GATTACH89_TRUE;
    w->sockets[id].owner_entity_id = owner_entity_id;
    w->sockets[id].kind = kind;
    w->sockets[id].bone_index = bone_index;
    w->sockets[id].parent_socket_id = parent_socket_id;
    gatt89_name_copy(w->sockets[id].name, name);
    w->sockets[id].local = (local != 0) ? *local : gatt89_xform_identity();
    w->sockets[id].world = gatt89_xform_identity();
    return id;
}

int gatt89_socket_find(const GAtt89_World *w, int owner_entity_id, const char *name)
{
    int i;
    if (w == 0 || name == 0) return GATTACH89_ERR_NULL;
    for (i = 0; i < GATTACH89_MAX_SOCKETS; ++i) {
        if (w->sockets[i].used && w->sockets[i].owner_entity_id == owner_entity_id && gatt89_name_eq(w->sockets[i].name, name)) {
            return i;
        }
    }
    return GATTACH89_ERR_NOT_FOUND;
}

int gatt89_socket_set_local(GAtt89_World *w, int socket_id, const GAtt89_Xform *local)
{
    if (w == 0 || local == 0) return GATTACH89_ERR_NULL;
    if (socket_id < 0 || socket_id >= GATTACH89_MAX_SOCKETS) return GATTACH89_ERR_BAD_ARG;
    if (!w->sockets[socket_id].used) return GATTACH89_ERR_NOT_FOUND;
    w->sockets[socket_id].local = *local;
    return GATTACH89_OK;
}

int gatt89_socket_get_world(const GAtt89_World *w, int socket_id, GAtt89_Xform *out_world)
{
    if (w == 0 || out_world == 0) return GATTACH89_ERR_NULL;
    if (socket_id < 0 || socket_id >= GATTACH89_MAX_SOCKETS) return GATTACH89_ERR_BAD_ARG;
    if (!w->sockets[socket_id].used) return GATTACH89_ERR_NOT_FOUND;
    *out_world = w->sockets[socket_id].world;
    return GATTACH89_OK;
}

int gatt89_attach_add_to_socket_id(GAtt89_World *w, int parent_entity_id, int child_entity_id, const char *attachment_name, int socket_id, int kind, unsigned int flags, const GAtt89_Xform *local)
{
    int id;
    if (w == 0 || attachment_name == 0) return GATTACH89_ERR_NULL;
    if (socket_id < 0 || socket_id >= GATTACH89_MAX_SOCKETS) return GATTACH89_ERR_BAD_ARG;
    if (!w->sockets[socket_id].used) return GATTACH89_ERR_NOT_FOUND;
    id = gatt89_slot_attachment(w);
    if (id < 0) return id;
    w->attachments[id].used = GATTACH89_TRUE;
    w->attachments[id].parent_entity_id = parent_entity_id;
    w->attachments[id].child_entity_id = child_entity_id;
    w->attachments[id].socket_id = socket_id;
    w->attachments[id].kind = kind;
    w->attachments[id].flags = flags | GATTACH89_AF_ACTIVE | GATTACH89_AF_VISIBLE;
    gatt89_name_copy(w->attachments[id].name, attachment_name);
    w->attachments[id].local = (local != 0) ? *local : gatt89_xform_identity();
    w->attachments[id].world = gatt89_xform_identity();
    return id;
}

int gatt89_attach_add(GAtt89_World *w, int parent_entity_id, int child_entity_id, const char *attachment_name, const char *socket_name, int kind, unsigned int flags, const GAtt89_Xform *local)
{
    int sid;
    if (w == 0 || socket_name == 0) return GATTACH89_ERR_NULL;
    sid = gatt89_socket_find(w, parent_entity_id, socket_name);
    if (sid < 0) return sid;
    return gatt89_attach_add_to_socket_id(w, parent_entity_id, child_entity_id, attachment_name, sid, kind, flags, local);
}

int gatt89_attach_find(const GAtt89_World *w, int parent_entity_id, int child_entity_id, const char *attachment_name)
{
    int i;
    if (w == 0 || attachment_name == 0) return GATTACH89_ERR_NULL;
    for (i = 0; i < GATTACH89_MAX_ATTACHMENTS; ++i) {
        if (w->attachments[i].used &&
            w->attachments[i].parent_entity_id == parent_entity_id &&
            w->attachments[i].child_entity_id == child_entity_id &&
            gatt89_name_eq(w->attachments[i].name, attachment_name)) {
            return i;
        }
    }
    return GATTACH89_ERR_NOT_FOUND;
}

int gatt89_attach_set_socket(GAtt89_World *w, int attachment_id, int socket_id)
{
    if (w == 0) return GATTACH89_ERR_NULL;
    if (attachment_id < 0 || attachment_id >= GATTACH89_MAX_ATTACHMENTS || socket_id < 0 || socket_id >= GATTACH89_MAX_SOCKETS) return GATTACH89_ERR_BAD_ARG;
    if (!w->attachments[attachment_id].used || !w->sockets[socket_id].used) return GATTACH89_ERR_NOT_FOUND;
    w->attachments[attachment_id].socket_id = socket_id;
    return GATTACH89_OK;
}

int gatt89_attach_set_local(GAtt89_World *w, int attachment_id, const GAtt89_Xform *local)
{
    if (w == 0 || local == 0) return GATTACH89_ERR_NULL;
    if (attachment_id < 0 || attachment_id >= GATTACH89_MAX_ATTACHMENTS) return GATTACH89_ERR_BAD_ARG;
    if (!w->attachments[attachment_id].used) return GATTACH89_ERR_NOT_FOUND;
    w->attachments[attachment_id].local = *local;
    return GATTACH89_OK;
}

int gatt89_attach_set_visible(GAtt89_World *w, int attachment_id, int visible)
{
    if (w == 0) return GATTACH89_ERR_NULL;
    if (attachment_id < 0 || attachment_id >= GATTACH89_MAX_ATTACHMENTS) return GATTACH89_ERR_BAD_ARG;
    if (!w->attachments[attachment_id].used) return GATTACH89_ERR_NOT_FOUND;
    if (visible) w->attachments[attachment_id].flags |= GATTACH89_AF_VISIBLE;
    else w->attachments[attachment_id].flags &= ~GATTACH89_AF_VISIBLE;
    return GATTACH89_OK;
}

int gatt89_attach_set_active(GAtt89_World *w, int attachment_id, int active)
{
    if (w == 0) return GATTACH89_ERR_NULL;
    if (attachment_id < 0 || attachment_id >= GATTACH89_MAX_ATTACHMENTS) return GATTACH89_ERR_BAD_ARG;
    if (!w->attachments[attachment_id].used) return GATTACH89_ERR_NOT_FOUND;
    if (active) w->attachments[attachment_id].flags |= GATTACH89_AF_ACTIVE;
    else w->attachments[attachment_id].flags &= ~GATTACH89_AF_ACTIVE;
    return GATTACH89_OK;
}

int gatt89_attach_detach(GAtt89_World *w, int attachment_id)
{
    if (w == 0) return GATTACH89_ERR_NULL;
    if (attachment_id < 0 || attachment_id >= GATTACH89_MAX_ATTACHMENTS) return GATTACH89_ERR_BAD_ARG;
    if (!w->attachments[attachment_id].used) return GATTACH89_ERR_NOT_FOUND;
    w->attachments[attachment_id].used = GATTACH89_FALSE;
    return GATTACH89_OK;
}

int gatt89_attach_get_world(const GAtt89_World *w, int attachment_id, GAtt89_Xform *out_world)
{
    if (w == 0 || out_world == 0) return GATTACH89_ERR_NULL;
    if (attachment_id < 0 || attachment_id >= GATTACH89_MAX_ATTACHMENTS) return GATTACH89_ERR_BAD_ARG;
    if (!w->attachments[attachment_id].used) return GATTACH89_ERR_NOT_FOUND;
    *out_world = w->attachments[attachment_id].world;
    return GATTACH89_OK;
}

int gatt89_part_add(GAtt89_World *w, int owner_entity_id, const char *name, int visible)
{
    int id;
    if (w == 0 || name == 0) return GATTACH89_ERR_NULL;
    id = gatt89_slot_part(w);
    if (id < 0) return id;
    w->parts[id].used = GATTACH89_TRUE;
    w->parts[id].owner_entity_id = owner_entity_id;
    w->parts[id].flags = visible ? GATTACH89_PF_VISIBLE : 0u;
    w->parts[id].flags |= GATTACH89_PF_DIRTY;
    gatt89_name_copy(w->parts[id].name, name);
    return id;
}

int gatt89_part_find(const GAtt89_World *w, int owner_entity_id, const char *name)
{
    int i;
    if (w == 0 || name == 0) return GATTACH89_ERR_NULL;
    for (i = 0; i < GATTACH89_MAX_PARTS; ++i) {
        if (w->parts[i].used && w->parts[i].owner_entity_id == owner_entity_id && gatt89_name_eq(w->parts[i].name, name)) {
            return i;
        }
    }
    return GATTACH89_ERR_NOT_FOUND;
}

int gatt89_part_set_visible(GAtt89_World *w, int part_id, int visible)
{
    unsigned int old_flags;
    if (w == 0) return GATTACH89_ERR_NULL;
    if (part_id < 0 || part_id >= GATTACH89_MAX_PARTS) return GATTACH89_ERR_BAD_ARG;
    if (!w->parts[part_id].used) return GATTACH89_ERR_NOT_FOUND;
    old_flags = w->parts[part_id].flags;
    if (visible) w->parts[part_id].flags |= GATTACH89_PF_VISIBLE;
    else w->parts[part_id].flags &= ~GATTACH89_PF_VISIBLE;
    if (old_flags != w->parts[part_id].flags) w->parts[part_id].flags |= GATTACH89_PF_DIRTY;
    return GATTACH89_OK;
}

int gatt89_part_is_visible(const GAtt89_World *w, int part_id)
{
    if (w == 0) return GATTACH89_FALSE;
    if (part_id < 0 || part_id >= GATTACH89_MAX_PARTS) return GATTACH89_FALSE;
    if (!w->parts[part_id].used) return GATTACH89_FALSE;
    return (w->parts[part_id].flags & GATTACH89_PF_VISIBLE) ? GATTACH89_TRUE : GATTACH89_FALSE;
}

int gatt89_update(GAtt89_World *w, const GAtt89_Callbacks *cb, GAtt89_Output *out)
{
    int i;
    GAtt89_Xform socket_world;
    GAtt89_Xform final_world;
    GAtt89_Attachment *a;
    GAtt89_Event ev;
    int parent_visible;

    if (w == 0 || cb == 0) return GATTACH89_ERR_NULL;
    if (cb->get_entity_xform == 0) return GATTACH89_ERR_BAD_ARG;
    if (out != 0) gatt89_output_init(out);

    for (i = 0; i < GATTACH89_MAX_PARTS; ++i) {
        if (w->parts[i].used && (w->parts[i].flags & GATTACH89_PF_DIRTY)) {
            if (cb->set_part_visible != 0) {
                cb->set_part_visible(cb->user, w->parts[i].owner_entity_id, w->parts[i].name, (w->parts[i].flags & GATTACH89_PF_VISIBLE) ? GATTACH89_TRUE : GATTACH89_FALSE);
            }
            ev.kind = GATTACH89_EVENT_PART_VIS_CHANGED;
            ev.owner_entity_id = w->parts[i].owner_entity_id;
            ev.child_entity_id = GATTACH89_INVALID_ID;
            ev.socket_id = GATTACH89_INVALID_ID;
            ev.attachment_id = GATTACH89_INVALID_ID;
            ev.part_id = i;
            ev.xform = gatt89_xform_identity();
            gatt89_emit(out, cb, &ev);
            w->parts[i].flags &= ~GATTACH89_PF_DIRTY;
        }
    }

    for (i = 0; i < GATTACH89_MAX_ATTACHMENTS; ++i) {
        a = &w->attachments[i];
        if (!a->used) continue;
        if (!(a->flags & GATTACH89_AF_ACTIVE)) continue;

        if (gatt89_resolve_socket(w, cb, a->socket_id, &socket_world, out) != GATTACH89_OK) continue;

        parent_visible = GATTACH89_TRUE;
        if ((a->flags & GATTACH89_AF_INHERIT_PARENT_VIS) && cb->get_entity_visible != 0) {
            parent_visible = cb->get_entity_visible(cb->user, a->parent_entity_id);
        }

        if (!(a->flags & GATTACH89_AF_VISIBLE) || !parent_visible) {
            ev.kind = GATTACH89_EVENT_ATTACHMENT_HIDDEN;
            ev.owner_entity_id = a->parent_entity_id;
            ev.child_entity_id = a->child_entity_id;
            ev.socket_id = a->socket_id;
            ev.attachment_id = i;
            ev.part_id = GATTACH89_INVALID_ID;
            ev.xform = a->world;
            gatt89_emit(out, cb, &ev);
            continue;
        }

        final_world = gatt89_xform_compose(&socket_world, &a->local);
        a->world = final_world;

        if ((a->flags & GATTACH89_AF_CALL_CHILD_SETTER) && cb->set_entity_xform != 0) {
            cb->set_entity_xform(cb->user, a->child_entity_id, &final_world);
        }

        ev.kind = GATTACH89_EVENT_ATTACHMENT_RESOLVED;
        ev.owner_entity_id = a->parent_entity_id;
        ev.child_entity_id = a->child_entity_id;
        ev.socket_id = a->socket_id;
        ev.attachment_id = i;
        ev.part_id = GATTACH89_INVALID_ID;
        ev.xform = final_world;
        gatt89_emit(out, cb, &ev);
    }

    return GATTACH89_OK;
}
