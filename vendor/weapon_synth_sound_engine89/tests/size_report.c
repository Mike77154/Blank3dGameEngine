#include <stdio.h>
#include "gsynthsoundengine89.h"

int main(void)
{
    printf("gv89_voice=%lu\n", (unsigned long)sizeof(gv89_voice));
    printf("gwv89_context=%lu\n", (unsigned long)sizeof(gwv89_context));
    printf("gt89_context=%lu\n", (unsigned long)sizeof(gt89_context));
    printf("gfire89=%lu\n", (unsigned long)sizeof(gfire89));
    printf("gsse89_casing_voice=%lu\n", (unsigned long)sizeof(gsse89_casing_voice));
    printf("gsse89_fire_voice=%lu\n", (unsigned long)sizeof(gsse89_fire_voice));
    printf("gsse89_context=%lu\n", (unsigned long)sizeof(gsse89_context));
    printf("recommended_extras_32c_4f=%lu\n",
           (unsigned long)(sizeof(gsse89_context) +
           32UL * sizeof(gsse89_casing_voice) +
           4UL * sizeof(gsse89_fire_voice)));
    printf("showcase_extras_128c_12f=%lu\n",
           (unsigned long)(sizeof(gsse89_context) +
           128UL * sizeof(gsse89_casing_voice) +
           12UL * sizeof(gsse89_fire_voice)));
    return 0;
}
