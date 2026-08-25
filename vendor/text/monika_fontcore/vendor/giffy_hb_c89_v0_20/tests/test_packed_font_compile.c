#include <stdio.h>
#include "giffy_hb.h"
#include "dejavu_serif_v20_smoke_pack.h"
int main(void) {
    ghb_buffer b;
    ghb_hvar_diag d;
    ghb_buffer_init(&b);
    ghb_shape_utf8(&dejavu_serif_v20_smoke_font, &b, "ffi", -1, 0, 0);
    if (ghb_font_get_hvar_diagnostics(&dejavu_serif_v20_smoke_font, &d) == GHB_OK) {
        printf("packed_font_len=%d sparse=%d segment=%d lossy=%d\n", b.len, d.sparse_map_count, d.segment_map_count, d.lossy);
    } else {
        printf("packed_font_len=%d\n", b.len);
    }
    return 0;
}
