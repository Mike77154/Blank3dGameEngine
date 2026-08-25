#include "morethanone89.h"
#include <string.h>

void mto89_profile_init(mto89_profile *profile)
{
    if (!profile) return;
    memset(profile, 0, sizeof(*profile));
}

int mto89_set_stage(mto89_profile *profile, int index,
                    const mto89_stage *stage)
{
    if (!profile || !stage) return 0;
    if (index < 0 || index >= MTO89_MAX_STAGES) return 0;
    profile->stages[index] = *stage;
    if (profile->stage_count <= index) profile->stage_count = index + 1;
    return 1;
}

int mto89_validate(const mto89_profile *profile)
{
    int i;
    if (!profile) return 0;
    if (!profile->enabled) return 1;
    if (profile->stage_count <= 0 || profile->stage_count > MTO89_MAX_STAGES)
        return 0;
    if (profile->stages[0].min_ms < profile->activation_ms) return 0;
    for (i = 1; i < profile->stage_count; ++i) {
        if (profile->stages[i].min_ms <= profile->stages[i - 1].min_ms)
            return 0;
    }
    return 1;
}

int mto89_resolve(const mto89_profile *profile, unsigned int hold_ms,
                  mto89_result *out_result)
{
    int i;
    int selected;
    if (out_result) memset(out_result, 0, sizeof(*out_result));
    if (!profile || !out_result || !profile->enabled) return 0;
    if (!mto89_validate(profile)) return 0;
    if (hold_ms < profile->activation_ms) return 0;
    selected = -1;
    for (i = 0; i < profile->stage_count; ++i) {
        if (hold_ms >= profile->stages[i].min_ms) selected = i;
        else break;
    }
    if (selected < 0) return 0;
    out_result->active = 1;
    out_result->stage_index = selected;
    out_result->stage = profile->stages[selected];
    return 1;
}
