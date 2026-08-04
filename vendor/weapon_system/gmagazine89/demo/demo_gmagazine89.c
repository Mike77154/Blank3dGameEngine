#include "gmagazine89.h"

#include <stdio.h>

int main(void)
{
    GMAG89_State mag;
    gmag89_init(&mag, 7, 7, 1);

    while (gmag89_can_fire(&mag)) {
        gmag89_spend_shot(&mag);
        printf("rounds left: %d\n", gmag89_rounds(&mag));
    }
    printf("empty=%d full=%d missing=%d\n",
           gmag89_is_empty(&mag),
           gmag89_is_full(&mag),
           gmag89_missing(&mag));
    return 0;
}
