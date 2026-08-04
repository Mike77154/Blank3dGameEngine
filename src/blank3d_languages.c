#include "blank3d_languages.h"
#include "blank3d_ddsl_input.h"

#include "compiler/compiler.h"
#include "VM/vm.h"
#include "store/store.h"
#include "value/value.h"
#include "fpi_api.h"
#include "rpyl.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define B3D_LANG_SOURCE_CAP 32768U
#define B3D_LANG_PREPROCESSED_CAP 49152U
#define B3D_DDSL_ARENA_CAP 196608U
#define B3D_RPYL_CONTEXT_CAP 65536U
#define B3D_RPYL_WORK_CAP 262144U
#define B3D_FPIL_MAX_SCRIPTS 16U
#define B3D_FPIL_PATH_CAP 160U

static Blank3DLanguageHost b3d_host;
static char b3d_status[192];

static char b3d_ddsl_raw_source[2][B3D_LANG_SOURCE_CAP];
static char b3d_ddsl_source[2][B3D_LANG_PREPROCESSED_CAP];
static Blank3DDdslInputRegistry b3d_ddsl_inputs[2];
static unsigned char b3d_ddsl_memory[2][B3D_DDSL_ARENA_CAP];
static ddsl_arena b3d_ddsl_arena[2];
static ddsl_bc_program *b3d_ddsl_program[2];
static int b3d_ddsl_active = -1;
static ddsl_store b3d_ddsl_store;
static ddsl_vm b3d_ddsl_vm;

typedef struct B3DFpilSlotTag {
    int used;
    int loaded;
    char path[B3D_FPIL_PATH_CAP];
    char file_buffer[B3D_LANG_SOURCE_CAP];
    FPI_Context context;
} B3DFpilSlot;

static B3DFpilSlot b3d_fpil_slots[B3D_FPIL_MAX_SCRIPTS];
static FPI_Context *b3d_fpil_active;
static int b3d_fpil_default_slot;

static unsigned char b3d_rpyl_context_memory[B3D_RPYL_CONTEXT_CAP];
static unsigned char b3d_rpyl_work_memory[B3D_RPYL_WORK_CAP];
static RpylContext *b3d_rpyl;

static void b3d_lang_status(const char *text)
{
    if (!text) text = "";
    strncpy(b3d_status, text, sizeof(b3d_status) - 1U);
    b3d_status[sizeof(b3d_status) - 1U] = '\0';
}

static int b3d_read_file(const char *path, char *buffer, unsigned int capacity)
{
    FILE *file;
    size_t count;
    int extra;
    if (!path || !buffer || capacity < 2U) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    count = fread(buffer, 1U, (size_t)(capacity - 1U), file);
    extra = fgetc(file);
    fclose(file);
    if (extra != EOF) return 0;
    buffer[count] = '\0';
    return 1;
}

static int b3d_ddsl_emit(void *user, const char *key, ddsl_value value)
{
    char text[128];
    long fixed_value;
    (void)user;
    text[0] = '\0';
    ddsl_value_to_cstr(value, text, (int)sizeof(text));
    fixed_value = 0L;
    if (value.kind == DDSL_VAL_NUM) fixed_value = (long)value.num;
    else if (value.kind == DDSL_VAL_BOOL)
        fixed_value = value.boolean ? DDSL_FIXED_ONE : DDSL_FIXED_ZERO;
    if (b3d_host.ddsl_action)
        b3d_host.ddsl_action(b3d_host.user, key, fixed_value, text);
    return 1;
}

static int b3d_ddsl_query_input(const char *state_name,
                                const char *control_name)
{
    if (b3d_host.input_query)
        return b3d_host.input_query(b3d_host.user, state_name, control_name);
    if (b3d_host.key_down &&
        (strcmp(state_name, "hold") == 0 || strcmp(state_name, "down") == 0)) {
        char legacy_name[B3D_DDSL_INPUT_CONTROL_CAP];
        unsigned int i;
        for (i = 0U; control_name[i] != '\0' &&
             i + 1U < (unsigned int)sizeof(legacy_name); ++i) {
            unsigned char c;
            c = (unsigned char)control_name[i];
            legacy_name[i] = c == '-' ? '_' : (char)toupper(c);
        }
        legacy_name[i] = '\0';
        return b3d_host.key_down(b3d_host.user, legacy_name);
    }
    return 0;
}

