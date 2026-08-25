#include "blank3d_condor.h"
#include <stdio.h>
#include <string.h>

static int t_allow = 1;
static int t_actions = 0;

static int t_cond(void *user, const gverb89_call *call, gverb89_result *out)
{
    (void)user; (void)call;
    memset(out, 0, sizeof(*out));
    out->truth = t_allow;
    return GVERB89_HANDLED;
}

static int t_action(void *user, const gverb89_call *call)
{
    (void)user; (void)call;
    ++t_actions;
    return GVERB89_HANDLED;
}

int main(void)
{
    Blank3DCondor c;
    Blank3DVariables vars;
    Blank3DInput input;
    gverb89_registry verbs;
    cea89_rule_id rule;
    cea89_condition_id vc;
    cea89_action_id va;
    vm89_value v;

    memset(&vars, 0, sizeof(vars));
    vr89_init(&vars.runtime);
    if (vr89_instance_create(&vars.runtime, 42UL) != VR89_OK) return 2;
    if (!blank3d_variables_set_q16(&vars, VR89_SCOPE_INSTANCE, 42UL,
                                    "health", 10L * 65536L)) return 3;

    gverb89_init(&verbs);
    if (!gverb89_register_condition(&verbs, "can_go", t_cond, 0)) return 4;
    if (!gverb89_register_action(&verbs, "go", t_action, 0)) return 5;

    blank3d_condor_init(&c, &verbs, &vars);
    rule = blank3d_condor_rule_gameverbs(&c,
                B3D_CEA_EVENT_INPUT_PRESS, 26, "can_go", "go");
    if (!rule) return 6;
    if (blank3d_condor_emit(&c, B3D_CEA_EVENT_INPUT_PRESS, 26,
                            1UL, 42UL, 0, 0) != 1) return 7;
    if (t_actions != 1) return 8;

    vc = blank3d_condor_condition_var_q16(&c, VR89_SCOPE_INSTANCE, 0UL,
                "health", B3D_CEA_CMP_GT, 0L);
    va = blank3d_condor_action_var_bool(&c, VR89_SCOPE_INSTANCE, 0UL,
                "triggered", 1);
    if (!vc || !va) return 9;
    if (!blank3d_condor_rule(&c, B3D_CEA_EVENT_CUSTOM, 7, vc, va)) return 10;
    if (blank3d_condor_emit(&c, B3D_CEA_EVENT_CUSTOM, 7,
                            1UL, 42UL, 0, 0) != 1) return 11;
    memset(&v, 0, sizeof(v));
    if (!blank3d_variables_get(&vars, VR89_SCOPE_INSTANCE, 42UL,
                               "triggered", &v)) return 12;
    if (v.type != VM89_VALUE_BOOL || !v.boolean) return 13;

    memset(&input, 0, sizeof(input));
    (void)input_scanner_init(&input.key_banks[0], 0, 0);
    (void)input_scanner_set_button_count(&input.key_banks[0], B3D_INPUT_BANK_WIDTH);
    (void)input_scanner_update_with_bits(&input.key_banks[0],
                                          ((input_bits_t)1UL << 5));
    blank3d_condor_emit_input(&c, &input, 1UL, 42UL, 0);
    if (c.input_events == 0UL) return 14;

    puts("blank3d condor bridge: PASS");
    return 0;
}
