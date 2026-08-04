#include "gweapon89.h"

#include <string.h>

#define GWP89_LINE_MAX 256
#define GWP89_LOCAL_TEXT_MAX 64

static void gwp89_fill_event_base(GWP89_Manager *m, GWP89_Event *ev, const GWP89_UserState *u, const GWP89_WeaponProfile *p, const GWP89_FireInput *input, const GWP89_PoseResult *pose);

static void gwp89_zero_event(GWP89_Event *ev)
{
    if (ev) memset(ev, 0, sizeof(*ev));
}

static int gwp89_is_space_char(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void gwp89_trim(char *s)
{
    int len;
    int start;
    int i;
    if (!s) return;
    len = (int)strlen(s);
    while (len > 0 && gwp89_is_space_char(s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
    start = 0;
    while (s[start] && gwp89_is_space_char(s[start])) start++;
    if (start > 0) {
        i = 0;
        while (s[start]) {
            s[i++] = s[start++];
        }
        s[i] = '\0';
    }
}

static void gwp89_unquote(char *s)
{
    int len;
    int i;
    if (!s) return;
    gwp89_trim(s);
    len = (int)strlen(s);
    if (len >= 2 && s[0] == '"' && s[len - 1] == '"') {
        for (i = 1; i < len - 1; i++) s[i - 1] = s[i];
        s[len - 2] = '\0';
    }
    gwp89_trim(s);
}

static void gwp89_strip_comment(char *s)
{
    int i;
    int quoted;
    if (!s) return;
    quoted = 0;
    for (i = 0; s[i]; i++) {
        if (s[i] == '"') quoted = !quoted;
        if (!quoted && (s[i] == '#' || s[i] == ';')) {
            s[i] = '\0';
            return;
        }
    }
}

static char gwp89_norm_char(char c)
{
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    if (c == '-' || c == ' ' || c == '.') c = '_';
    return c;
}

int gwp89_streq_id(const char *a, const char *b)
{
    int i;
    char ca;
    char cb;
    if (!a || !b) return 0;
    for (i = 0; ; i++) {
        ca = gwp89_norm_char(a[i]);
        cb = gwp89_norm_char(b[i]);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
    }
}

void gwp89_copy_id(char *dst, int cap, const char *src)
{
    int i;
    char c;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    for (i = 0; i < cap - 1 && src[i]; i++) {
        c = gwp89_norm_char(src[i]);
        dst[i] = c;
    }
    dst[i] = '\0';
}


static int gwp89_text_length_cap(const char *text, int cap)
{
    int n;
    if (!text || cap <= 0) return 0;
    n = 0;
    while (n < cap && text[n]) n++;
    return n;
}

static void gwp89_text_append(char *dst, int cap, const char *src)
{
    int at;
    int i;
    if (!dst || cap <= 0 || !src) return;
    at = gwp89_text_length_cap(dst, cap - 1);
    i = 0;
    while (at < cap - 1 && src[i]) dst[at++] = src[i++];
    dst[at] = '\0';
}

static void gwp89_text_append_int(char *dst, int cap, int value)
{
    char digits[16];
    char one[2];
    unsigned long magnitude;
    int count;
    int i;
    if (!dst || cap <= 0) return;
    if (value < 0) {
        gwp89_text_append(dst, cap, "-");
        magnitude = (unsigned long)(-(value + 1));
        magnitude += 1UL;
    } else {
        magnitude = (unsigned long)value;
    }
    count = 0;
    do {
        digits[count++] = (char)('0' + (magnitude % 10UL));
        magnitude /= 10UL;
    } while (magnitude > 0UL && count < (int)sizeof(digits));
    for (i = count - 1; i >= 0; i--) {
        one[0] = digits[i];
        one[1] = '\0';
        gwp89_text_append(dst, cap, one);
    }
}

static void gwp89_status_begin(GWP89_Manager *m, const char *prefix)
{
    if (!m) return;
    m->status[0] = '\0';
    gwp89_text_append(m->status, (int)sizeof(m->status), prefix ? prefix : "gweapon89");
}

static void gwp89_status_key_int(GWP89_Manager *m, const char *key, int value)
{
    if (!m) return;
    gwp89_text_append(m->status, (int)sizeof(m->status), " ");
    gwp89_text_append(m->status, (int)sizeof(m->status), key ? key : "value");
    gwp89_text_append(m->status, (int)sizeof(m->status), "=");
    gwp89_text_append_int(m->status, (int)sizeof(m->status), value);
}

static int gwp89_parse_int(const char *text, int fallback)
{
    long sign;
    long value;
    int have_digit;
    if (!text) return fallback;
    while (*text && gwp89_is_space_char(*text)) text++;
    sign = 1L;
    if (*text == '-') { sign = -1L; text++; }
    else if (*text == '+') text++;
    value = 0L;
    have_digit = 0;
    while (*text >= '0' && *text <= '9') {
        have_digit = 1;
        value = value * 10L + (long)(*text - '0');
        if (value > 2147480000L) value = 2147480000L;
        text++;
    }
    if (!have_digit) return fallback;
    value *= sign;
    if (value > 2147480000L) value = 2147480000L;
    if (value < -2147480000L) value = -2147480000L;
    return (int)value;
}

gwp89_fx gwp89_fx_from_int(int v)
{
    return ((gwp89_fx)v) << GWP89_FIX_SHIFT;
}

gwp89_fx gwp89_fx_from_text(const char *text)
{
    long sign;
    long whole;
    long frac;
    long scale;
    int i;
    char c;
    if (!text) return 0L;
    while (*text && gwp89_is_space_char(*text)) text++;
    sign = 1L;
    if (*text == '-') { sign = -1L; text++; }
    else if (*text == '+') { text++; }
    whole = 0L;
    while (*text >= '0' && *text <= '9') {
        whole = whole * 10L + (long)(*text - '0');
        text++;
        if (whole > 8000000L) whole = 8000000L;
    }
    frac = 0L;
    scale = 1L;
    if (*text == '.') {
        text++;
        for (i = 0; i < 5; i++) {
            c = *text;
            if (c < '0' || c > '9') break;
            frac = frac * 10L + (long)(c - '0');
            scale = scale * 10L;
            text++;
        }
    }
    return (gwp89_fx)(sign * ((whole << GWP89_FIX_SHIFT) + ((frac << GWP89_FIX_SHIFT) / scale)));
}

int gwp89_fx_to_int_round(gwp89_fx v)
{
    if (v >= 0) return (int)((v + GWP89_FIX_HALF) >> GWP89_FIX_SHIFT);
    return (int)(-(((-v) + GWP89_FIX_HALF) >> GWP89_FIX_SHIFT));
}

GWP89_Vec3 gwp89_v3(gwp89_fx x, gwp89_fx y, gwp89_fx z)
{
    GWP89_Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

GWP89_Vec3 gwp89_v3_zero(void)
{
    return gwp89_v3(0L, 0L, 0L);
}

static GWP89_Vec3 gwp89_v3_add(GWP89_Vec3 a, GWP89_Vec3 b)
{
    return gwp89_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static GWP89_Vec3 gwp89_v3_mul(GWP89_Vec3 a, gwp89_fx s)
{
    return gwp89_v3((a.x * s) >> GWP89_FIX_SHIFT,
                    (a.y * s) >> GWP89_FIX_SHIFT,
                    (a.z * s) >> GWP89_FIX_SHIFT);
}

static GWP89_Vec3 gwp89_apply_spread_fallback(const GWP89_PoseRequest *req, GWP89_Vec3 dir)
{
    static const int kx[12] = { 0, -1000, 1000, -500, 500, 0, -850, 850, -250, 250, -650, 650 };
    static const int ky[12] = { 0, 0, 0, 750, 750, -850, -450, -450, 350, 350, 150, 150 };
    GWP89_Vec3 right;
    GWP89_Vec3 up;
    gwp89_fx ox;
    gwp89_fx oy;
    gwp89_fx cone_radius;
    int pat;
    if (!req || req->pellet_count <= 1 || req->spread_fx == 0L) return dir;
    pat = req->pellet_index % 12;
    right = req->input.socket_right;
    up = req->input.socket_up;
    /* spread_fx is authored in degrees. The old fallback added seven whole
       vectors for a 7-degree shotgun cone, which erased the forward component
       and produced cardinal up/down/left/right pellets.  degrees / 57 is a
       fixed-point small-angle tangent approximation. Clamp the cone so forward
       always remains the dominant component. */
    cone_radius = req->spread_fx / 57L;
    if (cone_radius > GWP89_FIX_HALF) cone_radius = GWP89_FIX_HALF;
    if (cone_radius < -GWP89_FIX_HALF) cone_radius = -GWP89_FIX_HALF;
    ox = (cone_radius * (gwp89_fx)kx[pat]) / 1000L;
    oy = (cone_radius * (gwp89_fx)ky[pat]) / 1000L;
    return gwp89_v3_add(dir, gwp89_v3_add(gwp89_v3_mul(right, ox), gwp89_v3_mul(up, oy)));
}

int gwp89_fire_mode_from_name(const char *name)
{
    if (!name) return GWP89_FIRE_SEMI;
    if (gwp89_streq_id(name, "auto") || gwp89_streq_id(name, "automatic") ||
        gwp89_streq_id(name, "automatica") || gwp89_streq_id(name, "automatico") ||
        gwp89_streq_id(name, "rafaga") || gwp89_streq_id(name, "full_auto")) return GWP89_FIRE_AUTO;
    if (gwp89_streq_id(name, "semi") || gwp89_streq_id(name, "semi_auto") ||
        gwp89_streq_id(name, "semi_automatic") || gwp89_streq_id(name, "semiautomatica") ||
        gwp89_streq_id(name, "semiautomatico")) return GWP89_FIRE_SEMI;
    if (gwp89_streq_id(name, "hold_once") || gwp89_streq_id(name, "hold_single") ||
        gwp89_streq_id(name, "single_hold") || gwp89_streq_id(name, "one_per_hold")) return GWP89_FIRE_HOLD_ONCE;
    if (gwp89_streq_id(name, "burst") || gwp89_streq_id(name, "burst3") ||
        gwp89_streq_id(name, "rafaga_corta")) return GWP89_FIRE_BURST;
    return GWP89_FIRE_SEMI;
}

const char *gwp89_fire_mode_name(int fire_mode)
{
    if (fire_mode == GWP89_FIRE_AUTO) return "auto";
    if (fire_mode == GWP89_FIRE_HOLD_ONCE) return "hold_once";
    if (fire_mode == GWP89_FIRE_BURST) return "burst";
    return "semi";
}

void gwp89_profile_defaults(GWP89_WeaponProfile *p)
{
    if (!p) return;
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->weapon_id = 1;
    p->gun_id = 1;
    p->ammo_id = 1;
    p->projectile_id = 1;
    p->shell_id = 1;
    p->muzzle_id = 1;
    p->casing_id = 1;
    p->trail_id = 1;
    p->projectile_mesh_id = 1;
    p->shell_mesh_id = 1;
    gwp89_copy_id(p->name, GWP89_NAME_MAX, "pistol");
    gwp89_copy_id(p->gun_name, GWP89_NAME_MAX, "pistol");
    gwp89_copy_id(p->ammo_name, GWP89_NAME_MAX, "pistol");
    gwp89_copy_id(p->projectile_name, GWP89_NAME_MAX, "pistol_round");
    gwp89_copy_id(p->shell_name, GWP89_NAME_MAX, "pistol_shell");
    gwp89_copy_id(p->muzzle_name, GWP89_NAME_MAX, "pistol");
    gwp89_copy_id(p->casing_name, GWP89_NAME_MAX, "pistol");
    gwp89_copy_id(p->trail_name, GWP89_NAME_MAX, "bullet");
    gwp89_copy_id(p->projectile_mesh_name, GWP89_NAME_MAX, "pistol_projectile");
    gwp89_copy_id(p->shell_mesh_name, GWP89_NAME_MAX, "pistol_shell");
    p->fire_mode = GWP89_FIRE_SEMI;
    p->clip_size = 15;
    p->ammo_per_shot = 1;
    p->pellet_count = 1;
    p->burst_count = 3;
    p->allow_dry_fire_event = 1;
    p->active_reload_enabled = 0;
    p->cooldown_ms = 160u;
    p->reload_ms = 700u;
    p->projectile_life_ms = 2600u;
    p->active_reload_window_start_ms = 250u;
    p->active_reload_window_end_ms = 450u;
    p->active_reload_bonus_ms = 300u;
    p->active_reload_penalty_ms = 350u;
    p->damage_fx = gwp89_fx_from_int(10);
    p->speed_fx = gwp89_fx_from_int(34);
    p->range_fx = gwp89_fx_from_int(72);
    p->spread_fx = 0L;
    p->projectile_radius_fx = gwp89_fx_from_text("0.12");
    p->projectile_mesh_scale_fx = gwp89_fx_from_text("0.34");
    p->shell_mesh_scale_fx = gwp89_fx_from_text("0.30");
    p->recoil_fx = gwp89_fx_from_text("2.0");
}

void gwp89_init(GWP89_Manager *m)
{
    if (!m) return;
    memset(m, 0, sizeof(*m));
    gwp89_copy_id(m->status, (int)sizeof(m->status), "gweapon89:empty");
}

void gwp89_set_hooks(GWP89_Manager *m, const GWP89_Hooks *hooks)
{
    if (!m) return;
    if (hooks) m->hooks = *hooks;
    else memset(&m->hooks, 0, sizeof(m->hooks));
}


static void gwp89_provider_packet_defaults(GWP89_ProviderPacket *packet)
{
    if (!packet) return;
    memset(packet, 0, sizeof(*packet));
    packet->result_code = GWP89_OK;
}

const char *gwp89_service_name(int service)
{
    switch (service) {
        case GWP89_SERVICE_ANY: return "any";
        case GWP89_SERVICE_MATH3D: return "math3d";
        case GWP89_SERVICE_TRANSFORM: return "transform";
        case GWP89_SERVICE_NUMERIC: return "numeric";
        case GWP89_SERVICE_FLAGS: return "flags";
        case GWP89_SERVICE_CAMERA: return "camera";
        case GWP89_SERVICE_SOCKETS: return "sockets";
        case GWP89_SERVICE_INVENTORY: return "inventory";
        case GWP89_SERVICE_RAYCAST: return "raycast";
        case GWP89_SERVICE_HUD: return "hud";
        case GWP89_SERVICE_CROSSHAIR: return "crosshair";
        case GWP89_SERVICE_SCOPE: return "scope";
        case GWP89_SERVICE_DRAW: return "draw";
        case GWP89_SERVICE_PROJECTILE: return "projectile";
        case GWP89_SERVICE_ZOOM: return "zoom";
        case GWP89_SERVICE_SPREAD: return "spread";
        case GWP89_SERVICE_PROJECTILE_LIFE: return "projectile_life";
        case GWP89_SERVICE_HEALTH: return "health";
        case GWP89_SERVICE_MUZZLE: return "muzzle";
        case GWP89_SERVICE_RELOAD: return "reload";
        case GWP89_SERVICE_ACTIVE_RELOAD: return "active_reload";
        case GWP89_SERVICE_CASING: return "casing";
        case GWP89_SERVICE_TRAIL: return "trail";
        case GWP89_SERVICE_MESH: return "mesh";
        case GWP89_SERVICE_RECOIL: return "recoil";
        case GWP89_SERVICE_POSE: return "pose";
        case GWP89_SERVICE_EVENT_BUS: return "event_bus";
        case GWP89_SERVICE_IO: return "io";
        case GWP89_SERVICE_ACTOR_STATE: return "actor_state";
    }
    return "unknown";
}

int gwp89_add_provider(GWP89_Manager *m, int service, int priority, const char *name, void *ctx, GWP89_ProviderFn fn)
{
    int slot;
    if (!m || !fn) return GWP89_BAD_ARG;
    if (service < GWP89_SERVICE_ANY || service > GWP89_SERVICE_ACTOR_STATE) return GWP89_BAD_ARG;
    if (m->provider_count >= GWP89_MAX_PROVIDERS) return GWP89_FULL;
    for (slot = 0; slot < GWP89_MAX_PROVIDERS; slot++) if (!m->providers[slot].active) break;
    if (slot >= GWP89_MAX_PROVIDERS) return GWP89_FULL;
    memset(&m->providers[slot], 0, sizeof(m->providers[slot]));
    m->providers[slot].active = 1;
    m->providers[slot].service = service;
    m->providers[slot].priority = priority;
    m->providers[slot].serial = ++m->provider_serial;
    m->providers[slot].ctx = ctx;
    m->providers[slot].fn = fn;
    gwp89_copy_id(m->providers[slot].name, GWP89_PROVIDER_NAME_MAX, name ? name : gwp89_service_name(service));
    m->provider_count++;
    return slot;
}

int gwp89_remove_provider(GWP89_Manager *m, int provider_slot)
{
    if (!m || provider_slot < 0 || provider_slot >= GWP89_MAX_PROVIDERS) return GWP89_BAD_ARG;
    if (!m->providers[provider_slot].active) return GWP89_NOT_FOUND;
    memset(&m->providers[provider_slot], 0, sizeof(m->providers[provider_slot]));
    if (m->provider_count > 0) m->provider_count--;
    return GWP89_OK;
}

void gwp89_clear_providers(GWP89_Manager *m)
{
    if (!m) return;
    memset(m->providers, 0, sizeof(m->providers));
    m->provider_count = 0;
    m->provider_serial = 0UL;
}

int gwp89_provider_count(const GWP89_Manager *m, int service)
{
    int i;
    int count;
    if (!m) return 0;
    count = 0;
    for (i = 0; i < GWP89_MAX_PROVIDERS; i++) {
        if (m->providers[i].active && (service == GWP89_SERVICE_ANY || m->providers[i].service == service)) count++;
    }
    return count;
}

int gwp89_provider_call(GWP89_Manager *m, int service, int operation, int phase, GWP89_ProviderPacket *packet)
{
    int i;
    int j;
    int count;
    int index;
    int r;
    int all;
    int order[GWP89_MAX_PROVIDERS];
    GWP89_ProviderPacket local;
    if (!m) return GWP89_BAD_ARG;
    if (!packet) {
        gwp89_provider_packet_defaults(&local);
        packet = &local;
    }
    packet->service = service;
    packet->operation = operation;
    packet->phase = phase;
    count = 0;
    for (i = 0; i < GWP89_MAX_PROVIDERS; i++) {
        if (!m->providers[i].active || !m->providers[i].fn) continue;
        if (m->providers[i].service != GWP89_SERVICE_ANY && m->providers[i].service != service) continue;
        j = count;
        while (j > 0) {
            index = order[j - 1];
            if (m->providers[index].priority > m->providers[i].priority) break;
            if (m->providers[index].priority == m->providers[i].priority && m->providers[index].serial < m->providers[i].serial) break;
            order[j] = order[j - 1];
            j--;
        }
        order[j] = i;
        count++;
    }
    all = GWP89_PROVIDER_PASS;
    for (i = 0; i < count; i++) {
        index = order[i];
        r = m->providers[index].fn(m->providers[index].ctx, packet);
        all |= r;
        if (r & GWP89_PROVIDER_MODIFIED) {
            if (packet->event) packet->event->flags |= GWP89_EVENT_FLAG_PROVIDER_MODIFIED;
        }
        if (r & GWP89_PROVIDER_HANDLED) {
            if (packet->event) packet->event->flags |= GWP89_EVENT_FLAG_PROVIDER_HANDLED;
        }
        if (r & GWP89_PROVIDER_STOP) break;
    }
    return all;
}

int gwp89_resolve_numeric_int(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *profile, int key, int fallback)
{
    GWP89_ProviderPacket packet;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = profile ? profile->weapon_id : 0;
    packet.gun_id = profile ? profile->gun_id : 0;
    packet.ammo_id = profile ? profile->ammo_id : 0;
    packet.profile = profile;
    packet.key = key;
    packet.i_value = fallback;
    gwp89_provider_call(m, GWP89_SERVICE_NUMERIC, GWP89_OP_QUERY_INT, GWP89_PHASE_PRE, &packet);
    gwp89_provider_call(m, GWP89_SERVICE_NUMERIC, GWP89_OP_QUERY_INT, GWP89_PHASE_POST, &packet);
    return packet.i_value;
}

gwp89_fx gwp89_resolve_numeric_fx(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *profile, int key, gwp89_fx fallback)
{
    GWP89_ProviderPacket packet;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = profile ? profile->weapon_id : 0;
    packet.gun_id = profile ? profile->gun_id : 0;
    packet.ammo_id = profile ? profile->ammo_id : 0;
    packet.profile = profile;
    packet.key = key;
    packet.fx_value = fallback;
    gwp89_provider_call(m, GWP89_SERVICE_NUMERIC, GWP89_OP_QUERY_FX, GWP89_PHASE_PRE, &packet);
    gwp89_provider_call(m, GWP89_SERVICE_NUMERIC, GWP89_OP_QUERY_FX, GWP89_PHASE_POST, &packet);
    return packet.fx_value;
}

int gwp89_resolve_flag(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *profile, int key, int fallback)
{
    GWP89_ProviderPacket packet;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = profile ? profile->weapon_id : 0;
    packet.gun_id = profile ? profile->gun_id : 0;
    packet.ammo_id = profile ? profile->ammo_id : 0;
    packet.profile = profile;
    packet.key = key;
    packet.i_value = fallback ? 1 : 0;
    gwp89_provider_call(m, GWP89_SERVICE_FLAGS, GWP89_OP_QUERY_FLAG, GWP89_PHASE_PRE, &packet);
    gwp89_provider_call(m, GWP89_SERVICE_FLAGS, GWP89_OP_QUERY_FLAG, GWP89_PHASE_POST, &packet);
    return packet.i_value ? 1 : 0;
}

static gwp89_fx gwp89_resolve_zoom_value(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *profile, GWP89_FireInput *input, gwp89_fx fallback)
{
    GWP89_ProviderPacket packet;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = profile ? profile->weapon_id : 0;
    packet.gun_id = profile ? profile->gun_id : 0;
    packet.ammo_id = profile ? profile->ammo_id : 0;
    packet.profile = profile;
    packet.input = input;
    packet.fx_value = fallback != 0L ? fallback : GWP89_FIX_ONE;
    gwp89_provider_call(m, GWP89_SERVICE_ZOOM, GWP89_OP_QUERY_FX, GWP89_PHASE_PRE, &packet);
    gwp89_provider_call(m, GWP89_SERVICE_ZOOM, GWP89_OP_QUERY_FX, GWP89_PHASE_POST, &packet);
    return packet.fx_value != 0L ? packet.fx_value : GWP89_FIX_ONE;
}

static GWP89_Vec3 gwp89_math_add_provider(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *p, GWP89_Vec3 a, GWP89_Vec3 b)
{
    GWP89_ProviderPacket packet;
    int r;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = p ? p->weapon_id : 0;
    packet.profile = p;
    packet.vec_a = a;
    packet.vec_b = b;
    r = gwp89_provider_call(m, GWP89_SERVICE_MATH3D, GWP89_OP_MATH_ADD, GWP89_PHASE_PRE, &packet);
    if (!(r & GWP89_PROVIDER_HANDLED)) packet.vec_c = gwp89_v3_add(a, b);
    gwp89_provider_call(m, GWP89_SERVICE_MATH3D, GWP89_OP_MATH_ADD, GWP89_PHASE_POST, &packet);
    return packet.vec_c;
}

static GWP89_Vec3 gwp89_math_scale_provider(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *p, GWP89_Vec3 a, gwp89_fx scale)
{
    GWP89_ProviderPacket packet;
    int r;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = p ? p->weapon_id : 0;
    packet.profile = p;
    packet.vec_a = a;
    packet.fx_value = scale;
    r = gwp89_provider_call(m, GWP89_SERVICE_MATH3D, GWP89_OP_MATH_SCALE, GWP89_PHASE_PRE, &packet);
    if (!(r & GWP89_PROVIDER_HANDLED)) packet.vec_c = gwp89_v3_mul(a, scale);
    gwp89_provider_call(m, GWP89_SERVICE_MATH3D, GWP89_OP_MATH_SCALE, GWP89_PHASE_POST, &packet);
    return packet.vec_c;
}

static GWP89_Vec3 gwp89_math_normalize_provider(GWP89_Manager *m, int actor_id, const GWP89_WeaponProfile *p, GWP89_Vec3 a)
{
    GWP89_ProviderPacket packet;
    int r;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = p ? p->weapon_id : 0;
    packet.profile = p;
    packet.vec_a = a;
    r = gwp89_provider_call(m, GWP89_SERVICE_MATH3D, GWP89_OP_MATH_NORMALIZE, GWP89_PHASE_PRE, &packet);
    if (!(r & GWP89_PROVIDER_HANDLED)) packet.vec_c = a;
    gwp89_provider_call(m, GWP89_SERVICE_MATH3D, GWP89_OP_MATH_NORMALIZE, GWP89_PHASE_POST, &packet);
    return packet.vec_c;
}

const char *gwp89_status(const GWP89_Manager *m)
{
    if (!m) return "gweapon89:null";
    return m->status;
}

static void gwp89_statusf(GWP89_Manager *m, const char *prefix, const char *name)
{
    if (!m) return;
    gwp89_status_begin(m, prefix ? prefix : "gweapon89");
    gwp89_text_append(m->status, (int)sizeof(m->status), ":");
    gwp89_text_append(m->status, (int)sizeof(m->status), name ? name : "");
    gwp89_status_key_int(m, "weapons", m->weapon_count);
    gwp89_status_key_int(m, "events", m->event_count);
}

int gwp89_add_weapon(GWP89_Manager *m, const GWP89_WeaponProfile *profile)
{
    int slot;
    if (!m || !profile) return GWP89_BAD_ARG;
    slot = gwp89_find_weapon_slot(m, profile->name);
    if (slot < 0 && profile->weapon_id > 0) slot = gwp89_find_weapon_slot_by_id(m, profile->weapon_id);
    if (slot < 0) {
        if (m->weapon_count >= GWP89_MAX_WEAPONS) return GWP89_FULL;
        slot = m->weapon_count++;
    }
    m->weapons[slot] = *profile;
    m->weapons[slot].active = 1;
    if (m->weapons[slot].weapon_id <= 0) m->weapons[slot].weapon_id = slot + 1;
    if (m->weapons[slot].ammo_per_shot <= 0) m->weapons[slot].ammo_per_shot = 1;
    if (m->weapons[slot].pellet_count <= 0) m->weapons[slot].pellet_count = 1;
    if (m->weapons[slot].pellet_count > 12) m->weapons[slot].pellet_count = 12;
    if (m->weapons[slot].clip_size < 0) m->weapons[slot].clip_size = 0;
    if (m->weapons[slot].burst_count <= 0) m->weapons[slot].burst_count = 3;
    if (m->weapons[slot].cooldown_ms == 0u) m->weapons[slot].cooldown_ms = 1u;
    if (m->weapons[slot].projectile_life_ms == 0u) m->weapons[slot].projectile_life_ms = 1000u;
    gwp89_statusf(m, "gweapon89:add", m->weapons[slot].name);
    return slot;
}

int gwp89_find_weapon_slot(const GWP89_Manager *m, const char *name)
{
    int i;
    if (!m || !name || !name[0]) return GWP89_NOT_FOUND;
    for (i = 0; i < m->weapon_count; i++) {
        if (m->weapons[i].active && gwp89_streq_id(m->weapons[i].name, name)) return i;
    }
    return GWP89_NOT_FOUND;
}

int gwp89_find_weapon_slot_by_id(const GWP89_Manager *m, int weapon_id)
{
    int i;
    if (!m || weapon_id <= 0) return GWP89_NOT_FOUND;
    for (i = 0; i < m->weapon_count; i++) {
        if (m->weapons[i].active && m->weapons[i].weapon_id == weapon_id) return i;
    }
    return GWP89_NOT_FOUND;
}

const GWP89_WeaponProfile *gwp89_get_weapon(const GWP89_Manager *m, int weapon_slot)
{
    if (!m || weapon_slot < 0 || weapon_slot >= m->weapon_count) return 0;
    if (!m->weapons[weapon_slot].active) return 0;
    return &m->weapons[weapon_slot];
}

static int gwp89_alloc_user(GWP89_Manager *m, int actor_id, int actor_kind, int team_id)
{
    int i;
    if (!m) return GWP89_BAD_ARG;
    i = gwp89_find_user_slot(m, actor_id);
    if (i >= 0) return i;
    for (i = 0; i < GWP89_MAX_USERS; i++) {
        if (!m->users[i].active) {
            memset(&m->users[i], 0, sizeof(m->users[i]));
            m->users[i].active = 1;
            m->users[i].actor_id = actor_id;
            m->users[i].actor_kind = actor_kind;
            m->users[i].team_id = team_id;
            m->users[i].weapon_slot = -1;
            m->users[i].weapon_id = 0;
            return i;
        }
    }
    return GWP89_FULL;
}

int gwp89_bind_actor(GWP89_Manager *m, int actor_id, int actor_kind, int team_id)
{
    return gwp89_alloc_user(m, actor_id, actor_kind, team_id);
}

int gwp89_find_user_slot(const GWP89_Manager *m, int actor_id)
{
    int i;
    if (!m) return GWP89_BAD_ARG;
    for (i = 0; i < GWP89_MAX_USERS; i++) {
        if (m->users[i].active && m->users[i].actor_id == actor_id) return i;
    }
    return GWP89_NOT_FOUND;
}

static void gwp89_fill_weapon_event_names(GWP89_Event *ev, const GWP89_WeaponProfile *p)
{
    if (!ev || !p) return;
    gwp89_copy_id(ev->weapon_name, GWP89_NAME_MAX, p->name);
    gwp89_copy_id(ev->muzzle_name, GWP89_NAME_MAX, p->muzzle_name);
    gwp89_copy_id(ev->casing_name, GWP89_NAME_MAX, p->casing_name);
    gwp89_copy_id(ev->trail_name, GWP89_NAME_MAX, p->trail_name);
    gwp89_copy_id(ev->projectile_mesh_name, GWP89_NAME_MAX, p->projectile_mesh_name);
    gwp89_copy_id(ev->shell_mesh_name, GWP89_NAME_MAX, p->shell_mesh_name);
}

static int gwp89_event_primary_service(int event_type)
{
    if (event_type == GWP89_EVENT_PROJECTILE_REQUEST) return GWP89_SERVICE_PROJECTILE;
    if (event_type == GWP89_EVENT_MUZZLE_REQUEST) return GWP89_SERVICE_MUZZLE;
    if (event_type == GWP89_EVENT_CASING_REQUEST) return GWP89_SERVICE_CASING;
    if (event_type == GWP89_EVENT_TRAIL_REQUEST) return GWP89_SERVICE_TRAIL;
    if (event_type == GWP89_EVENT_MESH_ASSIGN_REQUEST) return GWP89_SERVICE_MESH;
    if (event_type == GWP89_EVENT_VISUAL_MOD_REQUEST) return GWP89_SERVICE_DRAW;
    if (event_type == GWP89_EVENT_AMMO_CHANGED || event_type == GWP89_EVENT_WEAPON_CHANGED) return GWP89_SERVICE_INVENTORY;
    if (event_type == GWP89_EVENT_RELOAD_BEGIN || event_type == GWP89_EVENT_RELOAD_END) return GWP89_SERVICE_RELOAD;
    if (event_type == GWP89_EVENT_ACTIVE_RELOAD_WINDOW ||
        event_type == GWP89_EVENT_ACTIVE_RELOAD_SUCCESS ||
        event_type == GWP89_EVENT_ACTIVE_RELOAD_FAIL) return GWP89_SERVICE_ACTIVE_RELOAD;
    return GWP89_SERVICE_EVENT_BUS;
}

static void gwp89_add_service_unique(int *services, int *count, int cap, int service)
{
    int i;
    if (!services || !count || service <= GWP89_SERVICE_ANY || *count >= cap) return;
    for (i = 0; i < *count; i++) if (services[i] == service) return;
    services[*count] = service;
    (*count)++;
}

static int gwp89_event_service_list(const GWP89_Event *ev, int *services, int cap)
{
    int count;
    int primary;
    if (!ev || !services || cap <= 0) return 0;
    count = 0;
    primary = gwp89_event_primary_service(ev->type);
    gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_EVENT_BUS);
    if (ev->type == GWP89_EVENT_PROJECTILE_REQUEST) gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_PROJECTILE_LIFE);
    gwp89_add_service_unique(services, &count, cap, primary);

    if (ev->type == GWP89_EVENT_FIRE_ACCEPTED || ev->type == GWP89_EVENT_DRY_FIRE ||
        ev->type == GWP89_EVENT_AMMO_CHANGED || ev->type == GWP89_EVENT_WEAPON_CHANGED ||
        ev->type == GWP89_EVENT_RELOAD_BEGIN || ev->type == GWP89_EVENT_RELOAD_END ||
        ev->type == GWP89_EVENT_ACTIVE_RELOAD_WINDOW || ev->type == GWP89_EVENT_ACTIVE_RELOAD_SUCCESS ||
        ev->type == GWP89_EVENT_ACTIVE_RELOAD_FAIL) {
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_HUD);
    }
    if (ev->type == GWP89_EVENT_FIRE_ACCEPTED || ev->type == GWP89_EVENT_DRY_FIRE ||
        ev->type == GWP89_EVENT_VISUAL_MOD_REQUEST || ev->type == GWP89_EVENT_WEAPON_CHANGED) {
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_CROSSHAIR);
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_SCOPE);
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_ZOOM);
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_RECOIL);
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_DRAW);
    }
    if (ev->type == GWP89_EVENT_PROJECTILE_REQUEST) {
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_PROJECTILE_LIFE);
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_DRAW);
        if (ev->flags & GWP89_EVENT_FLAG_HIT_VALID) gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_HEALTH);
    }
    if (ev->type == GWP89_EVENT_MUZZLE_REQUEST || ev->type == GWP89_EVENT_CASING_REQUEST ||
        ev->type == GWP89_EVENT_TRAIL_REQUEST || ev->type == GWP89_EVENT_MESH_ASSIGN_REQUEST) {
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_DRAW);
    }
    if (ev->type == GWP89_EVENT_RELOAD_BEGIN || ev->type == GWP89_EVENT_RELOAD_END) {
        gwp89_add_service_unique(services, &count, cap, GWP89_SERVICE_ACTIVE_RELOAD);
    }
    return count;
}

