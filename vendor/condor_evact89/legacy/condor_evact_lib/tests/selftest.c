/* selftest.c - C89 */

#include <stdio.h>
#include "evact.h"

static int g_failures = 0;
static int g_steps[32];
static int g_step_count = 0;
static int g_new_called = 0;
static int g_old_called = 0;
static int g_replacement_called = 0;
static int g_nested_called = 0;
static int g_rule_hits = 0;
static evcore_sub_id g_new_sub = EVCORE_SUB_INVALID;
static evcore_sub_id g_old_sub = EVCORE_SUB_INVALID;

static void fail_if(int expr, const char *msg)
{
    if (expr) {
        ++g_failures;
        printf("FAIL: %s\n", msg);
    }
}

static void reset_globals(void)
{
    int i;

    g_failures = 0;
    g_step_count = 0;
    g_new_called = 0;
    g_old_called = 0;
    g_replacement_called = 0;
    g_nested_called = 0;
    g_rule_hits = 0;
    g_new_sub = EVCORE_SUB_INVALID;
    g_old_sub = EVCORE_SUB_INVALID;

    for (i = 0; i < 32; ++i) {
        g_steps[i] = 0;
    }

    evcore_reset();
    condcore_reset();
    actcore_reset();
    evact_reset();
}

static void listener_new(const evcore_event_t *evt, void *user)
{
    (void)evt;
    (void)user;
    ++g_new_called;
    g_steps[g_step_count++] = 3;
}

static void listener_replacement(const evcore_event_t *evt, void *user)
{
    (void)evt;
    (void)user;
    ++g_replacement_called;
    g_steps[g_step_count++] = 4;
}

static void listener_add_during_emit(const evcore_event_t *evt, void *user)
{
    (void)evt;
    (void)user;
    g_steps[g_step_count++] = 1;
    g_new_sub = evcore_subscribe(10, listener_new, 0);
}

static void listener_remove_and_replace(const evcore_event_t *evt, void *user)
{
    (void)evt;
    (void)user;
    g_steps[g_step_count++] = 1;
    evcore_unsubscribe_id(g_old_sub);
    g_new_sub = evcore_subscribe(11, listener_replacement, 0);
}

static void listener_old(const evcore_event_t *evt, void *user)
{
    (void)evt;
    (void)user;
    ++g_old_called;
    g_steps[g_step_count++] = 2;
}

static void listener_nested_inner(const evcore_event_t *evt, void *user)
{
    (void)evt;
    (void)user;
    ++g_nested_called;
}

static void listener_nested_outer(const evcore_event_t *evt, void *user)
{
    (void)user;
    if (evt != 0) {
        evcore_emit_values(77, evt->code, 0);
    }
}

static int cond_code_7(const evcore_event_t *evt, void *user)
{
    int target;

    target = *(const int *)user;
    if (evt == 0) {
        return 0;
    }
    return evt->code == target;
}

static void action_hit(const evcore_event_t *evt, void *user)
{
    int *counter;

    (void)evt;
    counter = (int *)user;
    ++(*counter);
}

static void test_subscribe_during_emit(void)
{
    evcore_event_t evt;

    reset_globals();
    fail_if(evcore_subscribe(10, listener_add_during_emit, 0) == EVCORE_SUB_INVALID,
            "subscribe initial listener failed");

    evt.type = 10;
    evt.code = 1;
    evt.data = 0;

    evcore_emit(&evt);
    fail_if(g_new_called != 0, "new listener should not receive current event");
    fail_if(g_step_count != 1 || g_steps[0] != 1,
            "unexpected order in subscribe-during-emit");

    evcore_emit(&evt);
    fail_if(g_new_called != 1, "new listener should receive next event");
}

static void test_unsubscribe_and_slot_reuse(void)
{
    evcore_event_t evt;

    reset_globals();
    fail_if(evcore_subscribe(11, listener_remove_and_replace, 0) == EVCORE_SUB_INVALID,
            "subscribe remover failed");
    g_old_sub = evcore_subscribe(11, listener_old, 0);
    fail_if(g_old_sub == EVCORE_SUB_INVALID, "subscribe old listener failed");

    evt.type = 11;
    evt.code = 2;
    evt.data = 0;

    evcore_emit(&evt);

    fail_if(g_old_called != 0,
            "removed listener should not run after being removed before its turn");
    fail_if(g_replacement_called != 0,
            "replacement listener should not receive current event");
    fail_if(g_step_count != 1 || g_steps[0] != 1,
            "unexpected order in unsubscribe-and-reuse");

    evcore_emit(&evt);
    fail_if(g_replacement_called != 1,
            "replacement listener should receive next event");
}

static void test_nested_emit(void)
{
    evcore_event_t evt;

    reset_globals();
    fail_if(evcore_subscribe(66, listener_nested_outer, 0) == EVCORE_SUB_INVALID,
            "subscribe nested outer failed");
    fail_if(evcore_subscribe(77, listener_nested_inner, 0) == EVCORE_SUB_INVALID,
            "subscribe nested inner failed");

    evt.type = 66;
    evt.code = 99;
    evt.data = 0;

    evcore_emit(&evt);
    fail_if(g_nested_called != 1, "nested emit should work");
}

static void test_unique_and_rules(void)
{
    evcore_sub_id sub1;
    evcore_sub_id sub2;
    condcore_id cond;
    actcore_id act;
    evact_rule_id rule1;
    evact_rule_id rule2;
    evcore_event_t evt;
    int target;

    reset_globals();

    sub1 = evcore_subscribe_unique(1, listener_new, 0);
    sub2 = evcore_subscribe_unique(1, listener_new, 0);
    fail_if(sub1 == EVCORE_SUB_INVALID || sub2 == EVCORE_SUB_INVALID,
            "subscribe_unique returned invalid handle");
    fail_if(sub1 != sub2, "subscribe_unique should reuse existing handle");

    target = 7;
    cond = condcore_register(cond_code_7, &target);
    act = actcore_register(action_hit, &g_rule_hits);
    fail_if(cond == CONDCORE_ID_INVALID, "cond register failed");
    fail_if(act == ACTCORE_ID_INVALID, "act register failed");

    rule1 = evact_rule_register_unique(42, EVCORE_MATCH_ANY, cond, act);
    rule2 = evact_rule_register_unique(42, EVCORE_MATCH_ANY, cond, act);
    fail_if(rule1 == EVACT_RULE_INVALID || rule2 == EVACT_RULE_INVALID,
            "rule register failed");
    fail_if(rule1 != rule2, "rule_register_unique should reuse rule");

    fail_if(evact_attach_all() == EVCORE_SUB_INVALID, "evact_attach_all failed");

    evt.type = 42;
    evt.code = 6;
    evt.data = 0;
    evcore_emit(&evt);
    fail_if(g_rule_hits != 0, "rule should not fire for wrong code");

    evt.code = 7;
    evcore_emit(&evt);
    fail_if(g_rule_hits != 1, "rule should fire for matching code");

    fail_if(evact_rule_unregister(rule1) != 1, "rule unregister failed");
    evcore_emit(&evt);
    fail_if(g_rule_hits != 1, "rule should not fire after unregister");
}

int main(void)
{
    test_subscribe_during_emit();
    if (g_failures != 0) {
        return 1;
    }

    test_unsubscribe_and_slot_reuse();
    if (g_failures != 0) {
        return 1;
    }

    test_nested_emit();
    if (g_failures != 0) {
        return 1;
    }

    test_unique_and_rules();
    if (g_failures != 0) {
        return 1;
    }

    printf("OK\n");
    return 0;
}
