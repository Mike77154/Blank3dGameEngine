#include "blank3d_attachment.h"

#include <limits.h>
#include <string.h>

static gatt_fix b3d_att89_abs(gatt_fix value)
{
    if (value == INT_MIN) return INT_MAX;
    return value < 0 ? -value : value;
}

static gatt_fix b3d_att89_sqrt(gatt_fix value)
{
    gatt_fix current;
    gatt_fix next;
    int i;
    if (value <= 0) return 0;
    current = value < GATTACH89_FIX_ONE ? GATTACH89_FIX_ONE : value;
    for (i = 0; i < 14; ++i) {
        next = (current + gatt89_div(value, current)) / 2;
        if (next == current || b3d_att89_abs(next - current) <= 1) {
            current = next;
            break;
        }
        current = next;
    }
    return current;
}

static gatt_fix b3d_att89_q16_to_q12(soq3d_fx value)
{
    return (gatt_fix)(value / 16);
}

static soq3d_fx b3d_att89_q12_to_q16(gatt_fix value)
{
    long converted;
    converted = (long)value * 16L;
    if (converted > (long)INT_MAX) return (soq3d_fx)INT_MAX;
    if (converted < (long)INT_MIN) return (soq3d_fx)INT_MIN;
    return (soq3d_fx)converted;
}

static GAtt89_Quat b3d_att89_quat_normalize(GAtt89_Quat q)
{
    gatt_fix len2;
    gatt_fix len;
    len2 = gatt89_mul(q.x, q.x) + gatt89_mul(q.y, q.y) +
           gatt89_mul(q.z, q.z) + gatt89_mul(q.w, q.w);
    len = b3d_att89_sqrt(len2);
    if (len <= 0) return gatt89_quat_identity();
    q.x = gatt89_div(q.x, len);
    q.y = gatt89_div(q.y, len);
    q.z = gatt89_div(q.z, len);
    q.w = gatt89_div(q.w, len);
    return q;
}

static GAtt89_Quat b3d_att89_basis_to_quat(const soq3d_basis3 *basis)
{
    gatt_fix m00;
    gatt_fix m01;
    gatt_fix m02;
    gatt_fix m10;
    gatt_fix m11;
    gatt_fix m12;
    gatt_fix m20;
    gatt_fix m21;
    gatt_fix m22;
    gatt_fix trace;
    gatt_fix s;
    GAtt89_Quat q;

    if (basis == 0) return gatt89_quat_identity();
    m00 = b3d_att89_q16_to_q12(basis->m00);
    m01 = b3d_att89_q16_to_q12(basis->m01);
    m02 = b3d_att89_q16_to_q12(basis->m02);
    m10 = b3d_att89_q16_to_q12(basis->m10);
    m11 = b3d_att89_q16_to_q12(basis->m11);
    m12 = b3d_att89_q16_to_q12(basis->m12);
    m20 = b3d_att89_q16_to_q12(basis->m20);
    m21 = b3d_att89_q16_to_q12(basis->m21);
    m22 = b3d_att89_q16_to_q12(basis->m22);
    trace = m00 + m11 + m22;
    q = gatt89_quat_identity();

    if (trace > 0) {
        s = b3d_att89_sqrt(trace + GATTACH89_FIX_ONE) * 2;
        if (s == 0) return q;
        q.w = s / 4;
        q.x = gatt89_div(m21 - m12, s);
        q.y = gatt89_div(m02 - m20, s);
        q.z = gatt89_div(m10 - m01, s);
    } else if (m00 > m11 && m00 > m22) {
        s = b3d_att89_sqrt(GATTACH89_FIX_ONE + m00 - m11 - m22) * 2;
        if (s == 0) return q;
        q.w = gatt89_div(m21 - m12, s);
        q.x = s / 4;
        q.y = gatt89_div(m01 + m10, s);
        q.z = gatt89_div(m02 + m20, s);
    } else if (m11 > m22) {
        s = b3d_att89_sqrt(GATTACH89_FIX_ONE + m11 - m00 - m22) * 2;
        if (s == 0) return q;
        q.w = gatt89_div(m02 - m20, s);
        q.x = gatt89_div(m01 + m10, s);
        q.y = s / 4;
        q.z = gatt89_div(m12 + m21, s);
    } else {
        s = b3d_att89_sqrt(GATTACH89_FIX_ONE + m22 - m00 - m11) * 2;
        if (s == 0) return q;
        q.w = gatt89_div(m10 - m01, s);
        q.x = gatt89_div(m02 + m20, s);
        q.y = gatt89_div(m12 + m21, s);
        q.z = s / 4;
    }
    return b3d_att89_quat_normalize(q);
}