static int gwp89_event_operation_for_service(int service)
{
    if (service == GWP89_SERVICE_PROJECTILE_LIFE) return GWP89_OP_PROJECTILE_LIFE;
    if (service == GWP89_SERVICE_HEALTH) return GWP89_OP_DAMAGE_REQUEST;
    return GWP89_OP_EMIT_EVENT;
}

static int gwp89_event_provider_fanout(GWP89_Manager *m, GWP89_Event *ev, int phase, int *primary_result)
{
    int services[16];
    int count;
    int i;
    int r;
    int all;
    int primary;
    GWP89_ProviderPacket packet;
    if (!m || !ev) return GWP89_PROVIDER_PASS;
    count = gwp89_event_service_list(ev, services, 16);
    primary = gwp89_event_primary_service(ev->type);
    all = GWP89_PROVIDER_PASS;
    if (primary_result) *primary_result = GWP89_PROVIDER_PASS;
    for (i = 0; i < count; i++) {
        gwp89_provider_packet_defaults(&packet);
        packet.actor_id = ev->actor_id;
        packet.actor_kind = ev->actor_kind;
        packet.team_id = ev->team_id;
        packet.weapon_id = ev->weapon_id;
        packet.gun_id = ev->gun_id;
        packet.ammo_id = ev->ammo_id;
        packet.event = ev;
        r = gwp89_provider_call(m, services[i], gwp89_event_operation_for_service(services[i]), phase, &packet);
        all |= r;
        if (services[i] == primary && primary_result) *primary_result |= r;
        if (r & GWP89_PROVIDER_CANCEL) break;
    }
    return all;
}

