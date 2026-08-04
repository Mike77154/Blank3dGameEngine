#ifdef __linux__

#include "input_hook_backend_linux_evdev.h"
#include "ihk_kbmap_linux_evdev.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <pthread.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <linux/input.h>

/* C89: store pthread types in the struct as void* in header */
#define IHK_PTHREAD_T(ptr)   ((pthread_t*)(ptr))
#define IHK_MUTEX_T(ptr)     ((pthread_mutex_t*)(ptr))

/* Bit helpers */
static void ihk_set_usage_bit(ihk_u8 *bits, ihk_u8 usage)
{
    bits[(unsigned int)usage >> 3] = (ihk_u8)(bits[(unsigned int)usage >> 3] | (ihk_u8)(1u << ((unsigned int)usage & 7u)));
}
static void ihk_clear_usage_bit(ihk_u8 *bits, ihk_u8 usage)
{
    bits[(unsigned int)usage >> 3] = (ihk_u8)(bits[(unsigned int)usage >> 3] & (ihk_u8)~(ihk_u8)(1u << ((unsigned int)usage & 7u)));
}

#define IHK_WORD_BITS   ((int)(sizeof(unsigned long) * 8u))
#define IHK_TEST_BIT(bit, arr) (((arr)[(bit) / IHK_WORD_BITS] >> ((bit) % IHK_WORD_BITS)) & 1ul)

static int ihk_fd_looks_like_keyboard(int fd)
{
    unsigned long evbits[(EV_MAX + IHK_WORD_BITS) / IHK_WORD_BITS];
    unsigned long keybits[(KEY_MAX + IHK_WORD_BITS) / IHK_WORD_BITS];
    int r;

    memset(evbits, 0, sizeof(evbits));
    r = ioctl(fd, EVIOCGBIT(0, (int)sizeof(evbits)), evbits);
    if (r < 0) return 0;
    if (!IHK_TEST_BIT(EV_KEY, evbits)) return 0;

    memset(keybits, 0, sizeof(keybits));
    r = ioctl(fd, EVIOCGBIT(EV_KEY, (int)sizeof(keybits)), keybits);
    if (r < 0) return 0;

    /* Heuristic: must have A, Z and SPACE */
    if (!IHK_TEST_BIT(KEY_A, keybits)) return 0;
    if (!IHK_TEST_BIT(KEY_Z, keybits)) return 0;
    if (!IHK_TEST_BIT(KEY_SPACE, keybits)) return 0;

    return 1;
}

