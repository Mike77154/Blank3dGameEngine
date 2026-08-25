#ifndef INVARIANT_SPECIALOPERATIONS_89_DDSL2_H
#define INVARIANT_SPECIALOPERATIONS_89_DDSL2_H

#include "invariantSpecialoperations_89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ISO89_DDSL2_MAX_SYMBOLS
#define ISO89_DDSL2_MAX_SYMBOLS 64
#endif
#ifndef ISO89_DDSL2_SYMBOL_CAP
#define ISO89_DDSL2_SYMBOL_CAP 64
#endif

typedef struct iso89_ddsl2_symbols {
    char names[ISO89_DDSL2_MAX_SYMBOLS][ISO89_DDSL2_SYMBOL_CAP];
    const char *name_ptrs[ISO89_DDSL2_MAX_SYMBOLS];
    int count;
} iso89_ddsl2_symbols;

void iso89_ddsl2_symbols_init(iso89_ddsl2_symbols *symbols);
iso89_subject iso89_ddsl2_find_subject(const iso89_ddsl2_symbols *symbols,
                                       const char *name);
const char *iso89_ddsl2_subject_name(const iso89_ddsl2_symbols *symbols,
                                     iso89_subject subject);

/*
 * Extract invariant declarations from DDSL2-compatible source while copying
 * every ordinary DDSL2 line unchanged to output.
 *
 * Supported declaration (case-insensitive keywords):
 *
 *   If walk_forward TRUE then run_forward FALSE
 *   Apply SpecOp=Viceversa
 *
 * The first line is an invariant declaration because it has the distinct
 *   If <symbol> <bool> then <symbol> <bool>
 * shape.  The Apply line decorates the immediately preceding invariant.
 * Both lines are replaced by blank lines so DDSL2 source line numbers remain
 * stable for diagnostics.
 */
int iso89_ddsl2_preprocess(const char *source,
                           char *output,
                           unsigned int output_capacity,
                           iso89_context *ctx,
                           iso89_ddsl2_symbols *symbols,
                           char *error,
                           unsigned int error_capacity);

#ifdef __cplusplus
}
#endif

#endif
