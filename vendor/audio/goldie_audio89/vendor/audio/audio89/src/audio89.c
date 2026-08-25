#include "audio89.h"

void audio89_init(audio89_player *p)
{
    p->provider = 0;
    p->state = 0;
    p->opened = 0;
}

void audio89_bind(audio89_player *p, const audio89_provider *provider, void *state)
{
    audio89_close(p);
    p->provider = provider;
    p->state = state;
}

int audio89_open(audio89_player *p, const char *path)
{
    if (p->provider == 0 || p->provider->open == 0) return 0;
    p->opened = p->provider->open(p->state, path);
    return p->opened;
}

int audio89_play(audio89_player *p, int loop)
{
    if (!p->opened || p->provider == 0 || p->provider->play == 0) return 0;
    return p->provider->play(p->state, loop);
}

void audio89_update(audio89_player *p)
{
    if (p->opened && p->provider != 0 && p->provider->update != 0) p->provider->update(p->state);
}

void audio89_pause(audio89_player *p)
{
    if (p->opened && p->provider != 0 && p->provider->pause != 0) p->provider->pause(p->state);
}

void audio89_resume(audio89_player *p)
{
    if (p->opened && p->provider != 0 && p->provider->resume != 0) p->provider->resume(p->state);
}

void audio89_stop(audio89_player *p)
{
    if (p->opened && p->provider != 0 && p->provider->stop != 0) p->provider->stop(p->state);
}

void audio89_close(audio89_player *p)
{
    if (p->opened && p->provider != 0 && p->provider->close != 0) p->provider->close(p->state);
    p->opened = 0;
}

const char *audio89_last_error(audio89_player *p)
{
    if (p->provider != 0 && p->provider->last_error != 0) return p->provider->last_error(p->state);
    return "audio provider unavailable";
}
