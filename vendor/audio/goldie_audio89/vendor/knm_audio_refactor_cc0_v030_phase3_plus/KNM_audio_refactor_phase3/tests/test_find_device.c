#include <assert.h>
#include <string.h>
#include "../KNM_audio/knm_hwr_audio.h"

int main(void)
{
    knm_device_info info;
    assert(knm_audio_find_device(KNM_BACKEND_NULL, "null.default", &info) == KNM_OK);
    assert(strcmp(info.name, "Null Device") == 0);
    assert(knm_audio_find_device(KNM_BACKEND_NULL, "missing", &info) == KNM_DEVICE_NOT_FOUND);
    return 0;
}
