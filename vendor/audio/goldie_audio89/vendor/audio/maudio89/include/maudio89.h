#ifndef MAUDIO89_H
#define MAUDIO89_H

/* maudio89: tiny provider-driven playback facade, C89, no allocation. */

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char maudio89_u8;
typedef unsigned short maudio89_u16;
typedef unsigned int maudio89_u32;

enum maudio89_result {
    MAUDIO89_OK = 0,
    MAUDIO89_ERR_ARGUMENT = -1,
    MAUDIO89_ERR_PROVIDER = -2,
    MAUDIO89_ERR_NOT_OPEN = -3,
    MAUDIO89_ERR_BACKEND = -4
};

typedef struct maudio89_format {
    maudio89_u16 channels;
    maudio89_u32 sample_rate;
    maudio89_u32 byte_rate;
    maudio89_u16 block_align;
    maudio89_u16 bits_per_sample;
} maudio89_format;

typedef struct maudio89_provider {
    void *context;
    int (*open)(void *context, const maudio89_format *format);
    int (*play)(void *context, const maudio89_u8 *bytes, maudio89_u32 byte_count);
    int (*stop)(void *context);
    int (*is_playing)(void *context);
    void (*close)(void *context);
} maudio89_provider;

typedef struct maudio89_player {
    maudio89_provider provider;
    int provider_set;
    int opened;
} maudio89_player;

int maudio89_player_init(
    maudio89_player *player,
    const maudio89_provider *provider
);

int maudio89_player_open(
    maudio89_player *player,
    const maudio89_format *format
);

int maudio89_player_play(
    maudio89_player *player,
    const maudio89_u8 *bytes,
    maudio89_u32 byte_count
);

int maudio89_player_stop(maudio89_player *player);
int maudio89_player_is_playing(maudio89_player *player);
void maudio89_player_close(maudio89_player *player);

#ifdef __cplusplus
}
#endif

#endif
