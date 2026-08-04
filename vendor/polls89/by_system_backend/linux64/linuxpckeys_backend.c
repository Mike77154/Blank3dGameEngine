#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <linux/input.h>

#include "linuxpckeys_backend.h"

/* =========================================================...
   Linux backend (evdev)

   - Autodetecta un device tipo teclado
   - Usa EVIOCGKEY para leer un snapshot de teclas DOWN/UP
   - polls consumirá el estado via polls_button_state_fn
   ============================================================ */

#ifndef BITS_PER_LONG
#define BITS_PER_LONG (sizeof(unsigned long) * 8u)
#endif

static int test_bit(const unsigned long *array, int bit)
{
    unsigned int ubit;
    if (!array) return 0;
    if (bit < 0) return 0;
    ubit = (unsigned int)bit;
    return ( (array[ubit / BITS_PER_LONG] & (1UL << (ubit % BITS_PER_LONG))) != 0UL );
}

/* ============================================================
   key_pc_code -> Linux KEY_* (evdev)
   ============================================================ */

static int key_pc_to_evdev_code(key_pc_code key)
{
    switch (key) {
    case KEY_PC_NONE:
    case KEY_PC_ERROR_ROLLOVER:
    case KEY_PC_POST_FAIL:
    case KEY_PC_ERROR_UNDEFINED:
        return -1;

    /* Letras */
    case KEY_PC_A: return KEY_A;
    case KEY_PC_B: return KEY_B;
    case KEY_PC_C: return KEY_C;
    case KEY_PC_D: return KEY_D;
    case KEY_PC_E: return KEY_E;
    case KEY_PC_F: return KEY_F;
    case KEY_PC_G: return KEY_G;
    case KEY_PC_H: return KEY_H;
    case KEY_PC_I: return KEY_I;
    case KEY_PC_J: return KEY_J;
    case KEY_PC_K: return KEY_K;
    case KEY_PC_L: return KEY_L;
    case KEY_PC_M: return KEY_M;
    case KEY_PC_N: return KEY_N;
    case KEY_PC_O: return KEY_O;
    case KEY_PC_P: return KEY_P;
    case KEY_PC_Q: return KEY_Q;
    case KEY_PC_R: return KEY_R;
    case KEY_PC_S: return KEY_S;
    case KEY_PC_T: return KEY_T;
    case KEY_PC_U: return KEY_U;
    case KEY_PC_V: return KEY_V;
    case KEY_PC_W: return KEY_W;
    case KEY_PC_X: return KEY_X;
    case KEY_PC_Y: return KEY_Y;
    case KEY_PC_Z: return KEY_Z;

    /* Números fila superior */
    case KEY_PC_0: return KEY_0;
    case KEY_PC_1: return KEY_1;
    case KEY_PC_2: return KEY_2;
    case KEY_PC_3: return KEY_3;
    case KEY_PC_4: return KEY_4;
    case KEY_PC_5: return KEY_5;
    case KEY_PC_6: return KEY_6;
    case KEY_PC_7: return KEY_7;
    case KEY_PC_8: return KEY_8;
    case KEY_PC_9: return KEY_9;

    /* Principal */
    case KEY_PC_ENTER:     return KEY_ENTER;
    case KEY_PC_ESCAPE:    return KEY_ESC;
    case KEY_PC_BACKSPACE: return KEY_BACKSPACE;
    case KEY_PC_TAB:       return KEY_TAB;
    case KEY_PC_SPACE:     return KEY_SPACE;
    case KEY_PC_MINUS:     return KEY_MINUS;
    case KEY_PC_EQUAL:     return KEY_EQUAL;
    case KEY_PC_LBRACKET:  return KEY_LEFTBRACE;
    case KEY_PC_RBRACKET:  return KEY_RIGHTBRACE;
    case KEY_PC_BACKSLASH: return KEY_BACKSLASH;
    case KEY_PC_NONUS_HASH:
#ifdef KEY_NUMERIC_0
        return KEY_NUMERIC_0;
#else
        return KEY_BACKSLASH;
#endif
    case KEY_PC_SEMICOLON:  return KEY_SEMICOLON;
    case KEY_PC_APOSTROPHE: return KEY_APOSTROPHE;
    case KEY_PC_GRAVE:      return KEY_GRAVE;
    case KEY_PC_COMMA:      return KEY_COMMA;
    case KEY_PC_DOT:        return KEY_DOT;
    case KEY_PC_SLASH:      return KEY_SLASH;
    case KEY_PC_CAPSLOCK:   return KEY_CAPSLOCK;

    /* Funciones */
    case KEY_PC_F1:  return KEY_F1;
    case KEY_PC_F2:  return KEY_F2;
    case KEY_PC_F3:  return KEY_F3;
    case KEY_PC_F4:  return KEY_F4;
    case KEY_PC_F5:  return KEY_F5;
    case KEY_PC_F6:  return KEY_F6;
    case KEY_PC_F7:  return KEY_F7;
    case KEY_PC_F8:  return KEY_F8;
    case KEY_PC_F9:  return KEY_F9;
    case KEY_PC_F10: return KEY_F10;
    case KEY_PC_F11: return KEY_F11;
    case KEY_PC_F12: return KEY_F12;
#ifdef KEY_F13
    case KEY_PC_F13: return KEY_F13;
    case KEY_PC_F14: return KEY_F14;
    case KEY_PC_F15: return KEY_F15;
    case KEY_PC_F16: return KEY_F16;
    case KEY_PC_F17: return KEY_F17;
    case KEY_PC_F18: return KEY_F18;
    case KEY_PC_F19: return KEY_F19;
    case KEY_PC_F20: return KEY_F20;
    case KEY_PC_F21: return KEY_F21;
    case KEY_PC_F22: return KEY_F22;
    case KEY_PC_F23: return KEY_F23;
    case KEY_PC_F24: return KEY_F24;
#else
    case KEY_PC_F13:
    case KEY_PC_F14:
    case KEY_PC_F15:
    case KEY_PC_F16:
    case KEY_PC_F17:
    case KEY_PC_F18:
    case KEY_PC_F19:
    case KEY_PC_F20:
    case KEY_PC_F21:
    case KEY_PC_F22:
    case KEY_PC_F23:
    case KEY_PC_F24:
        return -1;
#endif

    /* Navegación */
    case KEY_PC_PRINT_SCREEN:
#ifdef KEY_SYSRQ
        return KEY_SYSRQ;
#else
        return KEY_PRINT;
#endif
    case KEY_PC_SCROLL_LOCK:
#ifdef KEY_SCROLLLOCK
        return KEY_SCROLLLOCK;
#else
        return KEY_SCROLLLOCK;
#endif
    case KEY_PC_PAUSE:     return KEY_PAUSE;
    case KEY_PC_INSERT:    return KEY_INSERT;
    case KEY_PC_HOME:      return KEY_HOME;
    case KEY_PC_PAGE_UP:   return KEY_PAGEUP;
    case KEY_PC_DELETE:    return KEY_DELETE;
    case KEY_PC_END:       return KEY_END;
    case KEY_PC_PAGE_DOWN: return KEY_PAGEDOWN;
    case KEY_PC_RIGHT:     return KEY_RIGHT;
    case KEY_PC_LEFT:      return KEY_LEFT;
    case KEY_PC_DOWN:      return KEY_DOWN;
    case KEY_PC_UP:        return KEY_UP;

    /* Keypad */
    case KEY_PC_NUM_LOCK: return KEY_NUMLOCK;
    case KEY_PC_KP_DIV:   return KEY_KPSLASH;
    case KEY_PC_KP_MUL:   return KEY_KPASTERISK;
    case KEY_PC_KP_SUB:   return KEY_KPMINUS;
    case KEY_PC_KP_ADD:   return KEY_KPPLUS;
    case KEY_PC_KP_ENTER: return KEY_KPENTER;
    case KEY_PC_KP_0:     return KEY_KP0;
    case KEY_PC_KP_1:     return KEY_KP1;
    case KEY_PC_KP_2:     return KEY_KP2;
    case KEY_PC_KP_3:     return KEY_KP3;
    case KEY_PC_KP_4:     return KEY_KP4;
    case KEY_PC_KP_5:     return KEY_KP5;
    case KEY_PC_KP_6:     return KEY_KP6;
    case KEY_PC_KP_7:     return KEY_KP7;
    case KEY_PC_KP_8:     return KEY_KP8;
    case KEY_PC_KP_9:     return KEY_KP9;
    case KEY_PC_KP_DOT:   return KEY_KPDOT;

    case KEY_PC_NONUS_BACKSLASH:
#ifdef KEY_102ND
        return KEY_102ND;
#else
        return KEY_BACKSLASH;
#endif

    case KEY_PC_APPLICATION:
#ifdef KEY_MENU
        return KEY_MENU;
#else
        return KEY_COMPOSE;
#endif

    case KEY_PC_POWER:
#ifdef KEY_POWER
        return KEY_POWER;
#else
        return -1;
#endif

    case KEY_PC_KP_EQUAL:
#ifdef KEY_KPEQUAL
        return KEY_KPEQUAL;
#else
        return -1;
#endif

    /* UI/Edit */
#ifdef KEY_EXECUTE
    case KEY_PC_EXECUTE: return KEY_EXECUTE;
#endif
#ifdef KEY_HELP
    case KEY_PC_HELP:    return KEY_HELP;
#endif
#ifdef KEY_MENU
    case KEY_PC_MENU:    return KEY_MENU;
#endif
#ifdef KEY_SELECT
    case KEY_PC_SELECT:  return KEY_SELECT;
#endif
#ifdef KEY_STOP
    case KEY_PC_STOP:    return KEY_STOP;
#endif
#ifdef KEY_AGAIN
    case KEY_PC_AGAIN:   return KEY_AGAIN;
#endif
#ifdef KEY_UNDO
    case KEY_PC_UNDO:    return KEY_UNDO;
#endif
#ifdef KEY_CUT
    case KEY_PC_CUT:     return KEY_CUT;
#endif
#ifdef KEY_COPY
    case KEY_PC_COPY:    return KEY_COPY;
#endif
#ifdef KEY_PASTE
    case KEY_PC_PASTE:   return KEY_PASTE;
#endif
#ifdef KEY_FIND
    case KEY_PC_FIND:    return KEY_FIND;
#endif

    /* Audio */
#ifdef KEY_MUTE
    case KEY_PC_MUTE:        return KEY_MUTE;
#endif
#ifdef KEY_VOLUMEUP
    case KEY_PC_VOLUME_UP:   return KEY_VOLUMEUP;
#endif
#ifdef KEY_VOLUMEDOWN
    case KEY_PC_VOLUME_DOWN: return KEY_VOLUMEDOWN;
#endif

    /* Locking: aproximación */
    case KEY_PC_LOCKING_CAPS_LOCK:   return KEY_CAPSLOCK;
    case KEY_PC_LOCKING_NUM_LOCK:    return KEY_NUMLOCK;
#ifdef KEY_SCROLLLOCK
    case KEY_PC_LOCKING_SCROLL_LOCK: return KEY_SCROLLLOCK;
#else
    case KEY_PC_LOCKING_SCROLL_LOCK: return -1;
#endif

    /* Sys */
#ifdef KEY_CANCEL
    case KEY_PC_CANCEL: return KEY_CANCEL;
#endif
#ifdef KEY_CLEAR
    case KEY_PC_CLEAR:  return KEY_CLEAR;
#endif
    case KEY_PC_PRIOR:  return KEY_PAGEUP;
    case KEY_PC_RETURN: return KEY_ENTER;

#ifdef KEY_CRSEL
    case KEY_PC_CRSEL:  return KEY_CRSEL;
#endif
#ifdef KEY_EXSEL
    case KEY_PC_EXSEL:  return KEY_EXSEL;
#endif

    /* Modificadores */
    case KEY_PC_LCTRL:  return KEY_LEFTCTRL;
    case KEY_PC_LSHIFT: return KEY_LEFTSHIFT;
    case KEY_PC_LALT:   return KEY_LEFTALT;
    case KEY_PC_LGUI:   return KEY_LEFTMETA;
    case KEY_PC_RCTRL:  return KEY_RIGHTCTRL;
    case KEY_PC_RSHIFT: return KEY_RIGHTSHIFT;
    case KEY_PC_RALT:   return KEY_RIGHTALT;
    case KEY_PC_RGUI:   return KEY_RIGHTMETA;

    /* El resto (intl/lang, keypad extendido, etc.) */
    default:
        return -1;
    }
}

