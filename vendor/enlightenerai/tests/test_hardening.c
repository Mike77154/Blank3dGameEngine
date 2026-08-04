#include "test_common.h"
#include "eai_math.h"
#include "eai_context.h"
#include "eai_nav.h"

int main(void)
{
    EAI_Vec3 a;
    EAI_Vec3 b;
    EAI_Vec3 out_v;
    EAI_Context ctx;
    EAI_NodeId n0;
    EAI_NodeId n1;
    EAI_U16 i;
    int ok;

    TEST_ASSERT(sizeof(EAI_Fixed) == 4u);
    TEST_ASSERT(sizeof(EAI_U32) == 4u);

    TEST_ASSERT(eai_fx_add_sat(EAI_FX_MAX, EAI_FX_ONE) == EAI_FX_MAX);
    TEST_ASSERT(eai_fx_sub_sat(EAI_FX_MIN, EAI_FX_ONE) == EAI_FX_MIN);
    TEST_ASSERT(eai_fx_add_sat(EAI_FX_ONE, EAI_FX_ONE) == EAI_FX_FROM_RAW(131072));
    TEST_ASSERT(eai_fx_sub_sat(EAI_FX_ONE, EAI_FX_HALF) == EAI_FX_HALF);
    TEST_ASSERT(eai_fx_from_int((EAI_Fixed)40000) == EAI_FX_MAX);
    TEST_ASSERT(eai_fx_from_int((EAI_Fixed)-40000) == EAI_FX_MIN);
    TEST_ASSERT(EAI_FX_FROM_INT(40000) == EAI_FX_MAX);
    TEST_ASSERT(EAI_FX_FROM_INT(-40000) == EAI_FX_MIN);

    eai_vec3_set(&a, EAI_FX_MAX, EAI_FX_MIN, EAI_FX_ONE);
    eai_vec3_set(&b, EAI_FX_ONE, EAI_FX_ONE, EAI_FX_MIN);
    eai_vec3_add(&out_v, &a, &b);
    TEST_ASSERT(out_v.x == EAI_FX_MAX);
    TEST_ASSERT(out_v.y == (EAI_Fixed)(EAI_FX_MIN + EAI_FX_ONE));
    TEST_ASSERT(out_v.z == (EAI_Fixed)(EAI_FX_MIN + EAI_FX_ONE));

    eai_vec3_sub(&out_v, &a, &b);
    TEST_ASSERT(out_v.x == (EAI_Fixed)(EAI_FX_MAX - EAI_FX_ONE));
    TEST_ASSERT(out_v.y == EAI_FX_MIN);
    TEST_ASSERT(out_v.z == EAI_FX_MAX);

    eai_nav_reset(&ctx);
    eai_vec3_set(&a, EAI_FX_ZERO, EAI_FX_ZERO, EAI_FX_ZERO);
    n0 = eai_nav_add_node(&ctx, &a, EAI_INVALID_ID, 0u);
    n1 = eai_nav_add_node(&ctx, &a, EAI_INVALID_ID, 0u);
    TEST_ASSERT(n0 != EAI_INVALID_ID);
    TEST_ASSERT(n1 != EAI_INVALID_ID);

    for (i = 0; i < (EAI_U16)(EAI_MAX_NAV_EDGES - 1u); ++i)
    {
        ok = eai_nav_add_edge(&ctx, n0, n1, EAI_FX_ONE, 0u);
        TEST_ASSERT(ok != 0);
    }

    TEST_ASSERT(ctx.edge_count == (EAI_U16)(EAI_MAX_NAV_EDGES - 1u));
    ok = eai_nav_add_bidirectional_edge(&ctx, n0, n1, EAI_FX_ONE, 0u);
    TEST_ASSERT(ok == 0);
    TEST_ASSERT(ctx.edge_count == (EAI_U16)(EAI_MAX_NAV_EDGES - 1u));

    TEST_PASS();
}
