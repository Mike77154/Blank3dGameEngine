#include "gweaponloadout89.h"
#include "gweaponio89.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define GWLOAD89_LINE_CAP 512

typedef struct GWLoad89TextReaderTag {
    const char *cursor;
} GWLoad89TextReader;

static void gwload_copy(char *dst, size_t cap, const char *src)
{
    size_t i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (i + 1U < cap && src[i] != '\0') { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}

static void gwload_status(char *dst, size_t cap, const char *src)
{
    gwload_copy(dst, cap, src);
}

static char *gwload_trim(char *s)
{
    char *end;
    if (!s) return s;
    while (*s && isspace((unsigned char)*s)) ++s;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

static int gwload_int(const char *s, int fallback)
{
    char *end;
    long v;
    if (!s || !*s) return fallback;
    v = strtol(s, &end, 10);
    end = gwload_trim(end);
    if (*end != '\0') return fallback;
    if (v < 0L) v = 0L;
    if (v > 65535L) v = 65535L;
    return (int)v;
}

static int gwload_next_line(GWLoad89TextReader *reader,
                            char *line, size_t capacity)
{
    size_t n;
    const char *p;
    if (!reader || !reader->cursor || !*reader->cursor ||
        !line || capacity == 0U) return 0;
    p = reader->cursor;
    n = 0U;
    while (*p && *p != '\n' && *p != '\r') {
        if (n + 1U < capacity) line[n++] = *p;
        ++p;
    }
    line[n] = '\0';
    while (*p == '\n' || *p == '\r') ++p;
    reader->cursor = p;
    return 1;
}

static int gwload_section_index(const char *section)
{
    if (!section || strncmp(section, "weapon.", 7U) != 0) return -1;
    return gwload_int(section + 7, 0) - 1;
}

void gweaponloadout89_catalog_defaults(GWeaponCatalog89 *catalog)
{
    static const char *names[10] = {
        "pistol", "machine_gun", "shotgun", "magnum", "sniper",
        "grenade_launcher", "rocket_launcher", "gatling_gun", "slingshot",
        "hand_grenade"
    };
    static const char *ammo_names[10] = {
        "9mm_ammo", "9mm_ammo", "shotgun_shells", "magnum_ammo",
        "sniper_ammo", "grenades_40mm", "rockets", "gatling_belt",
        "sling_stones", "hand_grenades"
    };
    static const char *profiles[10] = {
        "pistol.ini", "machine_gun.ini", "shotgun.ini", "magnum.ini",
        "sniper.ini", "grenade_launcher.ini", "rocket_launcher.ini",
        "gatling.ini", "slingshot.ini", "hand_grenade.ini"
    };
    static const int ammo_ids[10] = {1,1,2,3,4,5,6,7,8,9};
    static const int capacities[10] = {999,999,999,999,999,999,999,4000,999,99};
    int i;
    if (!catalog) return;
    memset(catalog, 0, sizeof(*catalog));
    for (i = 0; i < 10; ++i) {
        catalog->entries[i].used = 1;
        catalog->entries[i].weapon_id = i + 1;
        catalog->entries[i].ammo_id = ammo_ids[i];
        catalog->entries[i].max_owned = 1;
        catalog->entries[i].ammo_capacity = capacities[i];
        gwload_copy(catalog->entries[i].name,
                    sizeof(catalog->entries[i].name), names[i]);
        gwload_copy(catalog->entries[i].ammo_name,
                    sizeof(catalog->entries[i].ammo_name), ammo_names[i]);
        gwload_copy(catalog->entries[i].profile,
                    sizeof(catalog->entries[i].profile), profiles[i]);
    }
    catalog->count = 10;
}

void gweaponloadout89_defaults(GWeaponLoadout89 *loadout)
{
    int i;
    static const int ammo[9] = {180,40,24,30,18,8,900,60,6};
    if (!loadout) return;
    memset(loadout, 0, sizeof(*loadout));
    for (i = 1; i <= 10; ++i) loadout->weapon_amount[i] = 1;
    for (i = 1; i <= 9; ++i) loadout->ammo_amount[i] = ammo[i - 1];
    loadout->equipped_weapon_id = 1;
}

const GWeaponCatalogEntry89 *gweaponloadout89_catalog_find_id(
    const GWeaponCatalog89 *catalog, int weapon_id)
{
    int i;
    if (!catalog) return 0;
    for (i = 0; i < GWLOAD89_MAX_WEAPONS; ++i)
        if (catalog->entries[i].used &&
            catalog->entries[i].weapon_id == weapon_id)
            return &catalog->entries[i];
    return 0;
}

int gweaponloadout89_catalog_parse_text(GWeaponCatalog89 *catalog,
                                         const char *text,
                                         char *status,
                                         size_t status_capacity)
{
    GWLoad89TextReader reader;
    char line[GWLOAD89_LINE_CAP];
    char section[64];
    int current;
    int loaded;
    if (!catalog || !text) return 0;
    gweaponloadout89_catalog_defaults(catalog);
    reader.cursor = text;
    section[0] = '\0'; current = -1; loaded = 0;
    while (gwload_next_line(&reader, line, sizeof(line))) {
        char *s; char *eq; char *key; char *value;
        s = gwload_trim(line);
        if (*s == '\0' || *s == '#' || *s == ';') continue;
        if (*s == '[') {
            char *end = strchr(s, ']');
            if (!end) continue;
            *end = '\0';
            gwload_copy(section, sizeof(section), s + 1);
            current = gwload_section_index(section);
            if (current >= 0 && current < GWLOAD89_MAX_WEAPONS) {
                memset(&catalog->entries[current], 0,
                       sizeof(catalog->entries[current]));
                catalog->entries[current].used = 1;
                catalog->entries[current].weapon_id = current + 1;
                catalog->entries[current].max_owned = 1;
                ++loaded;
            }
            continue;
        }
        if (current < 0 || current >= GWLOAD89_MAX_WEAPONS) continue;
        eq = strchr(s, '='); if (!eq) continue; *eq = '\0';
        key = gwload_trim(s); value = gwload_trim(eq + 1);
        if (strcmp(key, "id") == 0)
            catalog->entries[current].weapon_id = gwload_int(value, current + 1);
        else if (strcmp(key, "name") == 0)
            gwload_copy(catalog->entries[current].name,
                        sizeof(catalog->entries[current].name), value);
        else if (strcmp(key, "profile") == 0)
            gwload_copy(catalog->entries[current].profile,
                        sizeof(catalog->entries[current].profile), value);
        else if (strcmp(key, "max_owned") == 0)
            catalog->entries[current].max_owned = gwload_int(value, 1);
        else if (strcmp(key, "ammo_id") == 0)
            catalog->entries[current].ammo_id = gwload_int(value, 0);
        else if (strcmp(key, "ammo_name") == 0)
            gwload_copy(catalog->entries[current].ammo_name,
                        sizeof(catalog->entries[current].ammo_name), value);
        else if (strcmp(key, "ammo_capacity") == 0)
            catalog->entries[current].ammo_capacity = gwload_int(value, 999);
    }
    catalog->count = loaded;
    if (loaded <= 0) {
        gweaponloadout89_catalog_defaults(catalog);
        gwload_status(status, status_capacity,
                      "weapon catalog invalid; compiled defaults used");
        return 0;
    }
    gwload_status(status, status_capacity, "weapon catalog loaded");
    return loaded;
}

int gweaponloadout89_parse_text(GWeaponLoadout89 *loadout,
                                 const char *text,
                                 char *status,
                                 size_t status_capacity)
{
    GWLoad89TextReader reader;
    char line[GWLOAD89_LINE_CAP];
    char section[32];
    int loaded;
    if (!loadout || !text) return 0;
    memset(loadout, 0, sizeof(*loadout));
    loadout->equipped_weapon_id = 1;
    reader.cursor = text; section[0] = '\0'; loaded = 0;
    while (gwload_next_line(&reader, line, sizeof(line))) {
        char *s; char *eq; char *key; char *value; int id;
        s = gwload_trim(line);
        if (*s == '\0' || *s == '#' || *s == ';') continue;
        if (*s == '[') {
            char *end = strchr(s, ']');
            if (!end) continue;
            *end = '\0';
            gwload_copy(section, sizeof(section), s + 1); continue;
        }
        eq = strchr(s, '='); if (!eq) continue; *eq = '\0';
        key = gwload_trim(s); value = gwload_trim(eq + 1);
        if (strcmp(section, "player") == 0 &&
            strcmp(key, "equipped_weapon_id") == 0) {
            loadout->equipped_weapon_id = gwload_int(value, 1); ++loaded;
        } else if (strcmp(section, "weapons") == 0 &&
                   strncmp(key, "weapon_", 7U) == 0) {
            id = gwload_int(key + 7, 0);
            if (id > 0 && id <= GWLOAD89_MAX_WEAPONS) {
                loadout->weapon_amount[id] = gwload_int(value, 0); ++loaded;
            }
        } else if (strcmp(section, "ammo") == 0 &&
                   strncmp(key, "ammo_", 5U) == 0) {
            id = gwload_int(key + 5, 0);
            if (id > 0 && id <= GWLOAD89_MAX_WEAPONS) {
                loadout->ammo_amount[id] = gwload_int(value, 0); ++loaded;
            }
        }
    }
    if (loaded <= 0) {
        gweaponloadout89_defaults(loadout);
        gwload_status(status, status_capacity,
                      "player weapon loadout invalid; defaults used");
        return 0;
    }
    gwload_status(status, status_capacity, "player weapon loadout loaded");
    return loaded;
}

int gweaponloadout89_catalog_load(GWP89_Manager *manager,
                                  GWeaponCatalog89 *catalog,
                                  const char *path,
                                  char *status,
                                  size_t status_capacity)
{
    char text[GWLOAD89_TEXT_CAP];
    if (!catalog || !path) return 0;
    if (!gweaponio89_read_text(manager, path, text, (int)sizeof(text), 0)) {
        gweaponloadout89_catalog_defaults(catalog);
        gwload_status(status, status_capacity,
                      "weapon catalog missing; compiled defaults used");
        return 0;
    }
    return gweaponloadout89_catalog_parse_text(catalog, text,
                                                status, status_capacity);
}

int gweaponloadout89_load(GWP89_Manager *manager,
                          GWeaponLoadout89 *loadout,
                          const char *path,
                          char *status,
                          size_t status_capacity)
{
    char text[GWLOAD89_TEXT_CAP];
    if (!loadout || !path) return 0;
    if (!gweaponio89_read_text(manager, path, text, (int)sizeof(text), 0)) {
        gweaponloadout89_defaults(loadout);
        gwload_status(status, status_capacity,
                      "player weapon loadout missing; defaults used");
        return 0;
    }
    return gweaponloadout89_parse_text(loadout, text,
                                        status, status_capacity);
}
