#include <stdio.h>
#include "reactiveloader.h"

static void cb_begin(RLD_Context *ctx, void *user)
{
    (void)user;
    printf("event: reload begin ent=%d weapon=%d\n", ctx->entity_id, ctx->weapon_id);
}

static void cb_sweep(RLD_Context *ctx, void *user)
{
    (void)ctx;
    (void)user;
    printf("event: active sweep begin\n");
}

static void cb_perfect(RLD_Context *ctx, void *user)
{
    (void)user;
    printf("event: PERFECT at %d%%\n", RLD_ToPercent(ctx->cursor_q16));
}

static void cb_good(RLD_Context *ctx, void *user)
{
    (void)user;
    printf("event: GOOD at %d%%\n", RLD_ToPercent(ctx->cursor_q16));
}

static void cb_fail(RLD_Context *ctx, void *user)
{
    (void)user;
    printf("event: FAIL at %d%%\n", RLD_ToPercent(ctx->cursor_q16));
}

static void cb_jam(RLD_Context *ctx, void *user)
{
    (void)ctx;
    (void)user;
    printf("event: JAM\n");
}

static void cb_normal(RLD_Context *ctx, void *user)
{
    (void)ctx;
    (void)user;
    printf("event: NORMAL reload\n");
}

static void cb_shell(RLD_Context *ctx, void *user)
{
    (void)user;
    printf("event: shell/unit loaded, ammo=%d\n", ctx->ammo_after);
}

static void cb_interrupt(RLD_Context *ctx, void *user)
{
    (void)user;
    printf("event: INTERRUPT, kept ammo=%d\n", ctx->ammo_after);
}

static void cb_done(RLD_Context *ctx, void *user)
{
    (void)user;
    printf("event: DONE result=%s ammo=%d ticks=%d bonus_left=%d\n",
           RLD_ResultName(ctx->result), ctx->ammo_after, ctx->total_ticks,
           ctx->bonus_ticks_left);
}

static void make_callbacks(RLD_Callbacks *cb)
{
    cb->on_reload_begin = cb_begin;
    cb->on_sweep_begin = cb_sweep;
    cb->on_good = cb_good;
    cb->on_perfect = cb_perfect;
    cb->on_fail = cb_fail;
    cb->on_normal = cb_normal;
    cb->on_jam = cb_jam;
    cb->on_shell_loaded = cb_shell;
    cb->on_interrupt = cb_interrupt;
    cb->on_done = cb_done;
    cb->on_cancel = 0;
    cb->user = 0;
}

static void run_pistol_perfect(void)
{
    RLD_Profile p;
    RLD_Context ctx;
    RLD_Callbacks cb;
    int tick;
    int input;

    printf("\n--- pistol perfect demo ---\n");
    RLD_MakePistolProfile(&p, 1);
    RLD_ContextInit(&ctx, 100, 1);
    make_callbacks(&cb);
    RLD_Begin(&ctx, &p, 3, 12);

    for (tick = 0; tick < 200 && RLD_IsBusy(&ctx) != 0; ++tick) {
        input = 0;
        if (ctx.state == RLD_STATE_ACTIVE_SWEEP) {
            if (RLD_ToPercent(ctx.cursor_q16) >= 58) {
                input = RLD_INPUT_RELOAD_PRESS;
            }
        }
        RLD_Tick(&ctx, &p, input, &cb);
    }
}

static void run_magnum_fail(void)
{
    RLD_Profile p;
    RLD_Context ctx;
    RLD_Callbacks cb;
    int tick;
    int input;

    printf("\n--- magnum fail/jam demo ---\n");
    RLD_MakeMagnumProfile(&p, 2);
    RLD_ContextInit(&ctx, 100, 2);
    make_callbacks(&cb);
    RLD_Begin(&ctx, &p, 0, 6);

    for (tick = 0; tick < 260 && RLD_IsBusy(&ctx) != 0; ++tick) {
        input = 0;
        if (ctx.state == RLD_STATE_ACTIVE_SWEEP) {
            if (RLD_ToPercent(ctx.cursor_q16) >= 20) {
                input = RLD_INPUT_RELOAD_PRESS;
            }
        }
        RLD_Tick(&ctx, &p, input, &cb);
    }
}

static void run_shotgun_interrupt(void)
{
    RLD_Profile p;
    RLD_Context ctx;
    RLD_Callbacks cb;
    int tick;
    int input;

    printf("\n--- shotgun shell-by-shell interrupt demo ---\n");
    RLD_MakeShotgunProfile(&p, 3);
    RLD_ContextInit(&ctx, 100, 3);
    make_callbacks(&cb);
    RLD_Begin(&ctx, &p, 2, 6);

    for (tick = 0; tick < 220 && RLD_IsBusy(&ctx) != 0; ++tick) {
        input = 0;
        if (ctx.state == RLD_STATE_ACTIVE_SWEEP) {
            if (RLD_ToPercent(ctx.cursor_q16) >= 58) {
                input = RLD_INPUT_RELOAD_PRESS;
            }
        }
        if (ctx.state == RLD_STATE_FINISHING && ctx.units_loaded >= 2) {
            input = RLD_INPUT_FIRE_PRESS;
        }
        RLD_Tick(&ctx, &p, input, &cb);
    }
}

int main(void)
{
    unsigned char arena_bytes[2048];
    RLD_Arena arena;
    RLD_ProfileBank bank;
    RLD_Profile temp;
    int rc;

    RLD_ArenaInit(&arena, arena_bytes, (unsigned long)sizeof(arena_bytes));
    rc = RLD_ProfileBankInitArena(&bank, &arena, 8);
    if (rc != 0) {
        printf("arena/profile bank error\n");
        return 1;
    }

    RLD_MakePistolProfile(&temp, 1);
    RLD_ProfileBankAdd(&bank, &temp);
    RLD_MakeShotgunProfile(&temp, 2);
    RLD_ProfileBankAdd(&bank, &temp);
    RLD_MakeMagnumProfile(&temp, 3);
    RLD_ProfileBankAdd(&bank, &temp);

    printf("Reactiveloader demo: profiles=%d\n", bank.count);
    run_pistol_perfect();
    run_magnum_fail();
    run_shotgun_interrupt();
    return 0;
}
