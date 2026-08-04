#include <stdio.h>
#include "../src/gdispersor89.h"

static int print_projectile(void *ctx, const GDP89_Projectile *p)
{
    (void)ctx;
    printf("pellet %d/%d dir=(%ld,%ld,%ld) damage=%ld life=%lu range=%ld\n",
           p->projectile_index + 1,
           p->projectile_count,
           p->direction.x,
           p->direction.y,
           p->direction.z,
           p->damage_fx,
           p->life_ms,
           p->max_distance_fx);
    return GDP89_OK;
}

int main(void)
{
    GDP89_Request request;
    gdp89_request_defaults(&request);
    request.mode = GDP89_MODE_SPREAD;
    request.projectile_count = 8;
    request.spread_fx = GDP89_FIX_ONE / 8L;
    request.damage_mode = GDP89_DAMAGE_SPLIT_TOTAL;
    request.damage_fx = gdp89_fx_from_int(80);
    request.speed_fx = gdp89_fx_from_int(38);
    request.max_distance_fx = gdp89_fx_from_int(24);
    request.life_ms = 700UL;
    return gdp89_emit(&request, print_projectile, 0) < 0 ? 1 : 0;
}
