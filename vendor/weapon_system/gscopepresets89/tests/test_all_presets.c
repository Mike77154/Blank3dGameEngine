#include <stdio.h>
#include "gscopepresets89.h"

typedef struct test_state {
    unsigned long command_count;
    unsigned long glyph_count;
    unsigned long sprite_count;
    unsigned long invalid_count;
} test_state;

static void test_emit(void *user, const gsp89_draw_cmd *cmd)
{
    test_state *state;
    state = (test_state *)user;
    if (!state || !cmd) return;
    state->command_count += 1UL;
    if (cmd->kind == GSP89_CMD_GLYPH) state->glyph_count += 1UL;
    if (cmd->kind == GSP89_CMD_SPRITE) state->sprite_count += 1UL;
    if (cmd->kind < GSP89_CMD_LINE || cmd->kind > GSP89_CMD_DEBUG) {
        state->invalid_count += 1UL;
    }
}

int main(void)
{
    gsp89_painter painter;
    gsv89_palette palette;
    const gsvp89_preset *preset;
    test_state state;
    unsigned long before;
    short i;
    short failures;

    state.command_count = 0UL;
    state.glyph_count = 0UL;
    state.sprite_count = 0UL;
    state.invalid_count = 0UL;
    failures = 0;

    gsp89_painter_init(&painter, 1920, 1080, test_emit, &state);
    gsp89_painter_set_view(&painter, 960, 540, 500,
                           GSP89_FX_ONE, 0, 0, 255);

    for (i = 0; i < gsvp89_count(); ++i) {
        preset = gsvp89_get(i);
        if (!preset) {
            failures = (short)(failures + 1);
            continue;
        }
        if (!(preset->flags & GSVP89_FLAG_PURE_VECTOR)) {
            failures = (short)(failures + 1);
        }
        before = state.command_count;
        gsvp89_default_palette(preset, &palette);
        gsvp89_emit(&painter, preset, &palette, 255);
        if (state.command_count == before) {
            failures = (short)(failures + 1);
        }
    }

    if (!gsvp89_find("svd_pso1_dragunov")) failures = (short)(failures + 1);
    if (!gsvp89_find("psg1_hensoldt_6x42")) failures = (short)(failures + 1);
    if (!gsvp89_find("rpg7_pgo7")) failures = (short)(failures + 1);
    if (!gsvp89_find("rpg7_pgo7v3")) failures = (short)(failures + 1);
    if (state.glyph_count != 0UL) failures = (short)(failures + 1);
    if (state.sprite_count != 0UL) failures = (short)(failures + 1);
    if (state.invalid_count != 0UL) failures = (short)(failures + 1);

    printf("presets=%d commands=%lu painter_glyphs=%lu sprites=%lu failures=%d\n",
           (int)gsvp89_count(), state.command_count, state.glyph_count,
           state.sprite_count, (int)failures);

    return failures == 0 ? 0 : 1;
}
