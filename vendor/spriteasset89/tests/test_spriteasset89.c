#include "spriteasset89.h"
#include "spriteasset89_softblit.h"
#include <stdio.h>

static unsigned char image_rgba[8U * 4U * 4U];
static unsigned char target_rgba[8U * 8U * 4U];

static int test_acquire(void *user, const char *path, sa89_u32 *h, sa89_u32 *w, sa89_u32 *ht, sa89_u32 *fc)
{
    (void)user; (void)path;
    *h = 7U; *w = 8U; *ht = 4U; *fc = 1U;
    return SA89_PROVIDER_HANDLED;
}
static int test_frame(void *user, sa89_u32 h, sa89_u32 i, SA89_ImageView *v)
{
    (void)user; (void)i;
    if (h != 7U) return SA89_PROVIDER_ERROR;
    v->pixels = image_rgba; v->width = 8U; v->height = 4U; v->stride = 32U;
    return SA89_PROVIDER_HANDLED;
}
static void test_release(void *user, sa89_u32 h) { (void)user; (void)h; }

int main(void)
{
    SpriteAsset89 ctx;
    SA89_ImageProvider ip;
    SA89_RenderProvider rp;
    SA89_SoftBlitTarget target;
    sa89_id s, a, f0, f1, c, p;
    unsigned int i;
    for (i = 0U; i < sizeof(image_rgba); i += 4U) { image_rgba[i]=255U; image_rgba[i+1]=0U; image_rgba[i+2]=0U; image_rgba[i+3]=255U; }
    for (i = 0U; i < sizeof(target_rgba); ++i) target_rgba[i] = 0U;
    sa89_init(&ctx);
    ip.acquire=test_acquire; ip.get_frame=test_frame; ip.release=test_release; ip.user=0;
    target.pixels=target_rgba; target.width=8U; target.height=8U; target.stride=32U;
    rp.draw=sa89_softblit_draw; rp.user=&target;
    sa89_set_image_provider(&ctx,&ip); sa89_set_render_provider(&ctx,&rp);
    s=sa89_add_source(&ctx,"hero.png"); a=sa89_add_asset(&ctx,"hero",2,3);
    f0=sa89_add_frame(&ctx,s,0U,0,0,4,4,0,0,50U);
    f1=sa89_add_frame(&ctx,s,0U,0,0,4,4,1,0,50U);
    c=sa89_add_clip(&ctx,a,"run",f0,2U,SA89_LOOP_FORWARD);
    p=sa89_player_create(&ctx);
    if (s==SA89_INVALID_ID||a==SA89_INVALID_ID||f0==SA89_INVALID_ID||f1==SA89_INVALID_ID||c==SA89_INVALID_ID||p==SA89_INVALID_ID) return 2;
    if (!sa89_player_play(&ctx,p,a,"run")) return 3;
    sa89_player_step(&ctx,p,60U);
    if (ctx.players[p].frame_pos != 1U) return 4;
    sa89_player_set_position(&ctx,p,4,4);
    if (!sa89_player_render(&ctx,p)) return 5;
    if (target_rgba[(1U*32U)+(3U*4U)] != 255U) return 6;
    if (!sa89_define_gamemaker_strip(&ctx,"walk","default","walk_strip4.png",4U,80U,SA89_LOOP_FORWARD)) return 7;
    { sa89_id ga; sa89_id gc; ga=sa89_find_asset(&ctx,"walk"); if(ga==SA89_INVALID_ID)return 8; gc=sa89_find_clip(&ctx,ga,"default"); if(gc==SA89_INVALID_ID)return 9; if(ctx.clips[gc].frame_count!=4U)return 10; if(ctx.frames[ctx.clips[gc].first_frame].w!=2||ctx.frames[ctx.clips[gc].first_frame].h!=4)return 11; }
    printf("SpriteAsset89 PASS assets=%u clips=%u frames=%u\n",(unsigned)ctx.asset_count,(unsigned)ctx.clip_count,(unsigned)ctx.frame_count);
    sa89_reset(&ctx);
    return 0;
}
