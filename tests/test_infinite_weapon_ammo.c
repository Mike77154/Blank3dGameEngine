#include <stdio.h>
#include <string.h>
#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_modules.h"
#include "blank3d_weapon_host_io.h"

#define ACTOR_ID 501

static void make_input(GWP89_FireInput *in, int flags, unsigned short dt)
{
    memset(in, 0, sizeof(*in));
    in->actor_id = ACTOR_ID;
    in->actor_kind = 1;
    in->team_id = 1;
    in->view_style = GWP89_VIEW_THIRD_PERSON;
    in->trigger_flags = flags;
    in->dt_ms = dt;
    in->socket_forward.z = GWP89_FIX_ONE;
    in->camera_forward.z = GWP89_FIX_ONE;
    in->zoom_fx = GWP89_FIX_ONE;
}

static int load_manager(GWP89_Manager *m)
{
    char status[160];
    gwp89_init(m);
    (void)blank3d_weapon_host_io_bind(m);
    return blank3d_weapon_ini_load_manifest(m, "config/weapons/weapons.ini",
                                            status, sizeof(status)) == 15;
}

static int test_buster_infinite(void)
{
    GWP89_Manager m;
    GWP89_FireInput in;
    const GWP89_WeaponProfile *p;
    int slot;
    int i;
    if (!load_manager(&m)) { puts("load fail"); return 0; }
    slot = gwp89_find_weapon_slot_by_id(&m, B3D_WEAPON_ID_BUSTER);
    if (slot < 0) { puts("slot fail"); return 0; }
    p = gwp89_get_weapon(&m, slot);
    if (!p) { puts("profile null"); return 0; }
    if (!p->infinite_ammo) { printf("infinite parsed=%d clip=%d ammo=%d\n", p->infinite_ammo, p->clip_size, p->ammo_id); return 0; }
    if (gwp89_bind_actor(&m, ACTOR_ID, 1, 1) < 0) return 0;
    if (gwp89_equip_slot(&m, ACTOR_ID, slot, 1) != GWP89_OK) return 0;
    (void)gwp89_set_clip(&m, ACTOR_ID, p->weapon_id, 1);
    (void)gwp89_set_ammo(&m, ACTOR_ID, p->ammo_id, 0);

    for (i = 0; i < 12; i++) {
        make_input(&in, GWP89_TRIGGER_DOWN | GWP89_TRIGGER_PRESSED, 0U);
        { int rr = gwp89_try_fire(&m, &in); if (rr != GWP89_OK) { printf("fire %d failed rc=%d clip=%d reserve=%d\n", i, rr, gwp89_query_clip(&m, ACTOR_ID, p->weapon_id), gwp89_query_ammo(&m, ACTOR_ID, p->ammo_id, p->weapon_id)); return 0; } }
        if (gwp89_query_clip(&m, ACTOR_ID, p->weapon_id) != 1) return 0;
        if (gwp89_query_ammo(&m, ACTOR_ID, p->ammo_id, p->weapon_id) != 0) return 0;
        make_input(&in, 0, 100U);
        if (gwp89_update_actor(&m, &in) < 0) return 0;
        gwp89_clear_events(&m);
    }
    if (gwp89_begin_reload(&m, ACTOR_ID) != GWP89_OK) return 0;
    return gwp89_query_clip(&m, ACTOR_ID, p->weapon_id) == 1;
}

static int test_pistol_still_finite(void)
{
    GWP89_Manager m;
    GWP89_FireInput in;
    const GWP89_WeaponProfile *p;
    int slot;
    if (!load_manager(&m)) return 0;
    slot = gwp89_find_weapon_slot_by_id(&m, 1);
    if (slot < 0) return 0;
    p = gwp89_get_weapon(&m, slot);
    if (!p || p->infinite_ammo) return 0;
    if (gwp89_bind_actor(&m, ACTOR_ID, 1, 1) < 0) return 0;
    if (gwp89_equip_slot(&m, ACTOR_ID, slot, 1) != GWP89_OK) return 0;
    (void)gwp89_set_clip(&m, ACTOR_ID, p->weapon_id, 2);
    (void)gwp89_set_ammo(&m, ACTOR_ID, p->ammo_id, 0);

    make_input(&in, GWP89_TRIGGER_DOWN | GWP89_TRIGGER_PRESSED, 0U);
    if (gwp89_try_fire(&m, &in) != GWP89_OK) return 0;
    make_input(&in, 0, 170U);
    if (gwp89_update_actor(&m, &in) < 0) return 0;
    make_input(&in, GWP89_TRIGGER_DOWN | GWP89_TRIGGER_PRESSED, 0U);
    if (gwp89_try_fire(&m, &in) != GWP89_OK) return 0;
    if (gwp89_query_clip(&m, ACTOR_ID, p->weapon_id) != 0) return 0;
    make_input(&in, 0, 170U);
    if (gwp89_update_actor(&m, &in) < 0) return 0;
    make_input(&in, GWP89_TRIGGER_DOWN | GWP89_TRIGGER_PRESSED, 0U);
    return gwp89_try_fire(&m, &in) == GWP89_NO_AMMO;
}

static int test_pistol_uses_buster_fire_dna(void)
{
    GWP89_Manager m;
    const Blank3DWeaponModules *pistol;
    const Blank3DWeaponModules *buster;
    if (!load_manager(&m)) return 0;
    pistol = blank3d_weapon_modules_get(1);
    buster = blank3d_weapon_modules_get(B3D_WEAPON_ID_BUSTER);
    if (!pistol || !buster) return 0;
    return pistol->audio_profile == buster->audio_profile &&
           pistol->audio_action == buster->audio_action &&
           pistol->audio_muzzle == buster->audio_muzzle &&
           pistol->audio_fire_gain_q15 == buster->audio_fire_gain_q15 &&
           pistol->audio_pressure_energy_q15 == buster->audio_pressure_energy_q15 &&
           pistol->audio_action_speed_q16 == buster->audio_action_speed_q16;
}

int main(void)
{
    if (!test_buster_infinite()) {
        puts("infinite weapon: Buster test failed");
        return 1;
    }
    if (!test_pistol_still_finite()) {
        puts("infinite weapon: finite pistol regression failed");
        return 2;
    }
    if (!test_pistol_uses_buster_fire_dna()) {
        puts("infinite weapon: pistol/Buster fire DNA mismatch");
        return 3;
    }
    puts("infinite weapon + pistol Buster fire DNA: PASS");
    return 0;
}
