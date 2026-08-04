#include "blank3d_numbar.h"
#include "blank3d_hud.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* BigHud asks for an ECG target while loading a document. Numbar presets are
 * not allowed to contain ECG nodes, so one static scratch target is enough. */
static Blank3DEcgVitals b3d_numbar_ecg_scratch;

static int nb_lower(int c)
{
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static int nb_eq(const char *a, const char *b)
{
    int ca;
    int cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = nb_lower((unsigned char)*a++);
        cb = nb_lower((unsigned char)*b++);
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static int nb_starts(const char *text, const char *prefix)
{
    int a;
    int b;
    if (!text || !prefix) return 0;
    while (*prefix) {
        if (!*text) return 0;
        a = nb_lower((unsigned char)*text++);
        b = nb_lower((unsigned char)*prefix++);
        if (a != b) return 0;
    }
    return 1;
}

static void nb_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static char *nb_trim(char *text)
{
    char *end;
    if (!text) return text;
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')
        ++text;
    end = text + strlen(text);
    while (end > text && (end[-1] == ' ' || end[-1] == '\t' ||
           end[-1] == '\r' || end[-1] == '\n')) --end;
    *end = '\0';
    if (end > text + 1 && ((*text == '"' && end[-1] == '"') ||
                           (*text == '\'' && end[-1] == '\''))) {
        ++text;
        --end;
        *end = '\0';
    }
    return text;
}

static int nb_bool(const char *text, int fallback)
{
    if (!text) return fallback;
    if (nb_eq(text, "true") || nb_eq(text, "yes") || nb_eq(text, "on"))
        return 1;
    if (nb_eq(text, "false") || nb_eq(text, "no") || nb_eq(text, "off"))
        return 0;
    return atoi(text) != 0;
}

static int nb_anchor(const char *text)
{
    if (nb_eq(text, "top_center")) return B3D_BIGHUD_ANCHOR_TOP_CENTER;
    if (nb_eq(text, "top_right")) return B3D_BIGHUD_ANCHOR_TOP_RIGHT;
    if (nb_eq(text, "center")) return B3D_BIGHUD_ANCHOR_CENTER;
    if (nb_eq(text, "bottom_left")) return B3D_BIGHUD_ANCHOR_BOTTOM_LEFT;
    if (nb_eq(text, "bottom_center")) return B3D_BIGHUD_ANCHOR_BOTTOM_CENTER;
    if (nb_eq(text, "bottom_right")) return B3D_BIGHUD_ANCHOR_BOTTOM_RIGHT;
    return B3D_BIGHUD_ANCHOR_TOP_LEFT;
}

static int nb_scale_q8(const char *text)
{
    int percent;
    percent = atoi(text);
    if (percent < 1) percent = 1;
    if (percent > 800) percent = 800;
    return (percent * 256) / 100;
}

static void nb_orchestrator_defaults(Blank3DNumbarOrchestrator *config,
                                     const char *name)
{
    if (!config) return;
    memset(config, 0, sizeof(*config));
    config->enabled = 1;
    config->anchor = B3D_BIGHUD_ANCHOR_TOP_LEFT;
    config->scale_x_q8 = 256;
    config->scale_y_q8 = 256;
    nb_copy(config->name, B3D_NUMBAR_NAME_CAP, name ? name : "numbar");
    nb_copy(config->value_bind, B3D_NUMBAR_BIND_CAP, "self.health");
    nb_copy(config->min_bind, B3D_NUMBAR_BIND_CAP, "zero");
    nb_copy(config->max_bind, B3D_NUMBAR_BIND_CAP, "self.health_max");
}

static int nb_path_exists(const char *path)
{
    FILE *file;
    if (!path || !*path) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    fclose(file);
    return 1;
}

static int nb_path_absolute(const char *path)
{
    if (!path || !*path) return 0;
    if (path[0] == '/' || path[0] == '\\') return 1;
    return path[0] && path[1] == ':';
}

static int nb_resolve_preset_path(const char *ini_path,
                                  const char *preset,
                                  char *out,
                                  unsigned int out_cap)
{
    const char *slash;
    const char *backslash;
    const char *last;
    unsigned int prefix_len;
    unsigned int i;
    if (!ini_path || !preset || !out || out_cap < 2U) return 0;
    if (nb_path_absolute(preset) || nb_path_exists(preset)) {
        nb_copy(out, out_cap, preset);
        return nb_path_exists(out);
    }
    slash = strrchr(ini_path, '/');
    backslash = strrchr(ini_path, '\\');
    last = slash;
    if (!last || (backslash && backslash > last)) last = backslash;
    prefix_len = last ? (unsigned int)(last - ini_path + 1) : 0U;
    if (prefix_len + strlen(preset) + 1U > out_cap) return 0;
    for (i = 0U; i < prefix_len; ++i) out[i] = ini_path[i];
    nb_copy(out + prefix_len, out_cap - prefix_len, preset);
    return nb_path_exists(out);
}

static int nb_section_name(const char *section,
                           char *name,
                           unsigned int name_cap)
{
    const char *p;
    if (!section || !nb_starts(section, "numbar")) return 0;
    p = section + 6;
    if (*p && *p != ' ' && *p != '\t' && *p != ':' && *p != '.') return 0;
    while (*p == ' ' || *p == '\t' || *p == ':' || *p == '.') ++p;
    nb_copy(name, name_cap, *p ? p : "default");
    return 1;
}

static int nb_apply_key(Blank3DNumbarOrchestrator *config,
                        const char *key,
                        const char *value,
                        char *status,
                        unsigned int status_cap)
{
    if (!config || !key || !value) return 0;
    if (nb_eq(key, "preset") || nb_eq(key, "style"))
        nb_copy(config->preset_path, B3D_NUMBAR_PATH_CAP, value);
    else if (nb_eq(key, "enabled")) config->enabled = nb_bool(value, 1);
    else if (nb_eq(key, "space")) {
        if (!nb_eq(value, "screen")) {
            if (status && status_cap)
                sprintf(status, "numbar space is not supported: %.180s", value);
            return 0;
        }
    } else if (nb_eq(key, "anchor")) config->anchor = nb_anchor(value);
    else if (nb_eq(key, "x")) config->x = atoi(value);
    else if (nb_eq(key, "y")) config->y = atoi(value);
    else if (nb_eq(key, "z")) config->z = atoi(value);
    else if (nb_eq(key, "scale") || nb_eq(key, "scale_percent")) {
        config->scale_x_q8 = nb_scale_q8(value);
        config->scale_y_q8 = config->scale_x_q8;
    } else if (nb_eq(key, "scale_x") || nb_eq(key, "scale_x_percent"))
        config->scale_x_q8 = nb_scale_q8(value);
    else if (nb_eq(key, "scale_y") || nb_eq(key, "scale_y_percent"))
        config->scale_y_q8 = nb_scale_q8(value);
    else if (nb_eq(key, "canvas_width") || nb_eq(key, "width"))
        config->canvas_width = atoi(value);
    else if (nb_eq(key, "canvas_height") || nb_eq(key, "height"))
        config->canvas_height = atoi(value);
    else if (nb_eq(key, "bind") || nb_eq(key, "value_bind") ||
             nb_eq(key, "bind.value"))
        nb_copy(config->value_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "min_bind") || nb_eq(key, "bind.min"))
        nb_copy(config->min_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "max_bind") || nb_eq(key, "bind.max"))
        nb_copy(config->max_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "overlay_bind") || nb_eq(key, "bind.overlay"))
        nb_copy(config->overlay_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "overlay_min_bind") || nb_eq(key, "bind.overlay_min"))
        nb_copy(config->overlay_min_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "overlay_max_bind") || nb_eq(key, "bind.overlay_max"))
        nb_copy(config->overlay_max_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "mid_bind") || nb_eq(key, "bind.mid"))
        nb_copy(config->mid_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "segments_bind") || nb_eq(key, "bind.segments"))
        nb_copy(config->segments_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "state_bind") || nb_eq(key, "bind.state"))
        nb_copy(config->state_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "phase_bind") || nb_eq(key, "radial_phase_bind") ||
             nb_eq(key, "bind.phase"))
        nb_copy(config->phase_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "units_bind") || nb_eq(key, "unit_count_bind") ||
             nb_eq(key, "bind.units"))
        nb_copy(config->units_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "layer_count_bind") || nb_eq(key, "bind.layer_count"))
        nb_copy(config->layer_count_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "layer_size_bind") || nb_eq(key, "bind.layer_size"))
        nb_copy(config->layer_size_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "bind2") || nb_eq(key, "value2_bind") ||
             nb_eq(key, "bind.value2") || nb_eq(key, "reserve_bind"))
        nb_copy(config->value2_bind, B3D_NUMBAR_BIND_CAP, value);
    else if (nb_eq(key, "visible_bind") || nb_eq(key, "visible_when") ||
             nb_eq(key, "bind.visible"))
        nb_copy(config->visible_bind, B3D_NUMBAR_BIND_CAP, value);
    else {
        if (status && status_cap)
            sprintf(status, "unknown numbar INI key: %.180s", key);
        return 0;
    }
    return 1;
}

