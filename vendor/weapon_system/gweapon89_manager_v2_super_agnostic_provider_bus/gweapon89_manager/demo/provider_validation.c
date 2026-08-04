#include "../src/gweapon89.h"

#include <stdio.h>
#include <string.h>

typedef struct TestHostTag {
    int reserve;
    int clip;
    int math_calls;
    int transform_calls;
    int camera_calls;
    int socket_calls;
    int raycast_calls;
    int numeric_calls;
    int flag_calls;
    int projectile_calls;
    int projectile_life_calls;
    int health_calls;
    int muzzle_calls;
    int casing_calls;
    int trail_calls;
    int hud_calls;
    int crosshair_calls;
    int scope_calls;
    int zoom_calls;
    int draw_calls;
    int reload_calls;
    int active_reload_calls;
    int actor_state_calls;
    int actor_state_mutation_seen;
    int last_life_ms;
    int last_damage;
} TestHost;

static int test_inventory(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    if (p->operation == GWP89_OP_AMMO_QUERY) {
        p->i_value = h->reserve;
        return GWP89_PROVIDER_HANDLED;
    }
    if (p->operation == GWP89_OP_AMMO_SET) {
        h->reserve = p->i_value;
        return GWP89_PROVIDER_HANDLED;
    }
    if (p->operation == GWP89_OP_AMMO_ADD) {
        h->reserve += p->amount;
        if (h->reserve < 0) h->reserve = 0;
        p->i_value = h->reserve;
        return GWP89_PROVIDER_HANDLED;
    }
    if (p->operation == GWP89_OP_AMMO_CONSUME) {
        if (h->reserve >= p->amount) {
            h->reserve -= p->amount;
            p->result_code = GWP89_OK;
        } else {
            p->result_code = GWP89_NO_AMMO;
        }
        return GWP89_PROVIDER_HANDLED;
    }
    if (p->operation == GWP89_OP_CLIP_QUERY) {
        p->i_value = h->clip;
        return GWP89_PROVIDER_HANDLED;
    }
    if (p->operation == GWP89_OP_CLIP_SET) {
        h->clip = p->i_value;
        return GWP89_PROVIDER_HANDLED;
    }
    return GWP89_PROVIDER_PASS;
}

static int test_numeric(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    h->numeric_calls++;
    if (p->operation == GWP89_OP_QUERY_INT && p->key == GWP89_NUM_PELLET_COUNT) {
        p->i_value = 3;
        return GWP89_PROVIDER_MODIFIED;
    }
    if (p->operation == GWP89_OP_QUERY_FX && p->key == GWP89_NUM_DAMAGE_FX) {
        p->fx_value = gwp89_fx_from_int(15);
        return GWP89_PROVIDER_MODIFIED;
    }
    return GWP89_PROVIDER_PASS;
}

static int test_flags(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    h->flag_calls++;
    if (p->operation == GWP89_OP_FIRE_VALIDATE && p->input && (p->input->input_flags & 0x4000)) {
        h->actor_state_mutation_seen = 1;
    }
    if (p->operation == GWP89_OP_QUERY_FLAG && p->key == GWP89_FLAG_ACTIVE_RELOAD_ENABLED) {
        p->i_value = 1;
        return GWP89_PROVIDER_MODIFIED;
    }
    return GWP89_PROVIDER_PASS;
}

static int test_actor_state(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    if (p->operation != GWP89_OP_UPDATE) return GWP89_PROVIDER_PASS;
    h->actor_state_calls++;
    if (p->input) p->input->input_flags |= 0x4000;
    return GWP89_PROVIDER_MODIFIED;
}

static int test_math(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    h->math_calls++;
    if (p->operation == GWP89_OP_MATH_SCALE) {
        p->vec_c.x = (p->vec_a.x * p->fx_value) >> GWP89_FIX_SHIFT;
        p->vec_c.y = (p->vec_a.y * p->fx_value) >> GWP89_FIX_SHIFT;
        p->vec_c.z = (p->vec_a.z * p->fx_value) >> GWP89_FIX_SHIFT;
        return GWP89_PROVIDER_HANDLED;
    }
    if (p->operation == GWP89_OP_MATH_ADD) {
        p->vec_c.x = p->vec_a.x + p->vec_b.x;
        p->vec_c.y = p->vec_a.y + p->vec_b.y;
        p->vec_c.z = p->vec_a.z + p->vec_b.z;
        return GWP89_PROVIDER_HANDLED;
    }
    return GWP89_PROVIDER_PASS;
}

