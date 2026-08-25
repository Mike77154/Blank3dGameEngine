#ifndef RENLIST89_IMAGESEQUENCER89_H
#define RENLIST89_IMAGESEQUENCER89_H
#include "renlist89.h"
#include "imagesequencer89.h"
int rl89_to_imagesequencer89(const RenList89 *doc, rl89_id animation_id,
                             ImageSequencer89 *seqs, is89_id *out_sequence_id);
#endif
