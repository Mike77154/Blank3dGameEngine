# gfiremode89

Standalone C89 trigger automation extracted from `gweapon89_manager`.

It owns only:

- semi-automatic: one request per press;
- automatic: repeated requests while held;
- hold-once: one request until release;
- burst: a fixed number of accepted shots;
- integer millisecond cadence/cooldown.

It does **not** know about magazines, ammo, reloading, projectiles, actors, rendering, or physics.

The API is intentionally two-phase:

```c
result = gfm89_request(&fire, trigger_flags, dt_ms);
if (result == GFM89_FIRE_REQUEST) {
    if (weapon_can_really_fire) gfm89_commit_fire(&fire);
    else gfm89_reject_fire(&fire);
}
```

That prevents an empty magazine or active reload from incorrectly spending the fire cadence.

Build:

```sh
make test
```
