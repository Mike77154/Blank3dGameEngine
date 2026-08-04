#ifdef _WIN32

#include "input_hook_backend_win32_llhook.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string.h>
#include <stdlib.h>

/* Single-instance guard (WH_KEYBOARD_LL doesn't let us attach arbitrary user data) */
static ihk_win32_llhook_backend *g_backend = 0;

static void ihk_set_usage_bit(ihk_u8 *bits, ihk_u8 usage)
{
    bits[(unsigned int)usage >> 3] = (ihk_u8)(bits[(unsigned int)usage >> 3] | (ihk_u8)(1u << ((unsigned int)usage & 7u)));
}
static void ihk_clear_usage_bit(ihk_u8 *bits, ihk_u8 usage)
{
    bits[(unsigned int)usage >> 3] = (ihk_u8)(bits[(unsigned int)usage >> 3] & (ihk_u8)~(ihk_u8)(1u << ((unsigned int)usage & 7u)));
}

/* Map Windows scancode (+ extended) to HID keyboard usage.
   Uses Set 1 scancodes as reported by KBDLLHOOKSTRUCT.scanCode.
   Returns 0 if unknown.
*/
static ihk_u8 ihk_usage_from_scancode(DWORD vkCode, DWORD sc, int extended)
{
    /* Disambiguations using vkCode */
    if (vkCode == VK_SNAPSHOT) return 0x46u; /* Print Screen */
    if (vkCode == VK_PAUSE)    return 0x48u; /* Pause/Break */
    if (vkCode == VK_NUMLOCK)  return 0x53u; /* Num Lock */

    /* Modifiers via scancode + extended */
    if (sc == 0x1D) return extended ? 0xE4u : 0xE0u; /* RCTRL / LCTRL */
    if (sc == 0x38) return extended ? 0xE6u : 0xE2u; /* RALT / LALT */
    if (sc == 0x2A) return 0xE1u; /* LSHIFT */
    if (sc == 0x36) return 0xE5u; /* RSHIFT */

    /* Windows / Menu keys: often have unique VKs */
    if (vkCode == VK_LWIN)  return 0xE3u;
    if (vkCode == VK_RWIN)  return 0xE7u;
    if (vkCode == VK_APPS)  return 0x65u; /* Application/Menu */

    /* Enter vs KP Enter */
    if (sc == 0x1C) return extended ? 0x58u : 0x28u;

    /* Keypad slash uses sc 0x35 with extended */
    if (sc == 0x35 && extended) return 0x54u;

    /* PrintScreen sometimes appears as sc 0x37 extended; handled above by vkCode. */

    /* Keypad and nav share scancodes; use extended to pick nav cluster */
    switch (sc) {
    case 0x52: return extended ? 0x49u : 0x62u; /* Insert / KP0 */
    case 0x47: return extended ? 0x4Au : 0x5Fu; /* Home / KP7 */
    case 0x49: return extended ? 0x4Bu : 0x61u; /* PgUp / KP9 */
    case 0x53: return extended ? 0x4Cu : 0x63u; /* Delete / KP. */
    case 0x4F: return extended ? 0x4Du : 0x59u; /* End / KP1 */
    case 0x51: return extended ? 0x4Eu : 0x5Bu; /* PgDn / KP3 */
    case 0x4D: return extended ? 0x4Fu : 0x5Eu; /* Right / KP6 */
    case 0x4B: return extended ? 0x50u : 0x5Cu; /* Left / KP4 */
    case 0x50: return extended ? 0x51u : 0x5Au; /* Down / KP2 */
    case 0x48: return extended ? 0x52u : 0x60u; /* Up / KP8 */
    }

    /* Common scancode mapping */
    switch (sc) {
    /* Digits row */
    case 0x02: return 0x1Eu; /* 1 */
    case 0x03: return 0x1Fu; /* 2 */
    case 0x04: return 0x20u; /* 3 */
    case 0x05: return 0x21u; /* 4 */
    case 0x06: return 0x22u; /* 5 */
    case 0x07: return 0x23u; /* 6 */
    case 0x08: return 0x24u; /* 7 */
    case 0x09: return 0x25u; /* 8 */
    case 0x0A: return 0x26u; /* 9 */
    case 0x0B: return 0x27u; /* 0 */

    /* Control keys */
    case 0x01: return 0x29u; /* Esc */
    case 0x0E: return 0x2Au; /* Backspace */
    case 0x0F: return 0x2Bu; /* Tab */
    case 0x39: return 0x2Cu; /* Space */
    case 0x3A: return 0x39u; /* CapsLock */

    /* Punctuation / symbols (US) */
    case 0x0C: return 0x2Du; /* - */
    case 0x0D: return 0x2Eu; /* = */
    case 0x1A: return 0x2Fu; /* [ */
    case 0x1B: return 0x30u; /* ] */
    case 0x2B: return 0x31u; /* \ */
    case 0x27: return 0x33u; /* ; */
    case 0x28: return 0x34u; /* ' */
    case 0x29: return 0x35u; /* ` */
    case 0x33: return 0x36u; /* , */
    case 0x34: return 0x37u; /* . */
    case 0x35: return 0x38u; /* / (non-extended) */
    case 0x56: return 0x64u; /* Non-US \ | (often) */

    /* Letters (set 1 scancodes) */
    case 0x1E: return 0x04u; /* A */
    case 0x30: return 0x05u; /* B */
    case 0x2E: return 0x06u; /* C */
    case 0x20: return 0x07u; /* D */
    case 0x12: return 0x08u; /* E */
    case 0x21: return 0x09u; /* F */
    case 0x22: return 0x0Au; /* G */
    case 0x23: return 0x0Bu; /* H */
    case 0x17: return 0x0Cu; /* I */
    case 0x24: return 0x0Du; /* J */
    case 0x25: return 0x0Eu; /* K */
    case 0x26: return 0x0Fu; /* L */
    case 0x32: return 0x10u; /* M */
    case 0x31: return 0x11u; /* N */
    case 0x18: return 0x12u; /* O */
    case 0x19: return 0x13u; /* P */
    case 0x10: return 0x14u; /* Q */
    case 0x13: return 0x15u; /* R */
    case 0x1F: return 0x16u; /* S */
    case 0x14: return 0x17u; /* T */
    case 0x16: return 0x18u; /* U */
    case 0x2F: return 0x19u; /* V */
    case 0x11: return 0x1Au; /* W */
    case 0x2D: return 0x1Bu; /* X */
    case 0x15: return 0x1Cu; /* Y */
    case 0x2C: return 0x1Du; /* Z */

    /* Function keys */
    case 0x3B: return 0x3Au; /* F1 */
    case 0x3C: return 0x3Bu; /* F2 */
    case 0x3D: return 0x3Cu; /* F3 */
    case 0x3E: return 0x3Du; /* F4 */
    case 0x3F: return 0x3Eu; /* F5 */
    case 0x40: return 0x3Fu; /* F6 */
    case 0x41: return 0x40u; /* F7 */
    case 0x42: return 0x41u; /* F8 */
    case 0x43: return 0x42u; /* F9 */
    case 0x44: return 0x43u; /* F10 */
    case 0x57: return 0x44u; /* F11 */
    case 0x58: return 0x45u; /* F12 */

    /* Scroll lock (sc 0x46) */
    case 0x46: return 0x47u;

    /* Keypad operators (non-extended) */
    case 0x37: return 0x55u; /* KP * */
    case 0x4A: return 0x56u; /* KP - */
    case 0x4E: return 0x57u; /* KP + */
    case 0x4C: return 0x5Du; /* KP 5 */

    default:
        break;
    }

    /* F13..F24 via VK fallback */
    if (vkCode >= VK_F13 && vkCode <= VK_F24) {
        return (ihk_u8)(0x68u + (ihk_u8)(vkCode - VK_F13));
    }

    return 0;
}

