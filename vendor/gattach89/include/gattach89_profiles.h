#ifndef GATTACH89_PROFILES_H
#define GATTACH89_PROFILES_H

#include "gattach89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GAtt89_SocketDef {
    const char *name;
    int kind;
    int bone_index;
    const char *parent_socket_name;
    GAtt89_Xform local;
} GAtt89_SocketDef;

typedef struct GAtt89_PartDef {
    const char *name;
    int visible;
} GAtt89_PartDef;

int gatt89_profile_add_sockets(GAtt89_World *w, int owner_entity_id, const GAtt89_SocketDef *defs, int def_count);
int gatt89_profile_add_parts(GAtt89_World *w, int owner_entity_id, const GAtt89_PartDef *defs, int def_count);

#ifdef __cplusplus
}
#endif

#endif
