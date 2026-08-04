/* flags_io_posix.h - POSIX adapter for FlagsIO (no stdio) - C89 */
#ifndef FLAGS_IO_POSIX_H
#define FLAGS_IO_POSIX_H

#include "flags_config.h"
#include "flags_io.h"

#if FLAGS_ENABLE_POSIX_IO

/* Create a FlagsIO object backed by POSIX open/read/stat.
 * user pointer is unused (may be NULL).
 */
FlagsIO flags_io_posix(void);

#endif /* FLAGS_ENABLE_POSIX_IO */

#endif /* FLAGS_IO_POSIX_H */
