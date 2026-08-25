#include "invariantSpecialoperations_89.h"

#include <ctype.h>
#include <string.h>

static int iso89_ci_equal(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = (unsigned char)tolower((unsigned char)*a++);
        cb = (unsigned char)tolower((unsigned char)*b++);
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

void iso89_context_init(iso89_context *ctx,
                        iso89_rule *rules,
                        int rule_capacity)
{
    if (!ctx) return;
    ctx->rules = rules;
    ctx->rule_capacity = rule_capacity > 0 ? rule_capacity : 0;
    ctx->rule_count = 0;
    ctx->max_propagation_depth = ISO89_DEFAULT_MAX_PROPAGATION_DEPTH;
}

void iso89_context_clear(iso89_context *ctx)
{
    if (!ctx) return;
    ctx->rule_count = 0;
}

void iso89_context_set_max_depth(iso89_context *ctx, int max_depth)
{
    if (!ctx) return;
    if (max_depth < 1) max_depth = 1;
    ctx->max_propagation_depth = max_depth;
}

int iso89_add_rule(iso89_context *ctx,
                   iso89_subject when_subject,
                   iso89_value when_value,
                   iso89_subject set_subject,
                   iso89_value set_value)
{
    iso89_rule *rule;
    if (!ctx || !ctx->rules) return ISO89_ERROR;
    if (when_subject == ISO89_SUBJECT_NONE ||
        set_subject == ISO89_SUBJECT_NONE) return ISO89_ERROR;
    if (ctx->rule_count >= ctx->rule_capacity) return ISO89_ERROR;
    rule = &ctx->rules[ctx->rule_count++];
    rule->when_subject = when_subject;
    rule->when_value = when_value;
    rule->set_subject = set_subject;
    rule->set_value = set_value;
    return ISO89_OK;
}

int iso89_add_relation(iso89_context *ctx,
                       iso89_subject left_subject,
                       iso89_value trigger_value,
                       iso89_subject right_subject,
                       iso89_value result_value,
                       iso89_specop specop)
{
    int before;
    if (!ctx) return ISO89_ERROR;
    before = ctx->rule_count;
    if (!iso89_add_rule(ctx, left_subject, trigger_value,
                        right_subject, result_value)) return ISO89_ERROR;
    if (specop == ISO89_SPECOP_VICEVERSA) {
        if (!iso89_add_rule(ctx, right_subject, trigger_value,
                            left_subject, result_value)) {
            ctx->rule_count = before;
            return ISO89_ERROR;
        }
    } else if (specop != ISO89_SPECOP_NONE) {
        ctx->rule_count = before;
        return ISO89_ERROR;
    }
    return ISO89_OK;
}

static int iso89_assign_and_propagate(iso89_context *ctx,
                                      const iso89_provider *provider,
                                      iso89_subject subject,
                                      iso89_value value,
                                      int depth)
{
    iso89_value old_value;
    int had_old;
    int i;
    if (!ctx || !provider || !provider->set_value) return ISO89_ERROR;
    if (subject == ISO89_SUBJECT_NONE) return ISO89_ERROR;
    if (depth > ctx->max_propagation_depth) return ISO89_ERROR;

    had_old = 0;
    old_value = 0L;
    if (provider->get_value)
        had_old = provider->get_value(provider->user, subject, &old_value);
    if (had_old && old_value == value) return ISO89_OK;
    if (!provider->set_value(provider->user, subject, value)) return ISO89_ERROR;

    for (i = 0; i < ctx->rule_count; ++i) {
        const iso89_rule *rule;
        rule = &ctx->rules[i];
        if (rule->when_subject == subject && rule->when_value == value) {
            if (!iso89_assign_and_propagate(ctx, provider,
                                            rule->set_subject,
                                            rule->set_value,
                                            depth + 1))
                return ISO89_ERROR;
        }
    }
    return ISO89_OK;
}

int iso89_apply_event(iso89_context *ctx,
                      const iso89_provider *provider,
                      iso89_subject subject,
                      iso89_value value)
{
    return iso89_assign_and_propagate(ctx, provider, subject, value, 0);
}

const char *iso89_specop_name(iso89_specop specop)
{
    switch (specop) {
    case ISO89_SPECOP_NONE: return "None";
    case ISO89_SPECOP_VICEVERSA: return "Viceversa";
    default: return "Unknown";
    }
}

int iso89_specop_from_name(const char *name, iso89_specop *out_specop)
{
    if (!name || !out_specop) return 0;
    if (iso89_ci_equal(name, "none") || iso89_ci_equal(name, "oneway") ||
        iso89_ci_equal(name, "one_way")) {
        *out_specop = ISO89_SPECOP_NONE;
        return 1;
    }
    if (iso89_ci_equal(name, "viceversa") ||
        iso89_ci_equal(name, "vice_versa") ||
        iso89_ci_equal(name, "vice-versa")) {
        *out_specop = ISO89_SPECOP_VICEVERSA;
        return 1;
    }
    return 0;
}