static GAtt89_Xform b3d_att89_pose_to_xform(const soq3d_pose *pose)
{
    GAtt89_Xform out;
    out = gatt89_xform_identity();
    if (pose == 0) return out;
    out.pos.x = b3d_att89_q16_to_q12(pose->position.x);
    out.pos.y = b3d_att89_q16_to_q12(pose->position.y);
    out.pos.z = b3d_att89_q16_to_q12(pose->position.z);
    out.rot = b3d_att89_basis_to_quat(&pose->basis);
    return out;
}

static soq3d_basis3 b3d_att89_quat_to_basis(GAtt89_Quat q,
                                             GAtt89_Vec3 scale)
{
    gatt_fix xx;
    gatt_fix yy;
    gatt_fix zz;
    gatt_fix xy;
    gatt_fix xz;
    gatt_fix yz;
    gatt_fix wx;
    gatt_fix wy;
    gatt_fix wz;
    gatt_fix two;
    gatt_fix m00;
    gatt_fix m01;
    gatt_fix m02;
    gatt_fix m10;
    gatt_fix m11;
    gatt_fix m12;
    gatt_fix m20;
    gatt_fix m21;
    gatt_fix m22;
    soq3d_basis3 out;

    q = b3d_att89_quat_normalize(q);
    two = GATTACH89_FIX_ONE * 2;
    xx = gatt89_mul(q.x, q.x);
    yy = gatt89_mul(q.y, q.y);
    zz = gatt89_mul(q.z, q.z);
    xy = gatt89_mul(q.x, q.y);
    xz = gatt89_mul(q.x, q.z);
    yz = gatt89_mul(q.y, q.z);
    wx = gatt89_mul(q.w, q.x);
    wy = gatt89_mul(q.w, q.y);
    wz = gatt89_mul(q.w, q.z);

    m00 = GATTACH89_FIX_ONE - gatt89_mul(two, yy + zz);
    m01 = gatt89_mul(two, xy - wz);
    m02 = gatt89_mul(two, xz + wy);
    m10 = gatt89_mul(two, xy + wz);
    m11 = GATTACH89_FIX_ONE - gatt89_mul(two, xx + zz);
    m12 = gatt89_mul(two, yz - wx);
    m20 = gatt89_mul(two, xz - wy);
    m21 = gatt89_mul(two, yz + wx);
    m22 = GATTACH89_FIX_ONE - gatt89_mul(two, xx + yy);

    out.m00 = b3d_att89_q12_to_q16(gatt89_mul(m00, scale.x));
    out.m10 = b3d_att89_q12_to_q16(gatt89_mul(m10, scale.x));
    out.m20 = b3d_att89_q12_to_q16(gatt89_mul(m20, scale.x));
    out.m01 = b3d_att89_q12_to_q16(gatt89_mul(m01, scale.y));
    out.m11 = b3d_att89_q12_to_q16(gatt89_mul(m11, scale.y));
    out.m21 = b3d_att89_q12_to_q16(gatt89_mul(m21, scale.y));
    out.m02 = b3d_att89_q12_to_q16(gatt89_mul(m02, scale.z));
    out.m12 = b3d_att89_q12_to_q16(gatt89_mul(m12, scale.z));
    out.m22 = b3d_att89_q12_to_q16(gatt89_mul(m22, scale.z));
    return out;
}

static int b3d_att89_find_entity(const Blank3DAttachmentWorld *attachments,
                                  int entity_id)
{
    int i;
    if (attachments == 0) return -1;
    for (i = 0; i < B3D_ATTACH89_MAX_ENTITY_STATES; ++i) {
        if (attachments->entities[i].used &&
            attachments->entities[i].entity_id == entity_id)
            return i;
    }
    return -1;
}

