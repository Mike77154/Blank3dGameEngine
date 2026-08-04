#ifdef __APPLE__

#include "input_hook_backend_macos_eventtap.h"

#include <string.h>
#include <stdlib.h>

#include <pthread.h>
#include <ApplicationServices/ApplicationServices.h>

/* C89: store pthread types in the struct as void* in header */
#define IHK_PTHREAD_T(ptr)   ((pthread_t*)(ptr))
#define IHK_MUTEX_T(ptr)     ((pthread_mutex_t*)(ptr))

/* Single-instance guard (event tap callback has no arbitrary user param in all paths we want) */
static ihk_macos_eventtap_backend *g_backend = 0;

/* Bit helpers */
static void ihk_set_usage_bit(ihk_u8 *bits, ihk_u8 usage)
{
    bits[(unsigned int)usage >> 3] = (ihk_u8)(bits[(unsigned int)usage >> 3] | (ihk_u8)(1u << ((unsigned int)usage & 7u)));
}
static void ihk_clear_usage_bit(ihk_u8 *bits, ihk_u8 usage)
{
    bits[(unsigned int)usage >> 3] = (ihk_u8)(bits[(unsigned int)usage >> 3] & (ihk_u8)~(ihk_u8)(1u << ((unsigned int)usage & 7u)));
}

/* Map macOS virtual keycode (kVK_*) to HID keyboard usage (page 0x07).
   Returns 0 if unknown/unmapped.
*/
static ihk_u8 ihk_usage_from_macos_keycode(int kc)
{
    switch (kc) {
    /* Letters */
    case 0:  return 0x04u; /* A */
    case 11: return 0x05u; /* B */
    case 8:  return 0x06u; /* C */
    case 2:  return 0x07u; /* D */
    case 14: return 0x08u; /* E */
    case 3:  return 0x09u; /* F */
    case 5:  return 0x0Au; /* G */
    case 4:  return 0x0Bu; /* H */
    case 34: return 0x0Cu; /* I */
    case 38: return 0x0Du; /* J */
    case 40: return 0x0Eu; /* K */
    case 37: return 0x0Fu; /* L */
    case 46: return 0x10u; /* M */
    case 45: return 0x11u; /* N */
    case 31: return 0x12u; /* O */
    case 35: return 0x13u; /* P */
    case 12: return 0x14u; /* Q */
    case 15: return 0x15u; /* R */
    case 1:  return 0x16u; /* S */
    case 17: return 0x17u; /* T */
    case 32: return 0x18u; /* U */
    case 9:  return 0x19u; /* V */
    case 13: return 0x1Au; /* W */
    case 7:  return 0x1Bu; /* X */
    case 16: return 0x1Cu; /* Y */
    case 6:  return 0x1Du; /* Z */

    /* Digits row */
    case 18: return 0x1Eu; /* 1 */
    case 19: return 0x1Fu; /* 2 */
    case 20: return 0x20u; /* 3 */
    case 21: return 0x21u; /* 4 */
    case 23: return 0x22u; /* 5 */
    case 22: return 0x23u; /* 6 */
    case 26: return 0x24u; /* 7 */
    case 28: return 0x25u; /* 8 */
    case 25: return 0x26u; /* 9 */
    case 29: return 0x27u; /* 0 */

    /* Controls */
    case 36: return 0x28u; /* Return */
    case 53: return 0x29u; /* Escape */
    case 51: return 0x2Au; /* Delete (Backspace) */
    case 48: return 0x2Bu; /* Tab */
    case 49: return 0x2Cu; /* Space */

    /* Punctuation / symbols (US) */
    case 27: return 0x2Du; /* - */
    case 24: return 0x2Eu; /* = */
    case 33: return 0x2Fu; /* [ */
    case 30: return 0x30u; /* ] */
    case 42: return 0x31u; /* \ */
    case 41: return 0x33u; /* ; */
    case 39: return 0x34u; /* ' */
    case 50: return 0x35u; /* ` */
    case 43: return 0x36u; /* , */
    case 47: return 0x37u; /* . */
    case 44: return 0x38u; /* / */

    case 57: return 0x39u; /* Caps Lock */

    /* Function keys */
    case 122: return 0x3Au; /* F1 */
    case 120: return 0x3Bu; /* F2 */
    case 99:  return 0x3Cu; /* F3 */
    case 118: return 0x3Du; /* F4 */
    case 96:  return 0x3Eu; /* F5 */
    case 97:  return 0x3Fu; /* F6 */
    case 98:  return 0x40u; /* F7 */
    case 100: return 0x41u; /* F8 */
    case 101: return 0x42u; /* F9 */
    case 109: return 0x43u; /* F10 */
    case 103: return 0x44u; /* F11 */
    case 111: return 0x45u; /* F12 */

    /* Nav cluster */
    case 114: return 0x49u; /* Help -> Insert */
    case 115: return 0x4Au; /* Home */
    case 116: return 0x4Bu; /* Page Up */
    case 117: return 0x4Cu; /* Forward Delete -> Delete */
    case 119: return 0x4Du; /* End */
    case 121: return 0x4Eu; /* Page Down */

    /* Arrows */
    case 124: return 0x4Fu; /* Right */
    case 123: return 0x50u; /* Left */
    case 125: return 0x51u; /* Down */
    case 126: return 0x52u; /* Up */

    /* Keypad */
    case 71: return 0x53u; /* Clear -> NumLock-ish */
    case 75: return 0x54u; /* KP / */
    case 67: return 0x55u; /* KP * */
    case 78: return 0x56u; /* KP - */
    case 69: return 0x57u; /* KP + */
    case 76: return 0x58u; /* KP Enter */
    case 83: return 0x59u; /* KP1 */
    case 84: return 0x5Au; /* KP2 */
    case 85: return 0x5Bu; /* KP3 */
    case 86: return 0x5Cu; /* KP4 */
    case 87: return 0x5Du; /* KP5 */
    case 88: return 0x5Eu; /* KP6 */
    case 89: return 0x5Fu; /* KP7 */
    case 91: return 0x60u; /* KP8 */
    case 92: return 0x61u; /* KP9 */
    case 82: return 0x62u; /* KP0 */
    case 65: return 0x63u; /* KP . */
    case 81: return 0x67u; /* KP = (HID 0x67) */

    /* Modifiers */
    case 59: return 0xE0u; /* LCtrl */
    case 56: return 0xE1u; /* LShift */
    case 58: return 0xE2u; /* LAlt/Option */
    case 55: return 0xE3u; /* LGUI/Command */
    case 62: return 0xE4u; /* RCtrl */
    case 60: return 0xE5u; /* RShift */
    case 61: return 0xE6u; /* RAlt/Option */
    case 54: return 0xE7u; /* RGUI/Command */

    default:
        break;
    }

    return 0;
}

