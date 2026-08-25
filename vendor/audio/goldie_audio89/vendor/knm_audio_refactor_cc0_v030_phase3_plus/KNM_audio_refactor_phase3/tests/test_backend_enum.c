#include <assert.h>
#include <string.h>
#include "../KNM_audio/knm_hwr_audio.h"

int main(void)
{
    knm_backend_info bi;
    knm_device_info di;
    int order[16];
    unsigned int count;
    unsigned int order_count;

    count = knm_audio_backend_count();
    assert(count > 0U);
    assert(knm_audio_backend_info(0U, &bi) == KNM_OK);
    assert(strcmp(bi.name, "null") == 0);
    assert(knm_audio_device_count(KNM_BACKEND_NULL) == 1U);
    assert(knm_audio_device_info(KNM_BACKEND_NULL, 0U, &di) == KNM_OK);
    assert(di.supports_output);
    assert(di.supports_input);
    assert(di.supports_duplex);
    assert(strcmp(di.device_id, "null.default") == 0);

    order_count = knm_audio_default_backend_order(order, (unsigned int)(sizeof(order) / sizeof(order[0])));
    assert(order_count > 0U);
    assert(order[order_count - 1U] == KNM_BACKEND_NULL);
    return 0;
}
