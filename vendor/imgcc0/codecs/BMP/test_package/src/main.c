#include <bmp/bmp.h>
#include <string.h>

int main(void)
{
    return strcmp(bmp_version_string(), "1.16.0") == 0 &&
           bmp_version_number() == 11600U ? 0 : 1;
}
