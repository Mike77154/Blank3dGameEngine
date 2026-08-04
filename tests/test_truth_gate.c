#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "blank3d_truth_gate.h"

static Blank3DTruthFacts baseline(void)
{
    Blank3DTruthFacts facts;
    memset(&facts, 0, sizeof(facts));
    facts.entity_alive = 1;
    facts.target_alive = 1;
    facts.socketer_has_target = 1;
    return facts;
}

int main(void)
{
    Blank3DTruthGate gate;
    Blank3DTruthFacts facts;

    memset(&facts, 0, sizeof(facts));
    blank3d_truth_gate_init(&gate, B3D_TRUTH_PROFILE_RETRO);
    assert(!blank3d_truth_gate_evaluate(&gate, &facts));

    facts = baseline();
    assert(blank3d_truth_gate_evaluate(&gate, &facts));
    assert(blank3d_truth_gate_truth_count(&gate) == 1U);

    assert(blank3d_truth_gate_set_profile_name(&gate, "proximity"));
    facts.within_range = 0;
    facts.eyes_visible = 0;
    assert(blank3d_truth_gate_evaluate(&gate, &facts));
    assert(blank3d_truth_gate_truth_count(&gate) == 3U);

    facts.within_range = 1;
    facts.eyes_visible = 1;
    assert(blank3d_truth_gate_evaluate(&gate, &facts));
    assert(blank3d_truth_gate_truth_count(&gate) == 5U);

    assert(blank3d_truth_gate_set_profile_name(&gate, "strict"));
    facts.line_of_sight = 0;
    facts.eyes_visible = 0;
    facts.enlightener_interpreted = 0;
    assert(blank3d_truth_gate_evaluate(&gate, &facts));
    assert(blank3d_truth_gate_truth_count(&gate) >= 3U);

    facts.line_of_sight = 1;
    facts.eyes_visible = 1;
    facts.enlightener_interpreted = 1;
    facts.target_remembered = 1;
    assert(blank3d_truth_gate_evaluate(&gate, &facts));
    assert(strcmp(blank3d_truth_gate_profile_name(gate.profile),
                  "strict") == 0);

    printf("Blank3D GEDER bridge: additive truths and Socketer fallback OK.\n");
    return 0;
}
