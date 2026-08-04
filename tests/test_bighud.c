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
    static Blank3DBigHud preset;
    static Blank3DEcgVitals ecg;
    Blank3DBigHudTelemetry telemetry;
    Blank3DBigHudNode *node;

    blank3d_ecg_vitals_init(&ecg);
    if (!blank3d_bighud_load(&hud, &ecg, "config/hud/gameplay.bighud"))
        return fail(blank3d_bighud_error(&hud));
    if (hud.node_count != 1) return fail("base gameplay HUD should contain only ECG");
    node = find_node(&hud, "vitals_ecg");
    if (!node || node->type != B3D_BIGHUD_NODE_ECG) return fail("ECG node missing");
    if (ecg.config.visible_cols != 32u ||
        ecg.config.active_render.signal_style != ECG_SIGNAL_STYLE_BARS ||
        ecg.config.active_render.glow_enabled != 0u)
        return fail("ECG visual surface changed");
    if (ecg.dynamics.fixed_offset != 58u ||
        ecg.dynamics.interval_numerator != 3200u ||
        ecg.dynamics.interval_min_ms != 18u ||
        ecg.dynamics.interval_max_ms != 58u)
        return fail("ECG dynamics changed");

    if (!blank3d_bighud_load(&preset, &ecg,
        "config/hud/presets/re5_radial_green_3d.bhud"))
        return fail(blank3d_bighud_error(&preset));
    if (preset.canvas_width != 108 || preset.canvas_height != 108)
        return fail("preset canvas was not parsed");
    node = find_node(&preset, "radial_health");
    if (!node || node->meter.kind != GBAR89_KIND_RADIAL_RING)
        return fail("radial preset missing");
    if (strcmp(node->bind, "numbar.value") != 0 ||
        strcmp(node->max_bind, "numbar.max") != 0)
        return fail("generic NumBar bindings missing");
    if ((node->meter.flags & GBAR89_FLAG_DAMAGE_LAG) == 0L ||
        (node->meter.style.fx_flags & GBAR89_FX_EXTRUDE) == 0UL)
        return fail("radial preset did not expose compositor features");

    memset(&telemetry, 0, sizeof(telemetry));
    telemetry.numbar_value = 73;
    telemetry.numbar_min = 0;
    telemetry.numbar_max = 100;
    telemetry.numbar_overlay = 16;
    telemetry.numbar_overlay_min = 0;
    telemetry.numbar_overlay_max = 100;
    blank3d_bighud_update_bar(node, &telemetry, 16u);
    if (node->meter.value != 73L || node->meter.max_value != 100L)
        return fail("generic runtime bindings failed");

    printf("BigVaderHudder base/preset split OK: base=%d preset=%d value=%ld\n",
           hud.node_count, preset.node_count, node->meter.value);
    return 0;
}