static void gwp89_dispatch(GWP89_Manager *m, const GWP89_Event *source_event)
{
    GWP89_Event ev;
    int pre;
    int primary_pre;
    if (!m || !source_event) return;
    ev = *source_event;
    primary_pre = GWP89_PROVIDER_PASS;
    pre = gwp89_event_provider_fanout(m, &ev, GWP89_PHASE_PRE, &primary_pre);
    if (pre & GWP89_PROVIDER_CANCEL) return;
    if (m->event_count < GWP89_MAX_EVENTS) {
        m->events[m->event_tail] = ev;
        m->event_tail++;
        if (m->event_tail >= GWP89_MAX_EVENTS) m->event_tail = 0;
        m->event_count++;
    }
    if (m->hooks.event) m->hooks.event(m->hooks.ctx, &ev);
    if (!(primary_pre & GWP89_PROVIDER_HANDLED)) {
        if (ev.type == GWP89_EVENT_PROJECTILE_REQUEST && m->hooks.emit_projectile) m->hooks.emit_projectile(m->hooks.ctx, &ev);
        else if (ev.type == GWP89_EVENT_MUZZLE_REQUEST && m->hooks.emit_muzzle) m->hooks.emit_muzzle(m->hooks.ctx, &ev);
        else if (ev.type == GWP89_EVENT_CASING_REQUEST && m->hooks.emit_casing) m->hooks.emit_casing(m->hooks.ctx, &ev);
        else if (ev.type == GWP89_EVENT_TRAIL_REQUEST && m->hooks.emit_trail) m->hooks.emit_trail(m->hooks.ctx, &ev);
        else if (ev.type == GWP89_EVENT_MESH_ASSIGN_REQUEST && m->hooks.assign_mesh) m->hooks.assign_mesh(m->hooks.ctx, &ev);
        else if (ev.type == GWP89_EVENT_VISUAL_MOD_REQUEST && m->hooks.visual_mod) m->hooks.visual_mod(m->hooks.ctx, &ev);
        else if (ev.type == GWP89_EVENT_AMMO_CHANGED && m->hooks.ammo_changed) m->hooks.ammo_changed(m->hooks.ctx, &ev);
    }
    gwp89_event_provider_fanout(m, &ev, GWP89_PHASE_POST, 0);
}

static void gwp89_emit_weapon_changed(GWP89_Manager *m, const GWP89_UserState *u, const GWP89_WeaponProfile *p)
{
    GWP89_Event ev;
    if (!m || !u || !p) return;
    gwp89_fill_event_base(m, &ev, u, p, 0, 0);
    ev.type = GWP89_EVENT_WEAPON_CHANGED;
    ev.reserve_ammo = gwp89_query_ammo(m, u->actor_id, p->ammo_id, p->weapon_id);
    gwp89_dispatch(m, &ev);
}

int gwp89_equip_slot(GWP89_Manager *m, int actor_id, int weapon_slot, int fill_clip)
{
    int us;
    int r;
    int clip_size;
    GWP89_UserState *u;
    const GWP89_WeaponProfile *p;
    GWP89_ProviderPacket packet;
    if (!m) return GWP89_BAD_ARG;
    p = gwp89_get_weapon(m, weapon_slot);
    if (!p) return GWP89_NOT_FOUND;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = p->weapon_id;
    packet.profile = p;
    packet.i_value = weapon_slot;
    packet.i_value2 = fill_clip;
    r = gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_EQUIP, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;
    weapon_slot = packet.i_value;
    p = gwp89_get_weapon(m, weapon_slot);
    if (!p) return GWP89_NOT_FOUND;
    us = gwp89_alloc_user(m, actor_id, 0, 0);
    if (us < 0) return us;
    u = &m->users[us];
    u->weapon_slot = weapon_slot;
    u->weapon_id = p->weapon_id;
    u->cooldown_ms_left = 0u;
    u->reload_ms_left = 0u;
    u->reload_elapsed_ms = 0u;
    u->reload_active = 0;
    u->trigger_latched = 0;
    u->burst_left = 0;
    u->active_reload_attempted = 0;
    u->active_reload_result = 0;
    clip_size = gwp89_resolve_numeric_int(m, actor_id, p, GWP89_NUM_CLIP_SIZE, p->clip_size);
    if (clip_size < 0) clip_size = 0;
    if (packet.i_value2) gwp89_set_clip(m, actor_id, p->weapon_id, clip_size);
    else if (gwp89_query_clip(m, actor_id, p->weapon_id) > clip_size) gwp89_set_clip(m, actor_id, p->weapon_id, clip_size);
    u->clip_ammo = gwp89_query_clip(m, actor_id, p->weapon_id);
    packet.user = u;
    gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_EQUIP, GWP89_PHASE_POST, &packet);
    gwp89_emit_weapon_changed(m, u, p);
    gwp89_statusf(m, "gweapon89:equip", p->name);
    return GWP89_OK;
}

