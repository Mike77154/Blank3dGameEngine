# Profile guide

A profile is declarative data. You can create your own `const GRecProfile` without touching solver code.

## Important fields

```txt
kick_pitch_vel       upward kick velocity per shot
kick_yaw_vel         base left/right kick velocity
kick_roll_vel        roll kick velocity
kick_side_vel        weapon model side offset velocity
kick_up_vel          weapon model upward offset velocity
kick_back_vel        weapon model backward offset velocity
random_pitch_vel     optional random pitch amplitude
random_yaw_vel       optional random yaw amplitude
angle_spring         how strongly aim/camera returns to neutral
angle_damping        how strongly angular velocity is damped after delay
angle_fire_damping   damping while recovery delay is active
pos_spring           viewmodel/weapon offset return strength
pos_damping          viewmodel/weapon damping after delay
pos_fire_damping     viewmodel/weapon damping while delay is active
max_pitch/yaw/roll   angular clamps
max_side/up/back     positional clamps
spread_per_shot      bloom added when firing
spread_max           spread clamp
spread_recover       spread decay per update tick
recovery_delay_ticks ticks before spring recovery pulls to neutral
burst_reset_ticks    ticks before burst index starts cooling down
pattern              optional pattern table
```

## Pistol feel

```txt
kick_pitch_vel: medium
kick_back_vel: medium
recovery_delay: short
angle_spring: medium
spread_per_shot: low
pattern: none
```

## Magnum feel

```txt
kick_pitch_vel: high
kick_back_vel: high
camera_scale: high
weapon_pos_scale: high
recovery_delay: medium
spread_per_shot: medium
```

## Rifle feel

```txt
kick_pitch_vel: medium
pattern: yes
random_yaw: small
spring: medium
burst_reset_ticks: medium
spread_per_shot: low/medium
```

## Shotgun feel

```txt
kick_pitch_vel: high
kick_back_vel: very high
weapon_pos_scale: high
spread_base: high
spread_per_shot: high
recovery_delay: medium/high
```

## Sniper feel

```txt
kick_pitch_vel: very high
camera_scale: very high
weapon_scale: lower if scope hides model
spread_base: very low
spread_per_shot: medium
recovery_delay: high
```

## Fixed camera / Resident Evil style

Use `GREC_MODE_FIXED_CAM`.

This keeps camera punch low and lets recoil affect:

- actual aim direction
- character/weapon mesh
- crosshair/laser spread
- optional micro screen shake in your engine

Do not slam the whole fixed camera around unless you want a stylized arcade effect.
