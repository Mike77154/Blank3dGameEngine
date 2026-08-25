#include "bmp.h"
#include <string.h>

int main(void)
{
    const char *a = bmp_version_string();
    const char *b = bmp_error_description(BMP_OK);
    const char *c = bmp_warning_string(BMP_WARN_NONE);
    return (strcmp(a, "1.16.0") == 0 && b != NULL && c != NULL) ? 0 : 1;
}
