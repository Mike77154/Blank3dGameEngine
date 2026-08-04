#include "blank3d_weapon_loadout.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B3D_WLOAD_LINE_CAP 512

static void b3d_wload_copy(char *dst, size_t cap, const char *src)
{
    size_t i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (i + 1U < cap && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void b3d_wload_status(char *dst, size_t cap, const char *src)
{
    b3d_wload_copy(dst, cap, src);
}

static char *b3d_wload_trim(char *s)
{
    char *end;
    while (*s && isspace((unsigned char)*s)) ++s;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

static int b3d_wload_int(const char *s, int fallback)
{
    char *end;
    long v;
    if (!s || !*s) return fallback;
    v = strtol(s, &end, 10);
    end = b3d_wload_trim(end);
    if (*end != '\0') return fallback;
    if (v < 0L) v = 0L;
    if (v > 65535L) v = 65535L;
    return (int)v;
}

static int b3d_wload_section_index(const char *section)
{
    const char *dot;
    if (!section) return -1;
    if (strncmp(section, "weapon.", 7U) != 0) return -1;
    dot = section + 7;
    return b3d_wload_int(dot, 0) - 1;
}

void blank3d_weapon_catalog_defaults(Blank3DWeaponCatalog *catalog)
{
    static const char *names[9] = {
        "pistol", "machine_gun", "shotgun", "magnum", "sniper",
        "grenade_launcher", "rocket_launcher", "gatling_gun", "slingshot"
    };
    static const char *ammo_names[9] = {
        "9mm_ammo", "9mm_ammo", "shotgun_shells", "magnum_ammo",
        "sniper_ammo", "grenades_40mm", "rockets", "gatling_belt",
        "sling_stones"
    };
    static const char *profiles[9] = {
        "pistol.ini", "machine_gun.ini", "shotgun.ini", "magnum.ini",
        "sniper.ini", "grenade_launcher.ini", "rocket_launcher.ini",
        "gatling.ini", "slingshot.ini"
    };
    static const int ammo_ids[9] = {1,1,2,3,4,5,6,7,8};
    static const int capacities[9] = {999,999,999,999,999,999,999,4000,999};
    int i;
    if (!catalog) return;
    memset(catalog, 0, sizeof(*catalog));
    for (i = 0; i < 9; ++i) {
        catalog->entries[i].used = 1;
        catalog->entries[i].weapon_id = i + 1;
        catalog->entries[i].ammo_id = ammo_ids[i];
        catalog->entries[i].max_owned = 1;
        catalog->entries[i].ammo_capacity = capacities[i];
        b3d_wload_copy(catalog->entries[i].name,
                       sizeof(catalog->entries[i].name), names[i]);
        b3d_wload_copy(catalog->entries[i].ammo_name,
                       sizeof(catalog->entries[i].ammo_name), ammo_names[i]);
        b3d_wload_copy(catalog->entries[i].profile,
                       sizeof(catalog->entries[i].profile), profiles[i]);
    }
    catalog->count = 9;
}

void blank3d_player_weapon_loadout_defaults(Blank3DPlayerWeaponLoadout *loadout)
{
    int i;
    static const int ammo[9] = {180,180,40,24,30,18,8,900,60};
    if (!loadout) return;
    memset(loadout, 0, sizeof(*loadout));
    for (i = 1; i <= 9; ++i) loadout->weapon_amount[i] = 1;
    for (i = 1; i <= 8; ++i) loadout->ammo_amount[i] = ammo[i - 1];
    loadout->equipped_weapon_id = 1;
}

const Blank3DWeaponCatalogEntry *blank3d_weapon_catalog_find_id(
    const Blank3DWeaponCatalog *catalog, int weapon_id)
{
    int i;
    if (!catalog) return 0;
    for (i = 0; i < B3D_WLOAD_MAX_WEAPONS; ++i) {
        if (catalog->entries[i].used &&
            catalog->entries[i].weapon_id == weapon_id)
            return &catalog->entries[i];
    }
    return 0;
}

int blank3d_weapon_catalog_load(Blank3DWeaponCatalog *catalog,
                                const char *path,
                                char *status,
                                size_t status_capacity)
{
    FILE *file;
    char line[B3D_WLOAD_LINE_CAP];
    char section[64];
    int current;
    int loaded;
    if (!catalog || !path) return 0;
    blank3d_weapon_catalog_defaults(catalog);
    file = fopen(path, "rb");
    if (!file) {
        b3d_wload_status(status, status_capacity,
                         "weapon catalog missing; compiled defaults used");
        return 0;
    }
    section[0] = '\0';
    current = -1;
    loaded = 0;
    while (fgets(line, sizeof(line), file)) {
        char *text;
        char *eq;
        char *key;
        char *value;
        text = b3d_wload_trim(line);
        if (*text == '\0' || *text == '#' || *text == ';') continue;
        if (*text == '[') {
            char *end;
            end = strchr(text, ']');
            if (!end) continue;
            *end = '\0';
            b3d_wload_copy(section, sizeof(section), text + 1);
            current = b3d_wload_section_index(section);
            if (current >= 0 && current < B3D_WLOAD_MAX_WEAPONS) {
                memset(&catalog->entries[current], 0,
                       sizeof(catalog->entries[current]));
                catalog->entries[current].used = 1;
                catalog->entries[current].weapon_id = current + 1;
                catalog->entries[current].max_owned = 1;
                ++loaded;
            }
            continue;
        }
        if (current < 0 || current >= B3D_WLOAD_MAX_WEAPONS) continue;
        eq = strchr(text, '=');
        if (!eq) continue;
        *eq = '\0';
        key = b3d_wload_trim(text);
        value = b3d_wload_trim(eq + 1);
        if (strcmp(key, "id") == 0)
            catalog->entries[current].weapon_id = b3d_wload_int(value, current + 1);
        else if (strcmp(key, "name") == 0)
            b3d_wload_copy(catalog->entries[current].name,
                           sizeof(catalog->entries[current].name), value);
        else if (strcmp(key, "profile") == 0)
            b3d_wload_copy(catalog->entries[current].profile,
                           sizeof(catalog->entries[current].profile), value);
        else if (strcmp(key, "max_owned") == 0)
            catalog->entries[current].max_owned = b3d_wload_int(value, 1);
        else if (strcmp(key, "ammo_id") == 0)
            catalog->entries[current].ammo_id = b3d_wload_int(value, 0);
        else if (strcmp(key, "ammo_name") == 0)
            b3d_wload_copy(catalog->entries[current].ammo_name,
                           sizeof(catalog->entries[current].ammo_name), value);
        else if (strcmp(key, "ammo_capacity") == 0)
            catalog->entries[current].ammo_capacity = b3d_wload_int(value, 999);
    }
    fclose(file);
    catalog->count = loaded;
    if (loaded <= 0) {
        blank3d_weapon_catalog_defaults(catalog);
        b3d_wload_status(status, status_capacity,
                         "weapon catalog invalid; compiled defaults used");
        return 0;
    }
    b3d_wload_status(status, status_capacity, "weapon catalog loaded");
    return loaded;
}

int blank3d_player_weapon_loadout_load(Blank3DPlayerWeaponLoadout *loadout,
                                       const char *path,
                                       char *status,
                                       size_t status_capacity)
{
    FILE *file;
    char line[B3D_WLOAD_LINE_CAP];
    char section[32];
    int loaded;
    if (!loadout || !path) return 0;
    blank3d_player_weapon_loadout_defaults(loadout);
    file = fopen(path, "rb");
    if (!file) {
        b3d_wload_status(status, status_capacity,
                         "player weapon loadout missing; defaults used");
        return 0;
    }
    memset(loadout, 0, sizeof(*loadout));
    loadout->equipped_weapon_id = 1;
    section[0] = '\0';
    loaded = 0;
    while (fgets(line, sizeof(line), file)) {
        char *text;
        char *eq;
        char *key;
        char *value;
        int id;
        text = b3d_wload_trim(line);
        if (*text == '\0' || *text == '#' || *text == ';') continue;
        if (*text == '[') {
            char *end;
            end = strchr(text, ']');
            if (!end) continue;
            *end = '\0';
            b3d_wload_copy(section, sizeof(section), text + 1);
            continue;
        }
        eq = strchr(text, '=');
        if (!eq) continue;
        *eq = '\0';
        key = b3d_wload_trim(text);
        value = b3d_wload_trim(eq + 1);
        if (strcmp(section, "player") == 0 &&
            strcmp(key, "equipped_weapon_id") == 0) {
            loadout->equipped_weapon_id = b3d_wload_int(value, 1);
            ++loaded;
        } else if (strcmp(section, "weapons") == 0 &&
                   strncmp(key, "weapon_", 7U) == 0) {
            id = b3d_wload_int(key + 7, 0);
            if (id > 0 && id <= B3D_WLOAD_MAX_WEAPONS) {
                loadout->weapon_amount[id] = b3d_wload_int(value, 0);
                ++loaded;
            }
        } else if (strcmp(section, "ammo") == 0 &&
                   strncmp(key, "ammo_", 5U) == 0) {
            id = b3d_wload_int(key + 5, 0);
            if (id > 0 && id <= B3D_WLOAD_MAX_WEAPONS) {
                loadout->ammo_amount[id] = b3d_wload_int(value, 0);
                ++loaded;
            }
        }
    }
    fclose(file);
    if (loaded <= 0) {
        blank3d_player_weapon_loadout_defaults(loadout);
        b3d_wload_status(status, status_capacity,
                         "player weapon loadout invalid; defaults used");
        return 0;
    }
    b3d_wload_status(status, status_capacity, "player weapon loadout loaded");
    return loaded;
}
