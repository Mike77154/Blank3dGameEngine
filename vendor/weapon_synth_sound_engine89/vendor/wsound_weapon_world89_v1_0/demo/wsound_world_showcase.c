#include <stdio.h>
#include <string.h>

#include "wsounddna89.h"
#include "wsoundreceiver89.h"
#include "wsoundprojectile89.h"
#include "wsoundprop89.h"
#include "wsoundroom89.h"
#include "wsoundaction89.h"
#include "wsoundimpact89.h"
#include "wsoundricochet89.h"
#include "wsoundcombatbus89.h"

#define DEMO_RATE 44100U
#define DEMO_SECONDS 12U
#define DEMO_FRAMES (DEMO_RATE * DEMO_SECONDS)
#define DEMO_MAX_EVENTS 256U
#define DEMO_MAX_SHOTS 16U
#define DEMO_MAX_PROJECTILES 24U
#define DEMO_MAX_IMPACTS 32U
#define DEMO_MAX_RICOCHETS 12U
#define DEMO_MAX_MECH 24U
#define DEMO_PROP_CAP 40000U
#define DEMO_ROOM_CAP 10000U

#define DEMO_EV_SHOT 1U
#define DEMO_EV_PROJECTILE 2U
#define DEMO_EV_IMPACT 3U
#define DEMO_EV_RICOCHET 4U
#define DEMO_EV_PUMP 5U

#define DEMO_WEAPON_PISTOL 0U
#define DEMO_WEAPON_MACHINE 1U
#define DEMO_WEAPON_RIFLE 2U
#define DEMO_WEAPON_SHOTGUN 3U

typedef struct demo_event_s {
    wsound89_u32 frame;
    wsound89_u16 type;
    wsound89_u16 subtype;
    wsound89_u16 energy_q15;
    wsound89_u16 aux_q15;
    wsound89_u32 distance_cm;
    wsound89_i16 pan_q15;
    wsound89_u32 seed;
} demo_event;

typedef struct demo_shot_s {
    wsound89_u8 active;
    wsound89_u8 weapon;
    wsound89_u32 age;
    wsound89_u32 state;
    wsound89_i32 env;
    wsound89_i32 low;
    wsound89_i16 pan_q15;
    wsounddna89_shot dna;
    wsoundreceiver89_context receiver;
    wsoundprop89_context prop;
} demo_shot;

typedef struct demo_projectile_s {
    wsound89_u8 active;
    wsound89_i16 pan_q15;
    wsoundprojectile89_context synth;
} demo_projectile;

typedef struct demo_impact_s {
    wsound89_u8 active;
    wsound89_i16 pan_q15;
    wsoundimpact89_context synth;
} demo_impact;

typedef struct demo_ricochet_s {
    wsound89_u8 active;
    wsound89_i16 pan_q15;
    wsoundricochet89_context synth;
} demo_ricochet;

typedef struct demo_mech_s {
    wsound89_u8 active;
    wsound89_i16 pan_q15;
    wsoundimpact89_context synth;
} demo_mech;

typedef struct demo_action_s {
    wsound89_u8 active;
    wsound89_i16 pan_q15;
    wsoundaction89_context seq;
} demo_action;

static demo_event demo_events[DEMO_MAX_EVENTS];
static wsound89_u16 demo_event_count;
static demo_shot demo_shots[DEMO_MAX_SHOTS];
static demo_projectile demo_projectiles[DEMO_MAX_PROJECTILES];
static demo_impact demo_impacts[DEMO_MAX_IMPACTS];
static demo_ricochet demo_ricochets[DEMO_MAX_RICOCHETS];
static demo_mech demo_mechs[DEMO_MAX_MECH];
static demo_action demo_actions[DEMO_MAX_SHOTS];
static wsound89_i16 demo_prop_memory[DEMO_MAX_SHOTS][DEMO_PROP_CAP];
static wsound89_i16 demo_room_memory[DEMO_ROOM_CAP];
static wsound89_i16 demo_output[DEMO_FRAMES * 2U];
static wsounddna89_context demo_dna;
static wsoundroom89_context demo_room;
static wsoundcombatbus89_context demo_bus;

static wsound89_i16 demo_sat16(wsound89_i32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (wsound89_i16)v;
}

static wsound89_u32 demo_ms(wsound89_u32 ms)
{
    return (DEMO_RATE * ms) / 1000U;
}

