#include <assert.h>
#include <string.h>
#include "../include/gmspritestrip89.h"

static void test_parse_count(void)
{
    gmss89_id n = 0U;
    assert(gmss89_parse_strip_count("spr_x_walk_strip14.png", &n));
    assert(n == 14U);
    assert(!gmss89_parse_strip_count("spr_x_walk.png", &n));
}

static void test_clean_name(void)
{
    char buf[64];
    assert(gmss89_make_clean_name("spr_x_walk_strip14.png", buf, sizeof(buf)));
    assert(strcmp(buf, "spr_x_walk") == 0);
}

int main(void)
{
    test_parse_count();
    test_clean_name();
    return 0;
}
