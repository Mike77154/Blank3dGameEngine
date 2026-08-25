#ifndef W2F_CONFIG_H
#define W2F_CONFIG_H

/*
   Monika WOFF2 Core Decoder configuration.
   C89-friendly, fixed buffers, no heap required by this library.

   Increase these caps for large webfont families. The CLI example uses
   static arrays sized from these values.
*/

#define W2F_MAX_TABLES 128u
#define W2F_MAX_WOFF2_BYTES (8u * 1024u * 1024u)
#define W2F_MAX_DECOMPRESSED_TABLE_BYTES (32u * 1024u * 1024u)
#define W2F_MAX_SFNT_BYTES (36u * 1024u * 1024u)

/* Optional arena size for the Google Brotli adapter, if you compile it. */
#define W2F_BROTLI_ARENA_SIZE (24u * 1024u * 1024u)

#endif
