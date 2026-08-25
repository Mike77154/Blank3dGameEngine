#include <pcx/pcx.h>
#include <string.h>

int main(void)
{
    return strcmp(pcx_version_string(), "1.16.0") == 0 && pcx_version_number() == 11600UL ? 0 : 1;
}