void blank3d_languages_init(const Blank3DLanguageHost *host)
{
    FPI_RunOptions options;
    memset(&b3d_host, 0, sizeof(b3d_host));
    if (host) b3d_host = *host;
    b3d_status[0] = '\0';
    b3d_ddsl_active = -1;
    b3d_ddsl_program[0] = 0;
    b3d_ddsl_program[1] = 0;
    blank3d_ddsl_input_registry_init(&b3d_ddsl_inputs[0]);
    blank3d_ddsl_input_registry_init(&b3d_ddsl_inputs[1]);
    ddsl_store_init(&b3d_ddsl_store);
    ddsl_vm_init(&b3d_ddsl_vm, &b3d_ddsl_store);
    ddsl_vm_set_emit(&b3d_ddsl_vm, b3d_ddsl_emit, 0);

    {
        unsigned int i;
        for (i = 0U; i < B3D_FPIL_MAX_SCRIPTS; ++i) {
            memset(&b3d_fpil_slots[i], 0, sizeof(b3d_fpil_slots[i]));
            fpi_context_init(&b3d_fpil_slots[i].context);
            options.stop_on_first_match = 1;
            options.exec_mode = FPI_EXEC_SEQUENTIAL_IMMEDIATE;
            fpi_set_run_options(&b3d_fpil_slots[i].context, &options);
        }
    }
    b3d_fpil_active = 0;
    b3d_fpil_default_slot = -1;

    b3d_rpyl = 0;
    if (rpyl_context_size() <= (size_t)B3D_RPYL_CONTEXT_CAP) {
        b3d_rpyl = rpyl_init(b3d_rpyl_context_memory,
                             (size_t)B3D_RPYL_CONTEXT_CAP,
                             b3d_rpyl_work_memory,
                             (size_t)B3D_RPYL_WORK_CAP);
    }
    b3d_lang_status("DDSL2 + FPIL + RPYL vendor runtimes initialized");
}

int blank3d_languages_reload_ddsl2(const char *path)
{
    int target;
    ddsl_error error;
    ddsl_bc_program *program;
    target = b3d_ddsl_active == 0 ? 1 : 0;
    if (!b3d_read_file(path, b3d_ddsl_raw_source[target], B3D_LANG_SOURCE_CAP)) {
        b3d_lang_status("DDSL2: could not read script");
        return 0;
    }
    {
        char input_error[160];
        if (!blank3d_ddsl_input_preprocess(
                b3d_ddsl_raw_source[target], b3d_ddsl_source[target],
                B3D_LANG_PREPROCESSED_CAP, &b3d_ddsl_inputs[target],
                input_error, (unsigned int)sizeof(input_error))) {
            char message[192];
            sprintf(message, "DDSL2 input syntax: %.150s", input_error);
            b3d_lang_status(message);
            return 0;
        }
    }
    ddsl_arena_init(&b3d_ddsl_arena[target], b3d_ddsl_memory[target],
                    (size_t)B3D_DDSL_ARENA_CAP);
    program = 0;
    if (!ddsl_compiler_source_to_bytecode(&b3d_ddsl_arena[target],
                                           b3d_ddsl_source[target],
                                           &program, &error)) {
        char message[192];
        sprintf(message, "DDSL2 error %d:%d: %.140s",
                error.line, error.col, error.message);
        b3d_lang_status(message);
        return 0;
    }
    b3d_ddsl_program[target] = program;
    b3d_ddsl_active = target;
    b3d_lang_status("DDSL2 compiled to vendor bytecode");
    return 1;
}

int blank3d_languages_tick_ddsl2(void)
{
    ddsl_error error;
    int i;
    Blank3DDdslInputRegistry *inputs;
    if (b3d_ddsl_active < 0 || !b3d_ddsl_program[b3d_ddsl_active]) return 0;
    inputs = &b3d_ddsl_inputs[b3d_ddsl_active];
    for (i = 0; i < inputs->count; ++i) {
        int active;
        active = b3d_ddsl_query_input(inputs->refs[i].state,
                                      inputs->refs[i].control);
        (void)ddsl_store_set_num(&b3d_ddsl_store,
                                 inputs->refs[i].store_name,
                                 active ? DDSL_FIXED_ONE : DDSL_FIXED_ZERO);
    }
    if (!ddsl_vm_run(&b3d_ddsl_vm,
                     b3d_ddsl_program[b3d_ddsl_active], &error)) {
        char message[192];
        sprintf(message, "DDSL2 VM error: %.160s", error.message);
        b3d_lang_status(message);
        return 0;
    }
    return 1;
}

