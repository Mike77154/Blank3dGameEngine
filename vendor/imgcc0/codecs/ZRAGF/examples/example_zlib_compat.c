#include <stdio.h>
#include <string.h>
#include "zragflib.h"

int main(void)
{
    const unsigned char payload[] = "compat stream example";
    unsigned char comp[512];
    unsigned char out[512];
    zragf_stream ds;
    zragf_stream is;
    int rc;
    zragf_size_t comp_size;

    memset(&ds, 0, sizeof(ds));
    memset(&is, 0, sizeof(is));

    rc = zragf_deflateInit2(&ds, 5, 8, 15, 8, 0);
    if (rc != ZRAGF_OK) return 1;
    ds.next_in = (zragf_u8 *)payload;
    ds.avail_in = sizeof(payload) - 1u;
    ds.next_out = comp;
    ds.avail_out = sizeof(comp);
    rc = zragf_deflateZ(&ds, ZRAGF_FINISH);
    zragf_deflateEndZ(&ds);
    if (rc != ZRAGF_STREAM_END) return 1;
    comp_size = sizeof(comp) - ds.avail_out;

    rc = zragf_inflateInit2(&is, 15);
    if (rc != ZRAGF_OK) return 1;
    is.next_in = comp;
    is.avail_in = comp_size;
    is.next_out = out;
    is.avail_out = sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    zragf_inflateEndZ(&is);
    if (rc != ZRAGF_STREAM_END) return 1;

    fwrite(out, 1, (sizeof(out) - is.avail_out), stdout);
    putchar('\n');
    return 0;
}
