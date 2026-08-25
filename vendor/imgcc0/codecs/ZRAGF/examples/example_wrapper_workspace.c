#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"

int main(void)
{
    const unsigned char src[] = "workspace-backed zlib wrapper example";
    unsigned char comp[512];
    unsigned char out[512];
    zragf_workspace dws;
    zragf_workspace iws;
    zragf_stream dstrm;
    zragf_stream istrm;
    zragf_size_t dneed;
    zragf_size_t ineed;
    void *dbuf;
    void *ibuf;
    int rc;

    dneed = zragf_deflate_workspace_bound_z((unsigned long)(sizeof(src) - 1u), 15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    dbuf = zragf_p89_host_take(dneed);
    if (!dbuf)
        return 1;

    memset(&dstrm, 0, sizeof(dstrm));
    zragf_workspace_init(&dws, dbuf, dneed);
    zragf_stream_set_workspace(&dstrm, &dws);
    if (zragf_deflateInit2(&dstrm, ZRAGF_LEVEL_DEFAULT, 8, 15, 8, ZRAGF_Z_DEFAULT_STRATEGY) != ZRAGF_OK)
        return 2;
    dstrm.next_in = (zragf_u8 *)src;
    dstrm.avail_in = sizeof(src) - 1u;
    dstrm.next_out = comp;
    dstrm.avail_out = sizeof(comp);
    rc = zragf_deflateZ(&dstrm, ZRAGF_FINISH);
    zragf_deflateEndZ(&dstrm);
    if (rc != ZRAGF_STREAM_END)
        return 3;

    ineed = zragf_inflate_workspace_bound_z((unsigned long)dstrm.total_out, 15);
    ibuf = zragf_p89_host_take(ineed ? ineed : 1u);
    if (!ibuf)
        return 4;

    memset(&istrm, 0, sizeof(istrm));
    zragf_workspace_init(&iws, ibuf, ineed);
    zragf_stream_set_workspace(&istrm, &iws);
    if (zragf_inflateInit2(&istrm, 15) != ZRAGF_OK)
        return 5;
    istrm.next_in = comp;
    istrm.avail_in = dstrm.total_out;
    istrm.next_out = out;
    istrm.avail_out = sizeof(out);
    rc = zragf_inflateZ(&istrm, ZRAGF_FINISH);
    zragf_inflateEndZ(&istrm);
    if (rc != ZRAGF_STREAM_END)
        return 6;

    printf("wrapper workspace roundtrip=%s comp=%lu used=%lu/%lu\n",
           (memcmp(src, out, sizeof(src) - 1u) == 0) ? "ok" : "bad",
           (unsigned long)dstrm.total_out,
           (unsigned long)zragf_workspace_used(&dws),
           (unsigned long)dneed);
    zragf_p89_host_release(ibuf);
    zragf_p89_host_release(dbuf);
    return 0;
}