static Blank3DNumbarInstance *nb_alloc(Blank3DNumbarSystem *system)
{
    int i;
    if (!system) return (Blank3DNumbarInstance *)0;
    for (i = 0; i < B3D_NUMBAR_MAX_INSTANCES; ++i) {
        if (!system->instances[i].alive) {
            memset(&system->instances[i], 0, sizeof(system->instances[i]));
            system->instances[i].alive = 1;
            if (i + 1 > system->instance_count) system->instance_count = i + 1;
            return &system->instances[i];
        }
    }
    nb_copy(system->status, B3D_NUMBAR_STATUS_CAP,
            "numbar instance pool exhausted");
    return (Blank3DNumbarInstance *)0;
}

static void nb_recount(Blank3DNumbarSystem *system)
{
    int i;
    if (!system) return;
    for (i = B3D_NUMBAR_MAX_INSTANCES - 1; i >= 0; --i) {
        if (system->instances[i].alive) {
            system->instance_count = i + 1;
            return;
        }
    }
    system->instance_count = 0;
}

static void nb_rollback(Blank3DNumbarSystem *system,
                        unsigned long entity_id,
                        const char *path)
{
    int i;
    if (!system || !path) return;
    for (i = 0; i < system->instance_count; ++i) {
        Blank3DNumbarInstance *instance;
        instance = &system->instances[i];
        if (instance->alive && instance->entity_id == entity_id &&
            strcmp(instance->orchestrator_path, path) == 0)
            memset(instance, 0, sizeof(*instance));
    }
    nb_recount(system);
}