static int ihk_open_first_kbd(char *out_path, size_t out_sz)
{
    DIR *d;
    struct dirent *ent;

    /* Prefer /dev/input/by-id/*kbd* if present */
    d = opendir("/dev/input/by-id");
    if (d) {
        while ((ent = readdir(d)) != 0) {
            const char *n = ent->d_name;
            int fd;

            if (n[0] == '.') continue;

            /* Common symlink names: *-event-kbd, *-kbd */
            if (strstr(n, "kbd") == 0) continue;

            /* Best effort: open the symlink path directly */
            {
                char p[512];
                size_t len = strlen(n);

                (void)len;
                snprintf(p, sizeof(p), "/dev/input/by-id/%s", n);

                fd = open(p, O_RDONLY | O_NONBLOCK);
                if (fd >= 0) {
                    if (ihk_fd_looks_like_keyboard(fd)) {
                        if (out_path && out_sz) {
                            strncpy(out_path, p, out_sz - 1);
                            out_path[out_sz - 1] = '\0';
                        }
                        closedir(d);
                        return fd;
                    }
                    close(fd);
                }
            }
        }
        closedir(d);
    }

    /* Fallback: scan event0..event63 */
    {
        int i;
        for (i = 0; i < 64; ++i) {
            char p[64];
            int fd;

            snprintf(p, sizeof(p), "/dev/input/event%d", i);
            fd = open(p, O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            if (ihk_fd_looks_like_keyboard(fd)) {
                if (out_path && out_sz) {
                    strncpy(out_path, p, out_sz - 1);
                    out_path[out_sz - 1] = '\0';
                }
                return fd;
            }
            close(fd);
        }
    }

    return -1;
}

static void ihk_resync_state(ihk_linux_evdev_backend *b)
{
    unsigned long keys[(KEY_MAX + IHK_WORD_BITS) / IHK_WORD_BITS];
    unsigned int code;

    memset(keys, 0, sizeof(keys));
    if (ioctl(b->fd, EVIOCGKEY((int)sizeof(keys)), keys) < 0) {
        return;
    }

    pthread_mutex_lock(IHK_MUTEX_T(b->mutex));
    memset(b->kb_bits, 0, sizeof(b->kb_bits));

    for (code = 0; code <= (unsigned int)KEY_MAX; ++code) {
        if (IHK_TEST_BIT((int)code, keys)) {
            ihk_u8 usage = ihk_kb_usage_from_linux_keycode(code);
            if (usage) {
                ihk_set_usage_bit(b->kb_bits, usage);
            }
        }
    }

    pthread_mutex_unlock(IHK_MUTEX_T(b->mutex));
}

static void* ihk_evdev_thread_main(void *arg)
{
    ihk_linux_evdev_backend *b = (ihk_linux_evdev_backend*)arg;

    /* Initial resync (helps in case keys are already held) */
    ihk_resync_state(b);

    while (b->running) {
        struct pollfd pfd;
        int pr;

        pfd.fd = b->fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        pr = poll(&pfd, 1, 50);
        if (pr < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (pr == 0) continue;

        if (pfd.revents & POLLIN) {
            struct input_event ev;
            ssize_t rr;

            /* Read all available events */
            for (;;) {
                rr = read(b->fd, &ev, sizeof(ev));
                if (rr == (ssize_t)sizeof(ev)) {
                    if (ev.type == EV_KEY) {
                        ihk_u8 usage = ihk_kb_usage_from_linux_keycode((unsigned int)ev.code);
                        if (usage) {
                            pthread_mutex_lock(IHK_MUTEX_T(b->mutex));
                            if (ev.value) {
                                ihk_set_usage_bit(b->kb_bits, usage);
                            } else {
                                ihk_clear_usage_bit(b->kb_bits, usage);
                            }
                            pthread_mutex_unlock(IHK_MUTEX_T(b->mutex));
                        }
                    } else if (ev.type == EV_SYN && ev.code == SYN_DROPPED) {
                        /* Kernel dropped events; resync from device state */
                        ihk_resync_state(b);
                    }
                } else {
                    if (rr < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        break;
                    }
                    /* EOF or error */
                    break;
                }
            }
        }
    }

    b->ok = 0;
    return 0;
}

static void ihk_linux_evdev_poll_keyboard(void *user, ihk_u8 *out_kb_bits, size_t out_bytes)
{
    ihk_linux_evdev_backend *b = (ihk_linux_evdev_backend*)user;
    if (!b || !out_kb_bits || out_bytes == 0) return;

    /* Always clear, then copy */
    memset(out_kb_bits, 0, out_bytes);

    if (!b->ok) return;

    pthread_mutex_lock(IHK_MUTEX_T(b->mutex));
    memcpy(out_kb_bits, b->kb_bits, IHK_KB_BITS_BYTES);
    pthread_mutex_unlock(IHK_MUTEX_T(b->mutex));
}

static void ihk_linux_evdev_shutdown(void *user)
{
    ihk_linux_evdev_backend *b = (ihk_linux_evdev_backend*)user;
    if (!b) return;

    if (b->running) {
        b->running = 0;
        if (b->thread) {
            pthread_join(*IHK_PTHREAD_T(b->thread), 0);
        }
    }

    if (b->fd >= 0 && b->owns_fd) {
        close(b->fd);
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

    b->fd = -1;
    b->owns_fd = 0;
    b->ok = 0;
}

int ihk_linux_evdev_backend_init(ihk_linux_evdev_backend *b, const char *device_path)
{
    pthread_t *t;
    pthread_mutex_t *m;
    int fd = -1;

    if (!b) return 0;
    memset(b, 0, sizeof(*b));
    b->fd = -1;
    b->owns_fd = 1;
    b->running = 0;
    b->ok = 0;
    b->device_path[0] = '\0';
    memset(b->kb_bits, 0, sizeof(b->kb_bits));

    if (device_path && device_path[0]) {
        fd = open(device_path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            return 0;
        }
        strncpy(b->device_path, device_path, sizeof(b->device_path) - 1);
        b->device_path[sizeof(b->device_path) - 1] = '\0';
    } else {
        fd = ihk_open_first_kbd(b->device_path, sizeof(b->device_path));
        if (fd < 0) {
            return 0;
        }
    }

    /* Optional sanity check */
    if (!ihk_fd_looks_like_keyboard(fd)) {
        close(fd);
        return 0;
    }

    b->fd = fd;

    /* Allocate pthread objects (keeps header clean) */
    t = (pthread_t*)malloc(sizeof(pthread_t));
    m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!t || !m) {
        if (t) free(t);
        if (m) free(m);
        close(fd);
        b->fd = -1;
        return 0;
    }

    pthread_mutex_init(m, 0);
    b->thread = (void*)t;
    b->mutex = (void*)m;

    b->running = 1;
    b->ok = 1;

    if (pthread_create(t, 0, ihk_evdev_thread_main, b) != 0) {
        b->running = 0;
        b->ok = 0;
        pthread_mutex_destroy(m);
        free(m);
        free(t);
        b->mutex = 0;
        b->thread = 0;
        close(fd);
        b->fd = -1;
        return 0;
    }

    return 1;
}

int ihk_linux_evdev_backend_is_ok(const ihk_linux_evdev_backend *b)
{
    return (b && b->ok) ? 1 : 0;
}

void ihk_linux_evdev_make_backend(ihk_linux_evdev_backend *b, ihk_backend *out)
{
    if (!out) return;
    out->user = b;
    out->poll_keyboard = ihk_linux_evdev_poll_keyboard;
    out->shutdown = ihk_linux_evdev_shutdown;
    out->capabilities = IHK_CAP_KEYBOARD | IHK_CAP_GLOBAL_CAPTURE | IHK_CAP_LAYOUT_INDEPENDENT;
}

#endif /* __linux__ */
