#include "gattach89_render_bridge.h"

void gatt89_render_queue_init(GAtt89_RenderQueue *q)
{
    int i;
    if (q == 0) return;
    q->packet_count = 0;
    q->dropped_packet_count = 0;
    for (i = 0; i < GATTACH89_MAX_RENDER_PACKETS; ++i) {
        q->packets[i].used = GATTACH89_FALSE;
        q->packets[i].parent_entity_id = GATTACH89_INVALID_ID;
        q->packets[i].child_entity_id = GATTACH89_INVALID_ID;
        q->packets[i].attachment_id = GATTACH89_INVALID_ID;
        q->packets[i].kind = 0;
        q->packets[i].world = gatt89_xform_identity();
    }
}

int gatt89_render_queue_from_output(const GAtt89_Output *out, const GAtt89_World *w, GAtt89_RenderQueue *q)
{
    int i;
    int aid;
    int pi;

    if (out == 0 || w == 0 || q == 0) return GATTACH89_ERR_NULL;
    gatt89_render_queue_init(q);

    for (i = 0; i < out->event_count; ++i) {
        if (out->events[i].kind == GATTACH89_EVENT_ATTACHMENT_RESOLVED) {
            aid = out->events[i].attachment_id;
            if (aid >= 0 && aid < GATTACH89_MAX_ATTACHMENTS && w->attachments[aid].used) {
                if (w->attachments[aid].flags & GATTACH89_AF_RENDER_PACKET) {
                    if (q->packet_count < GATTACH89_MAX_RENDER_PACKETS) {
                        pi = q->packet_count;
                        q->packets[pi].used = GATTACH89_TRUE;
                        q->packets[pi].parent_entity_id = out->events[i].owner_entity_id;
                        q->packets[pi].child_entity_id = out->events[i].child_entity_id;
                        q->packets[pi].attachment_id = aid;
                        q->packets[pi].kind = w->attachments[aid].kind;
                        q->packets[pi].world = out->events[i].xform;
                        q->packet_count += 1;
                    } else {
                        q->dropped_packet_count += 1;
                    }
                }
            }
        }
    }

    return GATTACH89_OK;
}
