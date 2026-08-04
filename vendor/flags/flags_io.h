/* flags_io.h - IO abstraction (no stdio) - C89 */
#ifndef FLAGS_IO_H
#define FLAGS_IO_H

/* Simple logging callback (optional). */
typedef void (*FlagsLogFn)(void *user, const char *msg);

/* File IO abstraction: user provides these functions. */
typedef struct FlagsIO {
    void *user;

    /* Return 1 on success, 0 if missing/error. */
    int (*get_mtime)(void *user, const char *path, long *out_mtime);

    /* Read entire file into out_buf (cap bytes). Should NUL-terminate if possible.
     * Return 1 on success, 0 on error/missing.
     * out_len returns bytes read (excluding trailing NUL if you add it).
     */
    int (*read_all)(void *user, const char *path, char *out_buf, int cap, int *out_len);
} FlagsIO;

#endif /* FLAGS_IO_H */
