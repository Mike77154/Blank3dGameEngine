#include "blank3d_pickups.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define B3D_PICKUP_CONTACT_CATEGORY 1UL

typedef struct B3DPickupSpecTag {
    char name[B3D_PICKUP_NAME_CAP];
    char gfo_path[B3D_PICKUP_PATH_CAP];
    char mesh_recipe_path[B3D_PICKUP_PATH_CAP];
    int kind;
    int resource_id;
    int amount;
    int auto_equip;
    long radius_q16;
} B3DPickupSpec;

typedef struct B3DPartSpecTag {
    int used;
    long x_q16;
    long y_q16;
    long z_q16;
    long width_q16;
    long height_q16;
    long depth_q16;
    int r;
    int g;
    int b;
    int a;
} B3DPartSpec;

static void b3d_pickup_status(Blank3DPickupWorld *world, const char *text)
{
    if (!world) return;
    if (!text) text = "";
    strncpy(world->status, text, sizeof(world->status) - 1U);
    world->status[sizeof(world->status) - 1U] = '\0';
}

static void b3d_copy(char *dst, unsigned int cap, const char *src)
{
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    strncpy(dst, src, (size_t)(cap - 1U));
    dst[cap - 1U] = '\0';
}

static char *b3d_trim(char *text)
{
    char *end;
    if (!text) return text;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

static int b3d_parse_q16(const char *text, long *out_value)
{
    int negative;
    unsigned long whole;
    unsigned long frac;
    unsigned long scale;
    unsigned int digits;
    const char *p;
    long value;
    if (!text || !out_value) return 0;
    p = text;
    while (*p && isspace((unsigned char)*p)) ++p;
    negative = 0;
    if (*p == '-') { negative = 1; ++p; }
    else if (*p == '+') ++p;
    if (!isdigit((unsigned char)*p) && *p != '.') return 0;
    whole = 0UL;
    while (isdigit((unsigned char)*p)) {
        if (whole < 30000UL) whole = whole * 10UL + (unsigned long)(*p - '0');
        ++p;
    }
    frac = 0UL;
    scale = 1UL;
    digits = 0U;
    if (*p == '.') {
        ++p;
        while (isdigit((unsigned char)*p) && digits < 5U) {
            frac = frac * 10UL + (unsigned long)(*p - '0');
            scale *= 10UL;
            ++digits;
            ++p;
        }
        while (isdigit((unsigned char)*p)) ++p;
    }
    while (*p && isspace((unsigned char)*p)) ++p;
    if (*p != '\0') return 0;
    value = (long)(whole * 65536UL);
    if (scale > 1UL)
        value += (long)((frac * 65536UL + scale / 2UL) / scale);
    if (negative) value = -value;
    *out_value = value;
    return 1;
}

static g3d_fix b3d_q16_to_q12(long value)
{
    return (g3d_fix)(value / 16L);
}

static long b3d_q12_to_q16(g3d_fix value)
{
    return (long)value * 16L;
}

static long b3d_abs_long(long value)
{
    return value < 0L ? -value : value;
}

static long b3d_max3(long a, long b, long c)
{
    long result;
    result = a > b ? a : b;
    return result > c ? result : c;
}


static int b3d_join_rpyl_path(const char **args, int count,
                              char *out, unsigned int out_cap)
{
    int i;
    unsigned int used;
    size_t n;
    if (!args || count <= 0 || !out || out_cap < 2U) return 0;
    out[0] = '\0';
    used = 0U;
    for (i = 0; i < count; ++i) {
        if (!args[i] || !args[i][0]) return 0;
        if (i > 0) {
            if (used + 1U >= out_cap) return 0;
            out[used++] = '/';
            out[used] = '\0';
        }
        n = strlen(args[i]);
        if (n > (size_t)(out_cap - used - 1U)) return 0;
        memcpy(out + used, args[i], n);
        used += (unsigned int)n;
        out[used] = '\0';
    }
    return used > 0U;
}

static int b3d_kind_from_text(const char *text)
{
    if (!text) return 0;
    if (strcmp(text, "weapon") == 0) return B3D_PICKUP_KIND_WEAPON;
    if (strcmp(text, "ammo") == 0 || strcmp(text, "ammunition") == 0)
        return B3D_PICKUP_KIND_AMMO;
    if (strcmp(text, "health") == 0 || strcmp(text, "heal") == 0 ||
        strcmp(text, "medkit") == 0)
        return B3D_PICKUP_KIND_HEALTH;
    return 0;
}

static void b3d_pickup_spec_defaults(B3DPickupSpec *spec)
{
    if (!spec) return;
    memset(spec, 0, sizeof(*spec));
    spec->amount = 1;
    spec->radius_q16 = 78643L; /* 1.2 in Q16.16 */
}

static int b3d_load_pickup_spec(const char *path, B3DPickupSpec *spec)
{
    FILE *file;
    char line[320];
    char section[48];
    char *text;
    char *eq;
    char *key;
    char *value;
    long parsed;
    if (!path || !spec) return 0;
    b3d_pickup_spec_defaults(spec);
    section[0] = '\0';
    file = fopen(path, "rb");
    if (!file) return 0;
    while (fgets(line, sizeof(line), file)) {
        text = b3d_trim(line);
        if (*text == '\0' || *text == '#' || *text == ';') continue;
        if (*text == '[') {
            char *close;
            close = strchr(text, ']');
            if (!close) continue;
            *close = '\0';
            b3d_copy(section, sizeof(section), text + 1);
            continue;
        }
        eq = strchr(text, '=');
        if (!eq) continue;
        *eq = '\0';
        key = b3d_trim(text);
        value = b3d_trim(eq + 1);
        if (strcmp(section, "entity") == 0) {
            if (strcmp(key, "name") == 0)
                b3d_copy(spec->name, sizeof(spec->name), value);
            else if (strcmp(key, "gfo") == 0)
                b3d_copy(spec->gfo_path, sizeof(spec->gfo_path), value);
        } else if (strcmp(section, "visual") == 0) {
            if (strcmp(key, "mesh_recipe") == 0 ||
                strcmp(key, "recipe") == 0)
                b3d_copy(spec->mesh_recipe_path,
                         sizeof(spec->mesh_recipe_path), value);
        } else if (strcmp(section, "pickup") == 0) {
            if (strcmp(key, "kind") == 0)
                spec->kind = b3d_kind_from_text(value);
            else if (strcmp(key, "resource_id") == 0 ||
                     strcmp(key, "weapon_id") == 0 ||
                     strcmp(key, "ammo_id") == 0)
                spec->resource_id = atoi(value);
            else if (strcmp(key, "amount") == 0)
                spec->amount = atoi(value);
            else if (strcmp(key, "auto_equip") == 0)
                spec->auto_equip = atoi(value) != 0;
            else if (strcmp(key, "radius") == 0 &&
                     b3d_parse_q16(value, &parsed))
                spec->radius_q16 = parsed;
            else if ((strcmp(key, "mesh_recipe") == 0 ||
                      strcmp(key, "recipe") == 0))
                b3d_copy(spec->mesh_recipe_path,
                         sizeof(spec->mesh_recipe_path), value);
        }
    }
    fclose(file);
    if (spec->name[0] == '\0') b3d_copy(spec->name, sizeof(spec->name), "pickup");
    if (spec->kind == B3D_PICKUP_KIND_WEAPON ||
        spec->kind == B3D_PICKUP_KIND_AMMO) {
        if (spec->resource_id <= 0) return 0;
    }
    return spec->kind != 0 && spec->amount > 0 &&
           spec->mesh_recipe_path[0] != '\0';
}

static void b3d_part_defaults(B3DPartSpec *part)
{
    memset(part, 0, sizeof(*part));
    part->width_q16 = 32768L;
    part->height_q16 = 32768L;
    part->depth_q16 = 32768L;
    part->r = 190;
    part->g = 190;
    part->b = 190;
    part->a = 255;
}

static int b3d_part_index(const char *section)
{
    const char *p;
    int index;
    if (!section || strncmp(section, "part.", 5U) != 0) return -1;
    p = section + 5;
    if (!isdigit((unsigned char)*p)) return -1;
    index = atoi(p);
    return index >= 0 && index < B3D_PICKUP_MAX_PARTS ? index : -1;
}

static int b3d_load_mesh_recipe(const char *path,
                                B3DPartSpec parts[B3D_PICKUP_MAX_PARTS],
                                int *out_count)
{
    FILE *file;
    char line[320];
    char section[48];
    char *text;
    char *eq;
    char *key;
    char *value;
    int index;
    int i;
    int count;
    long parsed;
    if (!path || !parts || !out_count) return 0;
    for (i = 0; i < B3D_PICKUP_MAX_PARTS; ++i) b3d_part_defaults(&parts[i]);
    section[0] = '\0';
    file = fopen(path, "rb");
    if (!file) return 0;
    while (fgets(line, sizeof(line), file)) {
        text = b3d_trim(line);
        if (*text == '\0' || *text == '#' || *text == ';') continue;
        if (*text == '[') {
            char *close;
            close = strchr(text, ']');
            if (!close) continue;
            *close = '\0';
            b3d_copy(section, sizeof(section), text + 1);
            continue;
        }
        index = b3d_part_index(section);
        if (index < 0) continue;
        eq = strchr(text, '=');
        if (!eq) continue;
        *eq = '\0';
        key = b3d_trim(text);
        value = b3d_trim(eq + 1);
        parts[index].used = 1;
        if (strcmp(key, "shape") == 0) {
            if (strcmp(value, "box") != 0) parts[index].used = 0;
        } else if (strcmp(key, "x") == 0 && b3d_parse_q16(value, &parsed))
            parts[index].x_q16 = parsed;
        else if (strcmp(key, "y") == 0 && b3d_parse_q16(value, &parsed))
            parts[index].y_q16 = parsed;
        else if (strcmp(key, "z") == 0 && b3d_parse_q16(value, &parsed))
            parts[index].z_q16 = parsed;
        else if (strcmp(key, "width") == 0 && b3d_parse_q16(value, &parsed))
            parts[index].width_q16 = parsed;
        else if (strcmp(key, "height") == 0 && b3d_parse_q16(value, &parsed))
            parts[index].height_q16 = parsed;
        else if (strcmp(key, "depth") == 0 && b3d_parse_q16(value, &parsed))
            parts[index].depth_q16 = parsed;
        else if (strcmp(key, "r") == 0) parts[index].r = atoi(value);
        else if (strcmp(key, "g") == 0) parts[index].g = atoi(value);
        else if (strcmp(key, "b") == 0) parts[index].b = atoi(value);
        else if (strcmp(key, "a") == 0) parts[index].a = atoi(value);
    }
    fclose(file);
    count = 0;
    for (i = 0; i < B3D_PICKUP_MAX_PARTS; ++i)
        if (parts[i].used) ++count;
    *out_count = count;
    return count > 0;
}

static int b3d_build_parts(Blank3DPickupInstance *instance,
                           const B3DPartSpec parts[B3D_PICKUP_MAX_PARTS])
{
    int i;
    int count;
    int result;
    g3d_color color;
    if (!instance || !parts) return 0;
    count = 0;
    for (i = 0; i < B3D_PICKUP_MAX_PARTS; ++i) {
        Blank3DPickupPart *part;
        if (!parts[i].used) continue;
        part = &instance->parts[count];
        memset(part, 0, sizeof(*part));
        part->used = 1;
        transform_init(&part->transform);
        part->transform.position.x = g3d_fix_add_sat(
            instance->transform.position.x, b3d_q16_to_q12(parts[i].x_q16));
        part->transform.position.y = g3d_fix_add_sat(
            instance->transform.position.y, b3d_q16_to_q12(parts[i].y_q16));
        part->transform.position.z = g3d_fix_add_sat(
            instance->transform.position.z, b3d_q16_to_q12(parts[i].z_q16));
        color = g3d_color_rgba((unsigned char)parts[i].r,
                               (unsigned char)parts[i].g,
                               (unsigned char)parts[i].b,
                               (unsigned char)parts[i].a);
        result = g3d_mesh_init(&part->mesh,
                               part->vertices, B3D_PICKUP_BOX_VERTEX_CAP,
                               part->indices, B3D_PICKUP_BOX_INDEX_CAP);
        if (result != G3D_OK) return 0;
        result = g3d_make_box(&part->mesh,
                              (g3d_fx)parts[i].width_q16,
                              (g3d_fx)parts[i].height_q16,
                              (g3d_fx)parts[i].depth_q16,
                              color);
        if (result != G3D_OK) return 0;
        ++count;
    }
    instance->part_count = count;
    return count > 0;
}

static int b3d_pickup_gather(void *user, const CT89_Probe *probe,
                             CT89_Candidate *out_candidates,
                             int max_candidates)
{
    Blank3DPickupWorld *world;
    const Blank3DPickupInstance *instance;
    long px;
    long py;
    long pz;
    long ix;
    long iy;
    long iz;
    long dx;
    long dy;
    long dz;
    long distance;
    if (!user || !probe || !out_candidates || max_candidates <= 0) return 0;
    world = (Blank3DPickupWorld *)user;
    if (!world->player_transform || !world->systems) return 0;
    instance = blank3d_pickups_find_subject_const(world, probe->owner);
    if (!instance || !blank3d_pickups_is_world_active(world,
            (unsigned int)(instance - world->instances))) return 0;

    px = b3d_q12_to_q16(world->player_transform->position.x);
    py = b3d_q12_to_q16(world->player_transform->position.y);
    pz = b3d_q12_to_q16(world->player_transform->position.z);
    ix = b3d_q12_to_q16(instance->transform.position.x);
    iy = b3d_q12_to_q16(instance->transform.position.y);
    iz = b3d_q12_to_q16(instance->transform.position.z);
    dx = b3d_abs_long(px - ix);
    dy = b3d_abs_long(py - iy);
    dz = b3d_abs_long(pz - iz);
    distance = b3d_max3(dx, dy, dz);
    if (distance > probe->radius_fx) return 0;

    out_candidates[0].subject = (CT89_Subject)B3D_PLAYER_ACTOR_ID;
    out_candidates[0].category_mask = B3D_PICKUP_CONTACT_CATEGORY;
    out_candidates[0].distance_fx = (CT89_FX)distance;
    return 1;
}

static void b3d_install_contact_provider(Blank3DPickupWorld *world)
{
    if (!world || !world->systems) return;
    ct89_sensor_provider_init(&world->contact_provider);
    world->contact_provider.user = world;
    world->contact_provider.gather = b3d_pickup_gather;
    ct89_set_contact_provider(blank3d_systems_contact_triggers(world->systems),
                              &world->contact_provider);
}

void blank3d_pickups_init(Blank3DPickupWorld *world,
                          Blank3DSystems *systems,
                          Transform *player_transform)
{
    if (!world) return;
    memset(world, 0, sizeof(*world));
    world->systems = systems;
    world->player_transform = player_transform;
    b3d_install_contact_provider(world);
    b3d_pickup_status(world, "pickup world ready");
}

void blank3d_pickups_reset(Blank3DPickupWorld *world)
{
    Blank3DSystems *systems;
    Transform *player;
    if (!world) return;
    systems = world->systems;
    player = world->player_transform;
    if (systems) blank3d_systems_reset_item_contact(systems);
    memset(world, 0, sizeof(*world));
    world->systems = systems;
    world->player_transform = player;
    b3d_install_contact_provider(world);
    b3d_pickup_status(world, "pickup world reset");
}

int blank3d_pickups_spawn_ini(Blank3DPickupWorld *world,
                              const char *config_path,
                              g3d_fix x, g3d_fix y, g3d_fix z,
                              int forced_kind,
                              unsigned int *out_slot)
{
    B3DPickupSpec spec;
    B3DPartSpec part_specs[B3D_PICKUP_MAX_PARTS];
    Blank3DPickupInstance *instance;
    int recipe_count;
    int ok;
    unsigned int slot;
    if (!world || !world->systems || !config_path) return 0;
    if (!b3d_load_pickup_spec(config_path, &spec)) {
        b3d_pickup_status(world, "pickup INI load failed");
        return 0;
    }
    if (forced_kind != 0) spec.kind = forced_kind;
    if (spec.kind != B3D_PICKUP_KIND_WEAPON &&
        spec.kind != B3D_PICKUP_KIND_AMMO &&
        spec.kind != B3D_PICKUP_KIND_HEALTH) {
        b3d_pickup_status(world, "pickup kind invalid");
        return 0;
    }
    if (!b3d_load_mesh_recipe(spec.mesh_recipe_path, part_specs,
                              &recipe_count)) {
        b3d_pickup_status(world, "pickup mesh recipe load failed");
        return 0;
    }
    (void)recipe_count;
    for (slot = 0U; slot < B3D_PICKUP_MAX_INSTANCES; ++slot)
        if (!world->instances[slot].used) break;
    if (slot >= B3D_PICKUP_MAX_INSTANCES) {
        b3d_pickup_status(world, "pickup table full");
        return 0;
    }
    instance = &world->instances[slot];
    memset(instance, 0, sizeof(*instance));
    instance->used = 1;
    instance->subject_id = B3D_PICKUP_SUBJECT_BASE + (unsigned long)slot;
    instance->kind = spec.kind;
    instance->resource_id = spec.resource_id;
    instance->amount = spec.amount;
    instance->auto_equip = spec.auto_equip;
    instance->radius_q16 = spec.radius_q16;
    instance->item_id = PBB_ITEM_INVALID_ID;
    instance->trigger = CT89_TRIGGER_INVALID;
    b3d_copy(instance->name, sizeof(instance->name), spec.name);
    b3d_copy(instance->config_path, sizeof(instance->config_path), config_path);
    b3d_copy(instance->gfo_path, sizeof(instance->gfo_path), spec.gfo_path);
    b3d_copy(instance->mesh_recipe_path,
             sizeof(instance->mesh_recipe_path), spec.mesh_recipe_path);
    transform_init(&instance->transform);
    instance->transform.position = gamlib_vec3(x, y, z);
    if (!b3d_build_parts(instance, part_specs)) {
        memset(instance, 0, sizeof(*instance));
        b3d_pickup_status(world, "pickup mesh build failed");
        return 0;
    }
    if (instance->kind == B3D_PICKUP_KIND_WEAPON) {
        ok = blank3d_systems_define_weapon_pickup(world->systems,
                instance->name, instance->resource_id, instance->amount,
                instance->auto_equip, 0,
                (CT89_Subject)instance->subject_id,
                CT89_SENSOR_TOUCH, (CT89_FX)instance->radius_q16,
                B3D_PICKUP_CONTACT_CATEGORY,
                CT89_CONSUME_DISABLE_TRIGGER,
                &instance->item_id, &instance->trigger);
    } else if (instance->kind == B3D_PICKUP_KIND_AMMO) {
        ok = blank3d_systems_define_ammo_pickup(world->systems,
                instance->name, instance->resource_id, instance->amount,
                0, (CT89_Subject)instance->subject_id,
                CT89_SENSOR_TOUCH, (CT89_FX)instance->radius_q16,
                B3D_PICKUP_CONTACT_CATEGORY,
                CT89_CONSUME_DISABLE_TRIGGER,
                &instance->item_id, &instance->trigger);
    } else {
        ok = blank3d_systems_define_health_pickup(world->systems,
                instance->name, instance->amount, 0,
                (CT89_Subject)instance->subject_id,
                CT89_SENSOR_TOUCH, (CT89_FX)instance->radius_q16,
                B3D_PICKUP_CONTACT_CATEGORY,
                CT89_CONSUME_DISABLE_TRIGGER,
                &instance->item_id, &instance->trigger);
    }
    if (!ok) {
        memset(instance, 0, sizeof(*instance));
        b3d_pickup_status(world, "pickup logical bind failed");
        return 0;
    }
    if (out_slot) *out_slot = slot;
    b3d_pickup_status(world, "pickup spawned from INI");
    return 1;
}


int blank3d_pickups_spawn_rpyl_args(Blank3DPickupWorld *world,
                                    const char **args, int argc,
                                    int forced_kind,
                                    unsigned int *out_slot)
{
    char config_path[B3D_PICKUP_PATH_CAP];
    int pos_index;
    long x_q16;
    long y_q16;
    long z_q16;
    if (!world || !args || argc < 5) return 0;
    pos_index = 1;
    while (pos_index < argc && strcmp(args[pos_index], "pos") != 0)
        ++pos_index;
    if (pos_index <= 0 || pos_index + 3 >= argc) {
        b3d_pickup_status(world, "pickup RPY args missing pos");
        return 0;
    }
    if (!b3d_join_rpyl_path(args, pos_index, config_path,
                            sizeof(config_path))) {
        b3d_pickup_status(world, "pickup RPY path invalid");
        return 0;
    }
    if (!b3d_parse_q16(args[pos_index + 1], &x_q16) ||
        !b3d_parse_q16(args[pos_index + 2], &y_q16) ||
        !b3d_parse_q16(args[pos_index + 3], &z_q16)) {
        b3d_pickup_status(world, "pickup RPY position invalid");
        return 0;
    }
    return blank3d_pickups_spawn_ini(world, config_path,
                                     b3d_q16_to_q12(x_q16),
                                     b3d_q16_to_q12(y_q16),
                                     b3d_q16_to_q12(z_q16),
                                     forced_kind, out_slot);
}

int blank3d_pickups_is_world_active(const Blank3DPickupWorld *world,
                                    unsigned int slot)
{
    const Blank3DPickupInstance *instance;
    if (!world || !world->systems || slot >= B3D_PICKUP_MAX_INSTANCES) return 0;
    instance = &world->instances[slot];
    if (!instance->used || instance->item_id == PBB_ITEM_INVALID_ID) return 0;
    return pbb_item_is_world_active(
        (PBB_ItemWorld *)&world->systems->item_world, instance->item_id);
}

Blank3DPickupInstance *blank3d_pickups_get(Blank3DPickupWorld *world,
                                          unsigned int slot)
{
    if (!world || slot >= B3D_PICKUP_MAX_INSTANCES ||
        !world->instances[slot].used) return 0;
    return &world->instances[slot];
}

const Blank3DPickupInstance *blank3d_pickups_get_const(
    const Blank3DPickupWorld *world, unsigned int slot)
{
    if (!world || slot >= B3D_PICKUP_MAX_INSTANCES ||
        !world->instances[slot].used) return 0;
    return &world->instances[slot];
}

Blank3DPickupInstance *blank3d_pickups_find_subject(Blank3DPickupWorld *world,
                                                    unsigned long subject_id)
{
    unsigned int i;
    if (!world) return 0;
    for (i = 0U; i < B3D_PICKUP_MAX_INSTANCES; ++i)
        if (world->instances[i].used &&
            world->instances[i].subject_id == subject_id)
            return &world->instances[i];
    return 0;
}

const Blank3DPickupInstance *blank3d_pickups_find_subject_const(
    const Blank3DPickupWorld *world, unsigned long subject_id)
{
    unsigned int i;
    if (!world) return 0;
    for (i = 0U; i < B3D_PICKUP_MAX_INSTANCES; ++i)
        if (world->instances[i].used &&
            world->instances[i].subject_id == subject_id)
            return &world->instances[i];
    return 0;
}

const char *blank3d_pickups_status(const Blank3DPickupWorld *world)
{
    return world ? world->status : "pickup world unavailable";
}