static CGEventRef ihk_event_tap_cb(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *userInfo)
{
    (void)proxy;
    (void)userInfo;

    if (!g_backend || !g_backend->running) {
        return event;
    }

    /* If the tap is disabled by timeout/user input, try to re-enable */
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (g_backend->tap) {
            CGEventTapEnable((CFMachPortRef)g_backend->tap, true);
        }
        return event;
    }

    if (type == kCGEventKeyDown || type == kCGEventKeyUp || type == kCGEventFlagsChanged) {
        int is_down = (type == kCGEventKeyDown) ? 1 : 0;
        int kc = (int)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        ihk_u8 usage = ihk_usage_from_macos_keycode(kc);

        /* FlagsChanged events are used for modifier changes; treat them as down when present in flags. */
        if (type == kCGEventFlagsChanged) {
            /* We'll decide based on current flags for known modifier keycodes */
            CGEventFlags f = CGEventGetFlags(event);
            switch (kc) {
            case 56: /* LShift */
            case 60: /* RShift */
                is_down = (f & kCGEventFlagMaskShift) ? 1 : 0;
                break;
            case 59: /* LCtrl */
            case 62: /* RCtrl */
                is_down = (f & kCGEventFlagMaskControl) ? 1 : 0;
                break;
            case 58: /* LAlt */
            case 61: /* RAlt */
                is_down = (f & kCGEventFlagMaskAlternate) ? 1 : 0;
                break;
            case 55: /* LCmd */
            case 54: /* RCmd */
                is_down = (f & kCGEventFlagMaskCommand) ? 1 : 0;
                break;
            case 57: /* CapsLock */
                is_down = (f & kCGEventFlagMaskAlphaShift) ? 1 : 0;
                break;
            default:
                break;
            }
        }

        if (usage) {
            pthread_mutex_lock(IHK_MUTEX_T(g_backend->mutex));
            if (is_down) ihk_set_usage_bit(g_backend->kb_bits, usage);
            else         ihk_clear_usage_bit(g_backend->kb_bits, usage);
            pthread_mutex_unlock(IHK_MUTEX_T(g_backend->mutex));
        }
    }

    return event;
}

