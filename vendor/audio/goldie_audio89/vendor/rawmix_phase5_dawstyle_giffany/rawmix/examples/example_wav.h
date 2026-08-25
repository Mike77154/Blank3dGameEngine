#ifndef EXAMPLE_WAV_H
#define EXAMPLE_WAV_H

#include <stdio.h>
#include "rawmix.h"

int example_wav_write_s16(const char *path,
                          const rm_s16 *samples,
                          rm_u32 frames,
                          rm_u16 channels,
                          rm_u32 sample_rate);

#endif
