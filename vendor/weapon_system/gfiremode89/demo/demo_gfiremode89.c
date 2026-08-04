#include "gfiremode89.h"

#include <stdio.h>

int main(void)
{
    GFM89_State fire;
    int result;
    int frame;

    gfm89_init(&fire);
    gfm89_configure(&fire, GFM89_FIRE_AUTO, 3, 120u);

    for (frame = 0; frame < 10; frame++) {
        result = gfm89_request(&fire, GFM89_TRIGGER_DOWN, 40u);
        if (result == GFM89_FIRE_REQUEST) {
            printf("frame %d: fire request accepted\n", frame);
            gfm89_commit_fire(&fire);
        }
    }
    return 0;
}
