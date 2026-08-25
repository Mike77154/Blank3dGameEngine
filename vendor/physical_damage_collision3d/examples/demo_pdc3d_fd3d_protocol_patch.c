#include "pdc3d.h"
#include <stdio.h>

int main(void)
{
    unsigned char memory[128];
    FD3D_Arena arena;
    void *a0;
    void *a_bad;
    void *a8;
    hb3_fx pdc_neg;
    FD3D_Fx fd_neg;
    hb3_fx pdc_back;

    FD3D_ArenaInit(&arena, memory, sizeof(memory));
    a0 = FD3D_ArenaAlloc(&arena, 16u, 0u);
    a_bad = FD3D_ArenaAlloc(&arena, 16u, 3u);
    a8 = FD3D_ArenaAlloc(&arena, 16u, 8u);

    pdc_neg = (hb3_fx)(-3 * PDC3D_FX_ONE);
    fd_neg = pdc3d_fd_from_pdc_fx(pdc_neg);
    pdc_back = pdc3d_fd_to_pdc_fx(fd_neg);

    printf("fd3d protocol patch: align0=%d badalign=%d align8=%d used=%lu\n",
           a0 != 0 ? 1 : 0,
           a_bad == 0 ? 1 : 0,
           a8 != 0 ? 1 : 0,
           (unsigned long)arena.used);
    printf("fd3d protocol patch: neg_pdc=%ld neg_fd=%ld roundtrip=%ld\n",
           (long)pdc_neg,
           (long)fd_neg,
           (long)pdc_back);
    return 0;
}
