#ifndef BLANK3D_VARIABLES_H
#define BLANK3D_VARIABLES_H

#include "var_runtime89.h"
#include "blank3d_systems.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_VAR_SCRIPT_CAP 16384U
#define B3D_VAR_FLAG_KEY_CAP 96U

typedef struct Blank3DVariablesTag {
    vr89_runtime runtime;
    Blank3DSystems *systems;
    unsigned long player_owner;
    char script_buffer[B3D_VAR_SCRIPT_CAP];
    char status[192];
} Blank3DVariables;

void blank3d_variables_init(Blank3DVariables *vars, Blank3DSystems *systems);
void blank3d_variables_reset_instances(Blank3DVariables *vars);
void blank3d_variables_set_player_owner(Blank3DVariables *vars,
                                        unsigned long owner);
int blank3d_variables_instance_create(Blank3DVariables *vars,
                                      unsigned long owner);
int blank3d_variables_instance_destroy(Blank3DVariables *vars,
                                       unsigned long owner);
int blank3d_variables_begin_event(Blank3DVariables *vars,
                                  unsigned long owner,
                                  unsigned long event_id);
int blank3d_variables_end_event(Blank3DVariables *vars);
int blank3d_variables_execute(Blank3DVariables *vars,
                              unsigned long owner,
                              const char *text);
int blank3d_variables_execute_slice(Blank3DVariables *vars,
                                    unsigned long owner,
                                    const char *text,
                                    unsigned int text_len);
int blank3d_variables_set_q16(Blank3DVariables *vars,
                              vr89_scope scope,
                              unsigned long owner,
                              const char *name,
                              long value_q16);
int blank3d_variables_set_bool(Blank3DVariables *vars,
                               vr89_scope scope,
                               unsigned long owner,
                               const char *name,
                               int value);
int blank3d_variables_get(Blank3DVariables *vars,
                          vr89_scope scope,
                          unsigned long owner,
                          const char *name,
                          vm89_value *out_value);
const char *blank3d_variables_status(const Blank3DVariables *vars);

#ifdef __cplusplus
}
#endif

#endif
