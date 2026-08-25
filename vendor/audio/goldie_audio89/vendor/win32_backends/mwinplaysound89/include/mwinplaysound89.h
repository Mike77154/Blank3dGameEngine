#ifndef MWINPLAYSOUND89_H
#define MWINPLAYSOUND89_H

#include "msoundprovider89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mwinplaysound89_state {
    int playing;
    int looping;
} mwinplaysound89_state;

void mwinplaysound89_init(mwinplaysound89_state *state);
msoundprovider89 mwinplaysound89_make_provider(mwinplaysound89_state *state);

#ifdef __cplusplus
}
#endif

#endif