static void b3d_fpi_value(const FPI_Value *value,
                          long *out_fixed, const char **out_text,
                          int *out_has)
{
    if (out_fixed) *out_fixed = 0L;
    if (out_text) *out_text = "";
    if (out_has) *out_has = 0;
    if (!value || !value->has) return;
    if (out_has) *out_has = 1;
    if (value->kind == FPI_VALUE_FIXED) {
        if (out_fixed) *out_fixed = (long)value->fixed;
    } else if (value->kind == FPI_VALUE_TEXT) {
        if (out_text) *out_text = value->s ? value->s : "";
    }
}

static int b3d_fpi_eval(void *entity, int condition_id,
                        const FPI_Value *value)
{
    const char *name;
    const char *text;
    long fixed_value;
    int has_value;
    name = b3d_fpil_active ? fpi_get_cond_name(b3d_fpil_active, condition_id) : 0;
    b3d_fpi_value(value, &fixed_value, &text, &has_value);
    if (!b3d_host.fpil_condition) return 0;
    return b3d_host.fpil_condition(b3d_host.user, entity,
                                   name ? name : "",
                                   fixed_value, text, has_value);
}

static void b3d_fpi_exec(void *entity, int action_id,
                         const FPI_Value *value)
{
    const char *name;
    const char *text;
    long fixed_value;
    int has_value;
    name = b3d_fpil_active ? fpi_get_act_name(b3d_fpil_active, action_id) : 0;
    b3d_fpi_value(value, &fixed_value, &text, &has_value);
    if (b3d_host.fpil_action)
        b3d_host.fpil_action(b3d_host.user, entity,
                             name ? name : "",
                             fixed_value, text, has_value);
}

static int b3d_fpil_find_slot(const char *path)
{
    unsigned int i;
    if (!path || !*path) return -1;
    for (i = 0U; i < B3D_FPIL_MAX_SCRIPTS; ++i)
        if (b3d_fpil_slots[i].used &&
            strcmp(b3d_fpil_slots[i].path, path) == 0)
            return (int)i;
    return -1;
}

static int b3d_fpil_acquire_slot(const char *path)
{
    unsigned int i;
    int found;
    found = b3d_fpil_find_slot(path);
    if (found >= 0) return found;
    for (i = 0U; i < B3D_FPIL_MAX_SCRIPTS; ++i) {
        B3DFpilSlot *slot;
        if (b3d_fpil_slots[i].used) continue;
        slot = &b3d_fpil_slots[i];
        slot->used = 1;
        slot->loaded = 0;
        strncpy(slot->path, path, sizeof(slot->path) - 1U);
        slot->path[sizeof(slot->path) - 1U] = '\0';
        return (int)i;
    }
    return -1;
}

int blank3d_languages_reload_fpil_path(const char *path)
{
    int index;
    int result;
    const FPI_Error *error;
    B3DFpilSlot *slot;
    index = b3d_fpil_acquire_slot(path);
    if (index < 0) {
        b3d_lang_status("FPIL: script context pool exhausted");
        return 0;
    }
    slot = &b3d_fpil_slots[index];
    {
        FPI_RunOptions options;
        fpi_context_init(&slot->context);
        options.stop_on_first_match = 1;
        options.exec_mode = FPI_EXEC_SEQUENTIAL_IMMEDIATE;
        fpi_set_run_options(&slot->context, &options);
    }
    result = fpi_context_load_buffered(&slot->context, path,
                                       slot->file_buffer,
                                       (FPI_U32)sizeof(slot->file_buffer));
    if (result != FPI_OK) {
        char message[192];
        error = fpi_context_error(&slot->context);
        sprintf(message, "FPIL error %d:%d: %s",
                error ? error->span.line : 0,
                error ? error->span.column : 0,
                error ? error->message : "load failed");
        b3d_lang_status(message);
        slot->loaded = 0;
        return 0;
    }
    slot->loaded = 1;
    b3d_lang_status("FPIL script loaded into isolated vendor context");
    return 1;
}

