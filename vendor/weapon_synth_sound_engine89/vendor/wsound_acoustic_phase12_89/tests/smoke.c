#include "wsoundacoustic89.h"

static wsound89_i16 direct_mem[48000];
static wsound89_i16 world_mem[14000];

int main(void)
{
    wsounda89_context ctx;
    wsounda89_path_params path;
    wsound89_i16 l;
    wsound89_i16 r;
    wsound89_u32 i;
    if (wsounda89_init(&ctx, 44100U, direct_mem, 48000U, world_mem, 14000U) != WSOUND89_OK) return 1;
    wsounda89_path_defaults(&path);
    path.distance_cm = 1200U;
    path.source = WSOUNDA89_SOURCE_EXPLOSION;
    if (wsounda89_set_path(&ctx, &path) != WSOUND89_OK) return 2;
    if (wsounda89_set_material(&ctx, WSOUNDA89_MATERIAL_CONCRETE, 24000U) != WSOUND89_OK) return 3;
    if (wsounda89_set_space(&ctx, WSOUNDA89_SPACE_WAREHOUSE) != WSOUND89_OK) return 4;
    wsounda89_trigger_pressure(&ctx, WSOUNDA89_PRESSURE_ROCKET, 30000U, 0, 7U);
    l = 0;
    r = 0;
    for (i = 0U; i < 88200U; ++i) {
        wsounda89_process_stereo(&ctx, i == 0U ? 26000 : 0, i == 0U ? 26000 : 0, &l, &r);
    }
    return 0;
}
