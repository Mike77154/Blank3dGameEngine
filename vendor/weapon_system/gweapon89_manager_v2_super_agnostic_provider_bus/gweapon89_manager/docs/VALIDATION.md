# Validation

## Strict builds

Validated with:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror
clang -std=c89 -pedantic -Wall -Wextra -Werror
```

## Runtime validation

`demo/provider_validation.c` installs 22 simultaneous providers and verifies:

- actor-state update interception and mutation propagation into later fire providers;
- external reserve ammo and magazine ownership;
- runtime numeric pellet and damage modification;
- flag provider participation;
- external actor transform, camera, zoom-ready input, and sockets;
- external raycast hit;
- projectile-life mutation before projectile spawn;
- projectile, health, muzzle, casing, trail, HUD, crosshair, scope, zoom, and draw fanout;
- event queue preserving provider mutations;
- stable provider handles after higher-priority registration;
- normal reload lifecycle;
- active-reload success;
- reload completion through external inventory.

`demo/legacy_validation.c` installs zero providers and verifies the original `GWP89_Hooks` projectile, muzzle, casing, trail, and pose fallbacks.

`demo/ini_validation.c` installs a memory-backed IO provider and verifies provider-oriented profile fields, including active-reload windows, through `gwp89_load_ini_file()`.

## Sanitizers

All three validation programs pass:

```sh
-fsanitize=address,undefined -fno-omit-frame-pointer
```

## Static analysis

`clang --analyze` completes without findings on `src/gweapon89.c`.

## Forbidden-feature scan

The core source contains no uses of or dependencies on:

```text
malloc calloc realloc free float double
stdio.h stdlib.h ctype.h math.h
```

The optional desktop IO adapter is intentionally allowed to include `stdio.h`; it lives outside the core boundary.

## Approximate default footprint

On the validation x86-64 build:

```text
sizeof(GWP89_Manager) = 168,864 bytes
sizeof(GWP89_ProviderSlot) = 72 bytes
sizeof(GWP89_Event) = 488 bytes
sizeof(GWP89_ProviderPacket) = 544 bytes
```

The manager footprint is dominated by the static 256-event queue. Reduce `GWP89_MAX_EVENTS`, `GWP89_MAX_WEAPONS`, or `GWP89_MAX_USERS` for smaller targets.
