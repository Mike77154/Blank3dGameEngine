/*
   Optional stdio-backed INI reader for hosts that want direct file loading.

   The gweapon89 core itself does not include stdio.h.  This adapter keeps file
   access at the application boundary and feeds text through GWP89_SERVICE_IO.
*/

#include "stdio_ini_provider_example.h"
#include <stdio.h>

static int gwp89_stdio_io_provider(void *ctx, GWP89_ProviderPacket *packet)
{
    FILE *file;
    size_t n;
    (void)ctx;
    if (!packet) return GWP89_PROVIDER_PASS;
    if (packet->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    if (packet->service != GWP89_SERVICE_IO) return GWP89_PROVIDER_PASS;
    if (packet->operation != GWP89_OP_READ_TEXT_FILE) return GWP89_PROVIDER_PASS;
    if (!packet->text_in || !packet->text_out || packet->text_capacity <= 0) {
        packet->result_code = GWP89_BAD_ARG;
        return GWP89_PROVIDER_HANDLED;
    }
    file = fopen(packet->text_in, "rb");
    if (!file) {
        packet->result_code = GWP89_NOT_FOUND;
        return GWP89_PROVIDER_HANDLED;
    }
    n = fread(packet->text_out, 1u, (size_t)(packet->text_capacity - 1), file);
    if (ferror(file)) {
        fclose(file);
        packet->text_out[0] = '\0';
        packet->text_length = 0;
        packet->result_code = GWP89_PROVIDER_ERROR;
        return GWP89_PROVIDER_HANDLED;
    }
    fclose(file);
    packet->text_out[n] = '\0';
    packet->text_length = (int)n;
    packet->result_code = GWP89_OK;
    return GWP89_PROVIDER_HANDLED;
}

int gwp89_install_stdio_ini_provider(GWP89_Manager *manager, int priority)
{
    if (!manager) return GWP89_BAD_ARG;
    return gwp89_add_provider(manager,
                              GWP89_SERVICE_IO,
                              priority,
                              "stdio_ini_io",
                              0,
                              gwp89_stdio_io_provider);
}