static wsound89_i16 demo_noise(wsound89_u32 *state)
{
    *state = *state * 1664525U + 1013904223U;
    return (wsound89_i16)((*state >> 16) & 65535U);
}

static int demo_write_u16(FILE *f, wsound89_u16 v)
{
    if (fputc((int)(v & 255U), f) == EOF) return 0;
    if (fputc((int)((v >> 8) & 255U), f) == EOF) return 0;
    return 1;
}

static int demo_write_u32(FILE *f, wsound89_u32 v)
{
    if (fputc((int)(v & 255U), f) == EOF) return 0;
    if (fputc((int)((v >> 8) & 255U), f) == EOF) return 0;
    if (fputc((int)((v >> 16) & 255U), f) == EOF) return 0;
    if (fputc((int)((v >> 24) & 255U), f) == EOF) return 0;
    return 1;
}

static int demo_write_wav(const char *path)
{
    FILE *f;
    wsound89_u32 samples;
    wsound89_u32 bytes;
    wsound89_u32 i;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    samples = DEMO_FRAMES * 2U;
    bytes = samples * 2U;
    if (fwrite("RIFF", 1U, 4U, f) != 4U) return 0;
    if (!demo_write_u32(f, 36U + bytes)) return 0;
    if (fwrite("WAVEfmt ", 1U, 8U, f) != 8U) return 0;
    if (!demo_write_u32(f, 16U)) return 0;
    if (!demo_write_u16(f, 1U)) return 0;
    if (!demo_write_u16(f, 2U)) return 0;
    if (!demo_write_u32(f, DEMO_RATE)) return 0;
    if (!demo_write_u32(f, DEMO_RATE * 4U)) return 0;
    if (!demo_write_u16(f, 4U)) return 0;
    if (!demo_write_u16(f, 16U)) return 0;
    if (fwrite("data", 1U, 4U, f) != 4U) return 0;
    if (!demo_write_u32(f, bytes)) return 0;
    for (i = 0U; i < samples; ++i) if (!demo_write_u16(f, (wsound89_u16)demo_output[i])) return 0;
    return fclose(f) == 0;
}

static int demo_add_event(wsound89_u32 ms, wsound89_u16 type, wsound89_u16 subtype,
                          wsound89_u16 energy_q15, wsound89_u16 aux_q15,
                          wsound89_u32 distance_cm, wsound89_i16 pan_q15,
                          wsound89_u32 seed)
{
    demo_event *e;
    if (demo_event_count >= DEMO_MAX_EVENTS) return 0;
    e = &demo_events[demo_event_count++];
    e->frame = demo_ms(ms);
    e->type = type;
    e->subtype = subtype;
    e->energy_q15 = energy_q15;
    e->aux_q15 = aux_q15;
    e->distance_cm = distance_cm;
    e->pan_q15 = pan_q15;
    e->seed = seed;
    return 1;
}

static void demo_sort_events(void)
{
    wsound89_u16 i;
    wsound89_u16 j;
    demo_event key;
    for (i = 1U; i < demo_event_count; ++i) {
        key = demo_events[i];
        j = i;
        while (j > 0U && demo_events[j - 1U].frame > key.frame) {
            demo_events[j] = demo_events[j - 1U];
            --j;
        }
        demo_events[j] = key;
    }
}

static void demo_add_weapon_event(wsound89_u32 ms, wsound89_u16 weapon,
                                  wsound89_u32 distance_cm, wsound89_i16 pan,
                                  wsound89_u16 energy, wsound89_u16 material,
                                  wsound89_u16 ricochet, wsound89_u32 seed)
{
    wsound89_u16 projectile;
    wsound89_u32 flight_ms;
    if (weapon == DEMO_WEAPON_SHOTGUN) projectile = WSOUNDPROJECTILE89_PELLET_SWARM;
    else if (weapon == DEMO_WEAPON_PISTOL) projectile = WSOUNDPROJECTILE89_NEAR_MISS_SNAP;
    else projectile = WSOUNDPROJECTILE89_SUPERSONIC_NWAVE;
    demo_add_event(ms, DEMO_EV_SHOT, weapon, energy, 0U, distance_cm, pan, seed);
    demo_add_event(ms + 9U + (seed & 7U), DEMO_EV_PROJECTILE, projectile,
                   (wsound89_u16)(energy * 3U / 4U), 27000U, 0U,
                   (wsound89_i16)(-pan / 2), seed + 1U);
    flight_ms = 65U + (distance_cm / 180U);
    demo_add_event(ms + flight_ms, DEMO_EV_IMPACT, material,
                   (wsound89_u16)(energy * 3U / 5U), 17000U, 0U,
                   (wsound89_i16)(pan / 2), seed + 2U);
    if (ricochet != 0U) {
        demo_add_event(ms + flight_ms + 6U, DEMO_EV_RICOCHET, 0U,
                       (wsound89_u16)(energy / 2U), 25000U, 0U,
                       (wsound89_i16)(-pan), seed + 3U);
    }
    if (weapon == DEMO_WEAPON_SHOTGUN) {
        demo_add_event(ms + 520U, DEMO_EV_PUMP, WSOUNDACTION89_PUMP_SHOTGUN,
                       28000U, 0U, 0U, pan, seed + 4U);
    }
}

