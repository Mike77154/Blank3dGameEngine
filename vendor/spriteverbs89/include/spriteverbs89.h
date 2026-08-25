#ifndef SPRITEVERBS89_H
#define SPRITEVERBS89_H

#include "spriteasset89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SV89_VERSION_MAJOR 0
#define SV89_VERSION_MINOR 1
#define SV89_VERSION_PATCH 0

#ifndef SV89_MAX_TARGETS
#define SV89_MAX_TARGETS 128
#endif
#ifndef SV89_TARGET_CAP
#define SV89_TARGET_CAP 64
#endif
#ifndef SV89_ARG_CAP
#define SV89_ARG_CAP 160
#endif

#define SV89_VERB_UNKNOWN   0
#define SV89_VERB_SHOW      1
#define SV89_VERB_HIDE      2
#define SV89_VERB_PLAY      3
#define SV89_VERB_STOP      4
#define SV89_VERB_SET_ASSET 5
#define SV89_VERB_SET_FRAME 6
#define SV89_VERB_SET_SPEED 7
#define SV89_VERB_RESET     8
#define SV89_VERB_FLIP_X    9
#define SV89_VERB_FLIP_Y   10
#define SV89_VERB_SET_X    11
#define SV89_VERB_SET_Y    12
#define SV89_VERB_SET_POS  13

typedef struct SV89_Target_s {
    char name[SV89_TARGET_CAP];
    sa89_id player_id;
    unsigned char used;
} SV89_Target;

typedef int (*sv89_lazy_asset_fn)(void *user, SpriteAsset89 *assets,
                                  const char *logical_name,
                                  sa89_id *out_asset_id);

typedef struct SpriteVerbs89_s {
    SpriteAsset89 *assets;
    SV89_Target targets[SV89_MAX_TARGETS];
    unsigned short target_count;
    sv89_lazy_asset_fn lazy_asset;
    void *lazy_asset_user;
    int last_error;
} SpriteVerbs89;

void sv89_init(SpriteVerbs89 *ctx, SpriteAsset89 *assets);
void sv89_set_lazy_asset_provider(SpriteVerbs89 *ctx, sv89_lazy_asset_fn fn, void *user);
int sv89_resolve_verb(const char *verb);
sa89_id sv89_bind_target(SpriteVerbs89 *ctx, const char *target_name);
sa89_id sv89_target_player(const SpriteVerbs89 *ctx, const char *target_name);
int sv89_execute(SpriteVerbs89 *ctx, const char *target_name,
                 const char *verb, const char *argument);
int sv89_execute_tokens(SpriteVerbs89 *ctx, int argc, const char **argv);
int sv89_parse_q16(const char *text, int *out_q16);

#ifdef __cplusplus
}
#endif
#endif
