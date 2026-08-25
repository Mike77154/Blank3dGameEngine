#include <stdio.h>
#include "../KNM_audio/knm_hwr_audio.h"

int main(void)
{
    unsigned int backend_count;
    unsigned int order_count;
    unsigned int i;
    int order[16];
    knm_backend_info bi;
    knm_device_info di;
    unsigned int j;

    backend_count = knm_audio_backend_count();
    printf("KNM_audio %s\n", knm_audio_version_string());
    printf("registered backends: %u\n", backend_count);

    order_count = knm_audio_default_backend_order(order, (unsigned int)(sizeof(order) / sizeof(order[0])));
    printf("default backend order:");
    for (i = 0U; i < order_count; ++i) {
        printf(" %s", knm_backend_name(order[i]));
    }
    printf("\n\n");

    for (i = 0U; i < backend_count; ++i) {
        if (knm_audio_backend_info(i, &bi) != KNM_OK) {
            continue;
        }
        printf("backend=%s compiled=%d available=%d devices=%u enum=%d\n",
               bi.name,
               bi.compiled,
               bi.available,
               bi.device_count,
               bi.supports_enumeration);

        for (j = 0U; j < bi.device_count; ++j) {
            if (knm_audio_device_info(bi.backend, j, &di) == KNM_OK) {
                printf("  [%u] id=%s name=%s in=%d out=%d duplex=%d default=%d\n",
                       j,
                       di.device_id,
                       di.name,
                       di.supports_input,
                       di.supports_output,
                       di.supports_duplex,
                       di.is_default);
            }
        }
    }

    return 0;
}