static void demo_build_schedule(void)
{
    wsound89_u16 i;
    wsound89_u32 state;
    wsound89_u32 t;
    wsound89_u16 weapon;
    wsound89_i16 pan;
    wsound89_u32 distance;
    wsound89_u16 material;
    demo_event_count = 0U;
    demo_add_weapon_event(350U, DEMO_WEAPON_PISTOL, 500U, -18000, 25000U,
                          WSOUNDIMPACT89_CONCRETE, 0U, 100U);
    demo_add_weapon_event(1250U, DEMO_WEAPON_RIFLE, 8500U, 22000, 29000U,
                          WSOUNDIMPACT89_METAL, 1U, 200U);
    demo_add_weapon_event(2350U, DEMO_WEAPON_SHOTGUN, 320U, 0, 31000U,
                          WSOUNDIMPACT89_WOOD, 0U, 300U);
    for (i = 0U; i < 12U; ++i) {
        demo_add_weapon_event(3900U + (wsound89_u32)i * 82U,
                              DEMO_WEAPON_MACHINE, 900U + (wsound89_u32)i * 45U,
                              (wsound89_i16)(-23000 + (wsound89_i32)i * 3600),
                              24500U, (i & 1U) ? WSOUNDIMPACT89_METAL : WSOUNDIMPACT89_CONCRETE,
                              (wsound89_u16)((i % 4U) == 0U), 500U + i * 11U);
    }
    state = 0x46495245U;
    t = 6500U;
    for (i = 0U; i < 52U; ++i) {
        state = state * 1664525U + 1013904223U;
        weapon = (wsound89_u16)((state >> 28) & 3U);
        state = state * 1664525U + 1013904223U;
        pan = (wsound89_i16)((wsound89_i32)((state >> 16) & 65535U) - 32768);
        state = state * 1664525U + 1013904223U;
        distance = 300U + ((state >> 16) % 12000U);
        material = (wsound89_u16)((state >> 9) % 5U);
        demo_add_weapon_event(t, weapon, distance, pan,
                              (wsound89_u16)(22000U + ((state >> 20) & 7000U)),
                              material, (wsound89_u16)((state & 7U) == 0U), state);
        t += 38U + ((state >> 24) & 55U);
        if (t > 10300U) t = 6500U + (i * 71U);
    }
    demo_sort_events();
}

static wsoundreceiver89_preset demo_receiver_preset(wsound89_u16 weapon)
{
    if (weapon == DEMO_WEAPON_PISTOL) return WSOUNDRECEIVER89_PISTOL_STEEL;
    if (weapon == DEMO_WEAPON_MACHINE) return WSOUNDRECEIVER89_POLYMER;
    if (weapon == DEMO_WEAPON_RIFLE) return WSOUNDRECEIVER89_RIFLE_STEEL;
    return WSOUNDRECEIVER89_SHOTGUN_WOOD;
}

static wsounddna89_class demo_dna_class(wsound89_u16 weapon)
{
    if (weapon == DEMO_WEAPON_PISTOL) return WSOUNDDNA89_PISTOL;
    if (weapon == DEMO_WEAPON_MACHINE) return WSOUNDDNA89_MACHINE;
    if (weapon == DEMO_WEAPON_RIFLE) return WSOUNDDNA89_RIFLE;
    return WSOUNDDNA89_SHOTGUN;
}

static wsoundaction89_type demo_action_type(wsound89_u16 weapon)
{
    if (weapon == DEMO_WEAPON_PISTOL) return WSOUNDACTION89_PISTOL;
    if (weapon == DEMO_WEAPON_MACHINE) return WSOUNDACTION89_MACHINE;
    if (weapon == DEMO_WEAPON_RIFLE) return WSOUNDACTION89_RIFLE;
    return WSOUNDACTION89_SEMI_SHOTGUN;
}

