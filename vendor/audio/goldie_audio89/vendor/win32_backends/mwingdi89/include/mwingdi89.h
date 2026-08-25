#ifndef MWINGDI89_H
#define MWINGDI89_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "mcanvas89.h"
#include "mtext89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mwingdi89_state {
    HDC dc;
} mwingdi89_state;

void mwingdi89_init(mwingdi89_state *state, HDC dc);
mcanvas89_provider mwingdi89_canvas_provider(mwingdi89_state *state);
mtext89_provider mwingdi89_text_provider(mwingdi89_state *state);

#ifdef __cplusplus
}
#endif

#endif
