#include "invariantSpecialoperations_89.h"
#include "invariantSpecialoperations_89_ddsl2.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *src;
    char out[1024];
    char err[128];
    iso89_rule rules[8];
    iso89_context ctx;
    iso89_ddsl2_symbols symbols;
    src = "If key_hold Up then walk_forward\n"
          "If key_hold Shift and key_hold Up then run_forward\n"
          "\n"
          "If walk_forward TRUE then run_forward FALSE\n"
          "Apply SpecOp=Viceversa\n";
    iso89_context_init(&ctx, rules, 8);
    if (!iso89_ddsl2_preprocess(src, out, sizeof(out), &ctx, &symbols,
                                err, sizeof(err))) {
        printf("adapter error: %s\n", err);
        return 1;
    }
    if (ctx.rule_count != 2) return 2;
    if (symbols.count != 2) return 3;
    if (!iso89_ddsl2_find_subject(&symbols, "walk_forward")) return 4;
    if (!iso89_ddsl2_find_subject(&symbols, "run_forward")) return 5;
    if (!strstr(out, "key_hold Shift and key_hold Up")) return 6;
    if (strstr(out, "SpecOp")) return 7;
    src = "If alpha TRUE then beta FALSE\n"
          "If gamma TRUE then delta FALSE\n";
    iso89_context_clear(&ctx);
    if (!iso89_ddsl2_preprocess(src, out, sizeof(out), &ctx, &symbols,
                                err, sizeof(err))) return 8;
    if (ctx.rule_count != 2 || symbols.count != 4) return 9;
    if (!iso89_ddsl2_find_subject(&symbols, "alpha")) return 10;
    if (!iso89_ddsl2_find_subject(&symbols, "gamma")) return 11;
    puts("invariantSpecialoperations_89 DDSL2 adapter: OK");
    return 0;
}
