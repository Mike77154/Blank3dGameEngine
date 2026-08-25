#include <stdio.h>
#include "cm89.h"

#define FN_ANIMAL_INIT  1UL
#define FN_ANIMAL_SPEAK 2UL
#define FN_DOG_SPEAK    3UL
#define FN_NAMED_LABEL  4UL

static cm89_result demo_invoke(
    void *user,
    cm89_value callable,
    cm89_value receiver,
    cm89_class_h owner_class,
    const cm89_call *call,
    cm89_value *out_value
)
{
    cm89_manager *manager;
    unsigned long fn;
    cm89_instance_h instance;
    cm89_value value;

    manager = (cm89_manager *)user;
    fn = callable.data.uint_value;
    (void)owner_class;
    (void)out_value;

    if (receiver.kind == CM89_VALUE_INSTANCE) {
        instance = (cm89_instance_h)receiver.data.uint_value;
    } else {
        instance = CM89_INSTANCE_NONE;
    }

    if (fn == FN_ANIMAL_INIT) {
        if (call != 0 && call->arg_count > 0) {
            return cm89_instance_set_attr(
                manager, instance, "name", call->args[0]
            );
        }
        return CM89_OK;
    }

    if (fn == FN_ANIMAL_SPEAK) {
        printf("Animal.speak()\n");
        return CM89_OK;
    }

    if (fn == FN_DOG_SPEAK) {
        printf("Dog.speak() -> woof\n");
        return CM89_OK;
    }

    if (fn == FN_NAMED_LABEL) {
        value = cm89_value_none();
        if (cm89_instance_get_attr(manager, instance, "name", &value) == CM89_OK &&
            value.kind == CM89_VALUE_HOST_HANDLE) {
            printf("Named.label(): host-string-handle=%lu\n",
                   value.data.uint_value);
        }
        return CM89_OK;
    }

    return CM89_ERR_NOT_FOUND;
}

int main(void)
{
    static cm89_manager manager;
    cm89_provider provider;
    cm89_class_h object_cls;
    cm89_class_h animal_cls;
    cm89_class_h named_cls;
    cm89_class_h dog_cls;
    cm89_class_h bases[2];
    cm89_instance_h dog;
    cm89_value arg;
    cm89_call init_call;
    const cm89_class_h *mro;
    cm89_count mro_count;
    cm89_count i;
    const char *name;

    provider.user = &manager;
    provider.invoke = demo_invoke;
    cm89_manager_init(&manager, &provider);

    if (cm89_class_create(&manager, "Object", 0, 0, &object_cls) != CM89_OK) {
        return 1;
    }

    bases[0] = object_cls;
    if (cm89_class_create(&manager, "Animal", bases, 1, &animal_cls) != CM89_OK) {
        return 2;
    }
    if (cm89_class_create(&manager, "Named", bases, 1, &named_cls) != CM89_OK) {
        return 3;
    }

    cm89_class_set_member(
        &manager, animal_cls, "__init__", CM89_MEMBER_METHOD,
        cm89_value_host_handle(FN_ANIMAL_INIT), cm89_value_none()
    );
    cm89_class_set_member(
        &manager, animal_cls, "speak", CM89_MEMBER_METHOD,
        cm89_value_host_handle(FN_ANIMAL_SPEAK), cm89_value_none()
    );
    cm89_class_set_member(
        &manager, named_cls, "label", CM89_MEMBER_METHOD,
        cm89_value_host_handle(FN_NAMED_LABEL), cm89_value_none()
    );

    bases[0] = animal_cls;
    bases[1] = named_cls;
    if (cm89_class_create(&manager, "Dog", bases, 2, &dog_cls) != CM89_OK) {
        return 4;
    }
    cm89_class_set_member(
        &manager, dog_cls, "speak", CM89_MEMBER_METHOD,
        cm89_value_host_handle(FN_DOG_SPEAK), cm89_value_none()
    );

    arg = cm89_value_host_handle(9001UL);
    init_call.args = &arg;
    init_call.arg_count = 1;
    init_call.kwargs_handle = cm89_value_none();

    if (cm89_instance_new(&manager, dog_cls, &init_call, &dog) != CM89_OK) {
        return 5;
    }

    cm89_call_method(&manager, dog, "speak", 0, 0);
    cm89_call_super(&manager, dog, dog_cls, "speak", 0, 0);
    cm89_call_method(&manager, dog, "label", 0, 0);

    if (cm89_class_get_mro(&manager, dog_cls, &mro, &mro_count) == CM89_OK) {
        printf("Dog MRO:");
        for (i = 0; i < mro_count; ++i) {
            if (cm89_class_get_name(&manager, mro[i], &name) == CM89_OK) {
                printf(" %s", name);
            }
        }
        printf("\n");
    }

    printf("isinstance(dog, Animal) = %s\n",
           cm89_is_instance(&manager, dog, animal_cls) ? "true" : "false");

    cm89_instance_release(&manager, dog);
    return 0;
}
