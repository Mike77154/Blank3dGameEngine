#include "rpyl_builtins.h"
#include "rpyl_common.h"

static const RpylBuiltinInfo g_builtins[] = {
    { "set", RPYL_BUILTIN_SET, 2, -1, RPYL_BUILTIN_FLAG_STATE | RPYL_BUILTIN_FLAG_ORCHESTRATION },
    { "setg", RPYL_BUILTIN_SET_GLOBAL, 2, -1, RPYL_BUILTIN_FLAG_STATE | RPYL_BUILTIN_FLAG_ORCHESTRATION },
    { "global", RPYL_BUILTIN_SET_GLOBAL, 2, -1, RPYL_BUILTIN_FLAG_STATE | RPYL_BUILTIN_FLAG_ORCHESTRATION },
    { "jump", RPYL_BUILTIN_JUMP, 1, -1, RPYL_BUILTIN_FLAG_CONTROL | RPYL_BUILTIN_FLAG_ORCHESTRATION },
    { "goto", RPYL_BUILTIN_JUMP, 1, -1, RPYL_BUILTIN_FLAG_CONTROL | RPYL_BUILTIN_FLAG_ORCHESTRATION },
    { "call", RPYL_BUILTIN_CALL, 1, -1, RPYL_BUILTIN_FLAG_CONTROL | RPYL_BUILTIN_FLAG_ORCHESTRATION },
    { "return", RPYL_BUILTIN_RETURN, 0, 0, RPYL_BUILTIN_FLAG_CONTROL | RPYL_BUILTIN_FLAG_ORCHESTRATION },
    { "yield", RPYL_BUILTIN_YIELD, 0, 1, RPYL_BUILTIN_FLAG_CONTROL | RPYL_BUILTIN_FLAG_ORCHESTRATION },
    { "pause", RPYL_BUILTIN_PAUSE, 0, 1, RPYL_BUILTIN_FLAG_CONTROL | RPYL_BUILTIN_FLAG_ORCHESTRATION },
    { "say", RPYL_BUILTIN_SAY, 1, 2, RPYL_BUILTIN_FLAG_TEXT },
    { "scene", RPYL_BUILTIN_SCENE, 1, -1, RPYL_BUILTIN_FLAG_VISUAL },
    { "show", RPYL_BUILTIN_SHOW, 1, -1, RPYL_BUILTIN_FLAG_VISUAL },
    { "hide", RPYL_BUILTIN_HIDE, 1, -1, RPYL_BUILTIN_FLAG_VISUAL },
    { "with", RPYL_BUILTIN_WITH, 1, -1, RPYL_BUILTIN_FLAG_VISUAL },
    { "play", RPYL_BUILTIN_PLAY, 1, -1, RPYL_BUILTIN_FLAG_AUDIO },
    { "stop", RPYL_BUILTIN_STOP, 1, -1, RPYL_BUILTIN_FLAG_AUDIO },
    { "queue", RPYL_BUILTIN_QUEUE, 1, -1, RPYL_BUILTIN_FLAG_AUDIO },
    { "window", RPYL_BUILTIN_WINDOW, 1, 1, RPYL_BUILTIN_FLAG_VISUAL },
    { "menu", RPYL_BUILTIN_MENU, 0, -1, RPYL_BUILTIN_FLAG_TEXT | RPYL_BUILTIN_FLAG_CONTROL }
};

int rpyl_builtins_count(void) {
    return (int)(sizeof(g_builtins) / sizeof(g_builtins[0]));
}

const RpylBuiltinInfo* rpyl_builtins_at(size_t index) {
    if (index >= sizeof(g_builtins) / sizeof(g_builtins[0])) return 0;
    return &g_builtins[index];
}

const RpylBuiltinInfo* rpyl_builtins_find(const char* name) {
    size_t i;
    if (!name) return 0;
    for (i = 0u; i < sizeof(g_builtins) / sizeof(g_builtins[0]); i++) {
        if (rpyl_common_streq(g_builtins[i].name, name)) return &g_builtins[i];
    }
    return 0;
}

RpylBuiltinKind rpyl_builtins_kind(const char* name) {
    const RpylBuiltinInfo* info;
    info = rpyl_builtins_find(name);
    if (!info) return RPYL_BUILTIN_UNKNOWN;
    return info->kind;
}

int rpyl_builtins_validate_arity(const char* name, int argc) {
    const RpylBuiltinInfo* info;
    info = rpyl_builtins_find(name);
    if (!info) return 1;
    if (argc < info->min_args) return 0;
    if (info->max_args >= 0 && argc > info->max_args) return 0;
    return 1;
}

int rpyl_builtins_is_control(const char* name) {
    const RpylBuiltinInfo* info;
    info = rpyl_builtins_find(name);
    if (!info) return 0;
    return (info->flags & RPYL_BUILTIN_FLAG_CONTROL) ? 1 : 0;
}

int rpyl_builtins_is_orchestration(const char* name) {
    const RpylBuiltinInfo* info;
    info = rpyl_builtins_find(name);
    if (!info) return 0;
    return (info->flags & RPYL_BUILTIN_FLAG_ORCHESTRATION) ? 1 : 0;
}

int rpyl_builtins_is_renpyish(const char* name) {
    const RpylBuiltinInfo* info;
    info = rpyl_builtins_find(name);
    if (!info) return 0;
    if (info->flags & (RPYL_BUILTIN_FLAG_TEXT | RPYL_BUILTIN_FLAG_VISUAL | RPYL_BUILTIN_FLAG_AUDIO)) return 1;
    return rpyl_builtins_is_orchestration(name);
}
