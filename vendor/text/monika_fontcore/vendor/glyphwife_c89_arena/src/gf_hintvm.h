#ifndef GF_HINTVM_H
#define GF_HINTVM_H
#include "gf_outline.h"
#define GF_OP_END       0
#define GF_OP_PUSH      1
#define GF_OP_MOVE_X    2
#define GF_OP_MOVE_Y    3
#define GF_OP_ALIGN_X   4
#define GF_OP_ALIGN_Y   5
#define GF_OP_ROUND_X   6
#define GF_OP_ROUND_Y   7
#define GF_OP_SCALE_X   8
#define GF_OP_SCALE_Y   9

typedef struct GF_HintProgram { unsigned char code[GF_VM_PROG]; unsigned short size; } GF_HintProgram;
int gf_hint_run(GF_Glyph *g, const GF_HintProgram *p);
#endif
