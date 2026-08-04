#include "gfaction89_query.h"

int gfa_decision_has_flag(const GFA_Decision *decision, int flag)
{
    if (!decision) return GFA_FALSE;
    return (decision->flags & flag) ? GFA_TRUE : GFA_FALSE;
}

int gfa_is_hostile(const GFA_Decision *decision)
{
    return gfa_decision_has_flag(decision, GFA_FLAG_CAN_ATTACK);
}

int gfa_is_ally(const GFA_Decision *decision)
{
    return gfa_decision_has_flag(decision, GFA_FLAG_CAN_ASSIST);
}

int gfa_should_flee(const GFA_Decision *decision)
{
    return gfa_decision_has_flag(decision, GFA_FLAG_CAN_FLEE);
}

int gfa_should_protect(const GFA_Decision *decision)
{
    return gfa_decision_has_flag(decision, GFA_FLAG_CAN_PROTECT);
}
