#include <pcx/pcx.h>
#include <stdio.h>

int main(void)
{
    unsigned long v = pcx_version_number();
    puts(pcx_version_string());
    return v == PCX_VERSION_NUMBER ? 0 : 1;
}
