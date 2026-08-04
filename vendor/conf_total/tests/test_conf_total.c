#include "../conf_total.h"
#include <stdio.h>

static int g_fails = 0;

static void check_int(const char *name, long got, long want) {
    if (got != want) {
        printf("FAIL %s: got %ld want %ld\n", name, got, want);
        g_fails++;
    }
}

static void check_fixed(const char *name, conf_fixed_t got, conf_fixed_t want) {
    if (got != want) {
        printf("FAIL %s: got %ld want %ld\n", name, (long)got, (long)want);
        g_fails++;
    }
}

static void check_bool(const char *name, int got, int want) {
    if (got != want) {
        printf("FAIL %s: got %d want %d\n", name, got, want);
        g_fails++;
    }
}

static void check_slice_cstr(const char *name, conf_slice_t got, const char *want) {
    unsigned int i = 0u;
    while (want[i] != '\0') i++;
    if (got.len != i) {
        printf("FAIL %s: len got %u want %u\n", name, got.len, i);
        g_fails++;
        return;
    }
    for (i = 0u; i < got.len; ++i) {
        if (got.ptr[i] != want[i]) {
            printf("FAIL %s: byte %u got %d want %d\n", name, i, (int)got.ptr[i], (int)want[i]);
            g_fails++;
            return;
        }
    }
}

static void check_err(const char *name, conf_err_t got) {
    if (got != CONF_OK) {
        printf("FAIL %s: err %d\n", name, (int)got);
        g_fails++;
    }
}

static void test_toml(void) {
    static unsigned char arena[32768];
    conf_ctx_t cfg;
    conf_slice_t def;
    const char toml[] =
        "[video]\n"
        "fullscreen=true\n"
        "width=1280\n"
        "scale=1.5\n"
        "quality=\"ultra\"\n"
        "[db]\n"
        "ports=[8000,8001]\n";

    def.ptr = 0;
    def.len = 0u;
    conf_ctx_init(&cfg, arena, (unsigned int)sizeof(arena));
    check_err("toml load", conf_load_toml(&cfg, toml, (unsigned int)(sizeof(toml) - 1u)));
    check_bool("toml bool", conf_get_bool(&cfg, "video.fullscreen", 0), 1);
    check_int("toml number", conf_get_int(&cfg, "video.width", 0), 1280L);
    check_fixed("toml fixed", conf_get_fixed(&cfg, "video.scale", 0), CONF_FIXED_FROM_INT(1) + (CONF_FIXED_ONE / 2L));
    check_slice_cstr("toml string", conf_get_string(&cfg, "video.quality", def), "ultra");
    check_int("toml array", conf_get_int(&cfg, "db.ports[1]", 0), 8001L);
}

static void test_yaml(void) {
    static unsigned char arena[32768];
    conf_ctx_t cfg;
    const char yaml[] =
        "a: 1\n"
        "b: true\n"
        "c:\n"
        "  nested: hello\n"
        "list:\n"
        "  - 10\n"
        "  - 20\n";

    conf_ctx_init(&cfg, arena, (unsigned int)sizeof(arena));
    check_err("yaml load", conf_load_yaml(&cfg, yaml, (unsigned int)(sizeof(yaml) - 1u)));
    {
        conf_slice_t def;
        def.ptr = 0;
        def.len = 0u;
        check_int("yaml int", conf_get_int(&cfg, "a", 0), 1L);
        check_bool("yaml bool", conf_get_bool(&cfg, "b", 0), 1);
        check_slice_cstr("yaml nested", conf_get_string(&cfg, "c.nested", def), "hello");
        check_int("yaml list", conf_get_int(&cfg, "list[1]", 0), 20L);
    }
}

static void test_ini_and_overrides(void) {
    static unsigned char arena[32768];
    conf_ctx_t cfg;
    const char ini[] =
        "[video]\n"
        "width=320\n"
        "scale: 2.0\n";

    conf_ctx_init(&cfg, arena, (unsigned int)sizeof(arena));
    check_err("ini load", conf_load_ini(&cfg, ini, (unsigned int)(sizeof(ini) - 1u)));
    check_int("ini int", conf_get_int(&cfg, "video.width", 0), 320L);
    check_fixed("ini fixed", conf_get_fixed(&cfg, "video.scale", 0), CONF_FIXED_FROM_INT(2));
    check_err("override fixed", conf_override_fixed(&cfg, "video.scale", CONF_FIXED_FROM_INT(3)));
    check_fixed("override fixed read", conf_get_fixed(&cfg, "video.scale", 0), CONF_FIXED_FROM_INT(3));
    check_err("override scalar fixed", conf_override_scalar_toml(&cfg, "video.gamma", "1.25", 4u));
    check_fixed("override scalar fixed read", conf_get_fixed(&cfg, "video.gamma", 0), CONF_FIXED_FROM_INT(1) + (CONF_FIXED_ONE / 4L));
}

int main(void) {
    test_toml();
    test_yaml();
    test_ini_and_overrides();
    if (g_fails) {
        printf("%d test(s) failed\n", g_fails);
        return 1;
    }
    printf("all conf_total tests passed\n");
    return 0;
}
