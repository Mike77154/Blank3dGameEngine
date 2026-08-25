#ifndef MFT_MFONT_CFG_H
#define MFT_MFONT_CFG_H

/*
 * Compile-time sizing.
 *
 * This library does not allocate from the heap. All temporary storage uses
 * fixed-size caller-visible workspaces and user-provided buffers. These
 * defaults target typical M.U.G.E.N font assets and can be overridden with
 * -D flags at compile time.
 */

#ifndef MFT_CFG_MAX_WIDTH
#define MFT_CFG_MAX_WIDTH 1024UL
#endif

#ifndef MFT_CFG_MAX_HEIGHT
#define MFT_CFG_MAX_HEIGHT 1024UL
#endif

#ifndef MFT_CFG_MAX_SCANLINE
#define MFT_CFG_MAX_SCANLINE ((MFT_CFG_MAX_WIDTH * 4UL) + 8UL)
#endif

#ifndef MFT_CFG_MAX_FILTERED
#define MFT_CFG_MAX_FILTERED ((MFT_CFG_MAX_SCANLINE + 1UL) * MFT_CFG_MAX_HEIGHT)
#endif

#ifndef MFT_CFG_MAX_PNG_IDAT
#define MFT_CFG_MAX_PNG_IDAT (MFT_CFG_MAX_FILTERED + 65536UL)
#endif

#ifndef MFT_CFG_MAX_FILE_BUFFER
#define MFT_CFG_MAX_FILE_BUFFER ((MFT_CFG_MAX_FILTERED * 3UL) + 4096UL)
#endif

#ifndef MFT_CFG_MAX_TEXT
#define MFT_CFG_MAX_TEXT 262144UL
#endif

#ifndef MFT_CFG_MAX_PATH
#define MFT_CFG_MAX_PATH 260
#endif

#endif
