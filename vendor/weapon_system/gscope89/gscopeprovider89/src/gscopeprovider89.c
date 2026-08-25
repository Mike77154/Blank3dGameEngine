#include "../include/gscopeprovider89.h"
#include <string.h>

static void gpr89_copy(char *dst, int cap, const char *src)
{
    int i;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    i = 0;
    while (i + 1 < cap && src[i]) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

const char *gpr89_domain_name(short domain)
{
    static const char *names[GPR89_DOMAIN_COUNT] = {
        "recipe", "preset", "vector", "primitive", "raster",
        "bars", "paint", "hud", "telemetry", "asset", "zoom", "animation"
    };
    if (domain < 0 || domain >= GPR89_DOMAIN_COUNT) return "";
    return names[domain];
}

void gpr89_provider_init(gpr89_provider *provider, const char *name)
{
    if (!provider) return;
    memset(provider, 0, sizeof(*provider));
    provider->abi_version = GPR89_ABI_VERSION;
    gpr89_copy(provider->name, GPR89_MAX_NAME, name);
}

void gpr89_hub_init(gpr89_hub *hub)
{
    short i;
    if (!hub) return;
    memset(hub, 0, sizeof(*hub));
    for (i = 0; i < GPR89_DOMAIN_COUNT; ++i) {
        hub->routes[i].mode = GPR89_MODE_INTERNAL;
        hub->routes[i].provider_name[0] = '\0';
    }
}

int gpr89_hub_register(gpr89_hub *hub, const gpr89_provider *provider)
{
    short i;
    if (!hub || !provider) return 0;
    if (provider->abi_version != GPR89_ABI_VERSION) {
        hub->last_error = GPR89_ERR_ABI;
        return 0;
    }
    if (!provider->name[0]) {
        hub->last_error = GPR89_ERR_NAME;
        return 0;
    }
    for (i = 0; i < hub->provider_count; ++i) {
        if (!strcmp(hub->providers[i].name, provider->name)) {
            hub->providers[i] = *provider;
            return 1;
        }
    }
    if (hub->provider_count >= GPR89_MAX_PROVIDERS) {
        hub->last_error = GPR89_ERR_FULL;
        return 0;
    }
    hub->providers[hub->provider_count] = *provider;
    ++hub->provider_count;
    return 1;
}

const gpr89_provider *gpr89_hub_find(const gpr89_hub *hub, const char *name)
{
    short i;
    if (!hub || !name || !name[0]) return 0;
    for (i = 0; i < hub->provider_count; ++i) {
        if (!strcmp(hub->providers[i].name, name)) return &hub->providers[i];
    }
    return 0;
}

void gpr89_hub_set_route(gpr89_hub *hub, short domain, short mode,
                         const char *provider_name)
{
    if (!hub || domain < 0 || domain >= GPR89_DOMAIN_COUNT) return;
    if (mode < GPR89_MODE_INTERNAL || mode > GPR89_MODE_EXTERNAL)
        mode = GPR89_MODE_INTERNAL;
    hub->routes[domain].mode = mode;
    if (mode == GPR89_MODE_INTERNAL) hub->routes[domain].provider_name[0] = '\0';
    else gpr89_copy(hub->routes[domain].provider_name,
                    GPR89_MAX_NAME, provider_name);
}

static short gpr89_domain_from_key(const char *key)
{
    short i;
    if (!key) return -1;
    for (i = 0; i < GPR89_DOMAIN_COUNT; ++i) {
        if (!strcmp(key, gpr89_domain_name(i))) return i;
    }
    return -1;
}

static int gpr89_parse_route(const char *value, short *out_mode,
                             char *out_name)
{
    const char *colon;
    if (!value || !out_mode || !out_name) return 0;
    if (!strcmp(value, "internal")) {
        *out_mode = GPR89_MODE_INTERNAL;
        out_name[0] = '\0';
        return 1;
    }
    colon = strchr(value, ':');
    if (colon) {
        int n;
        n = (int)(colon - value);
        if (n == 4 && !strncmp(value, "auto", 4))
            *out_mode = GPR89_MODE_AUTO;
        else if (n == 8 && !strncmp(value, "external", 8))
            *out_mode = GPR89_MODE_EXTERNAL;
        else return 0;
        gpr89_copy(out_name, GPR89_MAX_NAME, colon + 1);
        return out_name[0] ? 1 : 0;
    }
    /* Bare provider name means auto:<name>. */
    *out_mode = GPR89_MODE_AUTO;
    gpr89_copy(out_name, GPR89_MAX_NAME, value);
    return out_name[0] ? 1 : 0;
}

int gpr89_hub_apply_recipe(gpr89_hub *hub, const gri89_doc *doc)
{
    short i;
    if (!hub || !doc) return 0;
    for (i = 0; i < doc->count; ++i) {
        short domain;
        short mode;
        char name[GPR89_MAX_NAME];
        if (strcmp(doc->entries[i].section, "providers")) continue;
        domain = gpr89_domain_from_key(doc->entries[i].key);
        if (domain < 0) continue;
        if (!gpr89_parse_route(doc->entries[i].value, &mode, name)) {
            hub->last_error = GPR89_ERR_ROUTE;
            return 0;
        }
        gpr89_hub_set_route(hub, domain, mode, name);
    }
    return 1;
}

const gpr89_provider *gpr89_hub_provider_for(const gpr89_hub *hub,
                                              short domain,
                                              short *out_mode)
{
    const gpr89_route *route;
    if (out_mode) *out_mode = GPR89_MODE_INTERNAL;
    if (!hub || domain < 0 || domain >= GPR89_DOMAIN_COUNT) return 0;
    route = &hub->routes[domain];
    if (out_mode) *out_mode = route->mode;
    if (route->mode == GPR89_MODE_INTERNAL) return 0;
    return gpr89_hub_find(hub, route->provider_name);
}