int gwp89_equip_name(GWP89_Manager *m, int actor_id, const char *weapon_name, int fill_clip)
{
    int slot;
    slot = gwp89_find_weapon_slot(m, weapon_name);
    if (slot < 0) return slot;
    return gwp89_equip_slot(m, actor_id, slot, fill_clip);
}

int gwp89_cycle_next(GWP89_Manager *m, int actor_id, int fill_clip)
{
    int us;
    int next;
    if (!m || m->weapon_count <= 0) return GWP89_BAD_ARG;
    us = gwp89_alloc_user(m, actor_id, 0, 0);
    if (us < 0) return us;
    next = m->users[us].weapon_slot;
    if (next < 0 || next >= m->weapon_count) next = 0;
    else next++;
    if (next >= m->weapon_count) next = 0;
    return gwp89_equip_slot(m, actor_id, next, fill_clip);
}

int gwp89_cycle_prev(GWP89_Manager *m, int actor_id, int fill_clip)
{
    int us;
    int next;
    if (!m || m->weapon_count <= 0) return GWP89_BAD_ARG;
    us = gwp89_alloc_user(m, actor_id, 0, 0);
    if (us < 0) return us;
    next = m->users[us].weapon_slot;
    if (next < 0 || next >= m->weapon_count) next = m->weapon_count - 1;
    else next--;
    if (next < 0) next = m->weapon_count - 1;
    return gwp89_equip_slot(m, actor_id, next, fill_clip);
}

int gwp89_set_ammo(GWP89_Manager *m, int actor_id, int ammo_id, int amount)
{
    int us;
    int r;
    GWP89_ProviderPacket packet;
    if (!m || ammo_id < 0 || ammo_id >= GWP89_MAX_AMMO_TYPES) return GWP89_BAD_ARG;
    if (amount < 0) amount = 0;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.ammo_id = ammo_id;
    packet.i_value = amount;
    r = gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_AMMO_SET, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;
    if (!(r & GWP89_PROVIDER_HANDLED)) {
        us = gwp89_alloc_user(m, actor_id, 0, 0);
        if (us < 0) return us;
        m->ammo_bank[us][ammo_id] = packet.i_value;
    }
    gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_AMMO_SET, GWP89_PHASE_POST, &packet);
    return packet.i_value;
}

int gwp89_add_ammo(GWP89_Manager *m, int actor_id, int ammo_id, int amount)
{
    int us;
    int r;
    long v;
    GWP89_ProviderPacket packet;
    if (!m || ammo_id < 0 || ammo_id >= GWP89_MAX_AMMO_TYPES) return GWP89_BAD_ARG;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.ammo_id = ammo_id;
    packet.amount = amount;
    packet.i_value = gwp89_query_ammo(m, actor_id, ammo_id, 0);
    r = gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_AMMO_ADD, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;
    if (!(r & GWP89_PROVIDER_HANDLED)) {
        us = gwp89_alloc_user(m, actor_id, 0, 0);
        if (us < 0) return us;
        v = (long)m->ammo_bank[us][ammo_id] + (long)packet.amount;
        if (v < 0L) v = 0L;
        if (v > 2147480000L) v = 2147480000L;
        m->ammo_bank[us][ammo_id] = (int)v;
        packet.i_value = m->ammo_bank[us][ammo_id];
    }
    gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_AMMO_ADD, GWP89_PHASE_POST, &packet);
    return packet.i_value;
}

int gwp89_query_ammo(const GWP89_Manager *cm, int actor_id, int ammo_id, int weapon_id)
{
    GWP89_Manager *m;
    int us;
    int r;
    GWP89_ProviderPacket packet;
    if (!cm || ammo_id < 0 || ammo_id >= GWP89_MAX_AMMO_TYPES) return 0;
    m = (GWP89_Manager *)cm;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.ammo_id = ammo_id;
    packet.weapon_id = weapon_id;
    r = gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_AMMO_QUERY, GWP89_PHASE_PRE, &packet);
    if (!(r & GWP89_PROVIDER_HANDLED)) {
        if (m->hooks.ammo_query) packet.i_value = m->hooks.ammo_query(m->hooks.ctx, actor_id, ammo_id, weapon_id);
        else {
            us = gwp89_find_user_slot(m, actor_id);
            packet.i_value = (us < 0) ? 0 : m->ammo_bank[us][ammo_id];
        }
    }
    gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_AMMO_QUERY, GWP89_PHASE_POST, &packet);
    return packet.i_value < 0 ? 0 : packet.i_value;
}

int gwp89_query_clip(GWP89_Manager *m, int actor_id, int weapon_id)
{
    int us;
    int r;
    GWP89_ProviderPacket packet;
    if (!m) return 0;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = weapon_id;
    r = gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_CLIP_QUERY, GWP89_PHASE_PRE, &packet);
    if (!(r & GWP89_PROVIDER_HANDLED)) {
        us = gwp89_find_user_slot(m, actor_id);
        packet.i_value = us < 0 ? 0 : m->users[us].clip_ammo;
    }
    gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_CLIP_QUERY, GWP89_PHASE_POST, &packet);
    return packet.i_value < 0 ? 0 : packet.i_value;
}

int gwp89_set_clip(GWP89_Manager *m, int actor_id, int weapon_id, int amount)
{
    int us;
    int r;
    GWP89_ProviderPacket packet;
    if (!m) return GWP89_BAD_ARG;
    if (amount < 0) amount = 0;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.weapon_id = weapon_id;
    packet.i_value = amount;
    r = gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_CLIP_SET, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;
    if (!(r & GWP89_PROVIDER_HANDLED)) {
        us = gwp89_alloc_user(m, actor_id, 0, 0);
        if (us < 0) return us;
        m->users[us].clip_ammo = packet.i_value;
    }
    gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_CLIP_SET, GWP89_PHASE_POST, &packet);
    return packet.i_value;
}

static int gwp89_consume_ammo(GWP89_Manager *m, GWP89_UserState *u, const GWP89_WeaponProfile *p, int amount)
{
    int r;
    int us;
    GWP89_ProviderPacket packet;
    if (!m || !u || !p || amount <= 0) return GWP89_BAD_ARG;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = u->actor_id;
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.weapon_id = p->weapon_id;
    packet.ammo_id = p->ammo_id;
    packet.profile = p;
    packet.user = u;
    packet.amount = amount;
    packet.result_code = GWP89_NO_AMMO;
    r = gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_AMMO_CONSUME, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;
    if (!(r & GWP89_PROVIDER_HANDLED)) {
        if (m->hooks.ammo_consume) {
            packet.result_code = m->hooks.ammo_consume(m->hooks.ctx, u->actor_id, p->ammo_id, p->weapon_id, amount) ? GWP89_OK : GWP89_NO_AMMO;
        } else {
            us = gwp89_find_user_slot(m, u->actor_id);
            if (us < 0) packet.result_code = GWP89_NOT_FOUND;
            else if (p->ammo_id < 0 || p->ammo_id >= GWP89_MAX_AMMO_TYPES) packet.result_code = GWP89_BAD_ARG;
            else if (m->ammo_bank[us][p->ammo_id] < amount) packet.result_code = GWP89_NO_AMMO;
            else {
                m->ammo_bank[us][p->ammo_id] -= amount;
                packet.result_code = GWP89_OK;
            }
        }
    }
    gwp89_provider_call(m, GWP89_SERVICE_INVENTORY, GWP89_OP_AMMO_CONSUME, GWP89_PHASE_POST, &packet);
    return packet.result_code;
}

static unsigned short gwp89_sub_ms(unsigned short a, unsigned short b)
{
    if (b >= a) return 0u;
    return (unsigned short)(a - b);
}

static void gwp89_update_user_timers(GWP89_Manager *m, GWP89_UserState *u, unsigned short dt_ms)
{
    const GWP89_WeaponProfile *p;
    int need;
    int got;
    int clip_size;
    int active_enabled;
    int r;
    unsigned long elapsed;
    GWP89_Event ev;
    GWP89_ProviderPacket packet;
    if (!m || !u) return;
    u->cooldown_ms_left = gwp89_sub_ms(u->cooldown_ms_left, dt_ms);
    if (!u->reload_active) return;
    p = gwp89_get_weapon(m, u->weapon_slot);
    if (!p) {
        u->reload_active = 0;
        return;
    }

    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = u->actor_id;
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.weapon_id = p->weapon_id;
    packet.ammo_id = p->ammo_id;
    packet.profile = p;
    packet.user = u;
    packet.amount = (int)dt_ms;
    packet.ms_value = u->reload_ms_left;
    r = gwp89_provider_call(m, GWP89_SERVICE_RELOAD, GWP89_OP_RELOAD_TICK, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) {
        u->reload_active = 0;
        return;
    }
    if (!(r & GWP89_PROVIDER_HANDLED)) packet.ms_value = gwp89_sub_ms(packet.ms_value, dt_ms);
    gwp89_provider_call(m, GWP89_SERVICE_RELOAD, GWP89_OP_RELOAD_TICK, GWP89_PHASE_POST, &packet);
    u->reload_ms_left = packet.ms_value;
    elapsed = (unsigned long)u->reload_elapsed_ms + (unsigned long)dt_ms;
    if (elapsed > 65535UL) elapsed = 65535UL;
    u->reload_elapsed_ms = (unsigned short)elapsed;

    active_enabled = gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_ACTIVE_RELOAD_ENABLED, p->active_reload_enabled);
    if (active_enabled && !u->active_reload_attempted && u->active_reload_result == 0) {
        int start_ms;
        int end_ms;
        start_ms = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_ACTIVE_RELOAD_WINDOW_START_MS, p->active_reload_window_start_ms);
        end_ms = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_ACTIVE_RELOAD_WINDOW_END_MS, p->active_reload_window_end_ms);
        if ((int)u->reload_elapsed_ms >= start_ms && (int)u->reload_elapsed_ms <= end_ms) {
            u->active_reload_result = 2;
            gwp89_zero_event(&ev);
            ev.type = GWP89_EVENT_ACTIVE_RELOAD_WINDOW;
            ev.flags = GWP89_EVENT_FLAG_ACTIVE_RELOAD;
            ev.actor_id = u->actor_id;
            ev.actor_kind = u->actor_kind;
            ev.team_id = u->team_id;
            ev.weapon_id = p->weapon_id;
            ev.ammo_id = p->ammo_id;
            ev.cooldown_ms = u->reload_ms_left;
            ev.clip_ammo = gwp89_query_clip(m, u->actor_id, p->weapon_id);
            ev.reserve_ammo = gwp89_query_ammo(m, u->actor_id, p->ammo_id, p->weapon_id);
            gwp89_fill_weapon_event_names(&ev, p);
            gwp89_dispatch(m, &ev);
        }
    }

    if (u->reload_ms_left != 0u) return;
    clip_size = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_CLIP_SIZE, p->clip_size);
    need = clip_size - gwp89_query_clip(m, u->actor_id, p->weapon_id);
    if (need < 0) need = 0;
    got = gwp89_query_ammo(m, u->actor_id, p->ammo_id, p->weapon_id);
    if (got > need) got = need;

    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = u->actor_id;
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.weapon_id = p->weapon_id;
    packet.ammo_id = p->ammo_id;
    packet.profile = p;
    packet.user = u;
    packet.amount = got;
    packet.i_value = gwp89_query_clip(m, u->actor_id, p->weapon_id);
    packet.i_value2 = got;
    r = gwp89_provider_call(m, GWP89_SERVICE_RELOAD, GWP89_OP_RELOAD_COMPLETE, GWP89_PHASE_PRE, &packet);
    if (!(r & GWP89_PROVIDER_HANDLED) && got > 0 && gwp89_consume_ammo(m, u, p, got) == GWP89_OK) {
        gwp89_set_clip(m, u->actor_id, p->weapon_id, packet.i_value + got);
    }
    gwp89_provider_call(m, GWP89_SERVICE_RELOAD, GWP89_OP_RELOAD_COMPLETE, GWP89_PHASE_POST, &packet);
    u->clip_ammo = gwp89_query_clip(m, u->actor_id, p->weapon_id);
    u->reload_active = 0;
    u->reload_elapsed_ms = 0u;

    gwp89_zero_event(&ev);
    ev.type = GWP89_EVENT_RELOAD_END;
    ev.actor_id = u->actor_id;
    ev.actor_kind = u->actor_kind;
    ev.team_id = u->team_id;
    ev.weapon_id = p->weapon_id;
    ev.ammo_id = p->ammo_id;
    ev.clip_ammo = u->clip_ammo;
    ev.reserve_ammo = gwp89_query_ammo(m, u->actor_id, p->ammo_id, p->weapon_id);
    gwp89_fill_weapon_event_names(&ev, p);
    gwp89_dispatch(m, &ev);
}

