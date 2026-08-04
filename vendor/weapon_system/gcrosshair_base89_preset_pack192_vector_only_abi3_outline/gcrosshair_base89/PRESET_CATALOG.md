# gcrosshair_base89 — catálogo de 192 presets

Los IDs **0–95** se preservan sin renumerar. Los IDs **96–191** son el paquete ABI 2 nuevo.

## Resumen

- Presets totales: **192**
- Presets heredados: **96**
- Presets nuevos: **96**
- Nuevos presets con figura ABI 2 no-cross: **57**
- Presets vectoriales: **192**
- Modos imagen/híbrido totales: **0**

## Familias nuevas ABI 2

- `cross`: 39
- `circle`: 14
- `square`: 6
- `diamond`: 5
- `chevrons`: 6
- `hexagon`: 7
- `brackets`: 11
- `open_triangle`: 8

## Presets 96–191

| ID | Enum | Nombre | Categoría | Figura | Spread |
|---:|---|---|---|---|---|
| 96 | `GCB89_PRESET_TAC_DOT_MICRO` | `tac_dot_micro` | precision | cross | none |
| 97 | `GCB89_PRESET_TAC_DOT_RING` | `tac_dot_ring` | precision | cross | none |
| 98 | `GCB89_PRESET_HITSCAN_SHORT_CROSS` | `hitscan_short_cross` | hitscan | cross | none |
| 99 | `GCB89_PRESET_HITSCAN_PIXEL_GAP` | `hitscan_pixel_gap` | hitscan | cross | none |
| 100 | `GCB89_PRESET_RAIL_DOT` | `rail_dot` | arena | cross | none |
| 101 | `GCB89_PRESET_RAIL_CROSS` | `rail_cross` | arena | cross | none |
| 102 | `GCB89_PRESET_BURST_RIFLE_TIGHT` | `burst_rifle_tight` | rifle | cross | gap |
| 103 | `GCB89_PRESET_BURST_RIFLE_DYNAMIC` | `burst_rifle_dynamic` | rifle | cross | gap |
| 104 | `GCB89_PRESET_RECOIL_COMPENSATOR_T` | `recoil_compensator_t` | tactical | cross | gap |
| 105 | `GCB89_PRESET_HEADSHOT_TINY` | `headshot_tiny` | precision | cross | none |
| 106 | `GCB89_PRESET_CENTER_POST_UP` | `center_post_up` | precision | cross | none |
| 107 | `GCB89_PRESET_CENTER_POST_DOWN` | `center_post_down` | precision | cross | none |
| 108 | `GCB89_PRESET_CS_STATIC_MICRO` | `cs_static_micro` | competitive | cross | none |
| 109 | `GCB89_PRESET_CS_STATIC_DENSE` | `cs_static_dense` | competitive | cross | none |
| 110 | `GCB89_PRESET_CS_STATIC_OPEN` | `cs_static_open` | competitive | cross | none |
| 111 | `GCB89_PRESET_CS_DYNAMIC_CLASSIC` | `cs_dynamic_classic` | competitive | cross | gap |
| 112 | `GCB89_PRESET_CS_DYNAMIC_BURST` | `cs_dynamic_burst` | competitive | cross | gap |
| 113 | `GCB89_PRESET_VALORANT_INNER_SHORT` | `valorant_inner_short` | competitive | cross | none |
| 114 | `GCB89_PRESET_VALORANT_INNER_OPEN` | `valorant_inner_open` | competitive | cross | none |
| 115 | `GCB89_PRESET_VALORANT_SQUARE_DOT` | `valorant_square_dot` | competitive | square | none |
| 116 | `GCB89_PRESET_QUAKE_PLUS` | `quake_plus` | arena | cross | gap |
| 117 | `GCB89_PRESET_QUAKE_CROSS_CIRCLE` | `quake_cross_circle` | arena | cross | gap |
| 118 | `GCB89_PRESET_ARENA_LIGHTNING_DOT` | `arena_lightning_dot` | tracking | cross | none |
| 119 | `GCB89_PRESET_ARENA_RAIL_CROSS` | `arena_rail_cross` | arena | cross | none |
| 120 | `GCB89_PRESET_TRACKER_RING_TINY` | `tracker_ring_tiny` | tracking | circle | none |
| 121 | `GCB89_PRESET_TRACKER_RING_DOT` | `tracker_ring_dot` | tracking | circle | none |
| 122 | `GCB89_PRESET_TRACKING_ELLIPSE` | `tracking_ellipse` | tracking | circle | shape_size |
| 123 | `GCB89_PRESET_BEAM_RING` | `beam_ring` | tracking | circle | shape_size |
| 124 | `GCB89_PRESET_PROJECTILE_LEAD_CIRCLE` | `projectile_lead_circle` | projectile | circle | shape_size |
| 125 | `GCB89_PRESET_PROJECTILE_LEAD_BROKEN` | `projectile_lead_broken` | projectile | circle | shape_size |
| 126 | `GCB89_PRESET_MELEE_LARGE_RING` | `melee_large_ring` | melee | circle | shape_size |
| 127 | `GCB89_PRESET_AOE_WIDE_RING` | `aoe_wide_ring` | aoe | circle | shape_size |
| 128 | `GCB89_PRESET_SHOTGUN_CIRCLE` | `shotgun_circle` | shotgun | circle | shape_size |
| 129 | `GCB89_PRESET_SHOTGUN_CIRCLE_DYNAMIC` | `shotgun_circle_dynamic` | shotgun | circle | shape_size |
| 130 | `GCB89_PRESET_CIRCLE_TOP_ARC` | `circle_top_arc` | circular | circle | none |
| 131 | `GCB89_PRESET_CIRCLE_BOTTOM_ARC` | `circle_bottom_arc` | circular | circle | none |
| 132 | `GCB89_PRESET_SHOTGUN_FOUR_POSTS` | `shotgun_four_posts` | shotgun | cross | gap |
| 133 | `GCB89_PRESET_SHOTGUN_WIDE_CROSS` | `shotgun_wide_cross` | shotgun | cross | gap |
| 134 | `GCB89_PRESET_SHOTGUN_BLOOM_CROSS` | `shotgun_bloom_cross` | shotgun | cross | gap |
| 135 | `GCB89_PRESET_SHOTGUN_SQUARE` | `shotgun_square` | shotgun | square | shape_size |
| 136 | `GCB89_PRESET_SHOTGUN_DIAMOND` | `shotgun_diamond` | shotgun | diamond | shape_size |
| 137 | `GCB89_PRESET_SHOTGUN_HEX` | `shotgun_hex` | shotgun | hexagon | shape_size |
| 138 | `GCB89_PRESET_BUCKSHOT_BRACKETS` | `buckshot_brackets` | shotgun | brackets | shape_size |
| 139 | `GCB89_PRESET_SLUG_PRECISION` | `slug_precision` | shotgun | cross | none |
| 140 | `GCB89_PRESET_HIPFIRE_HEAVY` | `hipfire_heavy` | hipfire | cross | gap |
| 141 | `GCB89_PRESET_HIPFIRE_VEHICLE` | `hipfire_vehicle` | vehicle | brackets | shape_size |
| 142 | `GCB89_PRESET_SPRAY_CONTROL` | `spray_control` | smg | cross | gap |
| 143 | `GCB89_PRESET_SMG_BLOOM` | `smg_bloom` | smg | cross | gap |
| 144 | `GCB89_PRESET_SQUARE_MICRO_VECTOR` | `square_micro_vector` | geometric | square | none |
| 145 | `GCB89_PRESET_SQUARE_DOT_VECTOR` | `square_dot_vector` | geometric | square | none |
| 146 | `GCB89_PRESET_SQUARE_BROKEN_VECTOR` | `square_broken_vector` | geometric | square | shape_size |
| 147 | `GCB89_PRESET_SQUARE_TOP_OPEN` | `square_top_open` | geometric | square | none |
| 148 | `GCB89_PRESET_DIAMOND_MICRO_VECTOR` | `diamond_micro_vector` | geometric | diamond | none |
| 149 | `GCB89_PRESET_DIAMOND_DOT_VECTOR` | `diamond_dot_vector` | geometric | diamond | none |
| 150 | `GCB89_PRESET_DIAMOND_BROKEN_VECTOR` | `diamond_broken_vector` | geometric | diamond | shape_size |
| 151 | `GCB89_PRESET_DIAMOND_HORIZONTAL` | `diamond_horizontal` | geometric | diamond | none |
| 152 | `GCB89_PRESET_HEX_MICRO_VECTOR` | `hex_micro_vector` | geometric | hexagon | none |
| 153 | `GCB89_PRESET_HEX_DOT_VECTOR` | `hex_dot_vector` | geometric | hexagon | none |
| 154 | `GCB89_PRESET_HEX_BROKEN_VECTOR` | `hex_broken_vector` | geometric | hexagon | shape_size |
| 155 | `GCB89_PRESET_HEX_WIDE_VECTOR` | `hex_wide_vector` | geometric | hexagon | shape_size |
| 156 | `GCB89_PRESET_CHEVRON_UP_MICRO_VECTOR` | `chevron_up_micro_vector` | chevron | chevrons | none |
| 157 | `GCB89_PRESET_CHEVRON_DOWN_MICRO_VECTOR` | `chevron_down_micro_vector` | chevron | chevrons | none |
| 158 | `GCB89_PRESET_CHEVRON_LEFT_VECTOR` | `chevron_left_vector` | chevron | chevrons | none |
| 159 | `GCB89_PRESET_CHEVRON_RIGHT_VECTOR` | `chevron_right_vector` | chevron | chevrons | none |
| 160 | `GCB89_PRESET_CHEVRON_DOUBLE_VERTICAL` | `chevron_double_vertical` | chevron | chevrons | shape_size |
| 161 | `GCB89_PRESET_CHEVRON_HORIZONTAL` | `chevron_horizontal` | chevron | chevrons | shape_size |
| 162 | `GCB89_PRESET_BRACKET_LR_TIGHT_VECTOR` | `bracket_lr_tight_vector` | brackets | brackets | none |
| 163 | `GCB89_PRESET_BRACKET_LR_WIDE_VECTOR` | `bracket_lr_wide_vector` | brackets | brackets | shape_size |
| 164 | `GCB89_PRESET_BRACKET_UD_VECTOR` | `bracket_ud_vector` | brackets | brackets | none |
| 165 | `GCB89_PRESET_BRACKET_FOUR_VECTOR` | `bracket_four_vector` | brackets | brackets | shape_size |
| 166 | `GCB89_PRESET_BRACKET_THREE_SIDED` | `bracket_three_sided` | brackets | brackets | none |
| 167 | `GCB89_PRESET_BRACKET_DYNAMIC_LOCK` | `bracket_dynamic_lock` | brackets | brackets | shape_size |
| 168 | `GCB89_PRESET_TRIANGLE_OPEN_UP_VECTOR` | `triangle_open_up_vector` | triangle | open_triangle | none |
| 169 | `GCB89_PRESET_TRIANGLE_OPEN_DOWN_VECTOR` | `triangle_open_down_vector` | triangle | open_triangle | none |
| 170 | `GCB89_PRESET_TRIANGLE_OPEN_LEFT_VECTOR` | `triangle_open_left_vector` | triangle | open_triangle | none |
| 171 | `GCB89_PRESET_TRIANGLE_OPEN_RIGHT_VECTOR` | `triangle_open_right_vector` | triangle | open_triangle | none |
| 172 | `GCB89_PRESET_TRIANGLE_CORNER_THREE` | `triangle_corner_three` | triangle | open_triangle | shape_size |
| 173 | `GCB89_PRESET_TRIANGLE_DOT_VECTOR` | `triangle_dot_vector` | triangle | open_triangle | none |
| 174 | `GCB89_PRESET_TRIANGLE_BROKEN_VECTOR` | `triangle_broken_vector` | triangle | open_triangle | shape_size |
| 175 | `GCB89_PRESET_TRIANGLE_SCOPE_POST` | `triangle_scope_post` | scope | open_triangle | none |
| 176 | `GCB89_PRESET_SCOPE_DUPLEX_VECTOR` | `scope_duplex_vector` | scope | cross | none |
| 177 | `GCB89_PRESET_SCOPE_MILDOT_VECTOR` | `scope_mildot_vector` | scope | cross | none |
| 178 | `GCB89_PRESET_SCOPE_RANGE_CROSS` | `scope_range_cross` | scope | cross | none |
| 179 | `GCB89_PRESET_SCOPE_RING_PRECISION` | `scope_ring_precision` | scope | cross | none |
| 180 | `GCB89_PRESET_ACCESSIBILITY_CYAN_LARGE` | `accessibility_cyan_large` | accessibility | cross | none |
| 181 | `GCB89_PRESET_ACCESSIBILITY_YELLOW_LARGE` | `accessibility_yellow_large` | accessibility | circle | none |
| 182 | `GCB89_PRESET_ACCESSIBILITY_MAGENTA_BOLD` | `accessibility_magenta_bold` | accessibility | brackets | none |
| 183 | `GCB89_PRESET_ACCESSIBILITY_WHITE_BOLD` | `accessibility_white_bold` | accessibility | cross | none |
| 184 | `GCB89_PRESET_COLOR_CHANGE_THREAT_VECTOR` | `color_change_threat_vector` | feedback | hexagon | none |
| 185 | `GCB89_PRESET_COLOR_CHANGE_CONFIRM_VECTOR` | `color_change_confirm_vector` | feedback | cross | none |
| 186 | `GCB89_PRESET_DYNAMIC_JUMP_BLOOM` | `dynamic_jump_bloom` | dynamic | circle | shape_size |
| 187 | `GCB89_PRESET_DYNAMIC_FIRE_BLOOM` | `dynamic_fire_bloom` | dynamic | cross | gap |
| 188 | `GCB89_PRESET_DYNAMIC_FULL_BLOOM` | `dynamic_full_bloom` | dynamic | brackets | shape_size |
| 189 | `GCB89_PRESET_MINIMALIST_NO_DOT_VECTOR` | `minimalist_no_dot_vector` | minimal | cross | none |
| 190 | `GCB89_PRESET_RETRO_HEX_NEON_VECTOR` | `retro_hex_neon_vector` | themed | hexagon | shape_size |
| 191 | `GCB89_PRESET_SCI_FI_LOCK_VECTOR` | `sci_fi_lock_vector` | themed | brackets | shape_size |

El detalle completo de color, máscaras, radios, rotación, cortes, comportamiento y animación está en `PRESET_CATALOG.csv`.
