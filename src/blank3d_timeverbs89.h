#ifndef BLANK3D_TIMEVERBS89_H
#define BLANK3D_TIMEVERBS89_H

#include "blank3d_time89.h"
#include "gameverbs89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DTimeVerbs89 {
    Blank3DTime89 *time_value;
    gverb89_registry *registry;
    int registered_actions;
    int registered_conditions;
} Blank3DTimeVerbs89;

int blank3d_timeverbs89_init(Blank3DTimeVerbs89 *bridge,
                             Blank3DTime89 *time_value,
                             gverb89_registry *registry);
int blank3d_timeverbs89_execute(Blank3DTimeVerbs89 *bridge,
                                const char *name,
                                long value_q16,
                                const char *value_text,
                                const char **argv,
                                int argc);
int blank3d_timeverbs89_query(Blank3DTimeVerbs89 *bridge,
                              const char *name,
                              long value_q16,
                              const char *value_text,
                              const char **argv,
                              int argc,
                              int *out_truth);
/* Dispatch pending TimeClocker alarm events through the shared
   GameVerbs89 action bus. Returns the number of actions dispatched, or -1
   if at least one pending alarm could not be dispatched. */
int blank3d_timeverbs89_pump_alarms(Blank3DTimeVerbs89 *bridge);

#ifdef __cplusplus
}
#endif
#endif