int gwp89_begin_reload(GWP89_Manager *m, int actor_id)
{
    int us;
    int clip_size;
    int clip;
    int r;
    GWP89_UserState *u;
    const GWP89_WeaponProfile *p;
    GWP89_Event ev;
    GWP89_ProviderPacket packet;
    if (!m) return GWP89_BAD_ARG;
    us = gwp89_find_user_slot(m, actor_id);
    if (us < 0) return us;
    u = &m->users[us];
    p = gwp89_get_weapon(m, u->weapon_slot);
    if (!p) return GWP89_NOT_FOUND;
    if (!gwp89_resolve_flag(m, actor_id, p, GWP89_FLAG_CAN_RELOAD, 1)) return GWP89_CANCELLED;
    clip_size = gwp89_resolve_numeric_int(m, actor_id, p, GWP89_NUM_CLIP_SIZE, p->clip_size);
    clip = gwp89_query_clip(m, actor_id, p->weapon_id);
    if (clip_size <= 0 || clip >= clip_size) return GWP89_OK;
    if (gwp89_query_ammo(m, actor_id, p->ammo_id, p->weapon_id) <= 0) return GWP89_NO_AMMO;

    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.weapon_id = p->weapon_id;
    packet.ammo_id = p->ammo_id;
    packet.profile = p;
    packet.user = u;
    packet.ms_value = (unsigned short)gwp89_resolve_numeric_int(m, actor_id, p, GWP89_NUM_RELOAD_MS, p->reload_ms);
    r = gwp89_provider_call(m, GWP89_SERVICE_RELOAD, GWP89_OP_RELOAD_BEGIN, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;
    u->reload_active = 1;
    u->reload_ms_left = packet.ms_value;
    u->reload_elapsed_ms = 0u;
    u->active_reload_attempted = 0;
    u->active_reload_result = 0;
    gwp89_provider_call(m, GWP89_SERVICE_RELOAD, GWP89_OP_RELOAD_BEGIN, GWP89_PHASE_POST, &packet);

    gwp89_zero_event(&ev);
    ev.type = GWP89_EVENT_RELOAD_BEGIN;
    ev.actor_id = u->actor_id;
    ev.actor_kind = u->actor_kind;
    ev.team_id = u->team_id;
    ev.weapon_id = p->weapon_id;
    ev.ammo_id = p->ammo_id;
    ev.cooldown_ms = u->reload_ms_left;
    ev.clip_ammo = clip;
    ev.reserve_ammo = gwp89_query_ammo(m, actor_id, p->ammo_id, p->weapon_id);
    gwp89_fill_weapon_event_names(&ev, p);
    gwp89_dispatch(m, &ev);
    return GWP89_OK;
}

int gwp89_active_reload_press(GWP89_Manager *m, int actor_id)
{
    int us;
    int r;
    int start_ms;
    int end_ms;
    int bonus_ms;
    int penalty_ms;
    unsigned long penalized;
    GWP89_UserState *u;
    const GWP89_WeaponProfile *p;
    GWP89_ProviderPacket packet;
    GWP89_Event ev;
    if (!m) return GWP89_BAD_ARG;
    us = gwp89_find_user_slot(m, actor_id);
    if (us < 0) return us;
    u = &m->users[us];
    p = gwp89_get_weapon(m, u->weapon_slot);
    if (!p || !u->reload_active) return GWP89_NOT_FOUND;
    if (!gwp89_resolve_flag(m, actor_id, p, GWP89_FLAG_ACTIVE_RELOAD_ENABLED, p->active_reload_enabled)) return GWP89_CANCELLED;

    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = actor_id;
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.weapon_id = p->weapon_id;
    packet.ammo_id = p->ammo_id;
    packet.profile = p;
    packet.user = u;
    packet.i_value = (int)u->reload_elapsed_ms;
    packet.ms_value = u->reload_ms_left;
    packet.result_code = GWP89_ERR;
    r = gwp89_provider_call(m, GWP89_SERVICE_ACTIVE_RELOAD, GWP89_OP_ACTIVE_RELOAD_PRESS, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;
    if (!(r & GWP89_PROVIDER_HANDLED)) {
        start_ms = gwp89_resolve_numeric_int(m, actor_id, p, GWP89_NUM_ACTIVE_RELOAD_WINDOW_START_MS, p->active_reload_window_start_ms);
        end_ms = gwp89_resolve_numeric_int(m, actor_id, p, GWP89_NUM_ACTIVE_RELOAD_WINDOW_END_MS, p->active_reload_window_end_ms);
        bonus_ms = gwp89_resolve_numeric_int(m, actor_id, p, GWP89_NUM_ACTIVE_RELOAD_BONUS_MS, p->active_reload_bonus_ms);
        penalty_ms = gwp89_resolve_numeric_int(m, actor_id, p, GWP89_NUM_ACTIVE_RELOAD_PENALTY_MS, p->active_reload_penalty_ms);
        if ((int)u->reload_elapsed_ms >= start_ms && (int)u->reload_elapsed_ms <= end_ms) {
            packet.result_code = GWP89_OK;
            packet.ms_value = gwp89_sub_ms(packet.ms_value, (unsigned short)(bonus_ms < 0 ? 0 : bonus_ms));
        } else {
            packet.result_code = GWP89_ERR;
            penalized = (unsigned long)packet.ms_value + (unsigned long)(penalty_ms < 0 ? 0 : penalty_ms);
            if (penalized > 65535UL) penalized = 65535UL;
            packet.ms_value = (unsigned short)penalized;
        }
    }
    gwp89_provider_call(m, GWP89_SERVICE_ACTIVE_RELOAD, GWP89_OP_ACTIVE_RELOAD_PRESS, GWP89_PHASE_POST, &packet);
    u->reload_ms_left = packet.ms_value;
    u->active_reload_attempted = 1;
    u->active_reload_result = packet.result_code == GWP89_OK ? 1 : -1;

    gwp89_zero_event(&ev);
    ev.type = packet.result_code == GWP89_OK ? GWP89_EVENT_ACTIVE_RELOAD_SUCCESS : GWP89_EVENT_ACTIVE_RELOAD_FAIL;
    ev.flags = GWP89_EVENT_FLAG_ACTIVE_RELOAD;
    ev.actor_id = u->actor_id;
    ev.actor_kind = u->actor_kind;
    ev.team_id = u->team_id;
    ev.weapon_id = p->weapon_id;
    ev.ammo_id = p->ammo_id;
    ev.cooldown_ms = u->reload_ms_left;
    ev.clip_ammo = gwp89_query_clip(m, actor_id, p->weapon_id);
    ev.reserve_ammo = gwp89_query_ammo(m, actor_id, p->ammo_id, p->weapon_id);
    gwp89_fill_weapon_event_names(&ev, p);
    gwp89_dispatch(m, &ev);
    return packet.result_code;
}

static int gwp89_apply_actor_update(GWP89_Manager *m,
                                    const GWP89_FireInput *input,
                                    GWP89_FireInput *effective_input,
                                    GWP89_UserState **out_user)
{
    int us;
    int r;
    GWP89_UserState *u;
    GWP89_ProviderPacket packet;
    if (!m || !input || !effective_input || !out_user) return GWP89_BAD_ARG;
    *effective_input = *input;
    us = gwp89_alloc_user(m, input->actor_id, input->actor_kind, input->team_id);
    if (us < 0) return us;
    u = &m->users[us];
    u->actor_kind = input->actor_kind;
    u->team_id = input->team_id;

    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = input->actor_id;
    packet.actor_kind = input->actor_kind;
    packet.team_id = input->team_id;
    packet.user = u;
    packet.input = effective_input;
    packet.ms_value = input->dt_ms;
    packet.result_code = GWP89_OK;
    r = gwp89_provider_call(m, GWP89_SERVICE_ACTOR_STATE, GWP89_OP_UPDATE, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;

    effective_input->actor_id = input->actor_id;
    effective_input->dt_ms = packet.ms_value;
    u->actor_kind = effective_input->actor_kind;
    u->team_id = effective_input->team_id;
    if (!(r & GWP89_PROVIDER_HANDLED)) {
        gwp89_update_user_timers(m, u, effective_input->dt_ms);
        if (effective_input->trigger_flags & GWP89_TRIGGER_RELEASED) u->trigger_latched = 0;
    }
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.ms_value = effective_input->dt_ms;
    gwp89_provider_call(m, GWP89_SERVICE_ACTOR_STATE, GWP89_OP_UPDATE, GWP89_PHASE_POST, &packet);
    if (packet.result_code < 0) return packet.result_code;
    *out_user = u;
    return GWP89_OK;
}

int gwp89_update_actor(GWP89_Manager *m, const GWP89_FireInput *input)
{
    GWP89_FireInput effective_input;
    GWP89_UserState *u;
    return gwp89_apply_actor_update(m, input, &effective_input, &u);
}

static int gwp89_trigger_accept(GWP89_UserState *u, int fire_mode, int burst_count, int trigger_flags)
{
    if (!u) return 0;
    if (fire_mode == GWP89_FIRE_AUTO) return (trigger_flags & GWP89_TRIGGER_DOWN) ? 1 : 0;
    if (fire_mode == GWP89_FIRE_HOLD_ONCE) {
        if ((trigger_flags & GWP89_TRIGGER_DOWN) && !u->trigger_latched) {
            u->trigger_latched = 1;
            return 1;
        }
        return 0;
    }
    if (fire_mode == GWP89_FIRE_BURST) {
        if ((trigger_flags & GWP89_TRIGGER_PRESSED) && u->burst_left <= 0) u->burst_left = burst_count > 0 ? burst_count : 3;
        if ((trigger_flags & GWP89_TRIGGER_DOWN) && u->burst_left > 0) return 1;
        return 0;
    }
    return (trigger_flags & GWP89_TRIGGER_PRESSED) ? 1 : 0;
}

static void gwp89_fill_event_base(GWP89_Manager *m, GWP89_Event *ev, const GWP89_UserState *u, const GWP89_WeaponProfile *p, const GWP89_FireInput *input, const GWP89_PoseResult *pose)
{
    gwp89_zero_event(ev);
    if (!ev || !u || !p) return;
    ev->actor_id = u->actor_id;
    ev->actor_kind = u->actor_kind;
    ev->team_id = u->team_id;
    ev->view_style = input ? input->view_style : GWP89_VIEW_UNKNOWN;
    ev->weapon_id = p->weapon_id;
    ev->gun_id = p->gun_id;
    ev->ammo_id = p->ammo_id;
    ev->projectile_id = p->projectile_id;
    ev->shell_id = p->shell_id;
    ev->muzzle_id = p->muzzle_id;
    ev->casing_id = p->casing_id;
    ev->trail_id = p->trail_id;
    ev->projectile_mesh_id = p->projectile_mesh_id;
    ev->shell_mesh_id = p->shell_mesh_id;
    ev->clip_ammo = gwp89_query_clip(m, u->actor_id, p->weapon_id);
    ev->cooldown_ms = (unsigned short)gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_COOLDOWN_MS, p->cooldown_ms);
    ev->life_ms = (unsigned short)gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_PROJECTILE_LIFE_MS, p->projectile_life_ms);
    ev->damage_fx = gwp89_resolve_numeric_fx(m, u->actor_id, p, GWP89_NUM_DAMAGE_FX, p->damage_fx);
    ev->speed_fx = gwp89_resolve_numeric_fx(m, u->actor_id, p, GWP89_NUM_SPEED_FX, p->speed_fx);
    ev->radius_fx = gwp89_resolve_numeric_fx(m, u->actor_id, p, GWP89_NUM_PROJECTILE_RADIUS_FX, p->projectile_radius_fx);
    ev->projectile_mesh_scale_fx = gwp89_resolve_numeric_fx(m, u->actor_id, p, GWP89_NUM_PROJECTILE_MESH_SCALE_FX, p->projectile_mesh_scale_fx);
    ev->shell_mesh_scale_fx = gwp89_resolve_numeric_fx(m, u->actor_id, p, GWP89_NUM_SHELL_MESH_SCALE_FX, p->shell_mesh_scale_fx);
    ev->range_fx = gwp89_resolve_numeric_fx(m, u->actor_id, p, GWP89_NUM_RANGE_FX, p->range_fx);
    ev->spread_fx = gwp89_resolve_numeric_fx(m, u->actor_id, p, GWP89_NUM_SPREAD_FX, p->spread_fx);
    ev->recoil_fx = gwp89_resolve_numeric_fx(m, u->actor_id, p, GWP89_NUM_RECOIL_FX, p->recoil_fx);
    ev->travel_distance_fx = ev->range_fx;
    ev->zoom_fx = gwp89_resolve_zoom_value(m, u->actor_id, p, (GWP89_FireInput *)input, input ? input->zoom_fx : GWP89_FIX_ONE);
    if (input) {
        ev->camera_origin = input->camera_origin;
        ev->camera_forward = input->camera_forward;
        /* The engine feeds camera-aligned socket axes. Keeping them here also
           makes shotgun recovery deterministic after movement/recoil. */
        ev->camera_right = input->socket_right;
        ev->camera_up = input->socket_up;
    }
    if (pose) {
        ev->origin = pose->projectile_origin;
        ev->muzzle_origin = pose->muzzle_origin;
        ev->casing_origin = pose->casing_origin;
        ev->direction = pose->direction;
        ev->hit_point = pose->hit_point;
        if (pose->has_hit) ev->flags |= GWP89_EVENT_FLAG_HIT_VALID;
    }
    gwp89_fill_weapon_event_names(ev, p);
}

static void gwp89_seed_transform_from_input(GWP89_Transform *t, const GWP89_FireInput *input)
{
    if (!t || !input) return;
    memset(t, 0, sizeof(*t));
    t->position = input->socket_origin;
    t->forward = input->socket_forward;
    t->right = input->socket_right;
    t->up = input->socket_up;
    t->scale = gwp89_v3(GWP89_FIX_ONE, GWP89_FIX_ONE, GWP89_FIX_ONE);
}

static void gwp89_prepare_provider_input(GWP89_Manager *m, const GWP89_UserState *u, const GWP89_WeaponProfile *p, const GWP89_FireInput *source, GWP89_FireInput *out)
{
    GWP89_ProviderPacket packet;
    int r;
    if (!m || !u || !p || !source || !out) return;
    *out = *source;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = u->actor_id;
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.weapon_id = p->weapon_id;
    packet.profile = p;
    packet.user = (GWP89_UserState *)u;
    packet.input = out;
    gwp89_seed_transform_from_input(&packet.transform, out);
    r = gwp89_provider_call(m, GWP89_SERVICE_TRANSFORM, GWP89_OP_GET_ACTOR_TRANSFORM, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_HANDLED) {
        out->socket_origin = packet.transform.position;
        out->socket_forward = packet.transform.forward;
        out->socket_right = packet.transform.right;
        out->socket_up = packet.transform.up;
    }
    gwp89_provider_call(m, GWP89_SERVICE_TRANSFORM, GWP89_OP_GET_ACTOR_TRANSFORM, GWP89_PHASE_POST, &packet);

    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = u->actor_id;
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.weapon_id = p->weapon_id;
    packet.profile = p;
    packet.user = (GWP89_UserState *)u;
    packet.input = out;
    packet.camera.valid = 1;
    packet.camera.camera_id = out->camera_id;
    packet.camera.view_style = out->view_style;
    packet.camera.origin = out->camera_origin;
    packet.camera.forward = out->camera_forward;
    packet.camera.right = out->socket_right;
    packet.camera.up = out->socket_up;
    packet.camera.zoom_fx = out->zoom_fx != 0L ? out->zoom_fx : GWP89_FIX_ONE;
    r = gwp89_provider_call(m, GWP89_SERVICE_CAMERA, GWP89_OP_GET_CAMERA, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_HANDLED) {
        out->camera_id = packet.camera.camera_id;
        out->view_style = packet.camera.view_style;
        out->camera_origin = packet.camera.origin;
        out->camera_forward = packet.camera.forward;
    }
    gwp89_provider_call(m, GWP89_SERVICE_CAMERA, GWP89_OP_GET_CAMERA, GWP89_PHASE_POST, &packet);

    out->zoom_fx = gwp89_resolve_zoom_value(m, u->actor_id, p, out, packet.camera.zoom_fx);
}

static GWP89_Transform gwp89_resolve_socket(GWP89_Manager *m, const GWP89_PoseRequest *req, int socket_kind)
{
    GWP89_ProviderPacket packet;
    GWP89_Transform t;
    int r;
    gwp89_seed_transform_from_input(&t, &req->input);
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = req->actor_id;
    packet.actor_kind = req->actor_kind;
    packet.team_id = req->team_id;
    packet.weapon_id = req->weapon_id;
    packet.gun_id = req->gun_id;
    packet.socket_kind = socket_kind;
    packet.profile = req->profile;
    packet.input = (GWP89_FireInput *)&req->input;
    packet.pose_request = (GWP89_PoseRequest *)req;
    packet.transform = t;
    r = gwp89_provider_call(m, GWP89_SERVICE_SOCKETS, GWP89_OP_GET_SOCKET, GWP89_PHASE_PRE, &packet);
    if (!(r & GWP89_PROVIDER_HANDLED)) packet.transform = t;
    gwp89_provider_call(m, GWP89_SERVICE_SOCKETS, GWP89_OP_GET_SOCKET, GWP89_PHASE_POST, &packet);
    return packet.transform;
}

static GWP89_Vec3 gwp89_resolve_spread(GWP89_Manager *m, GWP89_PoseRequest *req, GWP89_Vec3 direction)
{
    GWP89_ProviderPacket packet;
    int r;
    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = req->actor_id;
    packet.actor_kind = req->actor_kind;
    packet.team_id = req->team_id;
    packet.weapon_id = req->weapon_id;
    packet.gun_id = req->gun_id;
    packet.profile = req->profile;
    packet.pose_request = req;
    packet.vec_a = direction;
    packet.fx_value = req->spread_fx;
    packet.i_value = req->pellet_index;
    packet.i_value2 = req->pellet_count;
    r = gwp89_provider_call(m, GWP89_SERVICE_SPREAD, GWP89_OP_APPLY_SPREAD, GWP89_PHASE_PRE, &packet);
    if (!(r & GWP89_PROVIDER_HANDLED)) packet.vec_b = gwp89_apply_spread_fallback(req, direction);
    gwp89_provider_call(m, GWP89_SERVICE_SPREAD, GWP89_OP_APPLY_SPREAD, GWP89_PHASE_POST, &packet);
    return gwp89_math_normalize_provider(m, req->actor_id, req->profile, packet.vec_b);
}

static void gwp89_default_pose(GWP89_Manager *m, GWP89_PoseRequest *req, GWP89_PoseResult *out_pose)
{
    GWP89_Transform projectile_socket;
    GWP89_Transform muzzle_socket;
    GWP89_Transform casing_socket;
    GWP89_ProviderPacket packet;
    GWP89_Vec3 aim_direction;
    GWP89_Vec3 target_delta;
    GWP89_Vec3 scaled;
    GWP89_Vec3 ray_origin;
    gwp89_fx range_fx;
    int r;
    if (!m || !req || !out_pose) return;
    memset(out_pose, 0, sizeof(*out_pose));
    projectile_socket = gwp89_resolve_socket(m, req, GWP89_SOCKET_PROJECTILE);
    muzzle_socket = gwp89_resolve_socket(m, req, GWP89_SOCKET_MUZZLE);
    casing_socket = gwp89_resolve_socket(m, req, GWP89_SOCKET_CASING);
    out_pose->projectile_origin = projectile_socket.position;
    out_pose->muzzle_origin = muzzle_socket.position;
    out_pose->casing_origin = casing_socket.position;

    /*
     * The HUD ray belongs to the camera, while the physical projectile belongs
     * to the muzzle. Resolve spread and collision on the camera ray first,
     * then converge the muzzle direction on that target point. The previous
     * fallback kept both rays parallel, so third-person shots missed the
     * crosshair by the camera/muzzle offset forever.
     */
    aim_direction = req->input.camera_forward;
    if (aim_direction.x == 0L && aim_direction.y == 0L &&
        aim_direction.z == 0L)
        aim_direction = projectile_socket.forward;
    aim_direction = gwp89_resolve_spread(m, req, aim_direction);
    range_fx = gwp89_resolve_numeric_fx(m, req->actor_id, req->profile,
        GWP89_NUM_RANGE_FX,
        req->profile ? req->profile->range_fx : gwp89_fx_from_int(64));
    ray_origin = req->input.camera_origin;
    if (ray_origin.x == 0L && ray_origin.y == 0L && ray_origin.z == 0L)
        ray_origin = out_pose->projectile_origin;

    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = req->actor_id;
    packet.actor_kind = req->actor_kind;
    packet.team_id = req->team_id;
    packet.weapon_id = req->weapon_id;
    packet.gun_id = req->gun_id;
    packet.profile = req->profile;
    packet.pose_request = req;
    packet.pose_result = out_pose;
    packet.vec_a = ray_origin;
    packet.vec_b = aim_direction;
    packet.fx_value = range_fx;
    r = gwp89_provider_call(m, GWP89_SERVICE_RAYCAST,
                            GWP89_OP_RAYCAST,
                            GWP89_PHASE_PRE, &packet);
    if ((r & GWP89_PROVIDER_HANDLED) && packet.hit.hit) {
        out_pose->has_hit = 1;
        out_pose->hit_point = packet.hit.point;
    } else {
        scaled = gwp89_math_scale_provider(m, req->actor_id,
                                           req->profile,
                                           aim_direction, range_fx);
        out_pose->hit_point = gwp89_math_add_provider(m, req->actor_id,
            req->profile, ray_origin, scaled);
        out_pose->has_hit = 0;
    }
    gwp89_provider_call(m, GWP89_SERVICE_RAYCAST,
                        GWP89_OP_RAYCAST,
                        GWP89_PHASE_POST, &packet);

    target_delta = gwp89_v3(
        out_pose->hit_point.x - out_pose->projectile_origin.x,
        out_pose->hit_point.y - out_pose->projectile_origin.y,
        out_pose->hit_point.z - out_pose->projectile_origin.z);
    out_pose->direction = gwp89_math_normalize_provider(m, req->actor_id,
                                                        req->profile,
                                                        target_delta);
    if (out_pose->direction.x == 0L && out_pose->direction.y == 0L &&
        out_pose->direction.z == 0L)
        out_pose->direction = aim_direction;
}

static void gwp89_make_pose(GWP89_Manager *m, const GWP89_UserState *u, const GWP89_WeaponProfile *p, const GWP89_FireInput *input, int pellet_index, GWP89_PoseResult *pose)
{
    GWP89_PoseRequest req;
    GWP89_FireInput provider_input;
    GWP89_ProviderPacket packet;
    int r;
    int ok;
    memset(&req, 0, sizeof(req));
    memset(pose, 0, sizeof(*pose));
    gwp89_prepare_provider_input(m, u, p, input, &provider_input);
    req.actor_id = u->actor_id;
    req.actor_kind = u->actor_kind;
    req.team_id = u->team_id;
    req.view_style = provider_input.view_style;
    req.weapon_id = p->weapon_id;
    req.gun_id = p->gun_id;
    req.pellet_index = pellet_index;
    req.pellet_count = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_PELLET_COUNT, p->pellet_count);
    req.spread_fx = gwp89_resolve_numeric_fx(m, u->actor_id, p, GWP89_NUM_SPREAD_FX, p->spread_fx);
    req.profile = p;
    req.input = provider_input;

    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = u->actor_id;
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.weapon_id = p->weapon_id;
    packet.gun_id = p->gun_id;
    packet.profile = p;
    packet.user = (GWP89_UserState *)u;
    packet.input = &provider_input;
    packet.pose_request = &req;
    packet.pose_result = pose;
    r = gwp89_provider_call(m, GWP89_SERVICE_POSE, GWP89_OP_RESOLVE_POSE, GWP89_PHASE_PRE, &packet);
    if (!(r & GWP89_PROVIDER_HANDLED)) {
        ok = 0;
        if (m->hooks.resolve_pose) ok = m->hooks.resolve_pose(m->hooks.ctx, &req, pose);
        if (!ok) gwp89_default_pose(m, &req, pose);
    }
    gwp89_provider_call(m, GWP89_SERVICE_POSE, GWP89_OP_RESOLVE_POSE, GWP89_PHASE_POST, &packet);
}

