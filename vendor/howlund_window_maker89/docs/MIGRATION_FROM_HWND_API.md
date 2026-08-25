# Migration from the old HWND-like API

The old namespace visually collided with Microsoft's `HWND` native handle.
HOWM89 intentionally breaks that naming.

| Old | New |
|---|---|
| `HWND_Result` | `HOWM89_Result` |
| `HWND_Bool` | `HOWM89_Bool` |
| `HWND_Window` | `HOWM89_Window` |
| `HWND_PlatformVTable` | `HOWM89_BackendVTable` |
| `hwnd_register_platform` | `howm89_register_backend` |
| `hwnd_library_init` | `howm89_library_init` |
| `hwnd_window_create` | `howm89_window_create` |
| `hwnd_window_destroy` | `howm89_window_destroy` |
| `hwnd_window_set_size` | `howm89_window_set_size` |
| `hwnd_window_set_opacity(float)` | `howm89_window_set_opacity_q16(HOWM89_Fix)` |
| `hwnd_window_get_scale_factor(float*)` | `howm89_window_get_scale_factor_q16(HOWM89_Fix*)` |
| internal logical-size API | moved out of HOWM89 |

Backend callbacks now receive a `backend_user` pointer.
That makes backend state host-owned instead of forcing all providers to use global state.
