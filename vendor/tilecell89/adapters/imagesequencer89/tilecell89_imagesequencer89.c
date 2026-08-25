#include "tilecell89_imagesequencer89.h"

int tc89_strip_to_imagesequencer89(const TileCell89 *tc, tc89_id atlas_id,
                                   tc89_u32 start_x, tc89_u32 start_y,
                                   int step_x, int step_y, tc89_u32 count,
                                   tc89_u32 duration_ms, const char *sequence_name,
                                   ImageSequencer89 *seqs, is89_id *out_sequence_id)
{
    const TC89_Atlas *a;
    is89_id seq;
    tc89_u32 i;
    int x;
    int y;
    TC89_Rect rect;
    if (!tc || !seqs || !sequence_name || atlas_id >= tc->atlas_count) return 0;
    a = &tc->atlases[atlas_id];
    if (!a->used) return 0;
    seq = is89_sequence_begin(seqs, sequence_name);
    if (seq == IS89_INVALID_ID) return 0;
    x = (int)start_x;
    y = (int)start_y;
    for (i = 0U; i < count; ++i) {
        if (x < 0 || y < 0 || !tc89_atlas_rect_xy(tc, atlas_id, (tc89_u32)x, (tc89_u32)y, &rect)) return 0;
        if (is89_sequence_add_frame_rect(seqs, seq, a->image_request,
                                         rect.x, rect.y, rect.w, rect.h,
                                         duration_ms) == IS89_INVALID_ID) return 0;
        x += step_x;
        y += step_y;
    }
    if (out_sequence_id) *out_sequence_id = seq;
    return 1;
}
