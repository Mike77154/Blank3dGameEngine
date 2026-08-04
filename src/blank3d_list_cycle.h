#ifndef BLANK3D_LIST_CYCLE_H
#define BLANK3D_LIST_CYCLE_H

#include "cycler89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_LIST_CYCLE_MAX_LISTS 8
#define B3D_LIST_CYCLE_NAME_CAP 64

typedef int (*Blank3DListCurrentFn)(void *context,
                                    cycler89_item *item_out);
typedef int (*Blank3DListActivateFn)(void *context,
                                     cycler89_item item);

typedef struct Blank3DListCycleBindingTag {
    int used;
    char name[B3D_LIST_CYCLE_NAME_CAP];
    Cycler89List list;
    void *context;
    Blank3DListCurrentFn current;
    Blank3DListActivateFn activate;
} Blank3DListCycleBinding;

typedef struct Blank3DListCycleRegistryTag {
    Blank3DListCycleBinding bindings[B3D_LIST_CYCLE_MAX_LISTS];
    int count;
} Blank3DListCycleRegistry;

void blank3d_list_cycle_init(Blank3DListCycleRegistry *registry);

int blank3d_list_cycle_register(Blank3DListCycleRegistry *registry,
                                const char *name,
                                const Cycler89List *list,
                                void *context,
                                Blank3DListCurrentFn current,
                                Blank3DListActivateFn activate);

int blank3d_list_cycle_step(Blank3DListCycleRegistry *registry,
                            const char *name,
                            int direction,
                            cycler89_item *selected_out);

int blank3d_list_cycle_next(Blank3DListCycleRegistry *registry,
                            const char *name,
                            cycler89_item *selected_out);

int blank3d_list_cycle_prev(Blank3DListCycleRegistry *registry,
                            const char *name,
                            cycler89_item *selected_out);

#ifdef __cplusplus
}
#endif

#endif