static int test_transform(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    if (p->operation != GWP89_OP_GET_ACTOR_TRANSFORM) return GWP89_PROVIDER_PASS;
    h->transform_calls++;
    p->transform.position = gwp89_v3(gwp89_fx_from_int(10), gwp89_fx_from_int(2), gwp89_fx_from_int(3));
    p->transform.forward = gwp89_v3(GWP89_FIX_ONE, 0L, 0L);
    p->transform.right = gwp89_v3(0L, GWP89_FIX_ONE, 0L);
    p->transform.up = gwp89_v3(0L, 0L, GWP89_FIX_ONE);
    p->transform.scale = gwp89_v3(GWP89_FIX_ONE, GWP89_FIX_ONE, GWP89_FIX_ONE);
    return GWP89_PROVIDER_HANDLED;
}

static int test_camera(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE || p->operation != GWP89_OP_GET_CAMERA) return GWP89_PROVIDER_PASS;
    h->camera_calls++;
    p->camera.valid = 1;
    p->camera.camera_id = 9;
    p->camera.view_style = GWP89_VIEW_OVER_SHOULDER;
    p->camera.origin = gwp89_v3(gwp89_fx_from_int(8), gwp89_fx_from_int(2), gwp89_fx_from_int(3));
    p->camera.forward = gwp89_v3(GWP89_FIX_ONE, 0L, 0L);
    return GWP89_PROVIDER_HANDLED;
}

static int test_sockets(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE || p->operation != GWP89_OP_GET_SOCKET) return GWP89_PROVIDER_PASS;
    h->socket_calls++;
    p->transform.position = gwp89_v3(gwp89_fx_from_int(10 + p->socket_kind), gwp89_fx_from_int(2), gwp89_fx_from_int(3));
    p->transform.forward = gwp89_v3(GWP89_FIX_ONE, 0L, 0L);
    p->transform.right = gwp89_v3(0L, GWP89_FIX_ONE, 0L);
    p->transform.up = gwp89_v3(0L, 0L, GWP89_FIX_ONE);
    return GWP89_PROVIDER_HANDLED;
}

static int test_raycast(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE || p->operation != GWP89_OP_RAYCAST) return GWP89_PROVIDER_PASS;
    h->raycast_calls++;
    p->hit.hit = 1;
    p->hit.actor_id = 900;
    p->hit.point = gwp89_v3(gwp89_fx_from_int(30), gwp89_fx_from_int(2), gwp89_fx_from_int(3));
    p->hit.distance_fx = gwp89_fx_from_int(20);
    return GWP89_PROVIDER_HANDLED;
}

