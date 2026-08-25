#include <assert.h>
#include "../KNM_audio/knm_hwr_audio.h"

int main(void)
{
    assert(knm_fix_from_int(1) == KNM_FIX_ONE);
    assert(knm_fix_from_ratio(1, 2) == KNM_FIX_HALF);
    assert(knm_fix_mul(KNM_FIX_HALF, KNM_FIX_HALF) == 16384);
    assert(knm_fix_abs(-KNM_FIX_HALF) == KNM_FIX_HALF);
    assert(knm_fix_clamp_unit(KNM_FIX_ONE + KNM_FIX_HALF) == KNM_FIX_ONE);
    return 0;
}
