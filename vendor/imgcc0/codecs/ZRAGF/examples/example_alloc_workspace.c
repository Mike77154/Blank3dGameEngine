#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zragflib.h"
#include "protocol89_hostmem.h"

int main(void)
{
    const unsigned char src[] = "example payload for workspace mode";
    unsigned char comp[512];
    unsigned char out[512];
    zragf_size_t comp_cap = sizeof(comp);
    zragf_size_t out_cap = sizeof(out);
    zragf_size_t work_need;
    zragf_size_t dwork_need;
    void *work;
    void *dwork;
    zragf_info info;

    work_need = zragf_compress_workspace_bound(sizeof(src) - 1u);
    work = zragf_p89_host_take(work_need);
    if (!work)
        return 1;

    if (zragf_compress_with_workspace(src, sizeof(src) - 1u,
                                      comp, &comp_cap,
                                      work, work_need,
                                      ZRAGF_LEVEL_DEFAULT) != ZRAGF_ST_OK)
        return 2;
    zragf_p89_host_release(work);

    if (zragf_decompress_workspace_bound(comp, comp_cap, &dwork_need) != ZRAGF_ST_OK)
        return 3;
    dwork = dwork_need ? zragf_p89_host_take(dwork_need) : NULL;
    if (dwork_need && !dwork)
        return 4;

    if (zragf_decompress_with_workspace(comp, comp_cap,
                                        out, &out_cap,
                                        dwork, dwork_need,
                                        &info) != ZRAGF_ST_OK)
        return 5;
    zragf_p89_host_release(dwork);

    printf("roundtrip=%s size=%lu\n",
           (memcmp(src, out, sizeof(src) - 1u) == 0) ? "ok" : "bad",
           (unsigned long)comp_cap);
    return 0;
}