static int nb_finalize(Blank3DNumbarSystem *system,
                       unsigned long entity_id,
                       void *native_entity,
                       const char *ini_path,
                       Blank3DNumbarOrchestrator *config)
{
    Blank3DNumbarInstance *instance;
    char resolved_path[B3D_NUMBAR_PATH_CAP];
    int i;
    if (!system || !ini_path || !config) return 0;
    if (!config->preset_path[0]) {
        sprintf(system->status, "numbar %.44s has no preset",
                config->name);
        return 0;
    }
    if (!nb_resolve_preset_path(ini_path, config->preset_path,
                                resolved_path, sizeof(resolved_path))) {
        sprintf(system->status, "cannot resolve numbar preset: %.170s",
                config->preset_path);
        return 0;
    }
    nb_copy(config->preset_path, B3D_NUMBAR_PATH_CAP, resolved_path);
    instance = nb_alloc(system);
    if (!instance) return 0;
    instance->entity_id = entity_id;
    instance->native_entity = native_entity;
    instance->requested = 1;
    nb_copy(instance->orchestrator_path, B3D_NUMBAR_PATH_CAP, ini_path);
    instance->orchestrator = *config;
    blank3d_ecg_vitals_init(&b3d_numbar_ecg_scratch);
    if (!blank3d_bighud_load(&instance->layout,
                             &b3d_numbar_ecg_scratch,
                             instance->orchestrator.preset_path)) {
        sprintf(system->status, "numbar preset %.92s failed: %.130s",
                instance->orchestrator.name,
                blank3d_bighud_error(&instance->layout));
        memset(instance, 0, sizeof(*instance));
        nb_recount(system);
        return 0;
    }
    for (i = 0; i < instance->layout.node_count; ++i) {
        if (instance->layout.nodes[i].type == B3D_BIGHUD_NODE_ECG) {
            sprintf(system->status,
                    "numbar preset %.120s contains an ECG node",
                    instance->orchestrator.name);
            memset(instance, 0, sizeof(*instance));
            nb_recount(system);
            return 0;
        }
    }
    if (instance->orchestrator.canvas_width < 1)
        instance->orchestrator.canvas_width = instance->layout.canvas_width;
    if (instance->orchestrator.canvas_height < 1)
        instance->orchestrator.canvas_height = instance->layout.canvas_height;
    if (instance->orchestrator.canvas_width < 1)
        instance->orchestrator.canvas_width = 1;
    if (instance->orchestrator.canvas_height < 1)
        instance->orchestrator.canvas_height = 1;
    return 1;
}

