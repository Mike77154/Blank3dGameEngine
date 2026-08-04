# greload89

Standalone C89 reload timer extracted from `gweapon89_manager`.

It owns only:

- accepting a reload command;
- integer millisecond duration and remaining time;
- active/completed state;
- reporting how many rounds the host should return to the magazine.

It deliberately does **not** touch a magazine or reserve inventory. This keeps it portable and lets the host decide whether reserve ammo is finite, external, infinite, per-shell, or magazine-swapped.

Typical flow:

```c
grel89_begin_full(&reload, reload_ms, current_rounds, capacity);
if (grel89_update(&reload, dt_ms) == GREL89_COMPLETED) {
    rounds_to_load = grel89_take_completed_rounds(&reload);
}
```

Build:

```sh
make test
```
