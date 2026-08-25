#include "blank3d_classes.h"
#include "blank3d_objects.h"
#include <stdio.h>
#include <string.h>

static cm89_value seen_receiver;
static cm89_class_h seen_owner;

static cm89_result capture(
    void *user, cm89_value callable, cm89_value receiver,
    cm89_class_h owner_class, const cm89_call *call, cm89_value *out_value)
{
    (void)user;
    (void)callable;
    (void)call;
    seen_receiver = receiver;
    seen_owner = owner_class;
    if (out_value) *out_value = cm89_value_uint(1UL);
    return CM89_OK;
}

int main(void)
{
    static Blank3DClassSystem classes;
    static Blank3DObjects objects;
    Blank3DObjectHost host;
    cm89_provider provider;
    unsigned int a;
    unsigned int b;
    cm89_class_h ca;
    cm89_class_h cb;
    cm89_value out;

    memset(&host, 0, sizeof(host));
    provider.user = 0;
    provider.invoke = capture;
    if (!blank3d_classes_init(&classes, &provider)) return 1;
    if (cm89_class_set_member(&classes.manager, classes.object_class,
            "describe", CM89_MEMBER_METHOD,
            cm89_value_host_handle(0xD35CUL), cm89_value_none()) != CM89_OK)
        return 2;
    blank3d_objects_init(&objects, &host, &classes);

    if (!blank3d_objects_spawn_ex(&objects, "config/entities/player.ini",
                                   1UL, 101UL, 0, &a)) return 3;
    if (!blank3d_objects_spawn_ex(&objects, "config/entities/player.ini",
                                   2UL, 102UL, 0, &b)) return 4;
    ca = blank3d_objects_class_of_slot(&objects, a);
    cb = blank3d_objects_class_of_slot(&objects, b);
    if (ca == CM89_CLASS_NONE || ca != cb) return 5;
    if (!blank3d_classes_is_subclass(&classes, ca, classes.object_class)) return 6;
    if (blank3d_objects_definition(&objects,
            blank3d_objects_get(&objects, a)->definition_slot)->class_handle != ca)
        return 7;
    if (blank3d_objects_call_class_method(&objects, a, "describe", 0, &out)
            != CM89_OK) return 8;
    if (seen_receiver.kind != CM89_VALUE_HOST_HANDLE ||
        seen_receiver.data.uint_value != 101UL) return 9;
    if (seen_owner != classes.object_class) return 10;

    if (blank3d_classes_seal(&classes) != CM89_OK) return 11;
    if (!cm89_manager_is_sealed(&classes.manager)) return 12;

    blank3d_objects_kill(&objects, a);
    blank3d_objects_kill(&objects, b);
    puts("Class -> GFO Object -> Thing receiver bridge: OK");
    return 0;
}
