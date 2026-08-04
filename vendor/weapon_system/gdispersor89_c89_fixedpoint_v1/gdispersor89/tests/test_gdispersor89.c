#include "../src/gdispersor89.h"

int main(void)
{
    GDP89_Request request;
    GDP89_Projectile out[8];
    int count;

    gdp89_request_defaults(&request);
    request.mode = GDP89_MODE_SPREAD;
    request.projectile_count = 7;
    request.spread_fx = gdp89_spread_from_degrees(gdp89_fx_from_int(7));
    request.damage_mode = GDP89_DAMAGE_SPLIT_TOTAL;
    request.damage_fx = gdp89_fx_from_int(35);
    count = gdp89_build(&request, out, 8);

    if (count != 7) return 1;
    if (!(out[0].flags & GDP89_PROJECTILE_CENTER)) return 2;
    if (!(out[0].flags & GDP89_PROJECTILE_FIRST)) return 3;
    if (!(out[6].flags & GDP89_PROJECTILE_LAST)) return 4;
    if (out[0].damage_fx != gdp89_fx_from_int(5)) return 5;
    if (out[1].direction.x == out[2].direction.x) return 6;
    return 0;
}
