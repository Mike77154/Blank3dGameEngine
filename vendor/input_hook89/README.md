# input_hook + robust backends (USB HID-style) 🧱

Core:
- `input_hook.h`
- `input_hook.c`

This package adds **multiple backends** that all implement:

```c
typedef struct ihk_backend {
    void *user;
    void (*poll_keyboard)(void *user, ihk_u8 *out_kb_bits, size_t out_bytes);
    void (*shutdown)(void *user); /* optional */
    ihk_u32 capabilities;
} ihk_backend;
```

Keyboard state is always exported as a **256-bit USB HID Keyboard Page (0x07) usage bitset**:
- usage `u` is DOWN if `out[u>>3] & (1<<(u&7))` is non-zero
- key code = `IHK_HID_KB(usage)` => `0x07uu`

---

## Backends included

### ✅ SDL2 (portable, simple)
Files:
- `input_hook_backend_sdl2.h/.c`

Notes:
- SDL scancodes already match HID usages (`SDL_SCANCODE_A == 0x04`, `SDL_SCANCODE_RETURN == 0x28`, etc.)
- Not global capture; depends on SDL focus.

Build:
- link SDL2

---

### ✅ Windows (simple poll) — GetAsyncKeyState
Files:
- `input_hook_backend_win32_async.h/.c`

Notes:
- “global-ish” in many cases
- does NOT perfectly differentiate some extended keys

Build:
- link `user32`

---

### ✅ Windows (robust global) — WH_KEYBOARD_LL
Files:
- `input_hook_backend_win32_llhook.h/.c`
- `input_hook_backend_win64_llhook.h/.c` (aliases)

Notes:
- Differentiates:
  - main Enter vs keypad Enter
  - left/right Ctrl, Alt
- Runs a dedicated message-loop thread internally

Build:
- link `user32`

---

### ✅ X11 (Linux desktops)
Files:
- `input_hook_backend_x11.h/.c`
- `ihk_kbmap_linux_evdev.h/.c`

Notes:
- Uses `XQueryKeymap()`
- Default assumes **evdev** mapping: `X11 keycode = linux keycode + 8` (common)
- You can override `evdev_offset` if your server is different
- Global inside the X server (not Wayland)

Build:
- `-lX11`

---

### ✅ Linux (evdev) — /dev/input/event*
Files:
- `input_hook_backend_linux_evdev.h/.c`
- `ihk_kbmap_linux_evdev.h/.c`

Notes:
- Truly global (works on Wayland too) **if** you have device permissions.
- Robust: handles `SYN_DROPPED` with a full resync via `EVIOCGKEY`.

Build:
- `-lpthread`
- Ensure permissions (root / input group / udev rule)

---

### ✅ macOS (robust global) — CGEventTap
Files:
- `input_hook_backend_macos_eventtap.h/.c`
- `input_hook_backend_macos32_eventtap.h/.c` (alias)
- `input_hook_backend_macos64_eventtap.h/.c` (alias)

Notes:
- Uses a session event tap to track key up/down.
- Requires Accessibility/Input Monitoring permission on modern macOS.
- Maps mac virtual keycodes (kVK_*) to HID usages.

Build:
- `-framework ApplicationServices -lpthread`

---

## Minimal usage examples

### SDL2
```c
#include "input_hook.h"
#include "input_hook_backend_sdl2.h"

static input_hook hook;
static ihk_sdl2_backend sdlb;

void init_input(void)
{
    ihk_backend be;
    ihk_sdl2_backend_init(&sdlb);
    ihk_sdl2_make_backend(&sdlb, &be);
    input_hook_init(&hook, &be);
}

void tick(void)
{
    input_hook_update(&hook);

    if (input_hook_pressed(&hook, IHK_HID_KB(0x2C))) {
        /* space */
    }
}
```

### Windows LL hook
```c
#include "input_hook.h"
#include "input_hook_backend_win32_llhook.h"

static input_hook hook;
static ihk_win32_llhook_backend winb;

void init_input(void)
{
    ihk_backend be;
    ihk_win32_llhook_backend_init(&winb);
    ihk_win32_llhook_make_backend(&winb, &be);
    input_hook_init(&hook, &be);
}

void shutdown_input(void)
{
    input_hook_shutdown(&hook); /* calls backend shutdown */
}
```

---

## ⚠️ Reality check (global capture)
The core is OS-agnostic, but **global capture** depends on backend + OS permissions:
- macOS: requires permissions
- Linux: evdev requires device permissions; Wayland blocks "X11-style global polling"
- Windows: global hooks generally work, but can be restricted by environment
