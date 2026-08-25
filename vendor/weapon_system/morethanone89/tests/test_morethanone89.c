#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "morethanone89.h"

static mto89_stage stage(unsigned int ms, int projectile)
{
    mto89_stage s;
    memset(&s, 0, sizeof(s));
    s.min_ms = ms;
    s.projectile_id = projectile;
    s.override_mask = MTO89_OVERRIDE_PROJECTILE_ID;
    return s;
}

int main(void)
{
    mto89_profile p;
    mto89_result r;
    mto89_stage s;
    mto89_profile_init(&p);
    p.enabled = 1;
    p.activation_ms = 250U;
    s = stage(250U, 16); assert(mto89_set_stage(&p, 0, &s));
    s = stage(600U, 17); assert(mto89_set_stage(&p, 1, &s));
    s = stage(1100U, 18); assert(mto89_set_stage(&p, 2, &s));
    assert(mto89_validate(&p));
    assert(!mto89_resolve(&p, 249U, &r));
    assert(mto89_resolve(&p, 250U, &r)); assert(r.stage_index == 0); assert(r.stage.projectile_id == 16);
    assert(mto89_resolve(&p, 999U, &r)); assert(r.stage_index == 1); assert(r.stage.projectile_id == 17);
    assert(mto89_resolve(&p, 4000U, &r)); assert(r.stage_index == 2); assert(r.stage.projectile_id == 18);
    p.stages[1].min_ms = 200U; assert(!mto89_validate(&p));
    puts("morethanone89: PASS");
    return 0;
}
