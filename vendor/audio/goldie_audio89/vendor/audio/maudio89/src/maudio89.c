#include "maudio89.h"

static int maudio89_provider_complete(const maudio89_provider *provider)
{
    return provider != (const maudio89_provider *)0 &&
        provider->open != 0 &&
        provider->play != 0 &&
        provider->stop != 0 &&
        provider->is_playing != 0 &&
        provider->close != 0;
}

int maudio89_player_init(
    maudio89_player *player,
    const maudio89_provider *provider)
{
    if (player == (maudio89_player *)0 || !maudio89_provider_complete(provider)) {
        return MAUDIO89_ERR_ARGUMENT;
    }

    player->provider = *provider;
    player->provider_set = 1;
    player->opened = 0;
    return MAUDIO89_OK;
}

int maudio89_player_open(
    maudio89_player *player,
    const maudio89_format *format)
{
    int result;

    if (player == (maudio89_player *)0 || format == (const maudio89_format *)0) {
        return MAUDIO89_ERR_ARGUMENT;
    }
    if (!player->provider_set) {
        return MAUDIO89_ERR_PROVIDER;
    }
    if (player->opened) {
        maudio89_player_close(player);
    }

    result = player->provider.open(player->provider.context, format);
    if (result != MAUDIO89_OK) {
        return MAUDIO89_ERR_BACKEND;
    }
    player->opened = 1;
    return MAUDIO89_OK;
}

int maudio89_player_play(
    maudio89_player *player,
    const maudio89_u8 *bytes,
    maudio89_u32 byte_count)
{
    int result;

    if (player == (maudio89_player *)0 ||
        bytes == (const maudio89_u8 *)0 || byte_count == 0U) {
        return MAUDIO89_ERR_ARGUMENT;
    }
    if (!player->opened) {
        return MAUDIO89_ERR_NOT_OPEN;
    }

    result = player->provider.play(player->provider.context, bytes, byte_count);
    return result == MAUDIO89_OK ? MAUDIO89_OK : MAUDIO89_ERR_BACKEND;
}

int maudio89_player_stop(maudio89_player *player)
{
    int result;

    if (player == (maudio89_player *)0) {
        return MAUDIO89_ERR_ARGUMENT;
    }
    if (!player->opened) {
        return MAUDIO89_ERR_NOT_OPEN;
    }
    result = player->provider.stop(player->provider.context);
    return result == MAUDIO89_OK ? MAUDIO89_OK : MAUDIO89_ERR_BACKEND;
}

int maudio89_player_is_playing(maudio89_player *player)
{
    if (player == (maudio89_player *)0 || !player->opened) {
        return 0;
    }
    return player->provider.is_playing(player->provider.context) ? 1 : 0;
}

void maudio89_player_close(maudio89_player *player)
{
    if (player == (maudio89_player *)0) {
        return;
    }
    if (player->provider_set && player->opened) {
        player->provider.close(player->provider.context);
    }
    player->opened = 0;
}
