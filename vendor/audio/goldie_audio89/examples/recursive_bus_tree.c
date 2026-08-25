#include "goldie_audio89.h"

/*
   Minimal hierarchy recipe. The five root buses are only a convention:
   Goldie itself allows any tree shape up to cfg.max_buses.
*/
int make_game_audio_tree(goldie_audio89 *g)
{
    goldie_audio89_bus_handle master;
    goldie_audio89_bus_handle dialogue;
    goldie_audio89_bus_handle sfx;
    goldie_audio89_bus_handle ui;
    goldie_audio89_bus_handle ambience;
    goldie_audio89_bus_handle music;
    goldie_audio89_bus_handle forest;
    goldie_audio89_bus_handle biofauna;
    goldie_audio89_bus_handle birds;
    int rc;

    master = goldie_audio89_master_bus(g);
    rc = goldie_audio89_bus_create(g, master, "Dialogue", &dialogue);
    if (rc != GOLDIE_AUDIO89_OK) return rc;
    rc = goldie_audio89_bus_create(g, master, "SFX", &sfx);
    if (rc != GOLDIE_AUDIO89_OK) return rc;
    rc = goldie_audio89_bus_create(g, master, "UI", &ui);
    if (rc != GOLDIE_AUDIO89_OK) return rc;
    rc = goldie_audio89_bus_create(g, master, "Ambience", &ambience);
    if (rc != GOLDIE_AUDIO89_OK) return rc;
    rc = goldie_audio89_bus_create(g, master, "Music", &music);
    if (rc != GOLDIE_AUDIO89_OK) return rc;

    rc = goldie_audio89_bus_create(g, ambience, "Forest", &forest);
    if (rc != GOLDIE_AUDIO89_OK) return rc;
    rc = goldie_audio89_bus_create(g, forest, "Biofauna", &biofauna);
    if (rc != GOLDIE_AUDIO89_OK) return rc;
    rc = goldie_audio89_bus_create(g, biofauna, "Birds", &birds);
    if (rc != GOLDIE_AUDIO89_OK) return rc;

    (void)dialogue;
    (void)sfx;
    (void)ui;
    (void)music;
    (void)birds;
    return GOLDIE_AUDIO89_OK;
}