static void gwp89_emit_ammo_changed(GWP89_Manager *m, GWP89_UserState *u, const GWP89_WeaponProfile *p)
{
    GWP89_Event ev;
    gwp89_fill_event_base(m, &ev, u, p, 0, 0);
    ev.type = GWP89_EVENT_AMMO_CHANGED;
    ev.amount = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_AMMO_PER_SHOT, p->ammo_per_shot);
    ev.reserve_ammo = gwp89_query_ammo(m, u->actor_id, p->ammo_id, p->weapon_id);
    gwp89_dispatch(m, &ev);
}

static void gwp89_emit_dry_fire(GWP89_Manager *m, GWP89_UserState *u, const GWP89_WeaponProfile *p, const GWP89_FireInput *input)
{
    GWP89_Event ev;
    if (!gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_ALLOW_DRY_FIRE, p->allow_dry_fire_event)) return;
    gwp89_fill_event_base(m, &ev, u, p, input, 0);
    ev.type = GWP89_EVENT_DRY_FIRE;
    ev.reserve_ammo = gwp89_query_ammo(m, u->actor_id, p->ammo_id, p->weapon_id);
    gwp89_dispatch(m, &ev);
}

int gwp89_try_fire(GWP89_Manager *m, const GWP89_FireInput *input)
{
    int i;
    int count;
    int cost;
    int ammo_ok;
    int clip_size;
    int clip;
    int fire_mode;
    int burst_count;
    int use_clip;
    int r;
    int emit_visuals;
    int emit_projectile;
    int emit_muzzle;
    int emit_casing;
    int emit_trail;
    GWP89_UserState *u;
    const GWP89_WeaponProfile *p;
    GWP89_PoseResult pose;
    GWP89_Event ev;
    GWP89_ProviderPacket packet;
    GWP89_FireInput effective_input;
    const GWP89_FireInput *fire_input;
    if (!m || !input) return GWP89_BAD_ARG;
    r = gwp89_apply_actor_update(m, input, &effective_input, &u);
    if (r < 0) return r;
    fire_input = &effective_input;
    p = gwp89_get_weapon(m, u->weapon_slot);
    if (!p) return GWP89_NOT_FOUND;
    if (!gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_WEAPON_ENABLED, 1)) return GWP89_CANCELLED;
    if (!gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_CAN_FIRE, 1)) return GWP89_CANCELLED;

    fire_mode = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_FIRE_MODE, p->fire_mode);
    burst_count = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_BURST_COUNT, p->burst_count);
    if (!gwp89_trigger_accept(u, fire_mode, burst_count, fire_input->trigger_flags)) return GWP89_TRIGGER_LOCKED;
    if (u->reload_active || u->cooldown_ms_left > 0u) return GWP89_COOLDOWN;

    gwp89_provider_packet_defaults(&packet);
    packet.actor_id = u->actor_id;
    packet.actor_kind = u->actor_kind;
    packet.team_id = u->team_id;
    packet.weapon_id = p->weapon_id;
    packet.gun_id = p->gun_id;
    packet.ammo_id = p->ammo_id;
    packet.profile = p;
    packet.user = u;
    packet.input = &effective_input;
    r = gwp89_provider_call(m, GWP89_SERVICE_FLAGS, GWP89_OP_FIRE_VALIDATE, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;

    cost = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_AMMO_PER_SHOT, p->ammo_per_shot);
    if (cost <= 0) cost = 1;
    clip_size = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_CLIP_SIZE, p->clip_size);
    use_clip = gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_USE_INTERNAL_CLIP, clip_size > 0);
    clip = gwp89_query_clip(m, u->actor_id, p->weapon_id);
    ammo_ok = use_clip ? (clip >= cost) : (gwp89_query_ammo(m, u->actor_id, p->ammo_id, p->weapon_id) >= cost);
    if (!ammo_ok) {
        gwp89_emit_dry_fire(m, u, p, fire_input);
        return GWP89_NO_AMMO;
    }
    if (use_clip) {
        gwp89_set_clip(m, u->actor_id, p->weapon_id, clip - cost);
    } else if (gwp89_consume_ammo(m, u, p, cost) != GWP89_OK) {
        gwp89_emit_dry_fire(m, u, p, fire_input);
        return GWP89_NO_AMMO;
    }
    u->clip_ammo = gwp89_query_clip(m, u->actor_id, p->weapon_id);
    u->cooldown_ms_left = (unsigned short)gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_COOLDOWN_MS, p->cooldown_ms);
    if (fire_mode == GWP89_FIRE_BURST && u->burst_left > 0) u->burst_left--;
    gwp89_emit_ammo_changed(m, u, p);

    count = gwp89_resolve_numeric_int(m, u->actor_id, p, GWP89_NUM_PELLET_COUNT, p->pellet_count);
    if (count < 1) count = 1;
    if (count > 12) count = 12;
    emit_visuals = gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_EMIT_VISUALS, 1);
    emit_projectile = gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_EMIT_PROJECTILE, 1);
    emit_muzzle = gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_EMIT_MUZZLE, 1);
    emit_casing = gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_EMIT_CASING, 1);
    emit_trail = gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_EMIT_TRAIL, 1);

    gwp89_make_pose(m, u, p, fire_input, 0, &pose);
    gwp89_fill_event_base(m, &ev, u, p, fire_input, &pose);
    ev.type = GWP89_EVENT_FIRE_ACCEPTED;
    ev.pellet_count = count;
    ev.reserve_ammo = gwp89_query_ammo(m, u->actor_id, p->ammo_id, p->weapon_id);
    if (!gwp89_resolve_flag(m, u->actor_id, p, GWP89_FLAG_APPLY_RECOIL, 1)) ev.recoil_fx = 0L;
    gwp89_dispatch(m, &ev);

    if (emit_visuals) {
        ev.type = GWP89_EVENT_VISUAL_MOD_REQUEST;
        gwp89_dispatch(m, &ev);
    }
    if (emit_muzzle) {
        ev.type = GWP89_EVENT_MUZZLE_REQUEST;
        ev.origin = pose.muzzle_origin;
        gwp89_dispatch(m, &ev);
    }
    if (emit_casing) {
        ev.type = GWP89_EVENT_CASING_REQUEST;
        ev.origin = pose.casing_origin;
        gwp89_dispatch(m, &ev);
    }

    for (i = 0; i < count; i++) {
        gwp89_make_pose(m, u, p, fire_input, i, &pose);
        gwp89_fill_event_base(m, &ev, u, p, fire_input, &pose);
        ev.pellet_index = i;
        ev.pellet_count = count;
        if (count > 1) ev.damage_fx /= (gwp89_fx)count;
        if (i == 0) ev.flags |= GWP89_EVENT_FLAG_FIRST_PELLET;
        if (i == count - 1) ev.flags |= GWP89_EVENT_FLAG_LAST_PELLET;
        ev.projectile_slot_hint = (u->actor_id * 31 + p->weapon_id * 7 + i) & 0x7fff;
        if (emit_projectile) {
            ev.type = GWP89_EVENT_MESH_ASSIGN_REQUEST;
            gwp89_dispatch(m, &ev);
            ev.type = GWP89_EVENT_PROJECTILE_REQUEST;
            gwp89_dispatch(m, &ev);
        }
        if (emit_trail) {
            ev.type = GWP89_EVENT_TRAIL_REQUEST;
            gwp89_dispatch(m, &ev);
        }
    }

    packet.i_value = count;
    packet.amount = cost;
    packet.result_code = GWP89_OK;
    gwp89_provider_call(m, GWP89_SERVICE_FLAGS, GWP89_OP_FIRE_ACCEPTED, GWP89_PHASE_POST, &packet);
    gwp89_status_begin(m, "gweapon89:fired");
    gwp89_status_key_int(m, "actor", u->actor_id);
    gwp89_text_append(m->status, (int)sizeof(m->status), " weapon=");
    gwp89_text_append(m->status, (int)sizeof(m->status), p->name);
    gwp89_status_key_int(m, "clip", gwp89_query_clip(m, u->actor_id, p->weapon_id));
    gwp89_status_key_int(m, "pellets", count);
    gwp89_status_key_int(m, "providers", m->provider_count);
    return GWP89_OK;
}