static int b3d_att89_ensure_entity(Blank3DAttachmentWorld *attachments,
                                   int entity_id)
{
    int i;
    i = b3d_att89_find_entity(attachments, entity_id);
    if (i >= 0) return i;
    for (i = 0; i < B3D_ATTACH89_MAX_ENTITY_STATES; ++i) {
        if (!attachments->entities[i].used) {
            attachments->entities[i].used = 1;
            attachments->entities[i].entity_id = entity_id;
            attachments->entities[i].visible = 1;
            attachments->entities[i].world = gatt89_xform_identity();
            return i;
        }
    }
    return -1;
}

static int b3d_att89_find_socket_state(
    const Blank3DAttachmentWorld *attachments,
    int owner_entity_id,
    int socket_index)
{
    int i;
    if (attachments == 0) return -1;
    for (i = 0; i < B3D_ATTACH89_MAX_SOCKET_STATES; ++i) {
        if (attachments->sockets[i].used &&
            attachments->sockets[i].owner_entity_id == owner_entity_id &&
            attachments->sockets[i].socket_index == socket_index)
            return i;
    }
    return -1;
}

static int b3d_att89_ensure_socket_state(
    Blank3DAttachmentWorld *attachments,
    int owner_entity_id,
    int socket_index)
{
    int i;
    i = b3d_att89_find_socket_state(attachments, owner_entity_id,
                                    socket_index);
    if (i >= 0) return i;
    for (i = 0; i < B3D_ATTACH89_MAX_SOCKET_STATES; ++i) {
        if (!attachments->sockets[i].used) {
            attachments->sockets[i].used = 1;
            attachments->sockets[i].owner_entity_id = owner_entity_id;
            attachments->sockets[i].socket_index = socket_index;
            attachments->sockets[i].world = gatt89_xform_identity();
            return i;
        }
    }
    return -1;
}

static int b3d_att89_get_entity(void *user,
                                 int entity_id,
                                 GAtt89_Xform *out_xform)
{
    Blank3DAttachmentWorld *attachments;
    int slot;
    attachments = (Blank3DAttachmentWorld *)user;
    if (attachments == 0 || out_xform == 0) return 0;
    slot = b3d_att89_find_entity(attachments, entity_id);
    if (slot < 0) return 0;
    *out_xform = attachments->entities[slot].world;
    return 1;
}

static int b3d_att89_get_bone(void *user,
                               int entity_id,
                               int bone_index,
                               GAtt89_Xform *out_xform)
{
    Blank3DAttachmentWorld *attachments;
    int slot;
    attachments = (Blank3DAttachmentWorld *)user;
    if (attachments == 0 || out_xform == 0) return 0;
    slot = b3d_att89_find_socket_state(attachments, entity_id, bone_index);
    if (slot < 0) return 0;
    *out_xform = attachments->sockets[slot].world;
    return 1;
}

static int b3d_att89_get_visible(void *user, int entity_id)
{
    Blank3DAttachmentWorld *attachments;
    int slot;
    attachments = (Blank3DAttachmentWorld *)user;
    if (attachments == 0) return 0;
    slot = b3d_att89_find_entity(attachments, entity_id);
    return slot >= 0 ? attachments->entities[slot].visible : 1;
}

static void b3d_att89_set_entity(void *user,
                                  int entity_id,
                                  const GAtt89_Xform *xform)
{
    Blank3DAttachmentWorld *attachments;
    int slot;
    attachments = (Blank3DAttachmentWorld *)user;
    if (attachments == 0 || xform == 0) return;
    slot = b3d_att89_ensure_entity(attachments, entity_id);
    if (slot < 0) return;
    attachments->entities[slot].world = *xform;
}

static void b3d_att89_on_event(void *user, const GAtt89_Event *event_value)
{
    Blank3DAttachmentWorld *attachments;
    attachments = (Blank3DAttachmentWorld *)user;
    if (attachments == 0 || event_value == 0) return;
    if (event_value->kind == GATTACH89_EVENT_ATTACHMENT_RESOLVED)
        attachments->resolved_events += 1;
    else if (event_value->kind == GATTACH89_EVENT_MISSING_BASE)
        attachments->missing_base_events += 1;
}

