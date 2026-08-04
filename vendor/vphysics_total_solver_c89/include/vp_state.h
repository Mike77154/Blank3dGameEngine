#ifndef VP_STATE_H
#define VP_STATE_H

#include "vp_types.h"

/* vpWorld forward typedef is provided by vp_types.h. */

/* State snapshot (for deterministic replay/debugging). */
vp_u32 vpWorldStateSize(const vpWorld* w);

/* Returns 1 on success, 0 on failure (buffer too small / bad magic). */
vp_u8 vpWorldSaveState(const vpWorld* w, void* outBytes, vp_u32 outCap);
vp_u8 vpWorldLoadState(vpWorld* w, const void* inBytes, vp_u32 inCap);

#endif /* VP_STATE_H */