/* ============================================================
   Autodetección de device de teclado
   ============================================================ */

static int fd_is_keyboard(int fd)
{
    unsigned long evbit[(EV_MAX + (int)BITS_PER_LONG) / (int)BITS_PER_LONG];
    unsigned long keybit[(KEY_MAX + (int)BITS_PER_LONG) / (int)BITS_PER_LONG];
    int has_ev_key;

    memset(evbit, 0, sizeof(evbit));
    memset(keybit, 0, sizeof(keybit));

    if (ioctl(fd, EVIOCGBIT(0, (int)sizeof(evbit)), evbit) < 0) {
        return 0;
    }

    has_ev_key = test_bit(evbit, EV_KEY);
    if (!has_ev_key) {
        return 0;
    }

    if (ioctl(fd, EVIOCGBIT(EV_KEY, (int)sizeof(keybit)), keybit) < 0) {
        return 0;
    }

    /* heurística: si soporta KEY_A y KEY_SPACE => probable teclado */
    if (test_bit(keybit, KEY_A) && test_bit(keybit, KEY_SPACE)) {
        return 1;
    }

    return 0;
}

static int open_first_by_path_kbd(void)
{
    DIR *d;
    struct dirent *ent;
    int fd = -1;

    d = opendir("/dev/input/by-path");
    if (!d) {
        return -1;
    }

    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;
        size_t len;

        if (!name) continue;
        len = strlen(name);
        if (len < 9) continue;

        /* termina con "-event-kbd" */
        if (len >= 9 && strcmp(name + (len - 9), "-event-kbd") == 0) {
            char path[512];
            int tryfd;

            /* construir path */
            path[0] = '\0';
            strncat(path, "/dev/input/by-path/", sizeof(path) - 1);
            strncat(path, name, sizeof(path) - strlen(path) - 1);

            tryfd = open(path, O_RDONLY | O_NONBLOCK);
            if (tryfd >= 0) {
                if (fd_is_keyboard(tryfd)) {
                    fd = tryfd;
                    break;
                }
                close(tryfd);
            }
        }
    }

    closedir(d);
    return fd;
}