void blank3d_attachment_init(Blank3DAttachmentWorld *attachments)
{
    if (attachments == 0) return;
    memset(attachments, 0, sizeof(*attachments));
    gatt89_world_init(&attachments->world);
    gatt89_output_init(&attachments->output);
    attachments->callbacks.user = attachments;
    attachments->callbacks.get_entity_xform = b3d_att89_get_entity;
    attachments->callbacks.get_bone_xform = b3d_att89_get_bone;
    attachments->callbacks.get_entity_visible = b3d_att89_get_visible;
    attachments->callbacks.set_entity_xform = b3d_att89_set_entity;
    attachments->callbacks.set_part_visible = 0;
    attachments->callbacks.on_event = b3d_att89_on_event;
    attachments->last_result = GATTACH89_OK;
    attachments->initialized = 1;
}

int blank3d_attachment_publish_entity_pose(
    Blank3DAttachmentWorld *attachments,
    int entity_id,
    const soq3d_pose *world_pose,
    int visible)
{
    int slot;
    if (attachments == 0 || world_pose == 0 || !attachments->initialized)
        return 0;
    slot = b3d_att89_ensure_entity(attachments, entity_id);
    if (slot < 0) return 0;
    attachments->entities[slot].world = b3d_att89_pose_to_xform(world_pose);
    attachments->entities[slot].visible = visible ? 1 : 0;
    return 1;
}

int blank3d_attachment_publish_socket_pose(
    Blank3DAttachmentWorld *attachments,
    int owner_entity_id,
    int socket_index,
    const soq3d_pose *world_pose)
{
    int slot;
    if (attachments == 0 || world_pose == 0 || !attachments->initialized)
        return 0;
    slot = b3d_att89_ensure_socket_state(attachments, owner_entity_id,
                                         socket_index);
    if (slot < 0) return 0;
    attachments->sockets[slot].world = b3d_att89_pose_to_xform(world_pose);
    return 1;
}

int blank3d_attachment_define_socket(
    Blank3DAttachmentWorld *attachments,
    int owner_entity_id,
    const char *socket_name,
    int socket_index,
    const GAtt89_Xform *local_offset)
{
    GAtt89_Xform identity;
    int existing;
    if (attachments == 0 || socket_name == 0 || !attachments->initialized)
        return GATTACH89_ERR_NULL;
    existing = gatt89_socket_find(&attachments->world, owner_entity_id,
                                  socket_name);
    if (existing >= 0) return existing;
    identity = gatt89_xform_identity();
    return gatt89_socket_add(&attachments->world,
                             owner_entity_id,
                             socket_name,
                             GATTACH89_SOCKET_BONE,
                             socket_index,
                             GATTACH89_INVALID_ID,
                             local_offset ? local_offset : &identity);
}

int blank3d_attachment_attach_object(
    Blank3DAttachmentWorld *attachments,
    int parent_entity_id,
    int child_entity_id,
    const char *attachment_name,
    const char *socket_name,
    const GAtt89_Xform *local_offset)
{
    int result;
    if (attachments == 0 || attachment_name == 0 || socket_name == 0)
        return GATTACH89_ERR_NULL;
    if (b3d_att89_ensure_entity(attachments, child_entity_id) < 0)
        return GATTACH89_ERR_FULL;
    result = gatt89_attach_find(&attachments->world, parent_entity_id,
                                child_entity_id, attachment_name);
    if (result >= 0)
        return blank3d_attachment_move_object(attachments,
                                               parent_entity_id,
                                               child_entity_id,
                                               attachment_name,
                                               socket_name,
                                               local_offset);
    return gatt89_attach_add(&attachments->world,
                             parent_entity_id,
                             child_entity_id,
                             attachment_name,
                             socket_name,
                             GATTACH89_ATTACH_ENTITY,
                             GATTACH89_AF_CALL_CHILD_SETTER |
                             GATTACH89_AF_RENDER_PACKET |
                             GATTACH89_AF_INHERIT_PARENT_VIS,
                             local_offset);
}

