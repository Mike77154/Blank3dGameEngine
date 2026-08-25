#include "imagesequencer89_staticsprite89.h"

int is89_apply_to_staticsprite89(const ImageSequencer89 *seqs, is89_id player_id,
                                 SS89_StaticSprite *sprite)
{
    const IS89_Frame *f;
    if (!seqs || !sprite) return 0;
    f = is89_player_current_frame(seqs, player_id);
    if (!f || !ss89_set_image(sprite, f->image_request)) return 0;
    if (f->flags & IS89_FRAME_SOURCE_RECT) {
        ss89_set_source_rect(sprite, f->source_x, f->source_y, f->source_w, f->source_h);
    } else {
        ss89_clear_source_rect(sprite);
    }
    ss89_set_scale_q16(sprite, f->scale_x_q16, f->scale_y_q16);
    ss89_set_flags(sprite,
                   ((f->flags & IS89_FRAME_FLIP_X) ? SS89_FLAG_FLIP_X : 0U) |
                   ((f->flags & IS89_FRAME_FLIP_Y) ? SS89_FLAG_FLIP_Y : 0U));
    ss89_set_user_tag(sprite, f->user_tag);
    return 1;
}
