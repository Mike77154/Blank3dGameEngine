#ifndef VP_EVENTS_H
#define VP_EVENTS_H

#include "vp_types.h"

typedef enum vpEventType {
    VP_EVENT_NONE = 0,

    /* contact events are per BODY PAIR (not per point) */
    VP_EVENT_CONTACT_BEGIN   = 1,
    VP_EVENT_CONTACT_PERSIST = 2,
    VP_EVENT_CONTACT_END     = 3,

    VP_EVENT_BODY_SLEEP      = 4,
    VP_EVENT_BODY_WAKE       = 5,

    VP_EVENT_JOINT_BROKE     = 6
} vpEventType;

typedef struct vpEvent {
    vp_u8  type;
    vp_u8  _pad;
    vp_u16 bodyA;   /* for contact: min(bodyId), for sleep/wake: body id */
    vp_u16 bodyB;   /* for contact: max(bodyId), else 0 */
    vp_u16 jointId; /* for JOINT_BROKE, else 0 */
    vp_u32 pairKey; /* for contact events, else 0 */
} vpEvent;

#endif /* VP_EVENTS_H */
