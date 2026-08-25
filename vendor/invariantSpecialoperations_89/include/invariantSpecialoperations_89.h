#ifndef INVARIANT_SPECIALOPERATIONS_89_H
#define INVARIANT_SPECIALOPERATIONS_89_H

/*
 * invariantSpecialoperations_89
 * C89, no heap, provider-driven invariant relation helper.
 *
 * The core knows no gameplay, flags, DSL, operating system or value meaning.
 * Subjects are opaque unsigned-long tokens and values are opaque signed-long
 * scalars.  A rule only means:
 *
 *     IF subject A has value X THEN set subject B to value Y
 *
 * SpecOp=Viceversa expands that relation structurally to:
 *
 *     IF A == X THEN B = Y
 *     IF B == X THEN A = Y
 *
 * This is useful for mutual exclusion, mirrored state rules and other simple
 * invariants.  The provider decides where subjects/values actually live.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long iso89_subject;
typedef long iso89_value;

#define ISO89_SUBJECT_NONE 0UL
#define ISO89_OK 1
#define ISO89_ERROR 0

#ifndef ISO89_DEFAULT_MAX_PROPAGATION_DEPTH
#define ISO89_DEFAULT_MAX_PROPAGATION_DEPTH 32
#endif

typedef enum iso89_specop {
    ISO89_SPECOP_NONE = 0,
    ISO89_SPECOP_VICEVERSA = 1
} iso89_specop;

typedef struct iso89_rule {
    iso89_subject when_subject;
    iso89_value when_value;
    iso89_subject set_subject;
    iso89_value set_value;
} iso89_rule;

typedef struct iso89_context {
    iso89_rule *rules;
    int rule_capacity;
    int rule_count;
    int max_propagation_depth;
} iso89_context;

/*
 * get_value: return non-zero when a value is available.
 * set_value: return non-zero when the value was accepted.
 *
 * The core reads before writing, so assigning an already-equal value does not
 * recursively retrigger the invariant graph.
 */
typedef int (*iso89_get_value_fn)(void *user,
                                  iso89_subject subject,
                                  iso89_value *out_value);
typedef int (*iso89_set_value_fn)(void *user,
                                  iso89_subject subject,
                                  iso89_value value);

typedef struct iso89_provider {
    iso89_get_value_fn get_value;
    iso89_set_value_fn set_value;
    void *user;
} iso89_provider;

void iso89_context_init(iso89_context *ctx,
                        iso89_rule *rules,
                        int rule_capacity);
void iso89_context_clear(iso89_context *ctx);
void iso89_context_set_max_depth(iso89_context *ctx, int max_depth);

int iso89_add_rule(iso89_context *ctx,
                   iso89_subject when_subject,
                   iso89_value when_value,
                   iso89_subject set_subject,
                   iso89_value set_value);

int iso89_add_relation(iso89_context *ctx,
                       iso89_subject left_subject,
                       iso89_value trigger_value,
                       iso89_subject right_subject,
                       iso89_value result_value,
                       iso89_specop specop);

/*
 * Apply one external assignment and propagate every invariant triggered by it.
 * Event order is preserved: if A and then B are asserted in the same host
 * frame, the second external event is allowed to become the final state.
 */
int iso89_apply_event(iso89_context *ctx,
                      const iso89_provider *provider,
                      iso89_subject subject,
                      iso89_value value);

const char *iso89_specop_name(iso89_specop specop);
int iso89_specop_from_name(const char *name, iso89_specop *out_specop);

#ifdef __cplusplus
}
#endif

#endif