int blank3d_languages_reload_fpil(const char *path)
{
    int result;
    result = blank3d_languages_reload_fpil_path(path);
    if (result) b3d_fpil_default_slot = b3d_fpil_find_slot(path);
    return result;
}

int blank3d_languages_tick_fpil_path(const char *path, void *entity)
{
    int index;
    int fired;
    B3DFpilSlot *slot;
    if (!path || !*path) return 0;
    index = b3d_fpil_find_slot(path);
    if (index < 0 || !b3d_fpil_slots[index].loaded) {
        if (!blank3d_languages_reload_fpil_path(path)) return 0;
        index = b3d_fpil_find_slot(path);
    }
    if (index < 0) return 0;
    slot = &b3d_fpil_slots[index];
    b3d_fpil_active = &slot->context;
    fired = fpi_tick_ex(&slot->context, entity,
                        b3d_fpi_eval, b3d_fpi_exec, 0);
    if (fpi_context_error(&slot->context)->code != FPI_OK) {
        b3d_lang_status(fpi_context_error(&slot->context)->message);
        b3d_fpil_active = 0;
        return 0;
    }
    b3d_fpil_active = 0;
    return fired;
}

int blank3d_languages_tick_fpil(void *entity)
{
    if (b3d_fpil_default_slot < 0 ||
        !b3d_fpil_slots[b3d_fpil_default_slot].loaded)
        return 0;
    return blank3d_languages_tick_fpil_path(
        b3d_fpil_slots[b3d_fpil_default_slot].path, entity);
}

static void b3d_rpyl_dispatch(const char *name,
                              const char **args, int argc)
{
    if (b3d_host.rpyl_command)
        b3d_host.rpyl_command(b3d_host.user, name, args, argc);
}

static void b3d_rpyl_scene(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("scene", args, argc); }
static void b3d_rpyl_window(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("window", args, argc); }
static void b3d_rpyl_camera(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("camera", args, argc); }
static void b3d_rpyl_show_grid(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("show_grid", args, argc); }
static void b3d_rpyl_player(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("player", args, argc); }
static void b3d_rpyl_movement(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("movement", args, argc); }
static void b3d_rpyl_weapon(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("weapon", args, argc); }
static void b3d_rpyl_enemy(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("enemy", args, argc); }
static void b3d_rpyl_zombie(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("zombie", args, argc); }
static void b3d_rpyl_gunner_enemy(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("gunner_enemy", args, argc); }
static void b3d_rpyl_armed_ally(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("armed_ally", args, argc); }
static void b3d_rpyl_ally(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("ally", args, argc); }
static void b3d_rpyl_ally_gunner(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("ally_gunner", args, argc); }
static void b3d_rpyl_hopper_enemy(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("hopper_enemy", args, argc); }
static void b3d_rpyl_dive_enemy(RpylContext *ctx, const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("dive_enemy", args, argc); }
static void b3d_rpyl_dive_bomber_enemy(RpylContext *ctx,
                                        const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("dive_bomber_enemy", args, argc); }
static void b3d_rpyl_air_lunger_enemy(RpylContext *ctx,
                                      const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("air_lunger_enemy", args, argc); }
static void b3d_rpyl_airlunge_enemy(RpylContext *ctx,
                                    const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("airlunge_enemy", args, argc); }
static void b3d_rpyl_midair_lunger_enemy(RpylContext *ctx,
                                         const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("midair_lunger_enemy", args, argc); }
static void b3d_rpyl_ground_lancer_enemy(RpylContext *ctx,
                                         const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("ground_lancer_enemy", args, argc); }
static void b3d_rpyl_groundlance_enemy(RpylContext *ctx,
                                       const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("groundlance_enemy", args, argc); }
static void b3d_rpyl_pegasus_enemy(RpylContext *ctx,
                                   const char **args, int argc)
{ (void)ctx; b3d_rpyl_dispatch("pegasus_enemy", args, argc); }