static LRESULT CALLBACK ihk_ll_keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && g_backend && g_backend->running) {
        const KBDLLHOOKSTRUCT *k = (const KBDLLHOOKSTRUCT*)lParam;
        int is_down = 0;
        int extended = 0;
        ihk_u8 usage = 0;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) is_down = 1;
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) is_down = 0;

        extended = (k->flags & LLKHF_EXTENDED) ? 1 : 0;

        usage = ihk_usage_from_scancode(k->vkCode, k->scanCode, extended);
        if (usage) {
            EnterCriticalSection((CRITICAL_SECTION*)g_backend->cs);
            if (is_down) {
                ihk_set_usage_bit(g_backend->kb_bits, usage);
            } else {
                ihk_clear_usage_bit(g_backend->kb_bits, usage);
            }
            LeaveCriticalSection((CRITICAL_SECTION*)g_backend->cs);
        }
    }

    return CallNextHookEx(0, nCode, wParam, lParam);
}

static DWORD WINAPI ihk_ll_thread_main(LPVOID param)
{
    ihk_win32_llhook_backend *b = (ihk_win32_llhook_backend*)param;
    MSG msg;

    b->thread_id = GetCurrentThreadId();

    b->hook = (void*)SetWindowsHookExA(WH_KEYBOARD_LL, ihk_ll_keyboard_proc, GetModuleHandleA(0), 0);
    if (!b->hook) {
        b->ok = 0;
        b->running = 0;
        return 0;
    }

    /* Message loop required for LL hooks */
    while (b->running && GetMessageA(&msg, 0, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (b->hook) {
        UnhookWindowsHookEx((HHOOK)b->hook);
        b->hook = 0;
    }

    b->ok = 0;
    b->running = 0;
    return 0;
}

static void ihk_win32_llhook_poll_keyboard(void *user, ihk_u8 *out_kb_bits, size_t out_bytes)
{
    ihk_win32_llhook_backend *b = (ihk_win32_llhook_backend*)user;
    if (!b || !out_kb_bits || out_bytes == 0) return;

    memset(out_kb_bits, 0, out_bytes);
    if (!b->ok) return;

    EnterCriticalSection((CRITICAL_SECTION*)b->cs);
    memcpy(out_kb_bits, b->kb_bits, IHK_KB_BITS_BYTES);
    LeaveCriticalSection((CRITICAL_SECTION*)b->cs);
}

static void ihk_win32_llhook_shutdown(void *user)
{
    ihk_win32_llhook_backend *b = (ihk_win32_llhook_backend*)user;
    if (!b) return;

    b->running = 0;

    if (b->thread_id) {
        PostThreadMessageA((DWORD)b->thread_id, WM_QUIT, 0, 0);
    }

    if (b->thread) {
        WaitForSingleObject((HANDLE)b->thread, INFINITE);
        CloseHandle((HANDLE)b->thread);
        b->thread = 0;
    }

    if (b->cs) {
        DeleteCriticalSection((CRITICAL_SECTION*)b->cs);
        free(b->cs);
        b->cs = 0;
    }

    if (g_backend == b) {
        g_backend = 0;
    }

    b->ok = 0;
    b->thread_id = 0;
}

int ihk_win32_llhook_backend_init(ihk_win32_llhook_backend *b)
{
    CRITICAL_SECTION *cs;

    if (!b) return 0;
    memset(b, 0, sizeof(*b));
    memset(b->kb_bits, 0, sizeof(b->kb_bits));

    if (g_backend != 0) {
        /* One instance at a time */
        return 0;
    }
    g_backend = b;

    cs = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    if (!cs) {
        g_backend = 0;
        return 0;
    }

    InitializeCriticalSection(cs);
    b->cs = (void*)cs;

    b->ok = 1;
    b->running = 1;
    b->thread = (void*)CreateThread(0, 0, ihk_ll_thread_main, b, 0, 0);
    if (!b->thread) {
        DeleteCriticalSection(cs);
        free(cs);
        b->cs = 0;
        b->ok = 0;
        b->running = 0;
        g_backend = 0;
        return 0;
    }

    return 1;
}

int ihk_win32_llhook_backend_is_ok(const ihk_win32_llhook_backend *b)
{
    return (b && b->ok) ? 1 : 0;
}

void ihk_win32_llhook_make_backend(ihk_win32_llhook_backend *b, ihk_backend *out)
{
    if (!out) return;
    out->user = b;
    out->poll_keyboard = ihk_win32_llhook_poll_keyboard;
    out->shutdown = ihk_win32_llhook_shutdown;
    out->capabilities = IHK_CAP_KEYBOARD | IHK_CAP_GLOBAL_CAPTURE | IHK_CAP_LAYOUT_INDEPENDENT;
}

#endif /* _WIN32 */
