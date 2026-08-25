#include "timeverbs89.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    int kind;
    int verb;
    int unit;
    assert(tv89_resolve("time_pause", &kind, &verb));
    assert(kind == TV89_KIND_ACTION && verb == TV89_ACT_TIME_PAUSE);
    assert(tv89_resolve_condition("timer_done", &verb));
    assert(verb == TV89_COND_TIMER_DONE);
    assert(tv89_unit_from_name("ms", &unit));
    assert(unit == TV89_UNIT_MILLISECONDS);
    assert(tv89_action_count() > 10);
    assert(tv89_condition_count() > 10);
    assert(strcmp(tv89_canonical_name(TV89_ACT_TIMER_LOOP),
                  "timer_loop") == 0);
    return 0;
}
