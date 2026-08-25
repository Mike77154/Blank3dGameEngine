#include "bulletinline89.h"
#include <stdio.h>

int main(void)
{
    bi89_request request;
    bi89_result result;
    request.origin.x = BI89_ONE; request.origin.y = 0L; request.origin.z = 0L;
    request.target.x = 0L; request.target.y = 0L; request.target.z = 10L * BI89_ONE;
    request.fallback_direction.x = 0L;
    request.fallback_direction.y = 0L;
    request.fallback_direction.z = BI89_ONE;
    request.target_valid = 1;
    if (!bulletinline89_resolve(&request, &result)) return 1;
    if (!result.valid || !result.used_target) return 2;
    if (result.direction.x >= 0L) return 3;
    if (result.direction.z <= 0L) return 4;
    request.target_valid = 0;
    if (!bulletinline89_resolve(&request, &result)) return 5;
    if (result.used_target) return 6;
    if (result.direction.z < 4000L) return 7;
    printf("bulletinline89: OK\n");
    return 0;
}
