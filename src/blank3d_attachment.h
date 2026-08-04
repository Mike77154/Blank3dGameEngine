#ifndef BLANK3D_ATTACHMENT_H
#define BLANK3D_ATTACHMENT_H

#include "../vendor/soquete3d/soquete3d.h"
#include "../vendor/gattach89/include/gattach89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_ATTACH89_MAX_ENTITY_STATES 96
#define B3D_ATTACH89_MAX_SOCKET_STATES 128

#define B3D_ATTACH89_SOCKET_WEAPON_R 1
#define B3D_ATTACH89_SOCKET_WEAPON_L 2
#define B3D_ATTACH89_SOCKET_BACK 3
#define B3D_ATTACH89_SOCKET_HIP 4
#define B3D_ATTACH89_SOCKET_HEAD 5

#define B3D_ATTACH89_OBJECT_PLAYER_WEAPON 10001

typedef struct Blank3DAttachEntityStateTag {
    int used;
    int entity_id;
    int visible;
    GAtt89_Xform world;
} Blank3DAttachEntityState;

typedef struct Blank3DAttachSocketStateTag {
    int used;
    int owner_entity_id;
    int socket_index;
    GAtt89_Xform world;
} Blank3DAttachSocketState;

typedef struct Blank3DAttachmentWorldTag {
    GAtt89_World world;
    GAtt89_Callbacks callbacks;
    GAtt89_Output output;
    Blank3DAttachEntityState entities[B3D_ATTACH89_MAX_ENTITY_STATES];
    Blank3DAttachSocketState sockets[B3D_ATTACH89_MAX_SOCKET_STATES];
    int resolved_events;
    int missing_base_events;
    int last_result;
    int initialized;
} Blank3DAttachmentWorld;

void blank3d_attachment_init(Blank3DAttachmentWorld *attachments);
int blank3d_attachment_publish_entity_pose(
    Blank3DAttachmentWorld *attachments,
    int entity_id,
    const soq3d_pose *world_pose,
    int visible);
int blank3d_attachment_publish_socket_pose(
    Blank3DAttachmentWorld *attachments,
    int owner_entity_id,
    int socket_index,
    const soq3d_pose *world_pose);
int blank3d_attachment_define_socket(
    Blank3DAttachmentWorld *attachments,
    int owner_entity_id,
    const char *socket_name,
    int socket_index,
    const GAtt89_Xform *local_offset);
int blank3d_attachment_attach_object(
    Blank3DAttachmentWorld *attachments,
    int parent_entity_id,
    int child_entity_id,
    const char *attachment_name,
    const char *socket_name,
    const GAtt89_Xform *local_offset);
int blank3d_attachment_move_object(
    Blank3DAttachmentWorld *attachments,
    int parent_entity_id,
    int child_entity_id,
    const char *attachment_name,
    const char *socket_name,
    const GAtt89_Xform *local_offset);
int blank3d_attachment_detach_object(
    Blank3DAttachmentWorld *attachments,
    int parent_entity_id,
    int child_entity_id,
    const char *attachment_name);
int blank3d_attachment_set_object_visible(
    Blank3DAttachmentWorld *attachments,
    int parent_entity_id,
    int child_entity_id,
    const char *attachment_name,
    int visible);
int blank3d_attachment_update(Blank3DAttachmentWorld *attachments);
int blank3d_attachment_get_object_xform(
    const Blank3DAttachmentWorld *attachments,
    int child_entity_id,
    GAtt89_Xform *out_world);
int blank3d_attachment_get_object_pose(
    const Blank3DAttachmentWorld *attachments,
    int child_entity_id,
    soq3d_pose *out_world);
const char *blank3d_attachment_status(
    const Blank3DAttachmentWorld *attachments);

#ifdef __cplusplus
}
#endif

#endif
