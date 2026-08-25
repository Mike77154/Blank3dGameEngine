#include <stdio.h>
#include <string.h>
#include "blank3d_weapon_ini.h"
#include "blank3d_weapon_modules.h"
#include "blank3d_weapon_host_io.h"
#include "gtrigger89.h"
#include "morethanone89.h"

#define ACTOR_ID 77
#define BUSTER_AMMO_ID 14
#define BUSTER_BASE_PROJECTILE_ID 15

static void make_input(GWP89_FireInput *in, int flags, unsigned short dt)
{
    memset(in, 0, sizeof(*in));
    in->actor_id = ACTOR_ID;
    in->actor_kind = 1;
    in->team_id = 1;
    in->view_style = GWP89_VIEW_THIRD_PERSON;
    in->dt_ms = dt;
    in->trigger_flags = flags;
    in->socket_forward.z = GWP89_FIX_ONE;
    in->camera_forward.z = GWP89_FIX_ONE;
    in->zoom_fx = GWP89_FIX_ONE;
}

static void apply_stage(GWP89_Event *ev, const mto89_result *charged)
{
    const mto89_stage *s;
    if (!ev || !charged || !charged->active) return;
    s = &charged->stage;
    if (s->override_mask & MTO89_OVERRIDE_PROJECTILE_ID)
        ev->projectile_id = s->projectile_id;
    if (s->override_mask & MTO89_OVERRIDE_PROJECTILE_MESH_ID)
        ev->projectile_mesh_id = s->projectile_mesh_id;
    if (s->override_mask & MTO89_OVERRIDE_DAMAGE_Q16)
        ev->damage_fx = (gwp89_fx)(s->damage_q16 >> 4);
    if (s->override_mask & MTO89_OVERRIDE_SPEED_Q16)
        ev->speed_fx = (gwp89_fx)(s->speed_q16 >> 4);
    if (s->override_mask & MTO89_OVERRIDE_RADIUS_Q16)
        ev->radius_fx = (gwp89_fx)(s->radius_q16 >> 4);
    if (s->override_mask & MTO89_OVERRIDE_MESH_SCALE_Q16)
        ev->projectile_mesh_scale_fx = (gwp89_fx)(s->mesh_scale_q16 >> 4);
    if (s->override_mask & MTO89_OVERRIDE_LIFE_MS)
        ev->life_ms = s->life_ms;
}

static int drain_projectiles(GWP89_Manager *m, GWP89_Event *events, int cap,
                             const mto89_result *charged)
{
    GWP89_Event ev;
    int count;
    count = 0;
    while (gwp89_poll_event(m, &ev)) {
        if (ev.type == GWP89_EVENT_PROJECTILE_REQUEST && count < cap) {
            apply_stage(&ev, charged);
            events[count++] = ev;
        }
    }
    return count;
}

static int run_gesture(GWP89_Manager *m, gtrigger89_state *ts,
                       const mto89_profile *profile, unsigned int hold_ms,
                       GWP89_Event *events, int cap)
{
    gtrigger89_config cfg;
    gtrigger89_output out;
    GWP89_FireInput in;
    mto89_result selected;
    unsigned int elapsed;
    unsigned int step;
    int count;
    int flags;

    memset(&cfg, 0, sizeof(cfg));
    cfg.model = GTRIGGER89_MODEL_PRESS_CHARGE_RELEASE;
    cfg.charge_max_ms = 1500U;
    memset(&selected, 0, sizeof(selected));
    count = 0;

    step = hold_ms > 16U ? 16U : hold_ms;
    if (step == 0U) step = 1U;
    gtrigger89_update(ts, &cfg, 1, step, &out);
    flags = 0;
    if (out.trigger_down) flags |= GWP89_TRIGGER_DOWN;
    if (out.trigger_pressed) flags |= GWP89_TRIGGER_PRESSED;
    make_input(&in, flags, (unsigned short)step);
    (void)gwp89_try_fire(m, &in);
    count += drain_projectiles(m, events + count, cap - count, 0);
    elapsed = step;

    while (elapsed < hold_ms) {
        step = hold_ms - elapsed;
        if (step > 50U) step = 50U;
        gtrigger89_update(ts, &cfg, 1, step, &out);
        flags = out.trigger_down ? GWP89_TRIGGER_DOWN : 0;
        make_input(&in, flags, (unsigned short)step);
        if (flags) (void)gwp89_try_fire(m, &in);
        else (void)gwp89_update_actor(m, &in);
        (void)drain_projectiles(m, events + count, cap - count, 0);
        elapsed += step;
    }

    gtrigger89_update(ts, &cfg, 0, 0U, &out);
    if (out.charge_release && mto89_resolve(profile, out.charge_elapsed_ms,
                                            &selected)) {
        make_input(&in, GWP89_TRIGGER_DOWN | GWP89_TRIGGER_PRESSED |
                       GWP89_TRIGGER_RELEASED, 0U);
        (void)gwp89_try_fire(m, &in);
        count += drain_projectiles(m, events + count, cap - count, &selected);
    } else {
        make_input(&in, 0, 0U);
        (void)gwp89_update_actor(m, &in);
        (void)drain_projectiles(m, events + count, cap - count, 0);
    }
    return count;
}

