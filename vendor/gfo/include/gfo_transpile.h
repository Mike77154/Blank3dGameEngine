/* gfo_transpile.h - transpile compiled bytecode to C header or text */
#ifndef GFO_TRANSPILE_H
#define GFO_TRANSPILE_H
#include "gfo_runtime.h"

/* Writes a C header that embeds the bytecode + symbol table names (for debugging).
   The output is written to a provided buffer (no file I/O inside the lib). */

typedef struct {
  char* buf;
  gfo_u32 cap;
  gfo_u32 len;
} gfo_out;

void gfo_out_init(gfo_out* o, char* buf, gfo_u32 cap);
void gfo_out_puts(gfo_out* o, const char* s);
void gfo_out_put_u32(gfo_out* o, gfo_u32 v);
void gfo_out_put_str_lit(gfo_out* o, gfo_str s);

/* transpile */
int gfo_transpile_c_header(const gfo_ctx* ctx, gfo_out* out, const char* guard_name);

#endif
