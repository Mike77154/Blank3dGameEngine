# gmagazine89

Standalone C89 magazine accounting extracted from `gweapon89_manager`.

It owns only:

- magazine capacity;
- rounds currently loaded;
- rounds consumed per shot;
- empty/full/can-fire reports;
- loading, filling, and spending rounds.

It does **not** own reserve inventory, trigger behavior, reload timers, actors, projectiles, or rendering.

A capacity of `0` means “no internal magazine”; spending returns `GMAG89_BYPASS` so an external ammo provider can handle the shot.

Build:

```sh
make test
```
