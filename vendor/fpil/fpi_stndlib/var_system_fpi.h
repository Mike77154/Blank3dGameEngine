#ifndef VAR_SYSTEM_FPI_H
#define VAR_SYSTEM_FPI_H

#include "value_resolver_fpi.h"

#ifndef FPI_STD_VARS
#define FPI_STD_VARS 100
#endif
#ifndef FPI_MAX_NAMED_VARS
#define FPI_MAX_NAMED_VARS 256
#endif
#ifndef FPI_VAR_NAME_MAX
#define FPI_VAR_NAME_MAX 64
#endif

typedef struct FPI_NamedVar {
    int used;
    char name[FPI_VAR_NAME_MAX];
    FPI_Fixed value;
} FPI_NamedVar;

typedef struct FPI_VarRef {
    int is_named;
    int scope;
    int index;
} FPI_VarRef;

typedef struct FPI_VarSystem {
    FPI_Context* context;
    FPI_Fixed gvars[FPI_STD_VARS];
    FPI_Fixed lvars[FPI_STD_VARS];
    FPI_NamedVar g_named[FPI_MAX_NAMED_VARS];
    FPI_NamedVar l_named[FPI_MAX_NAMED_VARS];
    FPI_VarRef current;
    int reset_globals_on_reload;
    FPI_U32 random_state;
    FPI_ValueResolver resolver;

    int ID_VAREQUAL;
    int ID_VARNOTEQUAL;
    int ID_VARGREATER;
    int ID_VARLESS;
    int ID_GLOBALVAR;
    int ID_GLOBVAR;
    int ID_LOCALVAR;
    int ID_LOCVAR;
    int ID_SETVAR;
    int ID_INCVAR;
    int ID_DECVAR;
    int ID_ADDVAR;
    int ID_SUBVAR;
    int ID_MULVAR;
    int ID_DIVVAR;
    int ID_MODVAR;
    int ID_WRAPVAR;
    int ID_SETVARRND;
    int ID_SIN;
    int ID_COS;
    int ID_DIMVAR;
    int ID_DIMLOCALVAR;
    int ID_RESETGLOBALSONRELOAD;
} FPI_VarSystem;

void fpi_var_system_init(FPI_VarSystem* vars, FPI_Context* context);
void fpi_var_system_init2(FPI_VarSystem* vars, FPI_Context* context, FPI_InternalVarFn internal_fn, void* internal_user);
void fpi_var_system_reset_globals(FPI_VarSystem* vars);
int fpi_eval_var_condition(FPI_VarSystem* vars, int condition_id, const FPI_Value* value);
int fpi_exec_var_action(FPI_VarSystem* vars, int action_id, const FPI_Value* value);
int fpi_var_try_get(FPI_VarSystem* vars, const char* name, FPI_Fixed* out_value);
int fpi_var_try_set(FPI_VarSystem* vars, const char* name, FPI_Fixed value);
void fpi_var_seed(FPI_VarSystem* vars, FPI_U32 seed);

#endif /* VAR_SYSTEM_FPI_H */