static int open_first_eventkbd(void)
{
    DIR *d;
    struct dirent *ent;
    int fd = -1;

    d = opendir("/dev/input");
    if (!d) {
        return -1;
    }

    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;
        if (!name) continue;

        /* "event" prefix */
        if (strncmp(name, "event", 5) == 0) {
            char path[512];
            int tryfd;

            path[0] = '\0';
            strncat(path, "/dev/input/", sizeof(path) - 1);
            strncat(path, name, sizeof(path) - strlen(path) - 1);

            tryfd = open(path, O_RDONLY | O_NONBLOCK);
            if (tryfd >= 0) {
                if (fd_is_keyboard(tryfd)) {
                    fd = tryfd;
                    break;
                }
                close(tryfd);
            }
        }
    }

    closedir(d);
    return fd;
}

static int open_keyboard_device(void)
{
    int fd;
    fd = open_first_by_path_kbd();
    if (fd >= 0) {
        return fd;
    }
    return open_first_eventkbd();
}

/* ============================================================
   bindings helpers
   ============================================================ */

static int ensure_capacity(linuxpckeys_backend *kb, int required)
{
    key_pc_code *new_ptr;
    int new_cap;
    int i;

    if (!kb) return -1;
    if (required <= 0) return 0;

    if (kb->button_keys && kb->button_capacity >= required) {
        return 0;
    }

    new_cap = kb->button_capacity;
    if (new_cap < 8) new_cap = 8;
    while (new_cap < required) {
        new_cap *= 2;
        if (new_cap < 0) return -1;
    }

    new_ptr = (key_pc_code*)realloc(kb->button_keys, (size_t)new_cap * sizeof(key_pc_code));
    if (!new_ptr) return -1;

    for (i = kb->button_capacity; i < new_cap; ++i) {
        new_ptr[i] = KEY_PC_NONE;
    }

    kb->button_keys = new_ptr;
    kb->button_capacity = new_cap;
    return 0;
}

