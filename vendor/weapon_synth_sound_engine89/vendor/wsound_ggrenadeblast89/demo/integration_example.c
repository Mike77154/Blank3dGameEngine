#include "wsound_ggrenadeblast89.h"

/*
 * No file I/O, no allocation and no platform audio dependency.
 * Connect game_audio_next() to the engine's mono callback or mixer.
 */

static ws_ggb89 g_grenade_voice;

void game_audio_init(void)
{
    ws_ggb89_init(&g_grenade_voice, 44100UL, 0x471989UL);
}

void game_grenade_hit_concrete(void)
{
    ws_ggb89_trigger(&g_grenade_voice,
                     WS_GGB89_CONCRETE_IMPACT,
                     32767);
}

ws_gs16 game_audio_next(void)
{
    return ws_ggb89_process(&g_grenade_voice);
}