static int test_event_sink(void *ctx, GWP89_ProviderPacket *p)
{
    TestHost *h;
    h = (TestHost *)ctx;
    if (!h || !p || p->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    if (p->operation == GWP89_OP_RELOAD_TICK || p->operation == GWP89_OP_RELOAD_BEGIN || p->operation == GWP89_OP_RELOAD_COMPLETE) {
        h->reload_calls++;
        return GWP89_PROVIDER_PASS;
    }
    if (p->operation == GWP89_OP_ACTIVE_RELOAD_PRESS) {
        h->active_reload_calls++;
        return GWP89_PROVIDER_PASS;
    }
    if (p->service == GWP89_SERVICE_ZOOM && p->operation == GWP89_OP_QUERY_FX) {
        p->fx_value = gwp89_fx_from_int(2);
        return GWP89_PROVIDER_MODIFIED;
    }
    if (p->operation != GWP89_OP_EMIT_EVENT && p->operation != GWP89_OP_PROJECTILE_LIFE && p->operation != GWP89_OP_DAMAGE_REQUEST) return GWP89_PROVIDER_PASS;
    if (!p->event) return GWP89_PROVIDER_PASS;
    switch (p->service) {
        case GWP89_SERVICE_PROJECTILE_LIFE:
            h->projectile_life_calls++;
            p->event->life_ms = 777u;
            return GWP89_PROVIDER_MODIFIED;
        case GWP89_SERVICE_PROJECTILE:
            h->projectile_calls++;
            h->last_life_ms = p->event->life_ms;
            h->last_damage = gwp89_fx_to_int_round(p->event->damage_fx);
            return GWP89_PROVIDER_HANDLED;
        case GWP89_SERVICE_HEALTH: h->health_calls++; break;
        case GWP89_SERVICE_MUZZLE: h->muzzle_calls++; break;
        case GWP89_SERVICE_CASING: h->casing_calls++; break;
        case GWP89_SERVICE_TRAIL: h->trail_calls++; break;
        case GWP89_SERVICE_HUD: h->hud_calls++; break;
        case GWP89_SERVICE_CROSSHAIR: h->crosshair_calls++; break;
        case GWP89_SERVICE_SCOPE: h->scope_calls++; break;
        case GWP89_SERVICE_ZOOM: h->zoom_calls++; break;
        case GWP89_SERVICE_DRAW: h->draw_calls++; break;
        case GWP89_SERVICE_ACTIVE_RELOAD: h->active_reload_calls++; break;
    }
    return GWP89_PROVIDER_PASS;
}

static int test_noop(void *ctx, GWP89_ProviderPacket *p)
{
    (void)ctx;
    (void)p;
    return GWP89_PROVIDER_PASS;
}

static int expect_true(int condition, const char *message)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    GWP89_Manager manager;
    GWP89_WeaponProfile weapon;
    GWP89_FireInput input;
    GWP89_Event event;
    GWP89_Manager handle_manager;
    TestHost host;
    int ok;
    int projectile_events;
    int low_handle;
    int high_handle;

    memset(&host, 0, sizeof(host));
    gwp89_init(&manager);
    gwp89_profile_defaults(&weapon);
    gwp89_copy_id(weapon.name, GWP89_NAME_MAX, "provider_rifle");
    weapon.weapon_id = 77;
    weapon.clip_size = 4;
    weapon.reload_ms = 1000u;
    weapon.active_reload_enabled = 1;
    weapon.active_reload_window_start_ms = 200u;
    weapon.active_reload_window_end_ms = 500u;
    weapon.active_reload_bonus_ms = 500u;
    weapon.active_reload_penalty_ms = 300u;
    gwp89_add_weapon(&manager, &weapon);

    gwp89_add_provider(&manager, GWP89_SERVICE_INVENTORY, 100, "inventory", &host, test_inventory);
    gwp89_add_provider(&manager, GWP89_SERVICE_NUMERIC, 90, "numeric", &host, test_numeric);
    gwp89_add_provider(&manager, GWP89_SERVICE_FLAGS, 90, "flags", &host, test_flags);
    gwp89_add_provider(&manager, GWP89_SERVICE_MATH3D, 80, "math", &host, test_math);
    gwp89_add_provider(&manager, GWP89_SERVICE_TRANSFORM, 80, "transform", &host, test_transform);
    gwp89_add_provider(&manager, GWP89_SERVICE_CAMERA, 80, "camera", &host, test_camera);
    gwp89_add_provider(&manager, GWP89_SERVICE_SOCKETS, 80, "sockets", &host, test_sockets);
    gwp89_add_provider(&manager, GWP89_SERVICE_RAYCAST, 80, "raycast", &host, test_raycast);
    gwp89_add_provider(&manager, GWP89_SERVICE_PROJECTILE_LIFE, 70, "life", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_PROJECTILE, 60, "projectile", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_HEALTH, 60, "health", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_MUZZLE, 60, "muzzle", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_CASING, 60, "casing", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_TRAIL, 60, "trail", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_HUD, 40, "hud", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_CROSSHAIR, 40, "crosshair", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_SCOPE, 40, "scope", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_ZOOM, 40, "zoom", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_DRAW, 40, "draw", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_RELOAD, 40, "reload", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_ACTIVE_RELOAD, 40, "active_reload", &host, test_event_sink);
    gwp89_add_provider(&manager, GWP89_SERVICE_ACTOR_STATE, 100, "actor_state", &host, test_actor_state);

    gwp89_bind_actor(&manager, 5, 1, 2);
    gwp89_equip_name(&manager, 5, "provider_rifle", 1);
    gwp89_set_ammo(&manager, 5, weapon.ammo_id, 10);

    memset(&input, 0, sizeof(input));
    input.actor_id = 5;
    input.actor_kind = 1;
    input.team_id = 2;
    input.view_style = GWP89_VIEW_FPS;
    input.trigger_flags = GWP89_TRIGGER_PRESSED | GWP89_TRIGGER_DOWN;
    input.socket_forward = gwp89_v3(GWP89_FIX_ONE, 0L, 0L);
    input.socket_right = gwp89_v3(0L, GWP89_FIX_ONE, 0L);
    input.socket_up = gwp89_v3(0L, 0L, GWP89_FIX_ONE);

    ok = 1;
    ok &= expect_true(gwp89_try_fire(&manager, &input) == GWP89_OK, "providerized fire should succeed");
    ok &= expect_true(host.clip == 3, "external clip provider should own magazine state");
    ok &= expect_true(host.projectile_calls == 3, "numeric provider should change pellet count to three");
    ok &= expect_true(host.projectile_life_calls == 3, "projectile life provider should receive each projectile");
    ok &= expect_true(host.last_life_ms == 777, "projectile provider should see life provider mutation");
    ok &= expect_true(host.last_damage == 5, "damage provider value should split across pellets");
    ok &= expect_true(host.health_calls == 3, "raycast hit should fan out to health provider");
    ok &= expect_true(host.muzzle_calls == 1 && host.casing_calls == 1, "muzzle and casing providers should receive one event");
    ok &= expect_true(host.trail_calls == 3, "trail provider should receive every pellet");
    ok &= expect_true(host.transform_calls > 0 && host.camera_calls > 0 && host.socket_calls >= 9 && host.raycast_calls == 4, "pose providers should participate in every resolved pose");
    ok &= expect_true(host.hud_calls > 0 && host.crosshair_calls > 0 && host.scope_calls > 0 && host.zoom_calls > 0 && host.draw_calls > 0, "UI and drawing providers should receive fanout events");
    ok &= expect_true(host.actor_state_calls > 0, "actor-state provider should receive frame updates before weapon policy");
    ok &= expect_true(host.actor_state_mutation_seen, "actor-state input mutation should reach later fire providers");

    projectile_events = 0;
    while (gwp89_poll_event(&manager, &event)) {
        if (event.type == GWP89_EVENT_PROJECTILE_REQUEST) {
            projectile_events++;
            ok &= expect_true(event.life_ms == 777u, "queued projectile event should preserve provider mutation");
            ok &= expect_true((event.flags & GWP89_EVENT_FLAG_HIT_VALID) != 0, "raycast provider hit should be preserved");
            ok &= expect_true(event.zoom_fx == gwp89_fx_from_int(2), "zoom provider query should affect queued projectile events");
        }
    }
    ok &= expect_true(projectile_events == 3, "event queue should contain three projectile requests");

    gwp89_set_clip(&manager, 5, weapon.weapon_id, 0);
    ok &= expect_true(gwp89_begin_reload(&manager, 5) == GWP89_OK, "reload should begin");
    input.trigger_flags = GWP89_TRIGGER_NONE;
    input.dt_ms = 250u;
    gwp89_update_actor(&manager, &input);
    ok &= expect_true(gwp89_active_reload_press(&manager, 5) == GWP89_OK, "active reload should succeed inside window");
    gwp89_update_actor(&manager, &input);
    ok &= expect_true(host.clip == 4 && host.reserve == 6, "reload completion should use external inventory provider");
    ok &= expect_true(host.reload_calls > 0 && host.active_reload_calls > 0, "reload providers should observe lifecycle and active input");

    gwp89_init(&handle_manager);
    low_handle = gwp89_add_provider(&handle_manager, GWP89_SERVICE_HUD, 1, "low", 0, test_noop);
    high_handle = gwp89_add_provider(&handle_manager, GWP89_SERVICE_HUD, 100, "high", 0, test_noop);
    ok &= expect_true(low_handle >= 0 && high_handle >= 0 && low_handle != high_handle, "provider handles should be distinct");
    ok &= expect_true(gwp89_remove_provider(&handle_manager, low_handle) == GWP89_OK, "low-priority stable handle should remain removable after higher-priority registration");
    ok &= expect_true(gwp89_provider_count(&handle_manager, GWP89_SERVICE_HUD) == 1, "removing one stable handle should preserve the other provider");
    ok &= expect_true(gwp89_remove_provider(&handle_manager, high_handle) == GWP89_OK, "high-priority provider handle should remain valid");

    if (!ok) return 1;
    printf("PASS: providerized gweapon89 validated (%d providers, %d HUD events, %d draw events)\n", manager.provider_count, host.hud_calls, host.draw_calls);
    return 0;
}
