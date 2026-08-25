#ifndef SPRITEASSET89_SOFTBLIT_H
#define SPRITEASSET89_SOFTBLIT_H
#include "spriteasset89.h"
typedef struct SA89_SoftBlitTarget_s {
    sa89_u8 *pixels;
    sa89_u32 width;
    sa89_u32 height;
    sa89_u32 stride;
} SA89_SoftBlitTarget;
int sa89_softblit_draw(void *user, const SA89_ImageView *view,
                       sa89_s32 src_x, sa89_s32 src_y, sa89_s32 src_w, sa89_s32 src_h,
                       sa89_s32 dst_x, sa89_s32 dst_y, sa89_u32 flags);
#endif
