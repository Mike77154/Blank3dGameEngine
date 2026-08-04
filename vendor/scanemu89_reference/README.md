# scanemu (C89)

**scanemu** is a tiny, backend-agnostic **capture library** for *symbolic hardware tokens*.

It is intentionally **NOT** a gameplay input library:
- No `key_down/held/released` state machine
- No polling API
- No backend dependency (SDL/Allegro/console SDK/etc)

Instead, scanemu solves one job extremely well:

> “Listen once, capture the next hardware input, store it under a symbolic name.”  
> Example: `jumpbutton = KEY:0:44`

This is ideal for **Options → Controls** menus, live binding, and writing `cfg/ini/save` values.

---

## Design

- You feed normalized events to scanemu: `scanemu_feed(ctx, &event)`
- If scanemu is listening, it captures the first acceptable token and stores it.
- Your engine/input layer uses the stored token later to map hardware → logical flags.

---

## Files

```
include/scanemu/scanemu.h
src/scanemu.c
examples/demo.c
```

---

## Minimal usage

```c
#include "scanemu/scanemu.h"

static scanemu_binding g_bindings[64];
static scanemu_ctx g_scan;

int main(void) {
  scanemu_init(&g_scan, g_bindings, 64);

  /* set defaults (example: KEY device 0, code 44) */
  {
    scanemu_token t;
    t.type = SCANEMU_T_KEY; t.device_id = 0; t.code = 44; t.value = 0.0f; t.extra = 0;
    scanemu_set(&g_scan, "jumpbutton", &t);
  }

  /* user clicks "Rebind Jump" */
  scanemu_listen(&g_scan, "jumpbutton", SCANEMU_LISTEN_ANY);

  /* backend delivers an event (simulate: SPACE press) */
  {
    scanemu_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.kind = SCANEMU_EV_PRESS;
    ev.token.type = SCANEMU_T_KEY;
    ev.token.device_id = 0;
    ev.token.code = 44;
    scanemu_feed(&g_scan, &ev);
  }

  return 0;
}
```

---

## Serialization

Use:
- `scanemu_token_to_string()` to save
- `scanemu_token_from_string()` to load

Examples:
- `KEY:0:44`
- `PADBTN:0:0`
- `PADAXIS+:0:2:0.50`

---

## Backend adapters

Your backend (SDL/Allegro/console) should convert events into `scanemu_event`:

- keyboard press → `SCANEMU_EV_PRESS + SCANEMU_T_KEY`
- mouse button → `SCANEMU_EV_PRESS + SCANEMU_T_MOUSE_BUTTON`
- gamepad axis move → `SCANEMU_EV_MOVE` with `token.code=axis_id` and `token.value` in `[-1..1]`

scanemu will store axis bindings as `PADAXIS+` or `PADAXIS-` with a threshold (default `0.5`).

---

## License

MIT
