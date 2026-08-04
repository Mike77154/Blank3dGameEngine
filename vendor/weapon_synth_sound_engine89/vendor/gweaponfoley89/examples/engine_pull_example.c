#include "gweaponfoley89.h"

/* Keep contexts static/global on small-stack systems. One context = one voice. */
static gwf89_context weapon_foley_voice;
static gwf89_s16 mix_block[256];

void weapon_audio_boot(void)
{
    gwf89_init(&weapon_foley_voice, 0x12345678UL);
}

void weapon_dry_fire(void)
{
    gwf89_trigger(&weapon_foley_voice, GWF89_PISTOL_EMPTY);
}

void weapon_bolt_fast(void)
{
    gwf89_trigger_ex(&weapon_foley_voice, GWF89_SNIPER_BOLT_DRY,
                     0x44A1B2C3UL, 1U, GWF89_SPEED_FAST);
}

void weapon_room_dry_only(void)
{
    gwf89_set_room_send(&weapon_foley_voice, 0);
}

/* Call from the engine audio callback. */
const gwf89_s16 *weapon_audio_pull(unsigned long frames)
{
    if (frames > 256UL) frames = 256UL;
    gwf89_process(&weapon_foley_voice, mix_block, frames);
    return mix_block;
}
