# Turn direction fix

`turn_left` and `turn_right` were inverted after the MovementBaseVerbs integration.

Blank3D/Gamlib3D use a right-handed transform convention with +Y up and local forward -Z. Under that convention:

- positive yaw turns left
- negative yaw turns right

The fix updates the MovementBaseVerbs built-in fallback, the Gamlib3D adapter, and the Blank3D semantic provider. Blank3D now delegates the sign convention to `blank3d_cameranaku_feed_left()` / `blank3d_cameranaku_feed_right()` rather than duplicating yaw sign logic.

DDSL2 bindings remain unchanged:

```text
If key_hold Q then turn_left
If key_hold E then turn_right
```
