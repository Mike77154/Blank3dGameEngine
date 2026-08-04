#include "../src/gweapon89.h"

#include <stdio.h>
#include <string.h>

typedef struct LegacyHostTag {
    int projectile_count;
    int muzzle_count;
    int casing_count;
    int trail_count;
} LegacyHost;

static int legacy_pose(void *ctx, const GWP89_PoseRequest *req, GWP89_PoseResult *out_pose)
{
    (void)ctx;
    if (!req || !out_pose) return 0;
    out_pose->has_hit = 0;
    out_pose->projectile_origin = req->input.socket_origin;
    out_pose->muzzle_origin = req->input.socket_origin;
    out_pose->casing_origin = req->input.socket_origin;
    out_pose->direction = req->input.socket_forward;
    out_pose->hit_point = req->input.socket_origin;
    return 1;
}

static void legacy_projectile(void *ctx, const GWP89_Event *ev)
{
    LegacyHost *host;
    host = (LegacyHost *)ctx;
    if (host && ev) host->projectile_count++;
}

static void legacy_muzzle(void *ctx, const GWP89_Event *ev)
{
    LegacyHost *host;
    host = (LegacyHost *)ctx;
    if (host && ev) host->muzzle_count++;
}

static void legacy_casing(void *ctx, const GWP89_Event *ev)
{
    LegacyHost *host;
    host = (LegacyHost *)ctx;
    if (host && ev) host->casing_count++;
}

static void legacy_trail(void *ctx, const GWP89_Event *ev)
{
    LegacyHost *host;
    host = (LegacyHost *)ctx;
    if (host && ev) host->trail_count++;
}

int main(void)
{
    GWP89_Manager manager;
    GWP89_WeaponProfile weapon;
    GWP89_Hooks hooks;
    GWP89_FireInput input;
    LegacyHost host;

    memset(&host, 0, sizeof(host));
    memset(&hooks, 0, sizeof(hooks));
    memset(&input, 0, sizeof(input));
    gwp89_init(&manager);
    gwp89_profile_defaults(&weapon);
    gwp89_copy_id(weapon.name, GWP89_NAME_MAX, "legacy_pistol");
    weapon.weapon_id = 88;
    weapon.clip_size = 2;
    weapon.pellet_count = 1;
    if (gwp89_add_weapon(&manager, &weapon) < 0) return 1;

    hooks.ctx = &host;
    hooks.resolve_pose = legacy_pose;
    hooks.emit_projectile = legacy_projectile;
    hooks.emit_muzzle = legacy_muzzle;
    hooks.emit_casing = legacy_casing;
    hooks.emit_trail = legacy_trail;
    gwp89_set_hooks(&manager, &hooks);

    if (gwp89_bind_actor(&manager, 3, 1, 1) < 0) return 1;
    if (gwp89_equip_name(&manager, 3, "legacy_pistol", 1) != GWP89_OK) return 1;
    input.actor_id = 3;
    input.actor_kind = 1;
    input.team_id = 1;
    input.trigger_flags = GWP89_TRIGGER_PRESSED | GWP89_TRIGGER_DOWN;
    input.socket_forward = gwp89_v3(GWP89_FIX_ONE, 0L, 0L);
    if (gwp89_try_fire(&manager, &input) != GWP89_OK) return 1;

    if (host.projectile_count != 1 || host.muzzle_count != 1 || host.casing_count != 1 || host.trail_count != 1) {
        printf("FAIL: legacy hooks projectile=%d muzzle=%d casing=%d trail=%d\n", host.projectile_count, host.muzzle_count, host.casing_count, host.trail_count);
        return 1;
    }
    if (gwp89_query_clip(&manager, 3, weapon.weapon_id) != 1) return 1;
    printf("PASS: zero-provider legacy hook fallback validated\n");
    return 0;
}
