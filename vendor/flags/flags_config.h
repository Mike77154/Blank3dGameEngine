/* flags_config.h - compile-time configuration for the flags system (C89)
 *
 * Constraints honored:
 * - No stdint.h
 * - No float/double (fixed-point Q16.16)
 * - No malloc/free (all memory is caller-provided)
 * - No stdio (no fopen/printf; optional POSIX IO via open/read/stat)
 */
#ifndef FLAGS_CONFIG_H
#define FLAGS_CONFIG_H

/* ---- Sizes (tune for your target) ---- */

/* Maximum dotted key length (e.g. "a.b.c") including '\0' */
#ifndef FLAGS_MAX_KEY
#define FLAGS_MAX_KEY 128
#endif

/* Maximum JSON nesting depth for flattening */
#ifndef FLAGS_MAX_JSON_DEPTH
#define FLAGS_MAX_JSON_DEPTH 16
#endif

/* Maximum length of a single JSON string token we will parse (key or value) */
#ifndef FLAGS_MAX_JSON_STRING
#define FLAGS_MAX_JSON_STRING 256
#endif

/* Maximum length of one line in INI-like mode we will process (longer lines are truncated) */
#ifndef FLAGS_MAX_LINE
#define FLAGS_MAX_LINE 512
#endif

/* If defined as 1, enables the optional POSIX file IO adapter (open/read/stat). */
#ifndef FLAGS_ENABLE_POSIX_IO
#define FLAGS_ENABLE_POSIX_IO 0
#endif

#endif /* FLAGS_CONFIG_H */