static void demo_start_action(wsoundaction89_type type, wsound89_u32 speed_q16,
                              wsound89_i16 pan)
{
    wsound89_u16 i;
    for (i = 0U; i < DEMO_MAX_SHOTS; ++i) {
        if (demo_actions[i].active == 0U) {
            wsoundaction89_init(&demo_actions[i].seq, DEMO_RATE);
            wsoundaction89_trigger(&demo_actions[i].seq, type, speed_q16);
            demo_actions[i].active = 1U;
            demo_actions[i].pan_q15 = pan;
            return;
        }
    }
}

static void demo_start_shot(const demo_event *e)
{
    wsound89_u16 i;
    demo_shot *v;
    for (i = 0U; i < DEMO_MAX_SHOTS; ++i) {
        if (demo_shots[i].active == 0U) {
            v = &demo_shots[i];
            memset(v, 0, sizeof(*v));
            v->active = 1U;
            v->weapon = (wsound89_u8)e->subtype;
            v->age = 0U;
            v->state = e->seed;
            v->pan_q15 = e->pan_q15;
            wsounddna89_next(&demo_dna, demo_dna_class(e->subtype), e->energy_q15, &v->dna);
            v->env = v->dna.pressure_q15;
            wsoundreceiver89_init(&v->receiver, demo_receiver_preset(e->subtype));
            wsoundreceiver89_excite(&v->receiver, (wsound89_i16)v->dna.receiver_q15);
            wsoundprop89_init(&v->prop, DEMO_RATE, demo_prop_memory[i], DEMO_PROP_CAP);
            wsoundprop89_set_path(&v->prop, e->distance_cm, 34300U,
                                  e->pan_q15 > 0 ? 22000 : 12000, 32767U);
            demo_start_action(demo_action_type(e->subtype), v->dna.cycle_q16, e->pan_q15);
            return;
        }
    }
}

static void demo_start_projectile(const demo_event *e)
{
    wsound89_u16 i;
    for (i = 0U; i < DEMO_MAX_PROJECTILES; ++i) {
        if (demo_projectiles[i].active == 0U) {
            wsoundprojectile89_init(&demo_projectiles[i].synth, DEMO_RATE, e->seed);
            wsoundprojectile89_trigger(&demo_projectiles[i].synth,
                                       (wsoundprojectile89_mode)e->subtype,
                                       e->energy_q15, e->aux_q15);
            demo_projectiles[i].pan_q15 = e->pan_q15;
            demo_projectiles[i].active = 1U;
            return;
        }
    }
}

static void demo_start_impact(const demo_event *e, int mechanism)
{
    wsound89_u16 i;
    if (mechanism) {
        for (i = 0U; i < DEMO_MAX_MECH; ++i) {
            if (demo_mechs[i].active == 0U) {
                wsoundimpact89_init(&demo_mechs[i].synth, e->seed);
                wsoundimpact89_trigger(&demo_mechs[i].synth, WSOUNDIMPACT89_METAL,
                                       e->energy_q15, 9000U);
                demo_mechs[i].pan_q15 = e->pan_q15;
                demo_mechs[i].active = 1U;
                return;
            }
        }
    } else {
        for (i = 0U; i < DEMO_MAX_IMPACTS; ++i) {
            if (demo_impacts[i].active == 0U) {
                wsoundimpact89_init(&demo_impacts[i].synth, e->seed);
                wsoundimpact89_trigger(&demo_impacts[i].synth,
                                       (wsoundimpact89_material)e->subtype,
                                       e->energy_q15, e->aux_q15);
                demo_impacts[i].pan_q15 = e->pan_q15;
                demo_impacts[i].active = 1U;
                return;
            }
        }
    }
}

static void demo_start_ricochet(const demo_event *e)
{
    wsound89_u16 i;
    for (i = 0U; i < DEMO_MAX_RICOCHETS; ++i) {
        if (demo_ricochets[i].active == 0U) {
            wsoundricochet89_init(&demo_ricochets[i].synth, e->seed);
            wsoundricochet89_trigger(&demo_ricochets[i].synth, e->energy_q15,
                                     e->aux_q15, 12000U, DEMO_RATE);
            demo_ricochets[i].pan_q15 = e->pan_q15;
            demo_ricochets[i].active = 1U;
            return;
        }
    }
}