int gwp89_poll_event(GWP89_Manager *m, GWP89_Event *out_event)
{
    if (!m || !out_event) return 0;
    if (m->event_count <= 0) return 0;
    *out_event = m->events[m->event_head];
    m->event_head++;
    if (m->event_head >= GWP89_MAX_EVENTS) m->event_head = 0;
    m->event_count--;
    return 1;
}

void gwp89_clear_events(GWP89_Manager *m)
{
    if (!m) return;
    m->event_head = 0;
    m->event_tail = 0;
    m->event_count = 0;
}

static unsigned short gwp89_rate_interval_ms(const char *text,
                                                  long milliseconds_per_unit,
                                                  unsigned short fallback)
{
    gwp89_fx rate_fx;
    long numerator;
    long interval;
    if (!text || !text[0] || milliseconds_per_unit <= 0L) return fallback;
    rate_fx = gwp89_fx_from_text(text);
    if (rate_fx <= 0L) return fallback;
    numerator = milliseconds_per_unit * 4096L;
    interval = (numerator + ((long)rate_fx / 2L)) / (long)rate_fx;
    if (interval < 1L) interval = 1L;
    if (interval > 65535L) interval = 65535L;
    return (unsigned short)interval;
}

static void gwp89_apply_profile_kv(GWP89_WeaponProfile *p, const char *key, const char *value)
{
    char v[GWP89_LOCAL_TEXT_MAX];
    if (!p || !key || !value) return;
    strncpy(v, value, sizeof(v) - 1u);
    v[sizeof(v) - 1u] = '\0';
    gwp89_unquote(v);
    if (gwp89_streq_id(key, "name") || gwp89_streq_id(key, "profile") || gwp89_streq_id(key, "weapon") || gwp89_streq_id(key, "weapon_name")) gwp89_copy_id(p->name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "weapon_id") || gwp89_streq_id(key, "id")) p->weapon_id = gwp89_parse_int(v, p->weapon_id);
    else if (gwp89_streq_id(key, "gun_id")) p->gun_id = gwp89_parse_int(v, p->gun_id);
    else if (gwp89_streq_id(key, "gun")) gwp89_copy_id(p->gun_name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "ammo_id")) p->ammo_id = gwp89_parse_int(v, p->ammo_id);
    else if (gwp89_streq_id(key, "ammo") || gwp89_streq_id(key, "ammo_type")) gwp89_copy_id(p->ammo_name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "projectile_id")) p->projectile_id = gwp89_parse_int(v, p->projectile_id);
    else if (gwp89_streq_id(key, "projectile") || gwp89_streq_id(key, "projectile_behavior")) gwp89_copy_id(p->projectile_name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "shell_id")) p->shell_id = gwp89_parse_int(v, p->shell_id);
    else if (gwp89_streq_id(key, "shell") || gwp89_streq_id(key, "casing_shell")) gwp89_copy_id(p->shell_name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "muzzle_id")) p->muzzle_id = gwp89_parse_int(v, p->muzzle_id);
    else if (gwp89_streq_id(key, "muzzle") || gwp89_streq_id(key, "muzzle_profile")) gwp89_copy_id(p->muzzle_name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "casing_id")) p->casing_id = gwp89_parse_int(v, p->casing_id);
    else if (gwp89_streq_id(key, "casing") || gwp89_streq_id(key, "gunslinger")) gwp89_copy_id(p->casing_name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "trail_id")) p->trail_id = gwp89_parse_int(v, p->trail_id);
    else if (gwp89_streq_id(key, "trail") || gwp89_streq_id(key, "trail_profile")) gwp89_copy_id(p->trail_name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "projectile_mesh_id") || gwp89_streq_id(key, "bullet_mesh_id")) p->projectile_mesh_id = gwp89_parse_int(v, p->projectile_mesh_id);
    else if (gwp89_streq_id(key, "projectile_mesh") || gwp89_streq_id(key, "bullet_mesh")) gwp89_copy_id(p->projectile_mesh_name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "shell_mesh_id")) p->shell_mesh_id = gwp89_parse_int(v, p->shell_mesh_id);
    else if (gwp89_streq_id(key, "shell_mesh")) gwp89_copy_id(p->shell_mesh_name, GWP89_NAME_MAX, v);
    else if (gwp89_streq_id(key, "fire_mode") || gwp89_streq_id(key, "trigger") || gwp89_streq_id(key, "shot_mode")) p->fire_mode = gwp89_fire_mode_from_name(v);
    else if (gwp89_streq_id(key, "clip_size") || gwp89_streq_id(key, "clip") || gwp89_streq_id(key, "capacity") || gwp89_streq_id(key, "magazine_capacity") || gwp89_streq_id(key, "cartucho_capacidad") || gwp89_streq_id(key, "capacidad")) p->clip_size = gwp89_parse_int(v, p->clip_size);
    else if (gwp89_streq_id(key, "ammo_per_shot") || gwp89_streq_id(key, "cost") || gwp89_streq_id(key, "balas_por_tiro") || gwp89_streq_id(key, "ammo_cost")) p->ammo_per_shot = gwp89_parse_int(v, p->ammo_per_shot);
    else if (gwp89_streq_id(key, "pellets") || gwp89_streq_id(key, "projectile_count")) p->pellet_count = gwp89_parse_int(v, p->pellet_count);
    else if (gwp89_streq_id(key, "burst_count")) p->burst_count = gwp89_parse_int(v, p->burst_count);
    else if (gwp89_streq_id(key, "dry_fire") || gwp89_streq_id(key, "allow_dry_fire_event")) p->allow_dry_fire_event = gwp89_parse_int(v, p->allow_dry_fire_event);
    else if (gwp89_streq_id(key, "active_reload") || gwp89_streq_id(key, "active_reload_enabled")) p->active_reload_enabled = gwp89_parse_int(v, p->active_reload_enabled);
    else if (gwp89_streq_id(key, "cooldown_ms") || gwp89_streq_id(key, "cooldown") || gwp89_streq_id(key, "fire_rate_ms") || gwp89_streq_id(key, "tempo_ms") || gwp89_streq_id(key, "shot_delay_ms") || gwp89_streq_id(key, "shot_interval_ms") || gwp89_streq_id(key, "fire_interval_ms")) p->cooldown_ms = (unsigned short)gwp89_parse_int(v, p->cooldown_ms);
    else if (gwp89_streq_id(key, "fire_rate_bps") || gwp89_streq_id(key, "shots_per_second") || gwp89_streq_id(key, "rounds_per_second") || gwp89_streq_id(key, "cadence_bps")) p->cooldown_ms = gwp89_rate_interval_ms(v, 1000L, p->cooldown_ms);
    else if (gwp89_streq_id(key, "fire_rate_rpm") || gwp89_streq_id(key, "rounds_per_minute") || gwp89_streq_id(key, "rpm")) p->cooldown_ms = gwp89_rate_interval_ms(v, 60000L, p->cooldown_ms);
    else if (gwp89_streq_id(key, "reload_ms") || gwp89_streq_id(key, "reload") || gwp89_streq_id(key, "reload_time_ms") || gwp89_streq_id(key, "tiempo_recarga_ms") || gwp89_streq_id(key, "reload_tempo_ms")) p->reload_ms = (unsigned short)gwp89_parse_int(v, p->reload_ms);
    else if (gwp89_streq_id(key, "projectile_life_ms") || gwp89_streq_id(key, "life_ms") || gwp89_streq_id(key, "lifetime_ms") || gwp89_streq_id(key, "tiempo_vida_ms")) p->projectile_life_ms = (unsigned short)gwp89_parse_int(v, p->projectile_life_ms);
    else if (gwp89_streq_id(key, "active_reload_window_start_ms")) p->active_reload_window_start_ms = (unsigned short)gwp89_parse_int(v, p->active_reload_window_start_ms);
    else if (gwp89_streq_id(key, "active_reload_window_end_ms")) p->active_reload_window_end_ms = (unsigned short)gwp89_parse_int(v, p->active_reload_window_end_ms);
    else if (gwp89_streq_id(key, "active_reload_bonus_ms")) p->active_reload_bonus_ms = (unsigned short)gwp89_parse_int(v, p->active_reload_bonus_ms);
    else if (gwp89_streq_id(key, "active_reload_penalty_ms")) p->active_reload_penalty_ms = (unsigned short)gwp89_parse_int(v, p->active_reload_penalty_ms);
    else if (gwp89_streq_id(key, "damage") || gwp89_streq_id(key, "damage_fx") || gwp89_streq_id(key, "damage_points") || gwp89_streq_id(key, "dmg")) p->damage_fx = gwp89_fx_from_text(v);
    else if (gwp89_streq_id(key, "projectile_speed") || gwp89_streq_id(key, "bullet_speed") || gwp89_streq_id(key, "muzzle_velocity") || gwp89_streq_id(key, "speed") || gwp89_streq_id(key, "speed_fx") || gwp89_streq_id(key, "velocity") || gwp89_streq_id(key, "velocidad")) p->speed_fx = gwp89_fx_from_text(v);
    else if (gwp89_streq_id(key, "range") || gwp89_streq_id(key, "range_fx") || gwp89_streq_id(key, "range_units") || gwp89_streq_id(key, "alcance")) p->range_fx = gwp89_fx_from_text(v);
    else if (gwp89_streq_id(key, "spread") || gwp89_streq_id(key, "spread_fx") || gwp89_streq_id(key, "spread_deg")) p->spread_fx = gwp89_fx_from_text(v);
    else if (gwp89_streq_id(key, "radius") || gwp89_streq_id(key, "projectile_radius")) p->projectile_radius_fx = gwp89_fx_from_text(v);
    else if (gwp89_streq_id(key, "projectile_mesh_scale") || gwp89_streq_id(key, "bullet_scale")) p->projectile_mesh_scale_fx = gwp89_fx_from_text(v);
    else if (gwp89_streq_id(key, "shell_mesh_scale") || gwp89_streq_id(key, "shell_scale")) p->shell_mesh_scale_fx = gwp89_fx_from_text(v);
    else if (gwp89_streq_id(key, "recoil") || gwp89_streq_id(key, "recoil_fx")) p->recoil_fx = gwp89_fx_from_text(v);
}

