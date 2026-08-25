#include "pcx.h"
#include <string.h>

int main(void)
{
    const char *a = pcx_version_string();
    const char *b = pcx_result_description(PCX_OK);
    const char *c = pcx_warning_flag_to_string(PCX_WARN_NONE);
    return (strcmp(a, "1.16.0") == 0 && b != NULL && c != NULL) ? 0 : 1;
}
