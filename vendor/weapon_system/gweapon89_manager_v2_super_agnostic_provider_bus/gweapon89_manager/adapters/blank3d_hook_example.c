/*
   Example only. Do not compile this file by default.

   This shows where Blank3D would plug its own systems:
   - ray/camera resolver
   - bolt3d projectile bridge
   - fcasing89 gunslinger casings
   - trail3d89 trails
   - fmuzzle3d/laminal muzzle flash
   - gbulletmesh89 mesh assignment

   gweapon89 itself never includes those headers.
*/

#include "../src/gweapon89.h"

/* Replace HostEngine with your actual engine type. */
typedef struct HostEngineTag HostEngine;

static int host_resolve_pose(void *ctx, const GWP89_PoseRequest *req, GWP89_PoseResult *out_pose)
{
    HostEngine *host;
    (void)host;
    host = (HostEngine *)ctx;
    if (!req || !out_pose) return 0;

    /* Host should raytrace from camera/crosshair/socket and return exact points. */
    out_pose->projectile_origin = req->input.socket_origin;
    out_pose->muzzle_origin = req->input.socket_origin;
    out_pose->casing_origin = req->input.socket_origin;
    out_pose->direction = req->input.socket_forward;
    out_pose->hit_point = req->input.socket_origin;
    out_pose->has_hit = 0;
    return 1;
}

static void host_projectile(void *ctx, const GWP89_Event *ev)
{
    HostEngine *host;
    (void)host;
    host = (HostEngine *)ctx;
    (void)ev;
    /* mx_bolt3d_bridge_spawn_bullet(... ev->origin, ev->direction, ev->speed_fx ...); */
}

static void host_muzzle(void *ctx, const GWP89_Event *ev)
{
    HostEngine *host;
    (void)host;
    host = (HostEngine *)ctx;
    (void)ev;
    /* mx_fmuzzle3d_bridge_spawn_shot(... ev->muzzle_origin, ev->direction ...); */
}

static void host_casing(void *ctx, const GWP89_Event *ev)
{
    HostEngine *host;
    (void)host;
    host = (HostEngine *)ctx;
    (void)ev;
    /* mx_fcasing89_bridge_spawn_shot(... ev->casing_origin, ev->direction ...); */
}

static void host_trail(void *ctx, const GWP89_Event *ev)
{
    HostEngine *host;
    (void)host;
    host = (HostEngine *)ctx;
    (void)ev;
    /* mx_trail3d89_bridge_spawn_bullet(... ev->projectile_slot_hint, ev->origin, ev->direction ...); */
}

static void host_mesh(void *ctx, const GWP89_Event *ev)
{
    HostEngine *host;
    (void)host;
    host = (HostEngine *)ctx;
    (void)ev;
    /* Map ev->projectile_mesh_id / ev->shell_mesh_id or names to your mesh library. */
}

void host_install_gweapon89_hooks(GWP89_Manager *wm, HostEngine *host)
{
    GWP89_Hooks hooks;
    if (!wm) return;
    hooks.ctx = host;
    hooks.resolve_pose = host_resolve_pose;
    hooks.ammo_query = 0;
    hooks.ammo_consume = 0;
    hooks.ammo_changed = 0;
    hooks.emit_projectile = host_projectile;
    hooks.emit_muzzle = host_muzzle;
    hooks.emit_casing = host_casing;
    hooks.emit_trail = host_trail;
    hooks.assign_mesh = host_mesh;
    hooks.visual_mod = 0;
    hooks.event = 0;
    gwp89_set_hooks(wm, &hooks);
}
