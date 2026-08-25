#ifndef BULLETSPIN89_H
#define BULLETSPIN89_H

#ifdef __cplusplus
extern "C" {
#endif

#define BS89_DIRECTION_FORWARD 1
#define BS89_DIRECTION_REVERSE (-1)

typedef struct bs89_config_s {
    int slot_count;
    int start_slot;
    int step;
    int direction;
} bs89_config;

typedef struct bs89_state_s {
    int initialized;
    int slot_count;
    int current_slot;
    int step;
    int direction;
    unsigned long shot_count;
} bs89_state;

void bulletspin89_init(bs89_state *state, const bs89_config *config);
int bulletspin89_next(bs89_state *state, int *slot_out);
int bulletspin89_peek(const bs89_state *state, int *slot_out);
void bulletspin89_reset(bs89_state *state, const bs89_config *config);

#ifdef __cplusplus
}
#endif

#endif
