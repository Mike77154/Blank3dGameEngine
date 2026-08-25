#include "blank3d_objects.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
        } else if (strcmp(section, "class") == 0) {
            if (strcmp(key, "name") == 0)
                b3d_copy(init->class_name, sizeof(init->class_name), value);
            else if (strcmp(key, "bases") == 0)
                b3d_copy(init->class_bases, sizeof(init->class_bases), value);
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


static Blank3DObjectEntity *b3d_find_self(Blank3DObjects *objects,
                                           gfo_self self)
{
    unsigned int i;
    if (!objects) return 0;
    for (i = 0U; i < B3D_OBJECT_MAX_ENTITIES; ++i) {
        Blank3DObjectEntity *entity;
        entity = &objects->entities[i];
        if (entity->alive && entity->runtime_key == (unsigned long)self)
            return entity;
    }
    return 0;
}

static Blank3DObjectDefinition *b3d_entity_definition(
    Blank3DObjects *objects, const Blank3DObjectEntity *entity)
{
    if (!objects || !entity || entity->definition_slot >= B3D_OBJECT_MAX_DEFINITIONS)
        return 0;
    if (!objects->definitions[entity->definition_slot].used) return 0;
    return &objects->definitions[entity->definition_slot];
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
    Blank3DObjectDefinition *definition;
    gfo_str name;
    objects = (Blank3DObjects *)user;
    entity = b3d_find_self(objects, self);
    definition = b3d_entity_definition(objects, entity);
    if (!objects || !entity || !definition) return;
    name = gfo_sym_name_by_id(&definition->gfo.sym, inv_id);
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
    Blank3DObjectDefinition *definition;
    gfo_str name;
    objects = (Blank3DObjects *)user;
    entity = b3d_find_self(objects, self);
    definition = b3d_entity_definition(objects, entity);
    if (!objects || !entity || !definition || !objects->host.handle) return;
    name = gfo_sym_name_by_id(&definition->gfo.sym, handler_id);
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
    Blank3DObjectDefinition *definition;
    gfo_str language;
    objects = (Blank3DObjects *)user;
    entity = b3d_find_self(objects, self);
    definition = b3d_entity_definition(objects, entity);
    if (!objects || !entity || !definition || !objects->host.draw_script) return;
    language = gfo_sym_name_by_id(&definition->gfo.sym, lang_id);
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

static int b3d_find_definition(const Blank3DObjects *objects,
                               const char *ini_path)
{
    unsigned int i;
    if (!objects || !ini_path) return -1;
    for (i = 0U; i < B3D_OBJECT_MAX_DEFINITIONS; ++i)
        if (objects->definitions[i].used &&
            strcmp(objects->definitions[i].ini_path, ini_path) == 0)
            return (int)i;
    return -1;
}

static int b3d_load_definition(Blank3DObjects *objects, const char *ini_path)
{
    unsigned int i;
    Blank3DObjectDefinition *definition;
    gfo_limits limits;
    gfo_caps caps;
    gfo_binds binds;
    int result;
    if (!objects || !ini_path) return -1;
    result = b3d_find_definition(objects, ini_path);
    if (result >= 0) return result;
    for (i = 0U; i < B3D_OBJECT_MAX_DEFINITIONS; ++i)
        if (!objects->definitions[i].used) break;
    if (i >= B3D_OBJECT_MAX_DEFINITIONS) {
        b3d_copy(objects->status, sizeof(objects->status), "GFO object definition table full");
        return -1;
    }
    definition = &objects->definitions[i];
    memset(definition, 0, sizeof(*definition));
    if (!b3d_load_ini(ini_path, &definition->init)) {
        b3d_copy(objects->status, sizeof(objects->status), "object INI load failed");
        return -1;
    }
    if (!definition->init.class_name[0])
        b3d_copy(definition->init.class_name, sizeof(definition->init.class_name),
                 definition->init.name);
    if (!objects->classes ||
        blank3d_classes_resolve_or_create(objects->classes,
            definition->init.class_name, definition->init.class_bases,
            &definition->class_handle) != CM89_OK) {
        b3d_copy(objects->status, sizeof(objects->status),
                 "object ClassManager registration failed");
        memset(definition, 0, sizeof(*definition));
        return -1;
    }
    if (!b3d_read_text(definition->init.gfo_path, definition->gfo_source,
                       sizeof(definition->gfo_source))) {
        b3d_copy(objects->status, sizeof(objects->status), "GFO source load failed");
        return -1;
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
    limits.max_instances = B3D_OBJECT_MAX_ENTITIES;
    limits.max_props = 64;
    caps = gfo_caps_default(limits);
    result = gfo_init_ex(&definition->gfo, definition->gfo_arena,
                         (gfo_u32)sizeof(definition->gfo_arena),
                         limits, caps, binds);
    if (result != GFO_OK ||
        gfo_compile(&definition->gfo, definition->gfo_source,
                    (gfo_u32)strlen(definition->gfo_source)) != GFO_OK) {
        b3d_copy(objects->status, sizeof(objects->status), "GFO object definition compile failed");
        memset(definition, 0, sizeof(*definition));
        return -1;
    }
    definition->used = 1;
    b3d_copy(definition->ini_path, sizeof(definition->ini_path), ini_path);
    if (i + 1U > objects->definition_count) objects->definition_count = i + 1U;
    return (int)i;
}

void blank3d_objects_init(Blank3DObjects *objects,
                          const Blank3DObjectHost *host,
                          Blank3DClassSystem *classes)
{
    if (!objects) return;
    memset(objects, 0, sizeof(*objects));
    if (host) objects->host = *host;
    objects->classes = classes;
    b3d_copy(objects->status, sizeof(objects->status),
             classes ? "Class -> GFO ObjectSystem initialized"
                     : "GFO ObjectSystem initialized without ClassSystem");
}

void blank3d_objects_clear_instances(Blank3DObjects *objects)
{
    unsigned int i;
    if (!objects) return;
    for (i = 0U; i < objects->count; ++i)
        if (objects->entities[i].alive) blank3d_objects_kill(objects, i);
    memset(objects->entities, 0, sizeof(objects->entities));
    objects->count = 0U;
}

int blank3d_objects_spawn_ex(Blank3DObjects *objects, const char *ini_path,
                             unsigned long entity_id,
                             unsigned long runtime_key,
                             void *native_entity,
                             unsigned int *out_slot)
{
    unsigned int i;
    int definition_slot;
    Blank3DObjectDefinition *definition;
    Blank3DObjectEntity *entity;
    gfo_str type_name;
    if (!objects || !ini_path || runtime_key == 0UL) return 0;
    for (i = 0U; i < B3D_OBJECT_MAX_ENTITIES; ++i)
        if (!objects->entities[i].alive) break;
    if (i >= B3D_OBJECT_MAX_ENTITIES) return 0;
    definition_slot = b3d_load_definition(objects, ini_path);
    if (definition_slot < 0) return 0;
    definition = &objects->definitions[definition_slot];
    entity = &objects->entities[i];
    memset(entity, 0, sizeof(*entity));
    entity->entity_id = entity_id;
    entity->runtime_key = runtime_key;
    entity->native_entity = native_entity;
    entity->definition_slot = (unsigned int)definition_slot;
    entity->init = definition->init;
    /* The GFO create lifecycle runs synchronously inside gfo_spawn().
       Publish the host record for self-resolution during construction; every
       failure path below clears it before returning. */
    entity->alive = 1;
    type_name.ptr = definition->init.name;
    type_name.len = (gfo_u16)strlen(definition->init.name);
    if (objects->host.begin_event &&
        !objects->host.begin_event(objects->host.user, entity_id, runtime_key,
                                   B3D_OBJECT_EVENT_CREATE)) {
        b3d_copy(objects->status, sizeof(objects->status),
                 "GFO create event frame begin failed");
        memset(entity, 0, sizeof(*entity));
        return 0;
    }
    if (gfo_spawn(&definition->gfo, type_name, (gfo_self)runtime_key,
                  &entity->gfo_instance) != GFO_OK) {
        if (objects->host.end_event)
            objects->host.end_event(objects->host.user, entity_id, runtime_key,
                                    B3D_OBJECT_EVENT_CREATE);
        b3d_copy(objects->status, sizeof(objects->status), "GFO Object instance spawn failed");
        memset(entity, 0, sizeof(*entity));
        return 0;
    }
    if (objects->host.end_event)
        objects->host.end_event(objects->host.user, entity_id, runtime_key,
                                B3D_OBJECT_EVENT_CREATE);
    ++definition->ref_count;
    if (i + 1U > objects->count) objects->count = i + 1U;
    /* gfo_spawn dispatches create exactly once. */
    if (out_slot) *out_slot = i;
    b3d_copy(objects->status, sizeof(objects->status), "GFO Object -> Thing instance spawned");
    return 1;
}

int blank3d_objects_spawn(Blank3DObjects *objects, const char *ini_path,
                          unsigned long entity_id, void *native_entity,
                          unsigned int *out_slot)
{
    return blank3d_objects_spawn_ex(objects, ini_path, entity_id,
                                    entity_id, native_entity, out_slot);
}

void blank3d_objects_tick(Blank3DObjects *objects)
{
    unsigned int i;
    if (!objects) return;
    for (i = 0U; i < objects->count; ++i) {
        Blank3DObjectEntity *entity;
        Blank3DObjectDefinition *definition;
        entity = &objects->entities[i];
        if (!entity->alive) continue;
        definition = b3d_entity_definition(objects, entity);
        if (definition) {
            int event_open;
            event_open = !objects->host.begin_event ||
                objects->host.begin_event(objects->host.user, entity->entity_id,
                    entity->runtime_key, B3D_OBJECT_EVENT_STEP);
            if (event_open) {
                gfo_tick_one(&definition->gfo, entity->gfo_instance, GFO_LC_STEP);
                if (objects->host.end_event)
                    objects->host.end_event(objects->host.user, entity->entity_id,
                        entity->runtime_key, B3D_OBJECT_EVENT_STEP);
            }
        }
    }
}

void blank3d_objects_render(Blank3DObjects *objects)
{
    unsigned int i;
    if (!objects) return;
    for (i = 0U; i < objects->count; ++i) {
        Blank3DObjectEntity *entity;
        Blank3DObjectDefinition *definition;
        entity = &objects->entities[i];
        if (!entity->alive) continue;
        definition = b3d_entity_definition(objects, entity);
        if (definition) {
            int event_open;
            event_open = !objects->host.begin_event ||
                objects->host.begin_event(objects->host.user, entity->entity_id,
                    entity->runtime_key, B3D_OBJECT_EVENT_RENDER);
            if (event_open) {
                gfo_tick_one(&definition->gfo, entity->gfo_instance, GFO_LC_RENDER);
                if (objects->host.end_event)
                    objects->host.end_event(objects->host.user, entity->entity_id,
                        entity->runtime_key, B3D_OBJECT_EVENT_RENDER);
            }
        }
    }
}

void blank3d_objects_kill(Blank3DObjects *objects, unsigned int slot)
{
    Blank3DObjectEntity *entity;
    Blank3DObjectDefinition *definition;
    if (!objects || slot >= objects->count || !objects->entities[slot].alive) return;
    entity = &objects->entities[slot];
    definition = b3d_entity_definition(objects, entity);
    if (definition) {
        int event_open;
        event_open = !objects->host.begin_event ||
            objects->host.begin_event(objects->host.user, entity->entity_id,
                entity->runtime_key, B3D_OBJECT_EVENT_DESTROY);
        (void)gfo_kill(&definition->gfo, entity->gfo_instance);
        if (event_open && objects->host.end_event)
            objects->host.end_event(objects->host.user, entity->entity_id,
                entity->runtime_key, B3D_OBJECT_EVENT_DESTROY);
        if (definition->ref_count > 0U) --definition->ref_count;
    }
    entity->alive = 0;
}

const Blank3DObjectEntity *blank3d_objects_get(
    const Blank3DObjects *objects, unsigned int slot)
{
    if (!objects || slot >= objects->count) return 0;
    return &objects->entities[slot];
}

const Blank3DObjectDefinition *blank3d_objects_definition(
    const Blank3DObjects *objects, unsigned int slot)
{
    if (!objects || slot >= B3D_OBJECT_MAX_DEFINITIONS ||
        !objects->definitions[slot].used) return 0;
    return &objects->definitions[slot];
}

cm89_class_h blank3d_objects_class_of_slot(
    const Blank3DObjects *objects, unsigned int slot)
{
    const Blank3DObjectDefinition *definition;
    if (!objects || slot >= objects->count || !objects->entities[slot].alive)
        return CM89_CLASS_NONE;
    definition = blank3d_objects_definition(objects,
        objects->entities[slot].definition_slot);
    return definition ? definition->class_handle : CM89_CLASS_NONE;
}

cm89_class_h blank3d_objects_class_of_runtime_key(
    const Blank3DObjects *objects, unsigned long runtime_key)
{
    unsigned int i;
    if (!objects || runtime_key == 0UL) return CM89_CLASS_NONE;
    for (i = 0U; i < objects->count; ++i)
        if (objects->entities[i].alive &&
            objects->entities[i].runtime_key == runtime_key)
            return blank3d_objects_class_of_slot(objects, i);
    return CM89_CLASS_NONE;
}

cm89_result blank3d_objects_call_class_method(
    Blank3DObjects *objects, unsigned int slot, const char *method_name,
    const cm89_call *call, cm89_value *out_value)
{
    cm89_class_h class_handle;
    if (!objects || !objects->classes || slot >= objects->count ||
        !objects->entities[slot].alive || !method_name)
        return CM89_ERR_ARGUMENT;
    class_handle = blank3d_objects_class_of_slot(objects, slot);
    if (class_handle == CM89_CLASS_NONE) return CM89_ERR_INVALID_HANDLE;
    return blank3d_classes_call_bound(objects->classes, class_handle,
        objects->entities[slot].runtime_key, method_name, call, out_value);
}

const char *blank3d_objects_status(const Blank3DObjects *objects)
{
    return objects ? objects->status : "GFO ObjectSystem unavailable";
}
