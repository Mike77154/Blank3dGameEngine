#include <stdio.h>
#include "cm89.h"

static int failures = 0;

static void expect_true(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL: %s\n", name);
        failures++;
    }
}

static cm89_result null_invoke(
    void *user,
    cm89_value callable,
    cm89_value receiver,
    cm89_class_h owner_class,
    const cm89_call *call,
    cm89_value *out_value
)
{
    (void)user;
    (void)callable;
    (void)receiver;
    (void)owner_class;
    (void)call;
    if (out_value != 0) {
        *out_value = cm89_value_none();
    }
    return CM89_OK;
}

static void test_single_inheritance(void)
{
    static cm89_manager m;
    cm89_provider p;
    cm89_class_h o;
    cm89_class_h a;
    cm89_class_h b;
    cm89_class_h bases[1];
    const cm89_class_h *mro;
    cm89_count count;

    p.user = 0;
    p.invoke = null_invoke;
    cm89_manager_init(&m, &p);

    expect_true(cm89_class_create(&m, "O", 0, 0, &o) == CM89_OK,
                "create O");
    bases[0] = o;
    expect_true(cm89_class_create(&m, "A", bases, 1, &a) == CM89_OK,
                "create A");
    bases[0] = a;
    expect_true(cm89_class_create(&m, "B", bases, 1, &b) == CM89_OK,
                "create B");

    expect_true(cm89_class_get_mro(&m, b, &mro, &count) == CM89_OK,
                "get B MRO");
    expect_true(count == 3, "B MRO size");
    expect_true(mro[0] == b && mro[1] == a && mro[2] == o,
                "B MRO order");
}

static void test_diamond_c3(void)
{
    static cm89_manager m;
    cm89_provider p;
    cm89_class_h o;
    cm89_class_h a;
    cm89_class_h b;
    cm89_class_h c;
    cm89_class_h d;
    cm89_class_h bases[2];
    const cm89_class_h *mro;
    cm89_count count;

    p.user = 0;
    p.invoke = null_invoke;
    cm89_manager_init(&m, &p);

    expect_true(cm89_class_create(&m, "O", 0, 0, &o) == CM89_OK,
                "diamond create O");
    bases[0] = o;
    expect_true(cm89_class_create(&m, "A", bases, 1, &a) == CM89_OK,
                "diamond create A");
    expect_true(cm89_class_create(&m, "B", bases, 1, &b) == CM89_OK,
                "diamond create B");
    bases[0] = a;
    bases[1] = b;
    expect_true(cm89_class_create(&m, "C", bases, 2, &c) == CM89_OK,
                "diamond create C");
    bases[0] = c;
    expect_true(cm89_class_create(&m, "D", bases, 1, &d) == CM89_OK,
                "diamond create D");

    expect_true(cm89_class_get_mro(&m, c, &mro, &count) == CM89_OK,
                "diamond get C MRO");
    expect_true(count == 4, "diamond C MRO size");
    expect_true(mro[0] == c && mro[1] == a && mro[2] == b && mro[3] == o,
                "diamond C3 order");
}

static void test_mro_conflict(void)
{
    static cm89_manager m;
    cm89_provider p;
    cm89_class_h o;
    cm89_class_h x;
    cm89_class_h y;
    cm89_class_h a;
    cm89_class_h b;
    cm89_class_h c;
    cm89_class_h bases[2];
    cm89_result result;

    p.user = 0;
    p.invoke = null_invoke;
    cm89_manager_init(&m, &p);

    cm89_class_create(&m, "O", 0, 0, &o);
    bases[0] = o;
    cm89_class_create(&m, "X", bases, 1, &x);
    cm89_class_create(&m, "Y", bases, 1, &y);

    bases[0] = x;
    bases[1] = y;
    cm89_class_create(&m, "A", bases, 2, &a);

    bases[0] = y;
    bases[1] = x;
    cm89_class_create(&m, "B", bases, 2, &b);

    bases[0] = a;
    bases[1] = b;
    result = cm89_class_create(&m, "C", bases, 2, &c);
    expect_true(result == CM89_ERR_MRO_CONFLICT, "reject ambiguous C3 MRO");
}

static void test_attribute_shadowing(void)
{
    static cm89_manager m;
    cm89_provider p;
    cm89_class_h o;
    cm89_class_h a;
    cm89_class_h bases[1];
    cm89_instance_h instance;
    cm89_value v;

    p.user = 0;
    p.invoke = null_invoke;
    cm89_manager_init(&m, &p);

    cm89_class_create(&m, "O", 0, 0, &o);
    bases[0] = o;
    cm89_class_create(&m, "A", bases, 1, &a);
    cm89_class_set_member(
        &m, a, "x", CM89_MEMBER_VALUE, cm89_value_sint(10L), cm89_value_none()
    );
    cm89_instance_new(&m, a, 0, &instance);

    expect_true(cm89_instance_get_attr(&m, instance, "x", &v) == CM89_OK,
                "get inherited/class attr");
    expect_true(v.kind == CM89_VALUE_SINT && v.data.sint_value == 10L,
                "class attr value");

    cm89_instance_set_attr(&m, instance, "x", cm89_value_sint(20L));
    expect_true(cm89_instance_get_attr(&m, instance, "x", &v) == CM89_OK,
                "get instance shadow");
    expect_true(v.kind == CM89_VALUE_SINT && v.data.sint_value == 20L,
                "instance shadows class");
}

int main(void)
{
    test_single_inheritance();
    test_diamond_c3();
    test_mro_conflict();
    test_attribute_shadowing();

    if (failures != 0) {
        printf("%d test(s) failed\n", failures);
        return 1;
    }

    printf("all class_manager89 tests passed\n");
    return 0;
}