static void demo_start_event(const demo_event *e)
{
    if (e->type == DEMO_EV_SHOT) demo_start_shot(e);
    else if (e->type == DEMO_EV_PROJECTILE) demo_start_projectile(e);
    else if (e->type == DEMO_EV_IMPACT) demo_start_impact(e, 0);
    else if (e->type == DEMO_EV_RICOCHET) demo_start_ricochet(e);
    else if (e->type == DEMO_EV_PUMP) demo_start_action((wsoundaction89_type)e->subtype, 65536U, e->pan_q15);
}

static void demo_pan_add(wsound89_i32 sample, wsound89_i16 pan,
                         wsound89_i32 *left, wsound89_i32 *right)
{
    wsound89_i32 gl;
    wsound89_i32 gr;
    gl = 32767 - pan;
    gr = 32767 + pan;
    *left += (sample * gl) >> 16;
    *right += (sample * gr) >> 16;
}

static void demo_zero_frame(wsoundcombatbus89_frame *f)
{
    wsound89_u16 i;
    for (i = 0U; i < WSOUNDCOMBATBUS89_BUSES; ++i) {
        f->left[i] = 0;
        f->right[i] = 0;
    }
}

static void demo_set_bus(wsoundcombatbus89_frame *f, wsound89_u16 bus,
                         wsound89_i32 left, wsound89_i32 right)
{
    f->left[bus] = demo_sat16(left);
    f->right[bus] = demo_sat16(right);
}