static void* ihk_macos_thread_main(void *arg)
{
    ihk_macos_eventtap_backend *b = (ihk_macos_eventtap_backend*)arg;
    CGEventMask mask;
    CFMachPortRef tap;
    CFRunLoopSourceRef src;
    CFRunLoopRef rl;

    mask = CGEventMaskBit(kCGEventKeyDown) |
           CGEventMaskBit(kCGEventKeyUp) |
           CGEventMaskBit(kCGEventFlagsChanged);

    tap = CGEventTapCreate(kCGSessionEventTap,
                           kCGHeadInsertEventTap,
                           0,
                           mask,
                           ihk_event_tap_cb,
                           0);

    if (!tap) {
        b->ok = 0;
        b->running = 0;
        return 0;
    }

    src = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
    if (!src) {
        CFRelease(tap);
        b->ok = 0;
        b->running = 0;
        return 0;
    }

    rl = CFRunLoopGetCurrent();

    b->tap = (void*)tap;
    b->source = (void*)src;
    b->runloop = (void*)rl;

    CFRunLoopAddSource(rl, src, kCFRunLoopCommonModes);
    CGEventTapEnable(tap, true);

    /* Run until stopped */
    while (b->running) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, true);
    }

    /* Cleanup */
    if (b->tap) {
        CGEventTapEnable((CFMachPortRef)b->tap, false);
    }
    if (b->source && b->runloop) {
        CFRunLoopRemoveSource((CFRunLoopRef)b->runloop, (CFRunLoopSourceRef)b->source, kCFRunLoopCommonModes);
    }

    if (b->source) {
        CFRelease((CFRunLoopSourceRef)b->source);
        b->source = 0;
    }
    if (b->tap) {
        CFRelease((CFMachPortRef)b->tap);
        b->tap = 0;
    }

    b->runloop = 0;
    b->ok = 0;
    b->running = 0;
    return 0;
}

static void ihk_macos_poll_keyboard(void *user, ihk_u8 *out_kb_bits, size_t out_bytes)
{
    ihk_macos_eventtap_backend *b = (ihk_macos_eventtap_backend*)user;
    if (!b || !out_kb_bits || out_bytes == 0) return;

    memset(out_kb_bits, 0, out_bytes);
    if (!b->ok) return;

    pthread_mutex_lock(IHK_MUTEX_T(b->mutex));
    memcpy(out_kb_bits, b->kb_bits, IHK_KB_BITS_BYTES);
    pthread_mutex_unlock(IHK_MUTEX_T(b->mutex));
}

static void ihk_macos_shutdown(void *user)
{
    ihk_macos_eventtap_backend *b = (ihk_macos_eventtap_backend*)user;
    if (!b) return;

    b->running = 0;

    /* Stop run loop if present */
    if (b->runloop) {
        CFRunLoopStop((CFRunLoopRef)b->runloop);
    }

    if (b->thread) {
        pthread_join(*IHK_PTHREAD_T(b->thread), 0);
    }

    if (b->mutex) {
        pthread_mutex_destroy(IHK_MUTEX_T(b->mutex));
        free(b->mutex);
        b->mutex = 0;
    }
    if (b->thread) {
        free(b->thread);
        b->thread = 0;
    }

    if (g_backend == b) g_backend = 0;

    b->ok = 0;
    b->tap = 0;
    b->source = 0;
    b->runloop = 0;
}

int ihk_macos_eventtap_backend_init(ihk_macos_eventtap_backend *b)
{
    pthread_t *t;
    pthread_mutex_t *m;

    if (!b) return 0;
    memset(b, 0, sizeof(*b));
    memset(b->kb_bits, 0, sizeof(b->kb_bits));

    if (g_backend) {
        /* one at a time */
        return 0;
    }
    g_backend = b;

    t = (pthread_t*)malloc(sizeof(pthread_t));
    m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!t || !m) {
        if (t) free(t);
        if (m) free(m);
        g_backend = 0;
        return 0;
    }

    pthread_mutex_init(m, 0);

    b->thread = (void*)t;
    b->mutex = (void*)m;
    b->ok = 1;
    b->running = 1;

    if (pthread_create(t, 0, ihk_macos_thread_main, b) != 0) {
        pthread_mutex_destroy(m);
        free(m);
        free(t);
        b->mutex = 0;
        b->thread = 0;
        b->ok = 0;
        b->running = 0;
        g_backend = 0;
        return 0;
    }

    return 1;
}

int ihk_macos_eventtap_backend_is_ok(const ihk_macos_eventtap_backend *b)
{
    return (b && b->ok) ? 1 : 0;
}

void ihk_macos_eventtap_make_backend(ihk_macos_eventtap_backend *b, ihk_backend *out)
{
    if (!out) return;
    out->user = b;
    out->poll_keyboard = ihk_macos_poll_keyboard;
    out->shutdown = ihk_macos_shutdown;
    out->capabilities = IHK_CAP_KEYBOARD | IHK_CAP_GLOBAL_CAPTURE | IHK_CAP_LAYOUT_INDEPENDENT;
}

#endif /* __APPLE__ */