/* ============================================================
   polls_button_state_fn
   ============================================================ */

static int linuxpckeys_button_state(void *user_data, int button_index)
{
    linuxpckeys_backend *kb = (linuxpckeys_backend*)user_data;
    key_pc_code key;
    int code;

    if (!kb) return 0;
    if (button_index < 0 || button_index >= kb->button_capacity) return 0;
    if (!kb->button_keys) return 0;
    if (!kb->key_state) return 0;

    key = kb->button_keys[button_index];
    if (key == KEY_PC_NONE) return 0;

    code = key_pc_to_evdev_code(key);
    if (code < 0) return 0;
    if (code > KEY_MAX) return 0;

    return test_bit(kb->key_state, code);
}

/* ============================================================
   API
   ============================================================ */

void linuxpckeys_backend_init(linuxpckeys_backend *kb)
{
    int words;

    if (!kb) return;

    key_pc_init(&kb->key_ctx);

    kb->button_keys = NULL;
    kb->button_capacity = 0;

    kb->scanner.scanner = NULL;
    kb->scanner.max_buttons = 0;
    kb->scanner.connect = NULL;
    kb->scanner.update = NULL;
    kb->scanner.hold = NULL;
    kb->scanner.pressed = NULL;
    kb->scanner.released = NULL;
    kb->scanner_attached = 0;

    kb->fd = open_keyboard_device();

    /* reservar buffer para EVIOCGKEY */
    words = (KEY_MAX + (int)BITS_PER_LONG) / (int)BITS_PER_LONG;
    if (words < 1) words = 1;
    kb->key_state_words = words;
    kb->key_state = (unsigned long*)malloc((size_t)words * sizeof(unsigned long));
    if (kb->key_state) {
        memset(kb->key_state, 0, (size_t)words * sizeof(unsigned long));
    }
}

