#include <stdio.h>
#include <string.h>
#include "blank3d_audio.h"
#include "blank3d_weapon_ini.h"
#include "gweapon89.h"

static Blank3DAudio audio;
static gv89_s16 render_buffer[4096];

int main(void)
{
    GWP89_Manager weapon_manager;
    char weapon_status[160];
    static const int capacities[9] = {
        0, 15, 30, 8, 6, 5, 1, 1, 300
    };
    static const int profiles[9] = {
        0,
        WSOUNDDNA89_PROFILE_SERVICE_PISTOL,
        WSOUNDDNA89_PROFILE_SMG,
        WSOUNDDNA89_PROFILE_SHOTGUN,
        WSOUNDDNA89_PROFILE_MAGNUM,
        WSOUNDDNA89_PROFILE_SNIPER,
        WSOUNDDNA89_PROFILE_LAUNCHER,
        WSOUNDDNA89_PROFILE_LAUNCHER,
        WSOUNDDNA89_PROFILE_HEAVY
    };
    static const int actions[9] = {
        0,
        WSOUNDACTION89_PISTOL,
        WSOUNDACTION89_MACHINE,
        WSOUNDACTION89_PUMP_SHOTGUN,
        WSOUNDACTION89_REVOLVER,
        WSOUNDACTION89_RIFLE,
        WSOUNDACTION89_PUMP_SHOTGUN,
        WSOUNDACTION89_RIFLE,
        WSOUNDACTION89_MACHINE
    };
    int weapon;
    gwp89_init(&weapon_manager);
    if (blank3d_weapon_ini_load_manifest(&weapon_manager,
            "config/weapons/weapons.ini", weapon_status,
            sizeof(weapon_status)) != 9) {
        fprintf(stderr, "audio weapon INI load failed: %s\n", weapon_status);
        return 12;
    }
    if (!blank3d_audio_init(&audio, 1)) return 1;
    if (!audio.initialized) return 2;
    for (weapon = 1; weapon <= 8; ++weapon) {
        int shot;
        int clip_ammo;
        if (weapon == 8) {
            blank3d_audio_gatling_begin(&audio);
            blank3d_audio_gatling_fire_start(&audio);
        }
        clip_ammo = capacities[weapon];
        for (shot = 0; shot < 12; ++shot) {
            gv89_u32 primary_key;
            gv89_u32 secondary_key;
            if (clip_ammo > 0) --clip_ammo;
            blank3d_audio_fire_sync(&audio, weapon, clip_ammo,
                                    capacities[weapon]);
            if ((int)audio.synth.world.dna.profile.id != profiles[weapon])
                return 6;
            if ((int)audio.synth.world.action.type != actions[weapon])
                return 7;
            blank3d_audio_projectile_begin(&audio, weapon, 35,
                                           &primary_key, &secondary_key);
            if (weapon == 7) {
                if (primary_key == 0U || secondary_key == 0U) return 8;
            } else if (primary_key != 0U || secondary_key != 0U) {
                return 9;
            }
            if (weapon == 7)
                blank3d_audio_projectile_motion(&audio, weapon, 32,
                                                primary_key, secondary_key);
            blank3d_audio_casing(&audio, weapon);
            (void)wsse89_render_stereo(&audio.synth, render_buffer, 2048U, 0);
            blank3d_audio_projectile_end(&audio, weapon,
                                         primary_key, secondary_key);
        }
        if (weapon == 8)
            blank3d_audio_gatling_release(&audio);
        blank3d_audio_reload_begin_sync(&audio, weapon, 0,
                                        capacities[weapon]);
        blank3d_audio_reload_end_sync(&audio, weapon, capacities[weapon],
                                      capacities[weapon]);
        blank3d_audio_dry_fire(&audio, weapon);
    }
    /* Slingshot is intentionally outside the firearm mechanism recipes. */
    {
        int profile_before;
        unsigned int failures_before;
        profile_before = (int)audio.synth.world.dna.profile.id;
        failures_before = audio.dispatch_failures;
        blank3d_audio_fire_sync(&audio, 9, 1, 1);
        blank3d_audio_casing(&audio, 9);
        if ((int)audio.synth.world.dna.profile.id != profile_before) return 10;
        if (audio.dispatch_failures != failures_before) return 11;
    }
    blank3d_audio_explosion(&audio, 6, 80);
    blank3d_audio_explosion(&audio, 7, 100);
    blank3d_audio_impact(&audio, 1, 75);
    blank3d_audio_impact(&audio, 2, 50);
    blank3d_audio_impact(&audio, 3, 25);
    (void)wsse89_render_stereo(&audio.synth, render_buffer, 2048U, 0);
    blank3d_audio_pump(&audio);
    if (audio.dispatch_failures != 0U) {
        fprintf(stderr, "audio dispatch failures=%u status=%s\n",
                audio.dispatch_failures, blank3d_audio_status(&audio));
        return 3;
    }
    if (strstr(blank3d_audio_status(&audio), "online") == 0) return 4;
    blank3d_audio_shutdown(&audio);
    if (audio.initialized) return 5;
    puts("Blank3D synchronized weapon mechanism test: OK");
    return 0;
}
