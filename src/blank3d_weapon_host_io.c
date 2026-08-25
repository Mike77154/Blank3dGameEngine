#include "blank3d_weapon_host_io.h"

#include <stdio.h>

int blank3d_weapon_host_io_provider(void *context,
                                    GWP89_ProviderPacket *packet)
{
    FILE *file;
    size_t got;
    (void)context;
    if (!packet || packet->phase != GWP89_PHASE_PRE ||
        packet->operation != GWP89_OP_READ_TEXT_FILE)
        return GWP89_PROVIDER_PASS;
    if (!packet->text_in || !packet->text_out || packet->text_capacity <= 1) {
        packet->result_code = GWP89_BAD_ARG;
        return GWP89_PROVIDER_HANDLED;
    }
    file = fopen(packet->text_in, "rb");
    if (!file) {
        packet->text_length = 0;
        packet->text_out[0] = '\0';
        packet->result_code = GWP89_NOT_FOUND;
        return GWP89_PROVIDER_HANDLED;
    }
    got = fread(packet->text_out, 1U,
                (size_t)(packet->text_capacity - 1), file);
    fclose(file);
    packet->text_out[got] = '\0';
    packet->text_length = (int)got;
    packet->result_code = GWP89_OK;
    return GWP89_PROVIDER_HANDLED;
}

int blank3d_weapon_host_io_bind(GWP89_Manager *manager)
{
    if (!manager) return GWP89_BAD_ARG;
    return gwp89_add_provider(manager, GWP89_SERVICE_IO, 200,
                              "blank3d.host-filesystem", 0,
                              blank3d_weapon_host_io_provider);
}
