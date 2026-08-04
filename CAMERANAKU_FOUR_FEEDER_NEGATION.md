# CamaraNaku four-feeder axis negation

This patch keeps `CNK_TRANSFORM_MODE_RECEIVE_PROVIDER` active. It does not
modify or disable the Gamlib3D move/scale/rotate callbacks.

The correction lives in the Blank3D-to-CamaraNaku input bridge and exposes four
explicit semantic feeders:

- `blank3d_cameranaku_feed_left`
- `blank3d_cameranaku_feed_right`
- `blank3d_cameranaku_feed_up`
- `blank3d_cameranaku_feed_down`

Each feeder applies the opposite sign required by the current
Gamlib3D/CamaraNaku convention boundary. Win32 mouse deltas remain raw and
signed; the main loop only selects which feeder to call.

Mapping:

```text
mouse left  -> feed_left  -> positive yaw delta
mouse right -> feed_right -> negative yaw delta
mouse up    -> feed_up    -> negative pitch delta
mouse down  -> feed_down  -> positive pitch delta
```

The weapon camera provider, recoil, sway, zoom, FPS/TPS solver and receive-provider
TRS mode remain enabled.
