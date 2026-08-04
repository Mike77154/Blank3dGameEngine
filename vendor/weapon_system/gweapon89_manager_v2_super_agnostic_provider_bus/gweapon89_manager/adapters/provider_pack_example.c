/*
   Generic host-side provider pack adapter.

   This file is an example and is not compiled by the default Makefile.
   A host can expose each subsystem through the same packet ABI without making
   gweapon89 include any engine header.
*/

#include "../src/gweapon89.h"

typedef int (*HostProviderPacketFn)(void *ctx, GWP89_ProviderPacket *packet);

typedef struct HostProviderPackTag {
    void *ctx;
    HostProviderPacketFn math3d;
    HostProviderPacketFn transform;
    HostProviderPacketFn numeric;
    HostProviderPacketFn flags;
    HostProviderPacketFn camera;
    HostProviderPacketFn sockets;
    HostProviderPacketFn inventory;
    HostProviderPacketFn raycast;
    HostProviderPacketFn hud;
    HostProviderPacketFn crosshair;
    HostProviderPacketFn scope;
    HostProviderPacketFn draw;
    HostProviderPacketFn projectile;
    HostProviderPacketFn zoom;
    HostProviderPacketFn spread;
    HostProviderPacketFn projectile_life;
    HostProviderPacketFn health;
    HostProviderPacketFn muzzle;
    HostProviderPacketFn reload;
    HostProviderPacketFn active_reload;
    HostProviderPacketFn casing;
    HostProviderPacketFn trail;
    HostProviderPacketFn mesh;
    HostProviderPacketFn recoil;
    HostProviderPacketFn pose;
    HostProviderPacketFn event_bus;
    HostProviderPacketFn io;
    HostProviderPacketFn actor_state;
} HostProviderPack;

static HostProviderPacketFn host_pack_select(HostProviderPack *pack, int service)
{
    if (!pack) return 0;
    switch (service) {
        case GWP89_SERVICE_MATH3D: return pack->math3d;
        case GWP89_SERVICE_TRANSFORM: return pack->transform;
        case GWP89_SERVICE_NUMERIC: return pack->numeric;
        case GWP89_SERVICE_FLAGS: return pack->flags;
        case GWP89_SERVICE_CAMERA: return pack->camera;
        case GWP89_SERVICE_SOCKETS: return pack->sockets;
        case GWP89_SERVICE_INVENTORY: return pack->inventory;
        case GWP89_SERVICE_RAYCAST: return pack->raycast;
        case GWP89_SERVICE_HUD: return pack->hud;
        case GWP89_SERVICE_CROSSHAIR: return pack->crosshair;
        case GWP89_SERVICE_SCOPE: return pack->scope;
        case GWP89_SERVICE_DRAW: return pack->draw;
        case GWP89_SERVICE_PROJECTILE: return pack->projectile;
        case GWP89_SERVICE_ZOOM: return pack->zoom;
        case GWP89_SERVICE_SPREAD: return pack->spread;
        case GWP89_SERVICE_PROJECTILE_LIFE: return pack->projectile_life;
        case GWP89_SERVICE_HEALTH: return pack->health;
        case GWP89_SERVICE_MUZZLE: return pack->muzzle;
        case GWP89_SERVICE_RELOAD: return pack->reload;
        case GWP89_SERVICE_ACTIVE_RELOAD: return pack->active_reload;
        case GWP89_SERVICE_CASING: return pack->casing;
        case GWP89_SERVICE_TRAIL: return pack->trail;
        case GWP89_SERVICE_MESH: return pack->mesh;
        case GWP89_SERVICE_RECOIL: return pack->recoil;
        case GWP89_SERVICE_POSE: return pack->pose;
        case GWP89_SERVICE_EVENT_BUS: return pack->event_bus;
        case GWP89_SERVICE_IO: return pack->io;
        case GWP89_SERVICE_ACTOR_STATE: return pack->actor_state;
    }
    return 0;
}

static int host_pack_provider(void *ctx, GWP89_ProviderPacket *packet)
{
    HostProviderPack *pack;
    HostProviderPacketFn fn;
    pack = (HostProviderPack *)ctx;
    if (!pack || !packet) return GWP89_PROVIDER_PASS;
    fn = host_pack_select(pack, packet->service);
    if (!fn) return GWP89_PROVIDER_PASS;
    return fn(pack->ctx, packet);
}

static void host_pack_add(GWP89_Manager *manager, HostProviderPack *pack, int service, HostProviderPacketFn fn)
{
    if (!manager || !pack || !fn) return;
    gwp89_add_provider(manager, service, 100, gwp89_service_name(service), pack, host_pack_provider);
}

void host_install_provider_pack(GWP89_Manager *manager, HostProviderPack *pack)
{
    if (!manager || !pack) return;
    host_pack_add(manager, pack, GWP89_SERVICE_MATH3D, pack->math3d);
    host_pack_add(manager, pack, GWP89_SERVICE_TRANSFORM, pack->transform);
    host_pack_add(manager, pack, GWP89_SERVICE_NUMERIC, pack->numeric);
    host_pack_add(manager, pack, GWP89_SERVICE_FLAGS, pack->flags);
    host_pack_add(manager, pack, GWP89_SERVICE_CAMERA, pack->camera);
    host_pack_add(manager, pack, GWP89_SERVICE_SOCKETS, pack->sockets);
    host_pack_add(manager, pack, GWP89_SERVICE_INVENTORY, pack->inventory);
    host_pack_add(manager, pack, GWP89_SERVICE_RAYCAST, pack->raycast);
    host_pack_add(manager, pack, GWP89_SERVICE_HUD, pack->hud);
    host_pack_add(manager, pack, GWP89_SERVICE_CROSSHAIR, pack->crosshair);
    host_pack_add(manager, pack, GWP89_SERVICE_SCOPE, pack->scope);
    host_pack_add(manager, pack, GWP89_SERVICE_DRAW, pack->draw);
    host_pack_add(manager, pack, GWP89_SERVICE_PROJECTILE, pack->projectile);
    host_pack_add(manager, pack, GWP89_SERVICE_ZOOM, pack->zoom);
    host_pack_add(manager, pack, GWP89_SERVICE_SPREAD, pack->spread);
    host_pack_add(manager, pack, GWP89_SERVICE_PROJECTILE_LIFE, pack->projectile_life);
    host_pack_add(manager, pack, GWP89_SERVICE_HEALTH, pack->health);
    host_pack_add(manager, pack, GWP89_SERVICE_MUZZLE, pack->muzzle);
    host_pack_add(manager, pack, GWP89_SERVICE_RELOAD, pack->reload);
    host_pack_add(manager, pack, GWP89_SERVICE_ACTIVE_RELOAD, pack->active_reload);
    host_pack_add(manager, pack, GWP89_SERVICE_CASING, pack->casing);
    host_pack_add(manager, pack, GWP89_SERVICE_TRAIL, pack->trail);
    host_pack_add(manager, pack, GWP89_SERVICE_MESH, pack->mesh);
    host_pack_add(manager, pack, GWP89_SERVICE_RECOIL, pack->recoil);
    host_pack_add(manager, pack, GWP89_SERVICE_POSE, pack->pose);
    host_pack_add(manager, pack, GWP89_SERVICE_EVENT_BUS, pack->event_bus);
    host_pack_add(manager, pack, GWP89_SERVICE_IO, pack->io);
    host_pack_add(manager, pack, GWP89_SERVICE_ACTOR_STATE, pack->actor_state);
}