static int nb_load_orchestrator(Blank3DNumbarSystem *system,
                                unsigned long entity_id,
                                void *native_entity,
                                const char *path)
{
    FILE *file;
    char line[512];
    char section[96];
    char numbar_name[B3D_NUMBAR_NAME_CAP];
    char *text;
    char *equal;
    char *key;
    char *value;
    Blank3DNumbarOrchestrator current;
    int has_current;
    int loaded;
    int global_enabled;
    if (!system || !path) return 0;
    file = fopen(path, "rb");
    if (!file) {
        sprintf(system->status, "cannot open numbar INI: %.180s", path);
        return 0;
    }
    section[0] = '\0';
    has_current = 0;
    loaded = 0;
    global_enabled = 1;
    while (fgets(line, sizeof(line), file)) {
        text = nb_trim(line);
        if (!*text || *text == ';' || *text == '#') continue;
        if (*text == '[') {
            char *close;
            if (has_current) {
                current.enabled = current.enabled && global_enabled;
                if (!nb_finalize(system, entity_id, native_entity,
                                 path, &current)) {
                    fclose(file);
                    return 0;
                }
                ++loaded;
                has_current = 0;
            }
            close = strchr(text + 1, ']');
            if (!close) {
                nb_copy(system->status, B3D_NUMBAR_STATUS_CAP,
                        "unterminated numbar INI section");
                fclose(file);
                return 0;
            }
            *close = '\0';
            nb_copy(section, sizeof(section), nb_trim(text + 1));
            if (nb_section_name(section, numbar_name,
                                sizeof(numbar_name))) {
                nb_orchestrator_defaults(&current, numbar_name);
                has_current = 1;
            }
            continue;
        }
        equal = strchr(text, '=');
        if (!equal) continue;
        *equal = '\0';
        key = nb_trim(text);
        value = nb_trim(equal + 1);
        if (has_current) {
            if (!nb_apply_key(&current, key, value,
                              system->status, sizeof(system->status))) {
                fclose(file);
                return 0;
            }
        } else if (nb_eq(section, "orchestrator") ||
                   nb_eq(section, "hud")) {
            if (nb_eq(key, "enabled")) global_enabled = nb_bool(value, 1);
            else {
                sprintf(system->status,
                        "unknown orchestrator INI key: %.170s", key);
                fclose(file);
                return 0;
            }
        }
    }
    if (has_current) {
        current.enabled = current.enabled && global_enabled;
        if (!nb_finalize(system, entity_id, native_entity, path, &current)) {
            fclose(file);
            return 0;
        }
        ++loaded;
    }
    fclose(file);
    if (loaded <= 0) {
        sprintf(system->status,
                "numbar INI contains no [numbar name] sections: %.140s",
                path);
        return 0;
    }
    sprintf(system->status, "loaded %d numbar instance(s) from %.150s",
            loaded, path);
    return loaded;
}

void blank3d_numbar_init(Blank3DNumbarSystem *system,
                          Blank3DNumbarResolveFn resolve,
                          void *resolve_user)
{
    if (!system) return;
    memset(system, 0, sizeof(*system));
    system->resolve = resolve;
    system->resolve_user = resolve_user;
    nb_copy(system->status, B3D_NUMBAR_STATUS_CAP,
            "numbar orchestrator ready");
}

void blank3d_numbar_begin_frame(Blank3DNumbarSystem *system)
{
    int i;
    if (!system) return;
    for (i = 0; i < system->instance_count; ++i)
        system->instances[i].requested = 0;
}

int blank3d_numbar_request(Blank3DNumbarSystem *system,
                           unsigned long entity_id,
                           void *native_entity,
                           const char *orchestrator_path)
{
    int i;
    int found;
    int loaded;
    if (!system || !orchestrator_path || !*orchestrator_path) return 0;
    found = 0;
    for (i = 0; i < system->instance_count; ++i) {
        Blank3DNumbarInstance *instance;
        instance = &system->instances[i];
        if (instance->alive && instance->entity_id == entity_id &&
            strcmp(instance->orchestrator_path, orchestrator_path) == 0) {
            instance->native_entity = native_entity;
            instance->requested = 1;
            ++found;
        }
    }
    if (found > 0) return found;
    loaded = nb_load_orchestrator(system, entity_id, native_entity,
                                  orchestrator_path);
    if (loaded <= 0) {
        nb_rollback(system, entity_id, orchestrator_path);
        return 0;
    }
    return loaded;
}

static long nb_resolve(Blank3DNumbarSystem *system,
                       Blank3DNumbarInstance *instance,
                       const char *binding,
                       long fallback)
{
    int resolved;
    char *end;
    long literal;
    if (!binding || !*binding) return fallback;
    literal = strtol(binding, &end, 0);
    while (*end == ' ' || *end == '\t') ++end;
    if (*end == '\0') return literal;
    if (nb_eq(binding, "zero")) return 0L;
    if (nb_eq(binding, "one")) return 1L;
    if (!system || !system->resolve || !instance) return fallback;
    resolved = 0;
    literal = system->resolve(system->resolve_user,
                              instance->entity_id,
                              instance->native_entity,
                              binding, fallback, &resolved);
    return resolved ? literal : fallback;
}