int blank3d_attachment_move_object(
    Blank3DAttachmentWorld *attachments,
    int parent_entity_id,
    int child_entity_id,
    const char *attachment_name,
    const char *socket_name,
    const GAtt89_Xform *local_offset)
{
    int attachment_id;
    int socket_id;
    int result;
    if (attachments == 0 || attachment_name == 0 || socket_name == 0)
        return GATTACH89_ERR_NULL;
    attachment_id = gatt89_attach_find(&attachments->world,
                                        parent_entity_id,
                                        child_entity_id,
                                        attachment_name);
    if (attachment_id < 0) return attachment_id;
    socket_id = gatt89_socket_find(&attachments->world,
                                    parent_entity_id,
                                    socket_name);
    if (socket_id < 0) return socket_id;
    result = gatt89_attach_set_socket(&attachments->world,
                                      attachment_id, socket_id);
    if (result != GATTACH89_OK) return result;
    if (local_offset != 0)
        result = gatt89_attach_set_local(&attachments->world,
                                         attachment_id, local_offset);
    return result;
}

int blank3d_attachment_detach_object(
    Blank3DAttachmentWorld *attachments,
    int parent_entity_id,
    int child_entity_id,
    const char *attachment_name)
{
    int attachment_id;
    if (attachments == 0 || attachment_name == 0)
        return GATTACH89_ERR_NULL;
    attachment_id = gatt89_attach_find(&attachments->world,
                                        parent_entity_id,
                                        child_entity_id,
                                        attachment_name);
    if (attachment_id < 0) return attachment_id;
    return gatt89_attach_detach(&attachments->world, attachment_id);
}

int blank3d_attachment_set_object_visible(
    Blank3DAttachmentWorld *attachments,
    int parent_entity_id,
    int child_entity_id,
    const char *attachment_name,
    int visible)
{
    int attachment_id;
    if (attachments == 0 || attachment_name == 0)
        return GATTACH89_ERR_NULL;
    attachment_id = gatt89_attach_find(&attachments->world,
                                        parent_entity_id,
                                        child_entity_id,
                                        attachment_name);
    if (attachment_id < 0) return attachment_id;
    return gatt89_attach_set_visible(&attachments->world,
                                     attachment_id,
                                     visible ? 1 : 0);
}

int blank3d_attachment_update(Blank3DAttachmentWorld *attachments)
{
    int result;
    if (attachments == 0 || !attachments->initialized) return 0;
    attachments->resolved_events = 0;
    attachments->missing_base_events = 0;
    gatt89_output_init(&attachments->output);
    result = gatt89_update(&attachments->world,
                           &attachments->callbacks,
                           &attachments->output);
    attachments->last_result = result;
    return result == GATTACH89_OK;
}

int blank3d_attachment_get_object_xform(
    const Blank3DAttachmentWorld *attachments,
    int child_entity_id,
    GAtt89_Xform *out_world)
{
    int slot;
    if (attachments == 0 || out_world == 0) return 0;
    slot = b3d_att89_find_entity(attachments, child_entity_id);
    if (slot < 0) return 0;
    *out_world = attachments->entities[slot].world;
    return 1;
}

int blank3d_attachment_get_object_pose(
    const Blank3DAttachmentWorld *attachments,
    int child_entity_id,
    soq3d_pose *out_world)
{
    GAtt89_Xform xform;
    if (out_world == 0 ||
        !blank3d_attachment_get_object_xform(attachments,
                                             child_entity_id,
                                             &xform))
        return 0;
    out_world->position.x = b3d_att89_q12_to_q16(xform.pos.x);
    out_world->position.y = b3d_att89_q12_to_q16(xform.pos.y);
    out_world->position.z = b3d_att89_q12_to_q16(xform.pos.z);
    out_world->basis = b3d_att89_quat_to_basis(xform.rot, xform.scale);
    return 1;
}

const char *blank3d_attachment_status(
    const Blank3DAttachmentWorld *attachments)
{
    if (attachments == 0) return "GAttach89 bridge: null";
    if (!attachments->initialized) return "GAttach89 bridge: not initialized";
    if (attachments->last_result != GATTACH89_OK)
        return "GAttach89 bridge: resolver error";
    if (attachments->missing_base_events > 0)
        return "GAttach89 bridge: missing carrier socket";
    if (attachments->resolved_events > 0)
        return "GAttach89 bridge: objects resolved";
    return "GAttach89 bridge: ready";
}