static int demo_render(void)
{
    wsound89_u32 frame;
    wsound89_u16 next_event;
    wsound89_u16 i;
    wsound89_i16 n;
    wsound89_i16 s;
    wsound89_i16 room_l;
    wsound89_i16 room_r;
    wsound89_i16 out_l;
    wsound89_i16 out_r;
    wsound89_i32 sample;
    wsound89_i32 report_l;
    wsound89_i32 report_r;
    wsound89_i32 projectile_l;
    wsound89_i32 projectile_r;
    wsound89_i32 impact_l;
    wsound89_i32 impact_r;
    wsound89_i32 rico_l;
    wsound89_i32 rico_r;
    wsound89_i32 mech_l;
    wsound89_i32 mech_r;
    wsound89_i32 room_in;
    wsoundcombatbus89_frame buses;
    wsoundaction89_event aev[WSOUNDACTION89_MAX_EVENTS];
    wsound89_u16 written;
    demo_event fake;
    memset(demo_shots, 0, sizeof(demo_shots));
    memset(demo_projectiles, 0, sizeof(demo_projectiles));
    memset(demo_impacts, 0, sizeof(demo_impacts));
    memset(demo_ricochets, 0, sizeof(demo_ricochets));
    memset(demo_mechs, 0, sizeof(demo_mechs));
    memset(demo_actions, 0, sizeof(demo_actions));
    wsounddna89_init(&demo_dna, 0x574F524CU);
    if (wsoundroom89_init(&demo_room, DEMO_RATE, WSOUNDROOM89_WAREHOUSE,
                          demo_room_memory, DEMO_ROOM_CAP) != WSOUND89_OK) return 0;
    wsoundcombatbus89_init(&demo_bus);
    next_event = 0U;
    for (frame = 0U; frame < DEMO_FRAMES; ++frame) {
        while (next_event < demo_event_count && demo_events[next_event].frame == frame) {
            demo_start_event(&demo_events[next_event]);
            next_event++;
        }
        report_l = 0; report_r = 0;
        projectile_l = 0; projectile_r = 0;
        impact_l = 0; impact_r = 0;
        rico_l = 0; rico_r = 0;
        mech_l = 0; mech_r = 0;
        for (i = 0U; i < DEMO_MAX_SHOTS; ++i) {
            demo_shot *v;
            wsound89_i32 dry;
            wsound89_i32 body;
            v = &demo_shots[i];
            if (v->active == 0U) continue;
            n = demo_noise(&v->state);
            v->low += ((wsound89_i32)n - v->low) >> 3;
            dry = ((wsound89_i32)n - v->low) * v->env >> 15;
            body = (v->low * v->dna.gas_q15) >> 16;
            if (v->age == 0U) dry += v->dna.pressure_q15;
            if (v->weapon == DEMO_WEAPON_SHOTGUN) body += v->env >> 2;
            s = wsoundreceiver89_process_sample(&v->receiver, demo_sat16((dry + body) >> 1));
            sample = dry + body + s;
            s = wsoundprop89_process_sample(&v->prop, demo_sat16(sample));
            demo_pan_add(s, v->pan_q15, &report_l, &report_r);
            v->env -= (v->env >> 7) + (v->weapon == DEMO_WEAPON_SHOTGUN ? 30 : 55);
            v->age++;
            if (v->env < 16 && v->age > demo_ms(900U)) v->active = 0U;
        }
        for (i = 0U; i < DEMO_MAX_PROJECTILES; ++i) {
            if (demo_projectiles[i].active == 0U) continue;
            s = wsoundprojectile89_process_sample(&demo_projectiles[i].synth);
            demo_pan_add(s, demo_projectiles[i].pan_q15, &projectile_l, &projectile_r);
            if (!wsoundprojectile89_is_active(&demo_projectiles[i].synth)) demo_projectiles[i].active = 0U;
        }
        for (i = 0U; i < DEMO_MAX_IMPACTS; ++i) {
            if (demo_impacts[i].active == 0U) continue;
            s = wsoundimpact89_process_sample(&demo_impacts[i].synth);
            demo_pan_add(s, demo_impacts[i].pan_q15, &impact_l, &impact_r);
            if (!wsoundimpact89_is_active(&demo_impacts[i].synth)) demo_impacts[i].active = 0U;
        }
        for (i = 0U; i < DEMO_MAX_RICOCHETS; ++i) {
            if (demo_ricochets[i].active == 0U) continue;
            s = wsoundricochet89_process_sample(&demo_ricochets[i].synth);
            demo_pan_add(s, demo_ricochets[i].pan_q15, &rico_l, &rico_r);
            if (!wsoundricochet89_is_active(&demo_ricochets[i].synth)) demo_ricochets[i].active = 0U;
        }
        for (i = 0U; i < DEMO_MAX_MECH; ++i) {
            if (demo_mechs[i].active == 0U) continue;
            s = wsoundimpact89_process_sample(&demo_mechs[i].synth);
            demo_pan_add(s, demo_mechs[i].pan_q15, &mech_l, &mech_r);
            if (!wsoundimpact89_is_active(&demo_mechs[i].synth)) demo_mechs[i].active = 0U;
        }
        for (i = 0U; i < DEMO_MAX_SHOTS; ++i) {
            if (demo_actions[i].active == 0U) continue;
            written = 0U;
            wsoundaction89_advance(&demo_actions[i].seq, 1U, aev, WSOUNDACTION89_MAX_EVENTS, &written);
            if (written != 0U) {
                memset(&fake, 0, sizeof(fake));
                fake.energy_q15 = aev[0].energy_q15;
                fake.pan_q15 = demo_actions[i].pan_q15;
                fake.seed = frame + i * 97U + aev[0].code;
                demo_start_impact(&fake, 1);
            }
            if (!wsoundaction89_is_active(&demo_actions[i].seq)) demo_actions[i].active = 0U;
        }
        room_in = (report_l + report_r + projectile_l + projectile_r + impact_l + impact_r + rico_l + rico_r + mech_l + mech_r) / 16;
        wsoundroom89_process_sample(&demo_room, demo_sat16(room_in), &room_l, &room_r);
        demo_zero_frame(&buses);
        demo_set_bus(&buses, WSOUNDCOMBATBUS89_REPORT, report_l, report_r);
        demo_set_bus(&buses, WSOUNDCOMBATBUS89_PROJECTILE, projectile_l, projectile_r);
        demo_set_bus(&buses, WSOUNDCOMBATBUS89_MECHANISM, mech_l, mech_r);
        demo_set_bus(&buses, WSOUNDCOMBATBUS89_IMPACT, impact_l, impact_r);
        demo_set_bus(&buses, WSOUNDCOMBATBUS89_RICOCHET, rico_l, rico_r);
        buses.left[WSOUNDCOMBATBUS89_ROOM] = room_l;
        buses.right[WSOUNDCOMBATBUS89_ROOM] = room_r;
        wsoundcombatbus89_process_sample(&demo_bus, &buses, &out_l, &out_r);
        demo_output[frame * 2U] = out_l;
        demo_output[frame * 2U + 1U] = out_r;
    }
    return 1;
}

int main(void)
{
    demo_build_schedule();
    if (!demo_render()) return 1;
    if (!demo_write_wav("audio/wsound_world_all_modules_showcase.wav")) return 2;
    printf("events=%u frames=%u bytes=%u\n", (unsigned)demo_event_count,
           (unsigned)DEMO_FRAMES, (unsigned)sizeof(demo_output));
    return 0;
}
