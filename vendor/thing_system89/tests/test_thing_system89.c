#include "thing_system89.h"
#include <stdio.h>
int main(void)
{
    TS89_System a;
    TS89_System b;
    TS89_Thing t;
    TS89_Thing stale;
    ts89_init(&a, 32);
    ts89_init(&b, 8);
    t = ts89_reserve(&a);
    if (t == TS89_THING_INVALID || !ts89_is_reserved(&a, t)) return 1;
    if (!ts89_set_namespace(&a, t, 7) || !ts89_set_user(&a, t, 1234UL)) return 2;
    if (!ts89_publish(&a, t) || !ts89_is_alive(&a, t)) return 3;
    if (!ts89_lock(&a, t) || ts89_destroy(&a, t)) return 4;
    if (!ts89_unlock(&a, t)) return 5;
    stale = t;
    if (!ts89_destroy(&a, t) || !ts89_is_stale(&a, stale)) return 6;
    if (!ts89_check_integrity(&a) || !ts89_check_integrity(&b)) return 7;
    if (ts89_alive_count(&b) != 0) return 8;
    puts("thing_system89 context/isolation test: OK");
    return 0;
}
