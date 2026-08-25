#include "pdc3d_bridge.h"

void pdc3d_bridge_init(pdc3d_bridge *b)
{
    if (b == 0) {
        return;
    }
    b->user = 0;
    b->world_probe = 0;
    b->damage_event = 0;
    b->socket_pose = 0;
}