static int gwp89_section_is_weapon(const char *section)
{
    if (!section) return 0;
    if (gwp89_streq_id(section, "weapon")) return 1;
    if (strncmp(section, "weapon:", 7) == 0) return 1;
    if (strncmp(section, "weapon_", 7) == 0) return 1;
    return 0;
}

static void gwp89_section_name_to_profile(const char *section, GWP89_WeaponProfile *p)
{
    const char *name;
    if (!section || !p) return;
    name = 0;
    if (strncmp(section, "weapon:", 7) == 0) name = section + 7;
    else if (strncmp(section, "weapon_", 7) == 0) name = section + 7;
    if (name && name[0]) gwp89_copy_id(p->name, GWP89_NAME_MAX, name);
}

static int gwp89_load_ini_text_named(GWP89_Manager *m, const char *text, const char *default_name)
{
    GWP89_WeaponProfile cur;
    int in_weapon;
    int have_any_kv;
    int added;
    char section[GWP89_NAME_MAX];
    char line[GWP89_LINE_MAX];
    char key[96];
    char value[128];
    const char *p;
    int li;
    char c;
    char *eq;

    if (!m || !text) return GWP89_BAD_ARG;
    gwp89_profile_defaults(&cur);
    in_weapon = 0;
    have_any_kv = 0;
    added = 0;
    memset(section, 0, sizeof(section));
    p = text;
    while (*p) {
        li = 0;
        while (*p && *p != '\n' && li < GWP89_LINE_MAX - 1) {
            line[li++] = *p++;
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        line[li] = '\0';
        gwp89_strip_comment(line);
        gwp89_trim(line);
        if (!line[0]) continue;
        if (line[0] == '[') {
            if (in_weapon && have_any_kv) {
                gwp89_add_weapon(m, &cur);
                added++;
            }
            gwp89_profile_defaults(&cur);
            have_any_kv = 0;
            memset(section, 0, sizeof(section));
            li = 1;
            while (li < (int)sizeof(section) && line[li] && line[li] != ']') {
                c = gwp89_norm_char(line[li]);
                section[li - 1] = c;
                li++;
            }
            section[li - 1] = '\0';
            gwp89_trim(section);
            in_weapon = gwp89_section_is_weapon(section);
            if (in_weapon) {
                if (default_name && default_name[0] && gwp89_streq_id(section, "weapon")) gwp89_copy_id(cur.name, GWP89_NAME_MAX, default_name);
                gwp89_section_name_to_profile(section, &cur);
            }
            continue;
        }
        if (!in_weapon) continue;
        eq = strchr(line, '=');
        if (!eq) eq = strchr(line, ':');
        if (!eq) continue;
        *eq = '\0';
        strncpy(key, line, sizeof(key) - 1u);
        key[sizeof(key) - 1u] = '\0';
        strncpy(value, eq + 1, sizeof(value) - 1u);
        value[sizeof(value) - 1u] = '\0';
        gwp89_trim(key);
        gwp89_trim(value);
        if (key[0]) {
            gwp89_apply_profile_kv(&cur, key, value);
            have_any_kv = 1;
        }
    }
    if (in_weapon && have_any_kv) {
        gwp89_add_weapon(m, &cur);
        added++;
    }
    gwp89_status_begin(m, "gweapon89:ini_text");
    gwp89_status_key_int(m, "added", added);
    gwp89_status_key_int(m, "weapons", m->weapon_count);
    return added;
}

int gwp89_load_ini_text(GWP89_Manager *m, const char *text)
{
    return gwp89_load_ini_text_named(m, text, 0);
}

static void gwp89_path_stem_to_id(const char *path, char *out_name, int cap)
{
    const char *base;
    const char *dot;
    int i;
    if (!out_name || cap <= 0) return;
    out_name[0] = '\0';
    if (!path) return;
    base = path;
    for (i = 0; path[i]; i++) {
        if (path[i] == '/' || path[i] == '\\') base = path + i + 1;
    }
    dot = 0;
    for (i = 0; base[i]; i++) {
        if (base[i] == '.') dot = base + i;
    }
    if (dot) {
        char tmp[GWP89_NAME_MAX];
        int n;
        n = (int)(dot - base);
        if (n >= GWP89_NAME_MAX) n = GWP89_NAME_MAX - 1;
        for (i = 0; i < n; i++) tmp[i] = base[i];
        tmp[n] = '\0';
        gwp89_copy_id(out_name, cap, tmp);
    } else {
        gwp89_copy_id(out_name, cap, base);
    }
}

int gwp89_load_ini_file(GWP89_Manager *m, const char *path)
{
    char text[8192];
    char default_name[GWP89_NAME_MAX];
    int r;
    GWP89_ProviderPacket packet;
    if (!m || !path) return GWP89_BAD_ARG;
    text[0] = '\0';
    gwp89_provider_packet_defaults(&packet);
    packet.text_in = path;
    packet.text_out = text;
    packet.text_capacity = (int)sizeof(text);
    packet.text_length = 0;
    packet.result_code = GWP89_NOT_FOUND;
    r = gwp89_provider_call(m, GWP89_SERVICE_IO, GWP89_OP_READ_TEXT_FILE, GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return GWP89_CANCELLED;
    if (!(r & GWP89_PROVIDER_HANDLED)) return GWP89_NOT_FOUND;
    if (packet.result_code < 0) return packet.result_code;
    if (!packet.text_out || packet.text_capacity <= 0) return GWP89_PROVIDER_ERROR;
    if (packet.text_length < 0) packet.text_length = 0;
    if (packet.text_length >= packet.text_capacity) packet.text_length = packet.text_capacity - 1;
    if (packet.text_length > 0) packet.text_out[packet.text_length] = '\0';
    else packet.text_out[packet.text_capacity - 1] = '\0';
    gwp89_provider_call(m, GWP89_SERVICE_IO, GWP89_OP_READ_TEXT_FILE, GWP89_PHASE_POST, &packet);
    gwp89_path_stem_to_id(path, default_name, (int)sizeof(default_name));
    return gwp89_load_ini_text_named(m, text, default_name);
}

static void gwp89_default_named(GWP89_WeaponProfile *p,
                                const char *name,
                                int weapon_id,
                                int gun_id,
                                int ammo_id,
                                int projectile_id,
                                int shell_id,
                                int muzzle_id,
                                int casing_id,
                                int trail_id,
                                int projectile_mesh_id,
                                int shell_mesh_id,
                                const char *fire_mode,
                                int clip_size,
                                int ammo_per_shot,
                                int cooldown_ms,
                                int reload_ms,
                                int life_ms,
                                const char *damage,
                                const char *speed,
                                const char *range,
                                int pellets,
                                const char *spread,
                                const char *radius,
                                const char *projectile_scale,
                                const char *shell_scale,
                                const char *muzzle,
                                const char *casing,
                                const char *trail,
                                const char *projectile_mesh,
                                const char *shell_mesh)
{
    gwp89_profile_defaults(p);
    gwp89_copy_id(p->name, GWP89_NAME_MAX, name);
    gwp89_copy_id(p->gun_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(p->ammo_name, GWP89_NAME_MAX, name);
    gwp89_copy_id(p->projectile_name, GWP89_NAME_MAX, projectile_mesh);
    gwp89_copy_id(p->shell_name, GWP89_NAME_MAX, shell_mesh);
    gwp89_copy_id(p->muzzle_name, GWP89_NAME_MAX, muzzle);
    gwp89_copy_id(p->casing_name, GWP89_NAME_MAX, casing);
    gwp89_copy_id(p->trail_name, GWP89_NAME_MAX, trail);
    gwp89_copy_id(p->projectile_mesh_name, GWP89_NAME_MAX, projectile_mesh);
    gwp89_copy_id(p->shell_mesh_name, GWP89_NAME_MAX, shell_mesh);
    p->weapon_id = weapon_id;
    p->gun_id = gun_id;
    p->ammo_id = ammo_id;
    p->projectile_id = projectile_id;
    p->shell_id = shell_id;
    p->muzzle_id = muzzle_id;
    p->casing_id = casing_id;
    p->trail_id = trail_id;
    p->projectile_mesh_id = projectile_mesh_id;
    p->shell_mesh_id = shell_mesh_id;
    p->fire_mode = gwp89_fire_mode_from_name(fire_mode);
    p->clip_size = clip_size;
    p->ammo_per_shot = ammo_per_shot;
    p->cooldown_ms = (unsigned short)cooldown_ms;
    p->reload_ms = (unsigned short)reload_ms;
    p->projectile_life_ms = (unsigned short)life_ms;
    p->damage_fx = gwp89_fx_from_text(damage);
    p->speed_fx = gwp89_fx_from_text(speed);
    p->range_fx = gwp89_fx_from_text(range);
    p->pellet_count = pellets;
    p->spread_fx = gwp89_fx_from_text(spread);
    p->projectile_radius_fx = gwp89_fx_from_text(radius);
    p->projectile_mesh_scale_fx = gwp89_fx_from_text(projectile_scale);
    p->shell_mesh_scale_fx = gwp89_fx_from_text(shell_scale);
}

int gwp89_load_default_profiles(GWP89_Manager *m)
{
    GWP89_WeaponProfile p;
    int added;
    if (!m) return GWP89_BAD_ARG;
    added = 0;

    gwp89_default_named(&p, "pistol", 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                        "semi", 15, 1, 160, 700, 2600,
                        "10.0", "34.0", "72.0", 1, "0.0", "0.12", "0.34", "0.30",
                        "pistol", "pistol", "bullet", "pistol_projectile", "pistol_shell");
    if (gwp89_add_weapon(m, &p) >= 0) added++;

    gwp89_default_named(&p, "machine_gun", 2, 2, 1, 2, 2, 2, 2, 2, 2, 2,
                        "auto", 40, 1, 75, 900, 2100,
                        "7.0", "46.0", "90.0", 1, "0.5", "0.11", "0.32", "0.28",
                        "machine_gun", "machine_gun", "beam", "machinegun_projectile", "machinegun_shell");
    if (gwp89_add_weapon(m, &p) >= 0) added++;

    gwp89_default_named(&p, "shotgun", 3, 3, 2, 3, 3, 3, 3, 1, 3, 3,
                        "hold_once", 7, 1, 580, 1100, 2800,
                        "35.0", "24.0", "28.0", 7, "7.0", "0.28", "0.30", "0.34",
                        "shotgun", "shotgun", "bullet", "shotgun_pellet", "shotgun_shell");
    if (gwp89_add_weapon(m, &p) >= 0) added++;

    gwp89_default_named(&p, "magnum", 4, 4, 3, 4, 4, 4, 4, 1, 4, 4,
                        "semi", 6, 1, 420, 1050, 2400,
                        "26.0", "42.0", "88.0", 1, "0.0", "0.15", "0.36", "0.32",
                        "magnum", "magnum", "bullet", "magnum_projectile", "magnum_shell");
    if (gwp89_add_weapon(m, &p) >= 0) added++;

    gwp89_default_named(&p, "sniper", 5, 5, 4, 5, 5, 5, 5, 2, 5, 5,
                        "semi", 5, 1, 950, 1350, 3100,
                        "38.0", "78.0", "210.0", 1, "0.0", "0.09", "0.34", "0.26",
                        "sniper", "sniper", "beam", "sniper_projectile", "sniper_shell");
    if (gwp89_add_weapon(m, &p) >= 0) added++;

    gwp89_status_begin(m, "gweapon89:defaults");
    gwp89_status_key_int(m, "added", added);
    gwp89_status_key_int(m, "weapons", m->weapon_count);
    return added;
}

