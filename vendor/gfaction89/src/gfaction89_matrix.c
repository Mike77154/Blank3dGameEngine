#include "gfaction89_matrix.h"

GFA_Relation gfa_make_relation(int disposition, int priority, int flags, int score_bias)
{
    GFA_Relation r;
    r.disposition = disposition;
    r.priority = priority;
    r.flags = flags | GFA_FLAG_VALID;
    r.score_bias = score_bias;
    return r;
}

int gfa_relation_flags_from_disposition(int disposition)
{
    switch (disposition) {
    case GFA_DISP_ALLY:
    case GFA_DISP_FRIENDLY:
    case GFA_DISP_OWNER:
        return GFA_FLAG_CAN_ASSIST | GFA_FLAG_CAN_FOLLOW;
    case GFA_DISP_HATE:
    case GFA_DISP_PREY:
    case GFA_DISP_RIVAL:
        return GFA_FLAG_CAN_ATTACK;
    case GFA_DISP_FEAR:
    case GFA_DISP_AVOID:
        return GFA_FLAG_CAN_FLEE;
    case GFA_DISP_PROTECT:
        return GFA_FLAG_CAN_PROTECT | GFA_FLAG_CAN_ASSIST;
    case GFA_DISP_CONTAIN:
        return GFA_FLAG_CAN_ATTACK | GFA_FLAG_CAN_CONTAIN;
    case GFA_DISP_SCRIPTED:
        return GFA_FLAG_SCRIPTED;
    case GFA_DISP_IGNORE:
        return GFA_FLAG_CAN_IGNORE;
    default:
        break;
    }
    return 0;
}

int gfa_relation_base_score(int disposition)
{
    switch (disposition) {
    case GFA_DISP_HATE: return 120;
    case GFA_DISP_PREY: return 100;
    case GFA_DISP_CONTAIN: return 95;
    case GFA_DISP_RIVAL: return 70;
    case GFA_DISP_FEAR: return 90;
    case GFA_DISP_PROTECT: return 85;
    case GFA_DISP_ALLY: return 60;
    case GFA_DISP_FRIENDLY: return 40;
    case GFA_DISP_AVOID: return 35;
    case GFA_DISP_OWNER: return 80;
    case GFA_DISP_SCRIPTED: return 100;
    case GFA_DISP_IGNORE: return -200;
    default: break;
    }
    return 0;
}
