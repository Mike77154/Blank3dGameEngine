#include <stdio.h>
#include "cm89.h"

static int failures = 0;
static cm89_value last_receiver;
static cm89_class_h last_owner;
static cm89_value last_callable;

static void expect_true(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL: %s\n", name);
        failures++;
    }
}

static cm89_result capture_invoke(
    void *user,
    cm89_value callable,
    cm89_value receiver,
    cm89_class_h owner_class,
    const cm89_call *call,
    cm89_value *out_value
)
{
    (void)user;
    (void)call;
    last_callable = callable;
    last_receiver = receiver;
    last_owner = owner_class;
    if (out_value) *out_value = cm89_value_uint(77UL);
    return CM89_OK;
}

int main(void)
{
    static cm89_manager m;
    cm89_provider p;
    cm89_class_h root;
    cm89_class_h base;
    cm89_class_h child;
    cm89_class_h replacement;
    cm89_class_h bases[1];
    cm89_value out;
    cm89_class_h stale;

    p.user = 0;
    p.invoke = capture_invoke;
    cm89_manager_init(&m, &p);

    expect_true(cm89_class_create(&m, "Object", 0, 0, &root) == CM89_OK,
                "create Object");
    bases[0] = root;
    expect_true(cm89_class_create(&m, "Base", bases, 1, &base) == CM89_OK,
                "create Base");
    bases[0] = base;
    expect_true(cm89_class_create(&m, "Child", bases, 1, &child) == CM89_OK,
                "create Child");

    expect_true(cm89_class_set_member(&m, base, "tick", CM89_MEMBER_METHOD,
                cm89_value_host_handle(1001UL), cm89_value_none()) == CM89_OK,
                "set inherited method");
    expect_true(cm89_call_bound(&m, child, cm89_value_host_handle(42UL),
                "tick", 0, &out) == CM89_OK,
                "bound call");
    expect_true(last_receiver.kind == CM89_VALUE_HOST_HANDLE &&
                last_receiver.data.uint_value == 42UL,
                "bound receiver preserved");
    expect_true(last_owner == base, "method owner is defining class");
    expect_true(last_callable.data.uint_value == 1001UL,
                "callable preserved");

    expect_true(cm89_class_set_member(&m, base, "kind", CM89_MEMBER_CLASS_METHOD,
                cm89_value_host_handle(2002UL), cm89_value_none()) == CM89_OK,
                "set inherited classmethod");
    expect_true(cm89_call_bound(&m, child, cm89_value_host_handle(42UL),
                "kind", 0, &out) == CM89_OK,
                "bound inherited classmethod");
    expect_true(last_receiver.kind == CM89_VALUE_CLASS &&
                (cm89_class_h)last_receiver.data.uint_value == child,
                "classmethod receives dynamic class");
    expect_true(last_owner == base, "classmethod owner preserved");

    expect_true(cm89_instance_new(&m, child, 0, 0) == CM89_ERR_DISABLED,
                "internal instances disabled");

    stale = child;
    expect_true(cm89_class_destroy(&m, child) == CM89_OK, "destroy Child");
    bases[0] = base;
    expect_true(cm89_class_create(&m, "Replacement", bases, 1, &replacement) == CM89_OK,
                "reuse class slot");
    expect_true(stale != replacement, "generation changed on slot reuse");
    expect_true(cm89_class_get_name(&m, stale, (const char **)&p.user) == CM89_ERR_INVALID_HANDLE,
                "stale class handle rejected");

    expect_true(cm89_manager_seal(&m) == CM89_OK, "seal registry");
    expect_true(cm89_class_create(&m, "Nope", 0, 0, &child) == CM89_ERR_SEALED,
                "sealed registry rejects create");
    expect_true(cm89_class_set_member(&m, base, "x", CM89_MEMBER_VALUE,
                cm89_value_uint(1UL), cm89_value_none()) == CM89_ERR_SEALED,
                "sealed registry rejects mutation");
    expect_true(cm89_manager_unseal(&m) == CM89_OK, "unseal registry");

    expect_true(sizeof(cm89_manager) < 300000UL,
                "external-receiver manager footprint stays bounded");

    if (failures) {
        printf("%d bound test(s) failed\n", failures);
        return 1;
    }
    printf("all class_manager89 bound tests passed; manager=%lu bytes\n",
           (unsigned long)sizeof(cm89_manager));
    return 0;
}
