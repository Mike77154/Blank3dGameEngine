#ifndef MWINFONT89_H
#define MWINFONT89_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "mfont89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MWINFONT89_MAX_HANDLES 16

typedef struct mwinfont89_state {
    HDC lookup_dc;
    HFONT handles[MWINFONT89_MAX_HANDLES];
    int owned[MWINFONT89_MAX_HANDLES];
    int count;
} mwinfont89_state;

void mwinfont89_init(mwinfont89_state *state, HDC lookup_dc);
void mwinfont89_set_lookup_dc(mwinfont89_state *state, HDC lookup_dc);
mfont89_provider mwinfont89_make_provider(mwinfont89_state *state);
void mwinfont89_shutdown(mwinfont89_state *state);

#ifdef __cplusplus
}
#endif

#endif
