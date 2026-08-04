#ifndef VP_DEBUGDRAW_H
#define VP_DEBUGDRAW_H

#include "vp_math3.h"
#include "vp_types.h"

/* vpWorld forward typedef is provided by vp_types.h. */

/* Simple immediate-mode debug draw interface. */
typedef struct vpDebugDraw {
    void* user;

    void (*line)(void* user, vpVec3 a, vpVec3 b, vp_u32 rgba);
    void (*point)(void* user, vpVec3 p, vp_u32 rgba);
} vpDebugDraw;

/* Draw contacts + joints (minimal) */
void vpDebugDrawWorld(const vpWorld* w, const vpDebugDraw* dd);

#endif /* VP_DEBUGDRAW_H */
