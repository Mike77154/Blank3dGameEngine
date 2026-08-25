#ifndef TILECELL89_IMAGESEQUENCER89_H
#define TILECELL89_IMAGESEQUENCER89_H
#include "tilecell89.h"
#include "imagesequencer89.h"
int tc89_strip_to_imagesequencer89(const TileCell89 *tc, tc89_id atlas_id,
                                   tc89_u32 start_x, tc89_u32 start_y,
                                   int step_x, int step_y, tc89_u32 count,
                                   tc89_u32 duration_ms, const char *sequence_name,
                                   ImageSequencer89 *seqs, is89_id *out_sequence_id);
#endif
