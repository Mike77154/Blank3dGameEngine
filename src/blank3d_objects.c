#include "blank3d_objects.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static Blank3DObjects *b3d_gfo_owner;

static void b3d_copy(char *dst, unsigned int cap, const char *src)
{
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    strncpy(dst, src, cap - 1U);
    dst[cap - 1U] = '\0';
}

static char *b3d_trim(char *text)
{
    char *end;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

static int b3d_read_text(const char *path, char *out, unsigned int cap)
{
    FILE *file;
    size_t n;
    int extra;
    if (!path || !out || cap < 2U) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    n = fread(out, 1U, cap - 1U, file);
    extra = fgetc(file);
    fclose(file);
    if (extra != EOF) return 0;
    out[n] = '\0';
    return 1;
}

static long b3d_parse_q16(const char *text, long fallback)
{
    long whole;
    long frac;
    long sign;
    const char *dot;
    int digits;
    if (!text || !*text) return fallback;
    sign = 1L;
    if (*text == '-') { sign = -1L; ++text; }
    whole = atol(text);
    dot = strchr(text, '.');
    frac = 0L;
    digits = 0;
    if (dot) {
        ++dot;
        while (*dot && digits < 4 && isdigit((unsigned char)*dot)) {
            frac = frac * 10L + (long)(*dot - '0');
            ++digits;
            ++dot;
        }
        while (digits < 4) { frac *= 10L; ++digits; }
    }
    return sign * (whole * 65536L + (frac * 65536L) / 10000L);
}

static Blank3DMeshKind b3d_mesh_kind(const char *spec)
{
    const char *dot;
    if (!spec) return B3D_MESH_NONE;
    if (strcmp(spec, "cube") == 0) return B3D_MESH_CUBE;
    if (strcmp(spec, "sphere") == 0) return B3D_MESH_SPHERE;
    if (strcmp(spec, "capsule") == 0) return B3D_MESH_CAPSULE;
    if (strcmp(spec, "composed") == 0) return B3D_MESH_COMPOSED;
    if (strncmp(spec, "customcalled:", 13U) == 0)
        return B3D_MESH_CUSTOMCALLED;
    dot = strrchr(spec, '.');
    if (dot && (strcmp(dot, ".obj") == 0 || strcmp(dot, ".fbx") == 0 ||
                strcmp(dot, ".dae") == 0 || strcmp(dot, ".gltf") == 0 ||
                strcmp(dot, ".glb") == 0)) return B3D_MESH_EXTERNAL;
    return B3D_MESH_NONE;
}

static Blank3DLogicKind b3d_logic_kind(const char *text)
{
    if (!text) return B3D_LOGIC_NONE;
    if (strcmp(text, "ddsl2") == 0) return B3D_LOGIC_DDSL2;
    if (strcmp(text, "fpi") == 0 || strcmp(text, "fpil") == 0)
        return B3D_LOGIC_FPIL;
    if (strcmp(text, "rpy") == 0 || strcmp(text, "rpyl") == 0)
        return B3D_LOGIC_RPYL;
    return B3D_LOGIC_NONE;
}

static int b3d_load_ini(const char *path, Blank3DObjectInit *init)
{
    FILE *file;
    char line[384];
    char section[48];
    memset(init, 0, sizeof(*init));
    init->hp = 100;
    init->speed_q16 = 65536L;
    init->scale_x_q16 = 65536L;
    init->scale_y_q16 = 65536L;
    init->scale_z_q16 = 65536L;
    file = fopen(path, "rb");
    if (!file) return 0;
    section[0] = '\0';
    while (fgets(line, sizeof(line), file)) {
        char *p;
        char *eq;
        char *key;
        char *value;
        p = b3d_trim(line);
        if (!*p || *p == ';' || *p == '#') continue;
        if (*p == '[') {
            char *close;
            close = strchr(p, ']');
            if (close) {
                *close = '\0';
                b3d_copy(section, sizeof(section), p + 1);
            }
            continue;
        }
        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = b3d_trim(p);
        value = b3d_trim(eq + 1);
        if (strcmp(section, "entity") == 0) {
            if (strcmp(key, "name") == 0) b3d_copy(init->name, sizeof(init->name), value);
            else if (strcmp(key, "gfo") == 0) b3d_copy(init->gfo_path, sizeof(init->gfo_path), value);
            else if (strcmp(key, "hp") == 0) init->hp = atoi(value);
            else if (strcmp(key, "speed") == 0) init->speed_q16 = b3d_parse_q16(value, init->speed_q16);
        } else if (strcmp(section, "logic") == 0) {
            if (strcmp(key, "type") == 0) init->logic_kind = b3d_logic_kind(value);
            else if (strcmp(key, "script") == 0) b3d_copy(init->logic_path, sizeof(init->logic_path), value);
        } else if (strcmp(section, "visual") == 0) {
            if (strcmp(key, "mesh") == 0) {
                b3d_copy(init->mesh_spec, sizeof(init->mesh_spec), value);
                init->mesh_kind = b3d_mesh_kind(value);
            } else if (strcmp(key, "draw_script") == 0) {
                b3d_copy(init->draw_script, sizeof(init->draw_script), value);
            } else if (strcmp(key, "scale_x") == 0) init->scale_x_q16 = b3d_parse_q16(value, init->scale_x_q16);
            else if (strcmp(key, "scale_y") == 0) init->scale_y_q16 = b3d_parse_q16(value, init->scale_y_q16);
            else if (strcmp(key, "scale_z") == 0) init->scale_z_q16 = b3d_parse_q16(value, init->scale_z_q16);
        }
    }
    fclose(file);
    return init->name[0] && init->gfo_path[0];
}

static Blank3DObjectEntity *b3d_find_self(gfo_self self)
{
    unsigned int i;
    if (!b3d_gfo_owner) return 0;
    for (i = 0U; i < B3D_OBJECT_MAX_ENTITIES; ++i) {
        Blank3DObjectEntity *entity;
        entity = &b3d_gfo_owner->entities[i];
        if (entity->alive && entity->entity_id == (unsigned long)self)
            return entity;
    }
    return 0;
}

static gfo_u16 b3d_resolve(void *user, gfo_u16 sym_id)
{
    (void)user;
    return sym_id;
}

static void b3d_invoker(void *user, gfo_u16 inv_id, gfo_self self,
                        const gfo_value *argv, gfo_u8 argc)
{
    Blank3DObjects *objects;
    Blank3DObjectEntity *entity;
    gfo_str name;
    objects = (Blank3DObjects *)user;
    entity = b3d_find_self(self);
    if (!objects || !entity) return;
    name = gfo_sym_name_by_id(&entity->gfo.sym, inv_id);
    if (!name.ptr) return;
    if (name.len == 9U && strncmp(name.ptr, "run_logic", 9U) == 0) {
        if (entity->init.logic_kind == B3D_LOGIC_DDSL2 && objects->host.run_ddsl2)
            objects->host.run_ddsl2(objects->host.user, entity->entity_id,
                                    entity->native_entity, entity->init.logic_path);
        else if (entity->init.logic_kind == B3D_LOGIC_FPIL && objects->host.run_fpil)
            objects->host.run_fpil(objects->host.user, entity->entity_id,
                                   entity->native_entity, entity->init.logic_path);
        else if (entity->init.logic_kind == B3D_LOGIC_RPYL && objects->host.run_rpyl)
            objects->host.run_rpyl(objects->host.user, entity->entity_id,
                                   entity->native_entity, entity->init.logic_path);
        return;
    }
    if (name.len == 9U && strncmp(name.ptr, "draw_mesh", 9U) == 0) {
        if (objects->host.draw_mesh)
            objects->host.draw_mesh(objects->host.user, entity->entity_id,
                                    entity->native_entity, &entity->init);
        return;
    }
    if (objects->host.invoke) {
        char temp[64];
        unsigned int n;
        n = name.len < sizeof(temp) - 1U ? name.len : sizeof(temp) - 1U;
        memcpy(temp, name.ptr, n);
        temp[n] = '\0';
        objects->host.invoke(objects->host.user, entity->entity_id,
                             entity->native_entity, temp, argv, argc);
    }
}

static void b3d_handler(void *user, gfo_u16 handler_id, gfo_self self)
{
    Blank3DObjects *objects;
    Blank3DObjectEntity *entity;
    gfo_str name;
    objects = (Blank3DObjects *)user;
    entity = b3d_find_self(self);
    if (!objects || !entity || !objects->host.handle) return;
    name = gfo_sym_name_by_id(&entity->gfo.sym, handler_id);
    if (name.ptr) {
        char temp[64];
        unsigned int n;
        n = name.len < sizeof(temp) - 1U ? name.len : sizeof(temp) - 1U;
        memcpy(temp, name.ptr, n);
        temp[n] = '\0';
        objects->host.handle(objects->host.user, entity->entity_id,
                             entity->native_entity, temp);
    }
}

static void b3d_script(void *user, gfo_u16 lang_id, gfo_self self,
                       gfo_str code)
{
    Blank3DObjects *objects;
    Blank3DObjectEntity *entity;
    gfo_str language;
    objects = (Blank3DObjects *)user;
    entity = b3d_find_self(self);
    if (!objects || !entity || !objects->host.draw_script) return;
    language = gfo_sym_name_by_id(&entity->gfo.sym, lang_id);
    if (language.ptr) {
        char temp[32];
        unsigned int n;
        n = language.len < sizeof(temp) - 1U ? language.len : sizeof(temp) - 1U;
        memcpy(temp, language.ptr, n);
        temp[n] = '\0';
        objects->host.draw_script(objects->host.user, entity->entity_id,
                                  entity->native_entity, temp,
                                  code.ptr, code.len);
    }
}

void blank3d_objects_init(Blank3DObjects *objects,
                          const Blank3DObjectHost *host)
{
    if (!objects) return;
    memset(objects, 0, sizeof(*objects));
    if (host) objects->host = *host;
    b3d_copy(objects->status, sizeof(objects->status), "GFO object host initialized");
    b3d_gfo_owner = objects;
}

int blank3d_objects_spawn(Blank3DObjects *objects, const char *ini_path,
                          unsigned long entity_id, void *native_entity,
                          unsigned int *out_slot)
{
    unsigned int i;
    Blank3DObjectEntity *entity;
    gfo_limits limits;
    gfo_caps caps;
    gfo_binds binds;
    gfo_str type_name;
    int result;
    if (!objects || !ini_path) return 0;
    for (i = 0U; i < B3D_OBJECT_MAX_ENTITIES; ++i)
        if (!objects->entities[i].alive) break;
    if (i >= B3D_OBJECT_MAX_ENTITIES) return 0;
    entity = &objects->entities[i];
    memset(entity, 0, sizeof(*entity));
    if (!b3d_load_ini(ini_path, &entity->init)) {
        b3d_copy(objects->status, sizeof(objects->status), "object INI load failed");
        return 0;
    }
    if (!b3d_read_text(entity->init.gfo_path, entity->gfo_source,
                       sizeof(entity->gfo_source))) {
        b3d_copy(objects->status, sizeof(objects->status), "GFO source load failed");
        return 0;
    }
    memset(&binds, 0, sizeof(binds));
    binds.user = objects;
    binds.resolve_invoker = b3d_resolve;
    binds.resolve_handler = b3d_resolve;
    binds.resolve_lang = b3d_resolve;
    binds.call_invoker = b3d_invoker;
    binds.call_handler = b3d_handler;
    binds.call_script_block = b3d_script;
    limits.max_types = 8;
    limits.max_instances = 2;
    limits.max_props = 64;
    caps = gfo_caps_default(limits);
    result = gfo_init_ex(&entity->gfo, entity->gfo_arena,
                         (gfo_u32)sizeof(entity->gfo_arena),
                         limits, caps, binds);
    if (result != GFO_OK ||
        gfo_compile(&entity->gfo, entity->gfo_source,
                    (gfo_u32)strlen(entity->gfo_source)) != GFO_OK) {
        b3d_copy(objects->status, sizeof(objects->status), "GFO compile failed");
        return 0;
    }
    type_name.ptr = entity->init.name;
    type_name.len = (gfo_u16)strlen(entity->init.name);
    if (gfo_spawn(&entity->gfo, type_name, (gfo_self)entity_id,
                  &entity->gfo_instance) != GFO_OK) {
        b3d_copy(objects->status, sizeof(objects->status), "GFO spawn failed");
        return 0;
    }
    entity->entity_id = entity_id;
    entity->native_entity = native_entity;
    entity->alive = 1;
    if (i + 1U > objects->count) objects->count = i + 1U;
    gfo_tick_one(&entity->gfo, entity->gfo_instance, GFO_LC_CREATE);
    if (out_slot) *out_slot = i;
    b3d_copy(objects->status, sizeof(objects->status), "GFO entity spawned");
    return 1;
}

void blank3d_objects_tick(Blank3DObjects *objects)
{
    unsigned int i;
    if (!objects) return;
    b3d_gfo_owner = objects;
    for (i = 0U; i < objects->count; ++i)
        if (objects->entities[i].alive)
            gfo_tick_one(&objects->entities[i].gfo,
                         objects->entities[i].gfo_instance, GFO_LC_STEP);
}

void blank3d_objects_render(Blank3DObjects *objects)
{
    unsigned int i;
    if (!objects) return;
    b3d_gfo_owner = objects;
    for (i = 0U; i < objects->count; ++i)
        if (objects->entities[i].alive)
            gfo_tick_one(&objects->entities[i].gfo,
                         objects->entities[i].gfo_instance, GFO_LC_RENDER);
}

void blank3d_objects_kill(Blank3DObjects *objects, unsigned int slot)
{
    if (!objects || slot >= objects->count || !objects->entities[slot].alive)
        return;
    b3d_gfo_owner = objects;
    gfo_kill(&objects->entities[slot].gfo,
             objects->entities[slot].gfo_instance);
    objects->entities[slot].alive = 0;
}

const Blank3DObjectEntity *blank3d_objects_get(
    const Blank3DObjects *objects, unsigned int slot)
{
    if (!objects || slot >= objects->count) return 0;
    return &objects->entities[slot];
}

const char *blank3d_objects_status(const Blank3DObjects *objects)
{
    return objects ? objects->status : "GFO object host unavailable";
}
