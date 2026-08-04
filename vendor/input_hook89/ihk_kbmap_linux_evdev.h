/* ihk_kbmap_linux_evdev.h
   Linux input keycode (evdev) -> USB HID keyboard usage mapping helper.

   Used by:
     - input_hook_backend_linux_evdev
     - input_hook_backend_x11 (when X keycodes are evdev-based, common on modern Linux)

   Notes:
     - This maps *common keyboard keys* to HID usage IDs on page 0x07.
     - Unknown/unmapped codes return 0.
*/

#ifndef IHK_KBMAP_LINUX_EVDEV_H
#define IHK_KBMAP_LINUX_EVDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "input_hook.h"

/* Linux input keycode -> keyboard usage (0..255), or 0 if unknown */
ihk_u8 ihk_kb_usage_from_linux_keycode(unsigned int linux_keycode);

#ifdef __cplusplus
}
#endif

#endif /* IHK_KBMAP_LINUX_EVDEV_H */
