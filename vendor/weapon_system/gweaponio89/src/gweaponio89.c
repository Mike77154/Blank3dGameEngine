#include "gweaponio89.h"

#include <string.h>

int gweaponio89_read_text(GWP89_Manager *manager,
                          const char *path,
                          char *buffer,
                          int capacity,
                          int *length_out)
{
    GWP89_ProviderPacket packet;
    int r;
    if (length_out) *length_out = 0;
    if (!manager || !path || !buffer || capacity <= 1) return 0;
    memset(&packet, 0, sizeof(packet));
    buffer[0] = '\0';
    packet.text_in = path;
    packet.text_out = buffer;
    packet.text_capacity = capacity;
    packet.text_length = 0;
    packet.result_code = GWP89_NOT_FOUND;
    r = gwp89_provider_call(manager, GWP89_SERVICE_IO,
                            GWP89_OP_READ_TEXT_FILE,
                            GWP89_PHASE_PRE, &packet);
    if (r & GWP89_PROVIDER_CANCEL) return 0;
    if (!(r & GWP89_PROVIDER_HANDLED)) return 0;
    if (packet.result_code < 0) return 0;
    if (packet.text_length < 0) packet.text_length = 0;
    if (packet.text_length >= capacity) packet.text_length = capacity - 1;
    buffer[packet.text_length] = '\0';
    if (length_out) *length_out = packet.text_length;
    (void)gwp89_provider_call(manager, GWP89_SERVICE_IO,
                              GWP89_OP_READ_TEXT_FILE,
                              GWP89_PHASE_POST, &packet);
    return 1;
}
