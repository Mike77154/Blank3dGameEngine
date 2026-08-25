#include "blank3d_classes.h"
#include "blank3d_objects.h"
#include <stdio.h>
#include <string.h>

static int render_calls;

static void test_draw_mesh(void *user, unsigned long entity_id,
                           void *native_entity,
                           const Blank3DObjectInit *init)
{
    (void)user;
    (void)entity_id;
    (void)native_entity;
    if (init && init->mesh_kind == B3D_MESH_CUSTOMCALLED) ++render_calls;
}

int main(void)
{
    static Blank3DClassSystem classes;
    static Blank3DObjects objects;
    Blank3DObjectHost host;
    unsigned int weapon_slot;
    unsigned int ammo_slot;
    const Blank3DObjectDefinition *weapon_def;
    const Blank3DObjectDefinition *ammo_def;

    memset(&host, 0, sizeof(host));
    host.draw_mesh = test_draw_mesh;
    render_calls = 0;
    if (!blank3d_classes_init(&classes, 0)) return 1;
    blank3d_objects_init(&objects, &host, &classes);

    if (!blank3d_objects_spawn_ex(&objects,
            "config/pickups/uzi_weapon.ini", 5000UL, 201UL, 0,
            &weapon_slot)) {
        printf("weapon GFO spawn failed: %s\n", blank3d_objects_status(&objects));
        return 2;
    }
    if (!blank3d_objects_spawn_ex(&objects,
            "config/pickups/uzi_ammo.ini", 5001UL, 202UL, 0,
            &ammo_slot)) {
        printf("ammo GFO spawn failed: %s\n", blank3d_objects_status(&objects));
        return 3;
    }

    weapon_def = blank3d_objects_definition(&objects,
        blank3d_objects_get(&objects, weapon_slot)->definition_slot);
    ammo_def = blank3d_objects_definition(&objects,
        blank3d_objects_get(&objects, ammo_slot)->definition_slot);
    if (!weapon_def || !ammo_def) return 4;
    if (weapon_def->init.mesh_kind != B3D_MESH_CUSTOMCALLED) return 5;
    if (ammo_def->init.mesh_kind != B3D_MESH_CUSTOMCALLED) return 6;
    if (strcmp(weapon_def->init.gfo_path, "objects/pickup_item.gfo") != 0)
        return 7;
    if (strcmp(ammo_def->init.gfo_path, "objects/pickup_item.gfo") != 0)
        return 8;

    blank3d_objects_render(&objects);
    if (render_calls != 2) return 9;

    blank3d_objects_kill(&objects, weapon_slot);
    blank3d_objects_kill(&objects, ammo_slot);
    puts("uzi pickup GFO/Object INI spine: PASS");
    return 0;
}
