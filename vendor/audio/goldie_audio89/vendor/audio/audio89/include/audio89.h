#ifndef AUDIO89_H
#define AUDIO89_H

typedef struct audio89_provider {
    int (*open)(void *state, const char *path);
    int (*play)(void *state, int loop);
    void (*update)(void *state);
    void (*pause)(void *state);
    void (*resume)(void *state);
    void (*stop)(void *state);
    void (*close)(void *state);
    const char *(*last_error)(void *state);
} audio89_provider;

typedef struct audio89_player {
    const audio89_provider *provider;
    void *state;
    int opened;
} audio89_player;

void audio89_init(audio89_player *p);
void audio89_bind(audio89_player *p, const audio89_provider *provider, void *state);
int audio89_open(audio89_player *p, const char *path);
int audio89_play(audio89_player *p, int loop);
void audio89_update(audio89_player *p);
void audio89_pause(audio89_player *p);
void audio89_resume(audio89_player *p);
void audio89_stop(audio89_player *p);
void audio89_close(audio89_player *p);
const char *audio89_last_error(audio89_player *p);

#endif
