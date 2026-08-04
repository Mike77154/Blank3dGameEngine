# Blank3D vertical displacement and hopper leap prototype (v3.10.2)

## Player DDSL2

- `SPACE` emits `move_up=1`.
- `X` emits `move_down=1`.
- Downward movement clamps to the player's initial scene Y.
- `MOUSE_LEFT` remains the only shoot binding in the sample DDSL2.
- Startup config accepts `movement move 7 strafe 5.5 turn 140 vertical 4`.

These player commands remain direct world-Y displacement, intentionally without gravity.

## Hopper enemy: advancing leap

`hopper_enemy` uses `scripts/hopper_enemy.fpi`. It no longer jumps in place.
The leap is composed from two independent direct-displacement paths:

- X/Z pursuit: `moveforeflat=8`
- Y ascent/descent: `moveup=7` and `movedown=7`

The sample leap reaches 3 world units above the hopper's spawn floor. It advances
roughly 6.8 world units during a complete unobstructed ascent/descent cycle at
the configured rates.

The hopper uses a horizontal stop radius of 5 units:

1. farther than 5 units and grounded: take off toward the player;
2. below 3 units of height: continue advancing and ascending;
3. at the apex: switch to descent while advancing;
4. if it comes within 5 horizontal units while airborne: stop horizontal pursuit
   and finish landing vertically;
5. once grounded and close: face the player but do not start another leap;
6. if the player moves farther than 5 units again: begin another leap.

## FPIL planar-distance conditions

- `plrflatdistwithin=N` / `playerflatdistwithin=N`
- `plrflatdistfurther=N` / `playerflatdistfurther=N`

These ignore the Y difference and measure only the X/Z distance. This prevents an
airborne hopper from overshooting merely because its current height keeps the full
3D distance above the stop radius.

## FPIL planar-pursuit action

- `moveforeflat=N`
- alias: `movetowardplayerflat=N`

This moves toward the player's current X/Z position without modifying Y. Existing
`movefore=N` is unchanged and still performs full 3D pursuit, which is why ordinary
zombies can continue following a vertically displaced player.

## Existing vertical FPIL interface

Conditions:

- `grounded` / `onground`
- `airborne` / `offground`
- `heightbelow=N` / `verticalbelow=N`
- `heightatleast=N` / `verticalatleast=N`

Actions:

- `moveup=N` / `move_y_up=N`
- `movedown=N` / `move_y_down=N`

The prototype still uses direct fixed-point displacement rather than velocity,
gravity or a physical ballistic jump. This keeps the current test focused on
routing and controllable movement composition.
