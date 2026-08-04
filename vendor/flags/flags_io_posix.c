/* flags_io_posix.c - POSIX adapter - C89 */
#include "flags_io_posix.h"

#if FLAGS_ENABLE_POSIX_IO

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static int _posix_get_mtime(void *user, const char *path, long *out_mtime)
{
    struct stat st;
    (void)user;
    if (!path || !out_mtime) return 0;
    if (stat(path, &st) != 0) return 0;
    *out_mtime = (long)st.st_mtime;
    return 1;
}

static int _posix_read_all(void *user, const char *path, char *out_buf, int cap, int *out_len)
{
    int fd;
    int total;
    (void)user;
    if (!path || !out_buf || cap <= 0 || !out_len) return 0;

    fd = open(path, O_RDONLY);
    if (fd < 0) return 0;

    total = 0;
    while (total < cap) {
        int n;
        n = (int)read(fd, out_buf + total, (unsigned int)(cap - total));
        if (n < 0) { close(fd); return 0; }
        if (n == 0) break;
        total += n;
    }
    close(fd);

    *out_len = total;
    return 1;
}

FlagsIO flags_io_posix(void)
{
    FlagsIO io;
    io.user = 0;
    io.get_mtime = _posix_get_mtime;
    io.read_all = _posix_read_all;
    return io;
}

#endif /* FLAGS_ENABLE_POSIX_IO */