void linuxpckeys_backend_shutdown(linuxpckeys_backend *kb)
{
    if (!kb) return;

    if (kb->fd >= 0) {
        close(kb->fd);
        kb->fd = -1;
    }

    if (kb->button_keys) {
        free(kb->button_keys);
        kb->button_keys = NULL;
    }
    kb->button_capacity = 0;

    if (kb->key_state) {
        free(kb->key_state);
        kb->key_state = NULL;
    }
    kb->key_state_words = 0;

    kb->scanner_attached = 0;
}

int linuxpckeys_backend_attach_scanner(linuxpckeys_backend *kb,
                                     const polls_scanner_iface *scanner_iface)
{
    int rc;

    if (!kb || !scanner_iface) return -1;
    if (!scanner_iface->scanner) return -1;
    if (!scanner_iface->connect || !scanner_iface->update) return -1;
    if (!scanner_iface->hold || !scanner_iface->pressed || !scanner_iface->released) return -1;

    if (scanner_iface->max_buttons > 0) {
        if (ensure_capacity(kb, scanner_iface->max_buttons) != 0) {
            return -1;
        }
    }

    kb->scanner = *scanner_iface;

    rc = kb->scanner.connect(kb->scanner.scanner, linuxpckeys_button_state, (void*)kb);
    if (rc != 0) {
        kb->scanner_attached = 0;
        return -1;
    }

    kb->scanner_attached = 1;
    return 0;
}

int linuxpckeys_bind_button(linuxpckeys_backend *kb, int button_index, key_pc_code key)
{
    if (!kb) return -1;
    if (button_index < 0) return -1;

    if (ensure_capacity(kb, button_index + 1) != 0) {
        return -1;
    }

    kb->button_keys[button_index] = key;
    return 0;
}

void linuxpckeys_backend_update(linuxpckeys_backend *kb)
{
    if (!kb) return;
    if (!kb->scanner_attached) return;
    if (!kb->scanner.update) return;

    /* refrescar snapshot EVIOCGKEY */
    if (kb->fd >= 0 && kb->key_state && kb->key_state_words > 0) {
        (void)ioctl(kb->fd, EVIOCGKEY((int)((size_t)kb->key_state_words * sizeof(unsigned long))), kb->key_state);
    }

    kb->scanner.update(kb->scanner.scanner);
}

int linuxpckeys_button_hold(const linuxpckeys_backend *kb, int button_index)
{
    if (!kb) return 0;
    if (!kb->scanner_attached) return 0;
    if (!kb->scanner.hold) return 0;
    return kb->scanner.hold(kb->scanner.scanner, button_index);
}

int linuxpckeys_button_pressed(const linuxpckeys_backend *kb, int button_index)
{
    if (!kb) return 0;
    if (!kb->scanner_attached) return 0;
    if (!kb->scanner.pressed) return 0;
    return kb->scanner.pressed(kb->scanner.scanner, button_index);
}

int linuxpckeys_button_released(const linuxpckeys_backend *kb, int button_index)
{
    if (!kb) return 0;
    if (!kb->scanner_attached) return 0;
    if (!kb->scanner.released) return 0;
    return kb->scanner.released(kb->scanner.scanner, button_index);
}
