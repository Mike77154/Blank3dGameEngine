#ifndef CHUECKA89_WAV_H
#define CHUECKA89_WAV_H

#include "chuecka89.h"

/* Demo/support writer. The synthesis core itself does not depend on stdio. */
int ch89_write_wav_mono16(
    const char *path,
    const ch89_i16 *samples,
    ch89_u32 frames,
    ch89_u32 sample_rate
);

#endif