static void b3d_rpyl_register_commands(void)
{
    rpyl_register_command(b3d_rpyl, "scene", b3d_rpyl_scene);
    rpyl_register_command(b3d_rpyl, "window", b3d_rpyl_window);
    rpyl_register_command(b3d_rpyl, "camera", b3d_rpyl_camera);
    rpyl_register_command(b3d_rpyl, "show_grid", b3d_rpyl_show_grid);
    rpyl_register_command(b3d_rpyl, "player", b3d_rpyl_player);
    rpyl_register_command(b3d_rpyl, "movement", b3d_rpyl_movement);
    rpyl_register_command(b3d_rpyl, "weapon", b3d_rpyl_weapon);
    rpyl_register_command(b3d_rpyl, "enemy", b3d_rpyl_enemy);
    rpyl_register_command(b3d_rpyl, "zombie", b3d_rpyl_zombie);
    rpyl_register_command(b3d_rpyl, "gunner_enemy", b3d_rpyl_gunner_enemy);
    rpyl_register_command(b3d_rpyl, "armed_ally", b3d_rpyl_armed_ally);
    rpyl_register_command(b3d_rpyl, "ally", b3d_rpyl_ally);
    rpyl_register_command(b3d_rpyl, "ally_gunner", b3d_rpyl_ally_gunner);
    rpyl_register_command(b3d_rpyl, "hopper_enemy", b3d_rpyl_hopper_enemy);
    rpyl_register_command(b3d_rpyl, "dive_enemy", b3d_rpyl_dive_enemy);
    rpyl_register_command(b3d_rpyl, "dive_bomber_enemy",
                          b3d_rpyl_dive_bomber_enemy);
    rpyl_register_command(b3d_rpyl, "air_lunger_enemy",
                          b3d_rpyl_air_lunger_enemy);
    rpyl_register_command(b3d_rpyl, "airlunge_enemy",
                          b3d_rpyl_airlunge_enemy);
    rpyl_register_command(b3d_rpyl, "midair_lunger_enemy",
                          b3d_rpyl_midair_lunger_enemy);
    rpyl_register_command(b3d_rpyl, "ground_lancer_enemy",
                          b3d_rpyl_ground_lancer_enemy);
    rpyl_register_command(b3d_rpyl, "groundlance_enemy",
                          b3d_rpyl_groundlance_enemy);
    rpyl_register_command(b3d_rpyl, "pegasus_enemy",
                          b3d_rpyl_pegasus_enemy);
}

int blank3d_languages_run_rpyl(const char *path)
{
    const RpylErrorInfo *error;
    int result;
    if (!b3d_rpyl) {
        b3d_lang_status("RPYL context storage too small");
        return 0;
    }
    rpyl_reset(b3d_rpyl);
    rpyl_set_label_keyword(b3d_rpyl, "label");
    rpyl_set_start_block(b3d_rpyl, "start");
    b3d_rpyl_register_commands();
    if (!rpyl_load_path_buffered(b3d_rpyl, path)) {
        char message[192];
        error = rpyl_get_last_error(b3d_rpyl);
        sprintf(message, "RPYL error %d:%d: %s",
                error ? error->line : 0,
                error ? error->column : 0,
                error ? error->message : "load failed");
        b3d_lang_status(message);
        return 0;
    }
    if (rpyl_compile(b3d_rpyl)) {
        result = rpyl_run_compiled(b3d_rpyl, "start");
        if (result == RPYL_EXEC_ERROR) {
            error = rpyl_get_last_error(b3d_rpyl);
            b3d_lang_status(error ? error->message
                                  : "RPYL bytecode execution failed");
            return 0;
        }
        b3d_lang_status("RPYL compiled and executed through vendor VM");
    } else {
        /* Some valid legacy-style argument forms are accepted by the vendor
           AST runtime but rejected by its optional bytecode validator. Keep
           the vendored parser/runtime authoritative and fall back without
           returning to Blank3D's former hand-written parser. */
        rpyl_clear_error(b3d_rpyl);
        result = rpyl_run(b3d_rpyl, "start");
        if (result == RPYL_EXEC_ERROR) {
            error = rpyl_get_last_error(b3d_rpyl);
            b3d_lang_status(error ? error->message
                                  : "RPYL AST execution failed");
            return 0;
        }
        b3d_lang_status("RPYL executed through vendor AST runtime");
    }
    return 1;
}

const char *blank3d_languages_status(void)
{
    return b3d_status;
}
