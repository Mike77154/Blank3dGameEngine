#ifndef VP_CALLBACKS_H
#define VP_CALLBACKS_H

#include "vp_math3.h"
#include "vp_types.h"

/* vpWorld forward typedef is provided by vp_types.h. */

/*
  Optional host callbacks.

  This library intentionally does NOT own collision detection.
  These callbacks are "integration points" so you can plug in:
   - broadphase/narrowphase (fill contacts),
   - scene queries (raycast/overlap/sweep),
   - CCD TOI computation (host-driven).
*/
typedef struct vpCallbacks {
    void* user;

    /* called at the very start of vpWorldStep() (before contact finalize/solve) */
    void (*preStep)(void* user, vpWorld* w, vp_fx dt);

    /* called at the very end of vpWorldStep() (after integration + events) */
    void (*postStep)(void* user, vpWorld* w, vp_fx dt);

    /*
      Host-driven CCD hook (optional).
      Return value: toi in [0,1] where 1 == no impact within the step.
      If you provide this, you can call vpWorldStepCCD() (see vp_world.h).
    */
    vp_fx (*bodySweepTOI)(
        void* user,
        vp_u16 bodyId,
        vpVec3 fromPos, vpQuat fromRot,
        vpVec3 toPos,   vpQuat toRot,
        vp_u16* hitBody,
        vpVec3* hitPoint,
        vpVec3* hitNormal
    );
} vpCallbacks;

#endif /* VP_CALLBACKS_H */
