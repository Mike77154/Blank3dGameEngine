#include <stdio.h>
#include "gameverbs89.h"

static int cond(void *u, const gverb89_call *c, gverb89_result *r)
{
    int *n = (int *)u;
    (void)c;
    ++*n;
    r->truth = 1;
    r->instance_id = 7;
    return GVERB89_HANDLED;
}
static int act(void *u, const gverb89_call *c)
{
    int *n = (int *)u;
    (void)c;
    *n += 10;
    return GVERB89_HANDLED;
}
int main(void)
{
    gverb89_registry r;
    gverb89_call c;
    gverb89_result out;
    int n = 0;
    gverb89_init(&r);
    if (!gverb89_register_condition(&r, "is_on_floor", cond, &n)) return 1;
    if (!gverb89_register_action(&r, "walk_forward", act, &n)) return 2;
    c.owner = 1UL; c.subject = 0; c.name = "is_on_floor";
    c.value_q16 = 0; c.value_text = ""; c.has_value = 0; c.argv = 0; c.argc = 0;
    if (gverb89_query(&r, &c, &out) != GVERB89_HANDLED || !out.truth || out.instance_id != 7) return 3;
    c.name = "walk_forward";
    if (gverb89_perform(&r, &c) != GVERB89_HANDLED) return 4;
    if (n != 11) return 5;
    if (gverb89_count(&r, GVERB89_KIND_CONDITION) != 1) return 6;
    if (gverb89_count(&r, GVERB89_KIND_ACTION) != 1) return 7;
    puts("OK: gameverbs89");
    return 0;
}
