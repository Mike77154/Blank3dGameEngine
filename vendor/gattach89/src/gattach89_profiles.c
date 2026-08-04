#include "gattach89_profiles.h"

int gatt89_profile_add_sockets(GAtt89_World *w, int owner_entity_id, const GAtt89_SocketDef *defs, int def_count)
{
    int i;
    int parent_id;
    int last_id;

    if (w == 0 || defs == 0) return GATTACH89_ERR_NULL;
    if (def_count < 0) return GATTACH89_ERR_BAD_ARG;

    last_id = GATTACH89_INVALID_ID;
    for (i = 0; i < def_count; ++i) {
        parent_id = GATTACH89_INVALID_ID;
        if (defs[i].parent_socket_name != 0) {
            parent_id = gatt89_socket_find(w, owner_entity_id, defs[i].parent_socket_name);
            if (parent_id < 0) return parent_id;
        }
        last_id = gatt89_socket_add(w, owner_entity_id, defs[i].name, defs[i].kind, defs[i].bone_index, parent_id, &defs[i].local);
        if (last_id < 0) return last_id;
    }
    return last_id;
}

int gatt89_profile_add_parts(GAtt89_World *w, int owner_entity_id, const GAtt89_PartDef *defs, int def_count)
{
    int i;
    int last_id;

    if (w == 0 || defs == 0) return GATTACH89_ERR_NULL;
    if (def_count < 0) return GATTACH89_ERR_BAD_ARG;

    last_id = GATTACH89_INVALID_ID;
    for (i = 0; i < def_count; ++i) {
        last_id = gatt89_part_add(w, owner_entity_id, defs[i].name, defs[i].visible);
        if (last_id < 0) return last_id;
    }
    return last_id;
}
