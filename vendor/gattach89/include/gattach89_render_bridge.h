#ifndef GATTACH89_RENDER_BRIDGE_H
#define GATTACH89_RENDER_BRIDGE_H

#include "gattach89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GATTACH89_MAX_RENDER_PACKETS GATTACH89_MAX_ATTACHMENTS

typedef struct GAtt89_RenderPacket {
    int used;
    int parent_entity_id;
    int child_entity_id;
    int attachment_id;
    int kind;
    GAtt89_Xform world;
} GAtt89_RenderPacket;

typedef struct GAtt89_RenderQueue {
    GAtt89_RenderPacket packets[GATTACH89_MAX_RENDER_PACKETS];
    int packet_count;
    int dropped_packet_count;
} GAtt89_RenderQueue;

void gatt89_render_queue_init(GAtt89_RenderQueue *q);
int gatt89_render_queue_from_output(const GAtt89_Output *out, const GAtt89_World *w, GAtt89_RenderQueue *q);

#ifdef __cplusplus
}
#endif

#endif
