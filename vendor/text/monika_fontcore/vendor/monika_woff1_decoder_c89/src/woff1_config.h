#ifndef WOFF1_CONFIG_H
#define WOFF1_CONFIG_H

/*
   Monika WOFF1 decoder configuration.
   C89, no heap allocation. Change these limits at build time if needed:
     cc -DWOFF1_MAX_WOFF_SIZE=... -DWOFF1_MAX_SFNT_SIZE=...
*/

#ifndef WOFF1_MAX_TABLES
#define WOFF1_MAX_TABLES 128u
#endif

#ifndef WOFF1_MAX_WOFF_SIZE
#define WOFF1_MAX_WOFF_SIZE (32ul * 1024ul * 1024ul)
#endif

#ifndef WOFF1_MAX_SFNT_SIZE
#define WOFF1_MAX_SFNT_SIZE (64ul * 1024ul * 1024ul)
#endif

#ifndef WOFF1_STRICT_CHECKSUMS
#define WOFF1_STRICT_CHECKSUMS 1
#endif

#ifndef WOFF1_REPAIR_CHECKSUM_ADJUSTMENT
#define WOFF1_REPAIR_CHECKSUM_ADJUSTMENT 1
#endif

#endif
