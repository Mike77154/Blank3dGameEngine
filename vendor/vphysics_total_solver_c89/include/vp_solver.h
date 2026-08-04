#ifndef VP_SOLVER_H
#define VP_SOLVER_H

#include "vp_world.h"

/* Warm-start + iterative solve over all constraints */
void vpSolverWarmStart(vpWorld* w, vp_fx dt);
void vpSolverSolve(vpWorld* w, vp_fx dt);

/* Island/subset variant (indices into w->contacts / w->joints arrays) */
void vpSolverWarmStartSubset(vpWorld* w, vp_fx dt,
    const vp_u16* contactIdx, vp_u16 contactCount,
    const vp_u16* jointIdx,   vp_u16 jointCount);

void vpSolverSolveSubset(vpWorld* w, vp_fx dt,
    const vp_u16* contactIdx, vp_u16 contactCount,
    const vp_u16* jointIdx,   vp_u16 jointCount);

#endif /* VP_SOLVER_H */
