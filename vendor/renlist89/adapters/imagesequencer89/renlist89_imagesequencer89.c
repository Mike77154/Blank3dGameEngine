#include "renlist89_imagesequencer89.h"

int rl89_to_imagesequencer89(const RenList89 *doc, rl89_id animation_id,
                             ImageSequencer89 *seqs, is89_id *out_sequence_id)
{
    const RL89_Animation *a;
    is89_id seq;
    rl89_id i;
    int loop_mode;
    char sequence_name[IS89_NAME_CAP];
    unsigned int n;
    unsigned int k;
    if (!doc || !seqs) return 0;
    a = rl89_animation(doc, animation_id);
    if (!a) return 0;
    n = 0U;
    k = 0U;
    while (a->asset_name[k] != '\0') {
        if (n + 1U >= IS89_NAME_CAP) return 0;
        sequence_name[n++] = a->asset_name[k++];
    }
    if (n + 1U >= IS89_NAME_CAP) return 0;
    sequence_name[n++] = ':';
    k = 0U;
    while (a->clip_name[k] != '\0') {
        if (n + 1U >= IS89_NAME_CAP) return 0;
        sequence_name[n++] = a->clip_name[k++];
    }
    sequence_name[n] = '\0';
    seq = is89_sequence_begin(seqs, sequence_name);
    if (seq == IS89_INVALID_ID) return 0;
    loop_mode = IS89_LOOP_NONE;
    if (a->loop_mode == RL89_LOOP_FORWARD) loop_mode = IS89_LOOP_FORWARD;
    else if (a->loop_mode == RL89_LOOP_REVERSE) loop_mode = IS89_LOOP_REVERSE;
    else if (a->loop_mode == RL89_LOOP_PINGPONG) loop_mode = IS89_LOOP_PINGPONG;
    else if (a->loop_mode == RL89_LOOP_HOLD) loop_mode = IS89_LOOP_HOLD;
    if (!is89_sequence_set_loop(seqs, seq, loop_mode)) return 0;
    for (i = 0U; i < a->frame_count; ++i) {
        const RL89_Frame *f;
        is89_id out_frame;
        f = rl89_frame(doc, animation_id, i);
        if (!f) return 0;
        out_frame = is89_sequence_add_frame(seqs, seq, f->request, f->duration_ms);
        if (out_frame == IS89_INVALID_ID) return 0;
        if (!is89_frame_set_transform(seqs, out_frame,
                                      f->offset_x_q16, f->offset_y_q16,
                                      f->scale_x_q16, f->scale_y_q16)) return 0;
        if (!is89_frame_set_flags(seqs, out_frame, f->flags)) return 0;
        if (!is89_frame_set_user_tag(seqs, out_frame, f->user_tag)) return 0;
    }
    if (out_sequence_id) *out_sequence_id = seq;
    return 1;
}
