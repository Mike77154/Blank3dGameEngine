#ifndef RPYL_BUILTINS_H
#define RPYL_BUILTINS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RpylBuiltinKind {
    RPYL_BUILTIN_UNKNOWN = 0,
    RPYL_BUILTIN_SET,
    RPYL_BUILTIN_SET_GLOBAL,
    RPYL_BUILTIN_JUMP,
    RPYL_BUILTIN_CALL,
    RPYL_BUILTIN_RETURN,
    RPYL_BUILTIN_YIELD,
    RPYL_BUILTIN_PAUSE,
    RPYL_BUILTIN_SAY,
    RPYL_BUILTIN_SCENE,
    RPYL_BUILTIN_SHOW,
    RPYL_BUILTIN_HIDE,
    RPYL_BUILTIN_WITH,
    RPYL_BUILTIN_PLAY,
    RPYL_BUILTIN_STOP,
    RPYL_BUILTIN_QUEUE,
    RPYL_BUILTIN_WINDOW,
    RPYL_BUILTIN_MENU
} RpylBuiltinKind;

#define RPYL_BUILTIN_FLAG_CONTROL       1UL
#define RPYL_BUILTIN_FLAG_STATE         2UL
#define RPYL_BUILTIN_FLAG_VISUAL        4UL
#define RPYL_BUILTIN_FLAG_AUDIO         8UL
#define RPYL_BUILTIN_FLAG_TEXT          16UL
#define RPYL_BUILTIN_FLAG_ORCHESTRATION 32UL

typedef struct RpylBuiltinInfo {
    const char* name;
    RpylBuiltinKind kind;
    int min_args;
    int max_args;
    unsigned long flags;
} RpylBuiltinInfo;

int rpyl_builtins_count(void);
const RpylBuiltinInfo* rpyl_builtins_at(size_t index);
const RpylBuiltinInfo* rpyl_builtins_find(const char* name);
RpylBuiltinKind rpyl_builtins_kind(const char* name);
int rpyl_builtins_validate_arity(const char* name, int argc);
int rpyl_builtins_is_control(const char* name);
int rpyl_builtins_is_orchestration(const char* name);
int rpyl_builtins_is_renpyish(const char* name);

#ifdef __cplusplus
}
#endif

#endif
