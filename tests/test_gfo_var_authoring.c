#include "blank3d_objects.h"
#include "blank3d_variables.h"

#include <stdio.h>
#include <string.h>

typedef struct TestHostTag {
    Blank3DVariables *vars;
} TestHost;

static int host_begin_event(void *user, unsigned long entity_id,
                            unsigned long runtime_key,
                            unsigned long event_id)
{
    TestHost *host;
    (void)entity_id;
    host = (TestHost *)user;
    return host && host->vars &&
           blank3d_variables_begin_event(host->vars, runtime_key, event_id);
}

static void host_end_event(void *user, unsigned long entity_id,
                           unsigned long runtime_key,
                           unsigned long event_id)
{
    TestHost *host;
    (void)entity_id;
    (void)runtime_key;
    (void)event_id;
    host = (TestHost *)user;
    if (host && host->vars) (void)blank3d_variables_end_event(host->vars);
}

static void host_script(void *user, unsigned long entity_id,
                        void *native_entity, const char *language,
                        const char *code, unsigned int code_len)
{
    TestHost *host;
    (void)native_entity;
    host = (TestHost *)user;
    if (!host || !host->vars || !language || !code) return;
    if (strcmp(language, "vars") == 0)
        (void)blank3d_variables_execute_slice(host->vars, entity_id,
                                               code, code_len);
}

int main(void)
{
    static Blank3DSystems systems;
    static Blank3DVariables vars;
    static Blank3DClassSystem classes;
    static Blank3DObjects objects;
    Blank3DObjectHost object_host;
    TestHost host;
    ns_id type_id;
    ns_id value_id;
    ns_fx numeric;
    FlagsValue flag;
    vm89_value value;
    unsigned int slot;

    memset(&systems, 0, sizeof(systems));
    ns_init(&systems.numbers);
    type_id = ns_define_type(&systems.numbers, "player.health",
                             NS_SCOPE_INSTANCE, NS_KIND_INT,
                             NS_FX_FROM_INT(100), NS_FX_ZERO,
                             NS_FX_FROM_INT(100), NS_OVERFLOW_CLAMP,
                             NS_FLAG_SAVE | NS_FLAG_HUD);
    if (type_id < 0) return 1;
    if (ns_attach_type(&systems.numbers, 1, type_id) < 0) return 2;
    flagstore_init(&systems.flags, systems.flag_entries, B3D_FLAG_CAPACITY,
                   systems.flag_pool, B3D_FLAG_POOL_CAPACITY);

    blank3d_variables_init(&vars, &systems);
    blank3d_variables_set_player_owner(&vars, 1UL);
    if (!blank3d_variables_instance_create(&vars, 1UL)) return 3;

    host.vars = &vars;
    memset(&object_host, 0, sizeof(object_host));
    object_host.user = &host;
    object_host.begin_event = host_begin_event;
    object_host.end_event = host_end_event;
    object_host.draw_script = host_script;

    if (!blank3d_classes_init(&classes, 0)) return 4;
    blank3d_objects_init(&objects, &object_host, &classes);
    if (!blank3d_objects_spawn_ex(&objects,
            "tests/fixtures/var_authoring_test.ini",
            1UL, 1UL, 0, &slot)) {
        puts(blank3d_objects_status(&objects));
        return 5;
    }

    value_id = ns_find_value(&systems.numbers, 1, "player.health");
    if (value_id < 0 ||
        ns_get_by_id(&systems.numbers, value_id, &numeric) != NS_OK ||
        numeric != NS_FX_FROM_INT(75)) return 6;
    if (!flagstore_get(&systems.flags, "thing.1.alerted", &flag) ||
        flag.type != FLAGS_VAL_BOOL || flag.as.i == 0L) return 7;
    if (!blank3d_variables_get(&vars, VR89_SCOPE_INSTANCE, 1UL,
                               "foo", &value) ||
        value.type != VM89_VALUE_FIXED ||
        value.fixed_q16 != 5L * 65536L) return 8;
    if (blank3d_variables_get(&vars, VR89_SCOPE_LOCAL, 1UL,
                              "temp", &value)) return 9;

    blank3d_objects_kill(&objects, slot);
    puts("GFO script:vars -> Thing-scoped NumSys/Flags/VarStore: OK");
    return 0;
}
