#include "gf_hintvm.h"
static long gf_round_grid(long v) { return ((v + GF_FP_HALF) >> GF_FP_SHIFT) << GF_FP_SHIFT; }
static int pop(long *st, int *sp, long *v) { if (*sp <= 0) return -1; *v = st[--(*sp)]; return 0; }
int gf_hint_run(GF_Glyph *g, const GF_HintProgram *p) {
    long st[GF_VM_STACK]; int sp, pc; sp = 0; pc = 0;
    while (pc < (int)p->size) {
        unsigned char op = p->code[pc++]; long a,b; int idx;
        if (op == GF_OP_END) break;
        else if (op == GF_OP_PUSH) { if (pc + 4 > (int)p->size || sp >= GF_VM_STACK) return -1; a = (long)((signed char)p->code[pc]) | ((long)p->code[pc+1] << 8) | ((long)p->code[pc+2] << 16) | ((long)p->code[pc+3] << 24); pc += 4; st[sp++] = a; }
        else if (op == GF_OP_MOVE_X) { if (pop(st,&sp,&b)||pop(st,&sp,&a)) return -1; idx=(int)a; if(idx>=0&&idx<(int)g->point_count) g->points[idx].x += b; }
        else if (op == GF_OP_MOVE_Y) { if (pop(st,&sp,&b)||pop(st,&sp,&a)) return -1; idx=(int)a; if(idx>=0&&idx<(int)g->point_count) g->points[idx].y += b; }
        else if (op == GF_OP_ALIGN_X) { if (pop(st,&sp,&b)||pop(st,&sp,&a)) return -1; if(a>=0&&a<g->point_count&&b>=0&&b<g->point_count) g->points[(int)a].x = g->points[(int)b].x; }
        else if (op == GF_OP_ALIGN_Y) { if (pop(st,&sp,&b)||pop(st,&sp,&a)) return -1; if(a>=0&&a<g->point_count&&b>=0&&b<g->point_count) g->points[(int)a].y = g->points[(int)b].y; }
        else if (op == GF_OP_ROUND_X) { if (pop(st,&sp,&a)) return -1; idx=(int)a; if(idx>=0&&idx<(int)g->point_count) g->points[idx].x = gf_round_grid(g->points[idx].x); }
        else if (op == GF_OP_ROUND_Y) { if (pop(st,&sp,&a)) return -1; idx=(int)a; if(idx>=0&&idx<(int)g->point_count) g->points[idx].y = gf_round_grid(g->points[idx].y); }
        else if (op == GF_OP_SCALE_X) { if (pop(st,&sp,&b)||pop(st,&sp,&a)) return -1; idx=(int)a; if(idx>=0&&idx<(int)g->point_count) g->points[idx].x = gf_mul(g->points[idx].x, b); }
        else if (op == GF_OP_SCALE_Y) { if (pop(st,&sp,&b)||pop(st,&sp,&a)) return -1; idx=(int)a; if(idx>=0&&idx<(int)g->point_count) g->points[idx].y = gf_mul(g->points[idx].y, b); }
        else return -2;
    }
    gf_glyph_bbox(g); return 0;
}
