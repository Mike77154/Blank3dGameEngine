#include "pcx.h"
#include <string.h>

int main(void)
{
    PCXFileInfo info;
    char buffer[128];
    memset(&info, 0, sizeof(info));
    return pcx_format_diagnostics_text(&info, buffer, sizeof(buffer));
}
