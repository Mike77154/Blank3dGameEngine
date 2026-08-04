#include "../src/gweapon89.h"
#include <stdio.h>

static const char k_provider_weapon_ini[] =
    "[weapon:provider_rifle]\n"
    "weapon_id=77\n"
    "gun_id=77\n"
    "ammo_id=1\n"
    "projectile_id=4\n"
    "clip_size=30\n"
    "ammo_per_shot=1\n"
    "fire_mode=auto\n"
    "active_reload_enabled=1\n"
    "active_reload_window_start_ms=380\n"
    "active_reload_window_end_ms=560\n";

static int memory_io_provider(void *ctx, GWP89_ProviderPacket *packet)
{
    const char *src;
    int i;
    (void)ctx;
    if (!packet) return GWP89_PROVIDER_PASS;
    if (packet->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    if (packet->service != GWP89_SERVICE_IO) return GWP89_PROVIDER_PASS;
    if (packet->operation != GWP89_OP_READ_TEXT_FILE) return GWP89_PROVIDER_PASS;
    if (!packet->text_out || packet->text_capacity <= 0) {
        packet->result_code = GWP89_BAD_ARG;
        return GWP89_PROVIDER_HANDLED;
    }
    src = k_provider_weapon_ini;
    i = 0;
    while (src[i] && i + 1 < packet->text_capacity) {
        packet->text_out[i] = src[i];
        i++;
    }
    packet->text_out[i] = '\0';
    packet->text_length = i;
    packet->result_code = GWP89_OK;
    return GWP89_PROVIDER_HANDLED;
}

int main(void)
{
    GWP89_Manager m;
    const GWP89_WeaponProfile *p;
    int n;
    gwp89_init(&m);
    if (gwp89_add_provider(&m,
                           GWP89_SERVICE_IO,
                           100,
                           "memory_ini_io",
                           0,
                           memory_io_provider) < 0) return 5;
    n = gwp89_load_ini_file(&m, "config/provider_weapon.ini");
    if (n != 1) return 1;
    p = gwp89_get_weapon(&m, 0);
    if (!p) return 2;
    if (!p->active_reload_enabled ||
        p->active_reload_window_start_ms != 380u ||
        p->active_reload_window_end_ms != 560u) return 3;
    if (p->weapon_id != 77 || p->clip_size != 30) return 4;
    printf("PASS: INI profile parsed through IO provider\n");
    return 0;
}