static int check_case(unsigned int hold_ms, int expected_count,
                      int expected_release_projectile,
                      int expected_release_mesh,
                      long expected_damage_q16,
                      long expected_speed_q16,
                      long expected_radius_q16,
                      long expected_scale_q16,
                      unsigned short expected_life_ms)
{
    GWP89_Manager m;
    const Blank3DWeaponModules *mods;
    gtrigger89_state ts;
    GWP89_Event ev[4];
    int slot;
    int n;
    char status[160];

    memset(ev, 0, sizeof(ev));
    gwp89_init(&m);
    (void)blank3d_weapon_host_io_bind(&m);
    if (blank3d_weapon_ini_load_manifest(&m, "config/weapons/weapons.ini",
                                         status, sizeof(status)) != 15) {
        printf("manifest failed: %s\n", status);
        return 0;
    }
    slot = gwp89_find_weapon_slot_by_id(&m, B3D_WEAPON_ID_BUSTER);
    mods = blank3d_weapon_modules_get(B3D_WEAPON_ID_BUSTER);
    if (slot < 0 || !mods || !mods->more_than_one.enabled) return 0;
    if (gwp89_bind_actor(&m, ACTOR_ID, 1, 1) < 0) return 0;
    if (gwp89_equip_slot(&m, ACTOR_ID, slot, 1) != GWP89_OK) return 0;
    (void)gwp89_set_ammo(&m, ACTOR_ID, BUSTER_AMMO_ID, 999);
    gwp89_clear_events(&m);
    gtrigger89_init(&ts);

    n = run_gesture(&m, &ts, &mods->more_than_one, hold_ms, ev, 4);
    if (n != expected_count) {
        printf("hold=%u expected_count=%d got=%d\n",
               hold_ms, expected_count, n);
        return 0;
    }
    if (n < 1 || ev[0].projectile_id != BUSTER_BASE_PROJECTILE_ID) {
        printf("hold=%u base projectile=%d\n", hold_ms,
               n > 0 ? ev[0].projectile_id : -1);
        return 0;
    }
    if (expected_count == 1) return 1;

    if (ev[1].projectile_id != expected_release_projectile ||
        ev[1].projectile_mesh_id != expected_release_mesh ||
        ev[1].damage_fx != (gwp89_fx)(expected_damage_q16 >> 4) ||
        ev[1].speed_fx != (gwp89_fx)(expected_speed_q16 >> 4) ||
        ev[1].radius_fx != (gwp89_fx)(expected_radius_q16 >> 4) ||
        ev[1].projectile_mesh_scale_fx != (gwp89_fx)(expected_scale_q16 >> 4) ||
        ev[1].life_ms != expected_life_ms) {
        printf("hold=%u release mismatch pid=%d mesh=%d dmg=%ld speed=%ld radius=%ld scale=%ld life=%u\n",
               hold_ms, ev[1].projectile_id, ev[1].projectile_mesh_id,
               (long)ev[1].damage_fx, (long)ev[1].speed_fx,
               (long)ev[1].radius_fx, (long)ev[1].projectile_mesh_scale_fx,
               (unsigned int)ev[1].life_ms);
        return 0;
    }
    return 1;
}

static int check_rearm_after_charge(void)
{
    GWP89_Manager m;
    const Blank3DWeaponModules *mods;
    gtrigger89_state ts;
    GWP89_FireInput idle;
    GWP89_Event ev[4];
    int slot;
    int n;
    char status[160];

    gwp89_init(&m);
    (void)blank3d_weapon_host_io_bind(&m);
    if (blank3d_weapon_ini_load_manifest(&m, "config/weapons/weapons.ini",
                                         status, sizeof(status)) != 15)
        return 0;
    slot = gwp89_find_weapon_slot_by_id(&m, B3D_WEAPON_ID_BUSTER);
    mods = blank3d_weapon_modules_get(B3D_WEAPON_ID_BUSTER);
    if (slot < 0 || !mods) return 0;
    if (gwp89_bind_actor(&m, ACTOR_ID, 1, 1) < 0) return 0;
    if (gwp89_equip_slot(&m, ACTOR_ID, slot, 1) != GWP89_OK) return 0;
    (void)gwp89_set_ammo(&m, ACTOR_ID, BUSTER_AMMO_ID, 999);
    gwp89_clear_events(&m);
    gtrigger89_init(&ts);

    n = run_gesture(&m, &ts, &mods->more_than_one, 1200U, ev, 4);
    if (n != 2 || ev[0].projectile_id != 15 || ev[1].projectile_id != 18)
        return 0;

    /* Let only the ordinary weapon cooldown expire; no special Buster reset
       is allowed to repair GWeapon state between the charged shot and tap. */
    make_input(&idle, 0, 120U);
    if (gwp89_update_actor(&m, &idle) < 0) return 0;
    gwp89_clear_events(&m);
    gtrigger89_reset(&ts);
    memset(ev, 0, sizeof(ev));
    n = run_gesture(&m, &ts, &mods->more_than_one, 100U, ev, 4);
    return n == 1 && ev[0].projectile_id == 15;
}

int main(void)
{
    if (!check_case(100U, 1, 0, 0, 0L, 0L, 0L, 0L, 0U)) return 1;
    if (!check_case(300U, 2, 16, 15,
                    12L << 16, 54L << 16, 13107L, 47186L, 2800U)) return 2;
    if (!check_case(700U, 2, 17, 16,
                    26L << 16, 50L << 16, 22282L, 1L << 16, 3100U)) return 3;
    if (!check_case(1200U, 2, 18, 17,
                    52L << 16, 46L << 16, 34079L, 88474L, 3600U)) return 4;
    if (!check_rearm_after_charge()) return 5;

    puts("Buster morethanone89 integration: PASS");
    return 0;
}
