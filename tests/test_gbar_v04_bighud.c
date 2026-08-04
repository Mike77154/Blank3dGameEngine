#include "blank3d_bighud.h"

#include <stdio.h>
#include <string.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static Blank3DBigHudNode *find_node(Blank3DBigHud *hud, const char *name)
{
    int i;
    if (!hud || !name) return (Blank3DBigHudNode *)0;
    for (i = 0; i < hud->node_count; ++i) {
        if (strcmp(hud->nodes[i].name, name) == 0) return &hud->nodes[i];
    }
    return (Blank3DBigHudNode *)0;
}

int main(void)
{
    static Blank3DBigHud hud;
    static Blank3DEcgVitals ecg;
    static GBar89_CommandBuffer commands;
    GBar89_RenderOps ops;
    Blank3DBigHudTelemetry telemetry;
    Blank3DBigHudNode *vertical;
    Blank3DBigHudNode *radial;
    Blank3DBigHudNode *layered;
    Blank3DBigHudNode *sprite;
    unsigned long radial_fx;
    int phase_before;

    if (GBAR89_VERSION_MAJOR != 0 || GBAR89_VERSION_MINOR != 4)
        return fail("Blank3D is not compiling GBar89 v0.4");

    blank3d_ecg_vitals_init(&ecg);
    if (!blank3d_bighud_load(&hud, &ecg, "tests/data/hud/gbar_v04_full.bighud"))
        return fail(blank3d_bighud_error(&hud));
    if (hud.node_count != 4) return fail("expected four v0.4 test nodes");

    vertical = find_node(&hud, "vertical_custom");
    radial = find_node(&hud, "radial_full");
    layered = find_node(&hud, "layered_full");
    sprite = find_node(&hud, "sprite_full");
    if (!vertical || !radial || !layered || !sprite) return fail("test nodes missing");

    if (vertical->meter.direction != GBAR89_DIR_BOTTOM_TO_TOP)
        return fail("vertical direction was not retained");
    if (vertical->meter.style.frame_kind != GBAR89_FRAME_RAIL ||
        vertical->meter.style.bg_kind != GBAR89_BG_SCANLINES)
        return fail("vertical frame/background surface missing");
    if ((vertical->meter.flags & GBAR89_FLAG_PIXEL_QUANTIZE) == 0L ||
        (vertical->meter.flags & GBAR89_FLAG_DRAW_MID_VALUE) == 0L ||
        (vertical->meter.flags & GBAR89_FLAG_DRAW_VECTOR_UNITS) == 0L)
        return fail("vertical v0.4 flags missing");
    if (vertical->unit_point_count != 4 ||
        vertical->meter.unit_point_count != 4 ||
        vertical->meter.unit_points != vertical->unit_points)
        return fail("custom unit vector was not retained after BVH sort");
    if (vertical->meter.unit_closed != 1 || vertical->meter.unit_filled != 1)
        return fail("boolean unit controls missing");

    radial_fx = GBAR89_FX_OUTER_OUTLINE |
                GBAR89_FX_DROP_SHADOW |
                GBAR89_FX_EXTRUDE |
                GBAR89_FX_INNER_SHADOW |
                GBAR89_FX_GLOSS |
                GBAR89_FX_FILL_GRID |
                GBAR89_FX_FILL_SCANLINES |
                GBAR89_FX_PIXEL_CELLS |
                GBAR89_FX_RADIAL_SPOKES |
                GBAR89_FX_RADIAL_RINGS |
                GBAR89_FX_RADIAL_TICKS |
                GBAR89_FX_RADIAL_SWEEP_HIGHLIGHT;
    if (radial->meter.kind != GBAR89_KIND_RADIAL_RING ||
        radial->meter.style.radial_segments != 24 ||
        radial->meter.style.radial_gap_deg != 2 ||
        radial->meter.style.radial_cap_kind != GBAR89_RADIAL_CAP_ROUND)
        return fail("radial parity fields missing");
    if ((radial->meter.style.fx_flags & radial_fx) != radial_fx)
        return fail("full compositor/radial FX surface missing");
    if (radial->meter.style.frame_kind != GBAR89_FRAME_BEVEL_OUT ||
        radial->meter.style.bg_kind != GBAR89_BG_GRID)
        return fail("radial frame/background surface missing");
    if (radial->meter.unit_point_count < 3 ||
        radial->meter.unit_points != radial->unit_points)
        return fail("built-in vector unit shape missing");

    if ((layered->meter.flags & GBAR89_FLAG_LAYERED) == 0L ||
        (layered->meter.flags & GBAR89_FLAG_DRAW_LAYER_PIPS) == 0L)
        return fail("layer controls missing");
    if (sprite->meter.kind != GBAR89_KIND_NINESLICE ||
        sprite->meter.style.sprite_bg != 1 ||
        sprite->meter.style.src_partial.w != 32 ||
        sprite->meter.style.margin_left != 4 ||
        sprite->meter.style.padding_top != 2)
        return fail("sprite/nineslice property surface missing");

    memset(&telemetry, 0, sizeof(telemetry));
    telemetry.player_health = 73;
    telemetry.player_health_max = 100;
    telemetry.weapon_loaded = 3;
    telemetry.weapon_capacity = 15;
    telemetry.weapon_reserve = 86;
    telemetry.gameplay_threat = 55;

    blank3d_bighud_update_bar(vertical, &telemetry, 16u);
    if (vertical->meter.value != 73L || vertical->meter.max_value != 100L ||
        vertical->meter.segments != 15 || vertical->meter.unit_count != 15 ||
        vertical->meter.mid_value != 86L)
        return fail("vertical dynamic bindings failed");

    phase_before = radial->meter.style.radial_phase_deg;
    blank3d_bighud_update_bar(radial, &telemetry, 1000u);
    if (radial->meter.overlay_value != 55L || radial->meter.mid_value != 86L ||
        radial->meter.unit_count != 15)
        return fail("radial dynamic bindings failed");
    if (radial->meter.style.radial_phase_deg == phase_before)
        return fail("radial phase animation did not advance");

    blank3d_bighud_update_bar(layered, &telemetry, 16u);
    if (layered->meter.layer_count != 3 || layered->meter.layer_size != 86L)
        return fail("dynamic layer bindings failed");

    gbar89_set_rect(&radial->meter, 0, 0, 144, 144);
    gbar89_command_buffer_init(&commands);
    ops = gbar89_command_buffer_make_ops(&commands);
    gbar89_draw(&radial->meter, &ops);
    if (commands.command_count <= 0 || commands.vertex_count <= 0 ||
        commands.index_count <= 0)
        return fail("radial compositor emitted no geometry");
    if (gbar89_command_buffer_overflowed(&commands)) {
        fprintf(stderr, "overflow counts: commands=%d/%d vertices=%d/%d indices=%d/%d\n",
                commands.command_count, GBAR89_MAX_COMMANDS,
                commands.vertex_count, GBAR89_MAX_COMMAND_VERTICES,
                commands.index_count, GBAR89_MAX_COMMAND_INDICES);
        return fail("radial compositor overflowed fixed command buffer");
    }

    printf("GBar89 v0.4/BVH full surface OK: nodes=%d commands=%d vertices=%d indices=%d phase=%d\n",
           hud.node_count, commands.command_count, commands.vertex_count,
           commands.index_count, radial->meter.style.radial_phase_deg);
    return 0;
}
