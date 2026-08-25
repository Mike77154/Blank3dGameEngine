#include "spriteasset89_softblit.h"

int sa89_softblit_draw(void *user, const SA89_ImageView *view,
                       sa89_s32 src_x, sa89_s32 src_y, sa89_s32 src_w, sa89_s32 src_h,
                       sa89_s32 dst_x, sa89_s32 dst_y, sa89_u32 flags)
{
    SA89_SoftBlitTarget *dst;
    sa89_s32 x;
    sa89_s32 y;
    sa89_s32 sx;
    sa89_s32 sy;
    sa89_s32 dx;
    sa89_s32 dy;
    const sa89_u8 *sp;
    sa89_u8 *dp;
    sa89_u32 a;
    sa89_u32 ia;
    if (!user || !view || !view->pixels) return SA89_PROVIDER_ERROR;
    dst = (SA89_SoftBlitTarget *)user;
    if (!dst->pixels || src_w <= 0 || src_h <= 0) return SA89_PROVIDER_ERROR;
    for (y = 0; y < src_h; ++y) {
        sy = (flags & SA89_DRAW_FLIP_Y) ? (src_y + src_h - 1 - y) : (src_y + y);
        dy = dst_y + y;
        if (sy < 0 || dy < 0 || (sa89_u32)sy >= view->height || (sa89_u32)dy >= dst->height) continue;
        for (x = 0; x < src_w; ++x) {
            sx = (flags & SA89_DRAW_FLIP_X) ? (src_x + src_w - 1 - x) : (src_x + x);
            dx = dst_x + x;
            if (sx < 0 || dx < 0 || (sa89_u32)sx >= view->width || (sa89_u32)dx >= dst->width) continue;
            sp = view->pixels + (sa89_u32)sy * view->stride + (sa89_u32)sx * 4U;
            dp = dst->pixels + (sa89_u32)dy * dst->stride + (sa89_u32)dx * 4U;
            a = sp[3];
            ia = 255U - a;
            dp[0] = (sa89_u8)((sp[0] * a + dp[0] * ia + 127U) / 255U);
            dp[1] = (sa89_u8)((sp[1] * a + dp[1] * ia + 127U) / 255U);
            dp[2] = (sa89_u8)((sp[2] * a + dp[2] * ia + 127U) / 255U);
            dp[3] = (sa89_u8)(a + (dp[3] * ia + 127U) / 255U);
        }
    }
    return SA89_PROVIDER_HANDLED;
}