static void nb_telemetry(Blank3DNumbarSystem *system,
                         Blank3DNumbarInstance *instance,
                         Blank3DBigHudTelemetry *telemetry)
{
    Blank3DNumbarOrchestrator *config;
    if (!telemetry || !instance) return;
    memset(telemetry, 0, sizeof(*telemetry));
    config = &instance->orchestrator;
    telemetry->numbar_value = nb_resolve(system, instance,
                                         config->value_bind, 0L);
    telemetry->numbar_min = nb_resolve(system, instance,
                                       config->min_bind, 0L);
    telemetry->numbar_max = nb_resolve(system, instance,
                                       config->max_bind, 100L);
    telemetry->numbar_overlay = nb_resolve(system, instance,
                                            config->overlay_bind,
                                            telemetry->numbar_value);
    telemetry->numbar_overlay_min = nb_resolve(system, instance,
                                                config->overlay_min_bind,
                                                telemetry->numbar_min);
    telemetry->numbar_overlay_max = nb_resolve(system, instance,
                                                config->overlay_max_bind,
                                                telemetry->numbar_max);
    telemetry->numbar_mid = nb_resolve(system, instance, config->mid_bind,
                                        telemetry->numbar_value);
    telemetry->numbar_segments = nb_resolve(system, instance,
                                             config->segments_bind, 0L);
    telemetry->numbar_state = nb_resolve(system, instance,
                                          config->state_bind, 0L);
    telemetry->numbar_phase = nb_resolve(system, instance,
                                          config->phase_bind, 0L);
    telemetry->numbar_units = nb_resolve(system, instance,
                                          config->units_bind, 0L);
    telemetry->numbar_layer_count = nb_resolve(system, instance,
                                                config->layer_count_bind, 0L);
    telemetry->numbar_layer_size = nb_resolve(system, instance,
                                               config->layer_size_bind, 0L);
    telemetry->numbar_value2 = nb_resolve(system, instance,
                                           config->value2_bind, 0L);
}

void blank3d_numbar_draw(Blank3DNumbarSystem *system,
                          struct Blank3DHudTag *hud,
                          unsigned int frame_ms)
{
    int order[B3D_NUMBAR_MAX_INSTANCES];
    int count;
    int i;
    int j;
    int key;
    long visible;
    Blank3DNumbarInstance *instance;
    Blank3DBigHudTelemetry telemetry;
    if (!system || !hud) return;
    count = 0;
    for (i = 0; i < system->instance_count; ++i) {
        instance = &system->instances[i];
        if (instance->alive && instance->requested &&
            instance->orchestrator.enabled)
            order[count++] = i;
    }
    for (i = 1; i < count; ++i) {
        key = order[i];
        j = i - 1;
        while (j >= 0 && system->instances[order[j]].orchestrator.z >
                         system->instances[key].orchestrator.z) {
            order[j + 1] = order[j];
            --j;
        }
        order[j + 1] = key;
    }
    for (i = 0; i < count; ++i) {
        instance = &system->instances[order[i]];
        if (instance->orchestrator.visible_bind[0]) {
            visible = nb_resolve(system, instance,
                                 instance->orchestrator.visible_bind, 1L);
            if (!visible) continue;
        }
        nb_telemetry(system, instance, &telemetry);
        blank3d_hud_draw_layout_scaled(
            hud, &instance->layout, &telemetry, frame_ms,
            instance->orchestrator.anchor,
            instance->orchestrator.x,
            instance->orchestrator.y,
            instance->orchestrator.scale_x_q8,
            instance->orchestrator.scale_y_q8,
            instance->orchestrator.canvas_width,
            instance->orchestrator.canvas_height);
    }
    /* A GFO that stopped requesting an orchestrator relinquishes its fixed
     * slots after the frame. This keeps spawned/destroyed actors from
     * exhausting the pool while still avoiding file I/O for active HUDs. */
    for (i = 0; i < system->instance_count; ++i) {
        if (system->instances[i].alive && !system->instances[i].requested)
            memset(&system->instances[i], 0, sizeof(system->instances[i]));
    }
    nb_recount(system);
}

const char *blank3d_numbar_status(const Blank3DNumbarSystem *system)
{
    return system ? system->status : "numbar system unavailable";
}
