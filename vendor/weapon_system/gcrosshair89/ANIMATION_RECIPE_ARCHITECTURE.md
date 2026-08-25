# Animation Recipe Architecture

The original 192 preset/style/legacy-animation bytes are preserved. Event animation is a new additive layer.

Pipeline:

preset INI -> legacy style + legacy ranges -> event recipe -> anim89 -> core scale + DrawSpec modifiers -> providers/core

Automatic input edges: AIM enter/exit, FIRE rising edge, HIT rising edge, DISABLED enter/exit. Manual `custom1/custom2` and all standard events can be triggered with `gc89r_trigger_event()`.

Event keys: enabled, scale_target (neutral/micro/maxi/custom/current), scale_percent, rotation_deg, offset_x_px/y_px, alpha_percent, thickness_percent, dot_percent, attack_ticks, hold_ticks, return_ticks, attack_curve, return_curve, return_mode (none/start/neutral), repeat_count, retrigger.

Curves are fixed-point: linear, ease_in, ease_out, smoothstep, snap. No heap and no float/double.

All 192 preset INIs include one reusable animation profile from `recipes/animations/` while retaining their exact legacy `[animation]` values.
