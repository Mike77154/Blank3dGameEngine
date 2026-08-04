#include "gmagazine89.h"

#include <stdio.h>

static int failures = 0;

static void check(int condition, const char *message)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        failures++;
    }
}

int main(void)
{
    GMAG89_State mag;
    GMAG89_Report report;

    gmag89_init(&mag, 15, 15, 1);
    check(gmag89_is_full(&mag), "starts full");
    check(gmag89_spend_shot(&mag) == GMAG89_OK, "spend one shot");
    check(gmag89_rounds(&mag) == 14, "fourteen remain");
    check(gmag89_missing(&mag) == 1, "one missing");
    check(gmag89_fill(&mag) == 1, "fill adds one");
    check(gmag89_is_full(&mag), "full again");

    gmag89_configure(&mag, 2, 2, 2);
    check(gmag89_spend_shot(&mag) == GMAG89_OK, "two-round shot");
    check(gmag89_is_empty(&mag), "empty after two-round shot");
    check(gmag89_spend_shot(&mag) == GMAG89_EMPTY, "empty report");

    gmag89_report(&mag, &report);
    check((report.flags & GMAG89_FLAG_EMPTY) != 0, "report says empty");
    check((report.flags & GMAG89_FLAG_FULL) == 0, "report does not say full");

    gmag89_configure(&mag, 0, 0, 1);
    check(gmag89_spend_shot(&mag) == GMAG89_BYPASS, "zero capacity bypass");

    if (failures) {
        printf("gmagazine89: %d failure(s)\n", failures);
        return 1;
    }
    printf("gmagazine89: all tests passed\n");
    return 0;
}
