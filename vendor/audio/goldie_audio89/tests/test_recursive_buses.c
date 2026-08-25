#include "goldie_audio89.h"

#include <stdio.h>
#include <string.h>

static short test_pcm[2048];

static int fail(const char *text)
{
    printf("recursive bus FAIL: %s\n", text);
    return 1;
}

static short mul_q15(short a, short b)
{
    long value;
    if (a <= 0 || b <= 0) return (short)0;
    if (a >= 32767) return b;
    if (b >= 32767) return a;
    value = (long)a * (long)b;
    value += 16383L;
    value /= 32767L;
    if (value > 32767L) value = 32767L;
    return (short)value;
}

int main(void)
{
    goldie_audio89 g;
    goldie_audio89_config cfg;
    goldie_audio89_pcm_view pcm;
    goldie_audio89_bus_handle master;
    goldie_audio89_bus_handle dialogue;
    goldie_audio89_bus_handle sfx;
    goldie_audio89_bus_handle ui;
    goldie_audio89_bus_handle ambience;
    goldie_audio89_bus_handle music;
    goldie_audio89_bus_handle forest;
    goldie_audio89_bus_handle biofauna;
    goldie_audio89_bus_handle birds;
    goldie_audio89_bus_handle score;
    goldie_audio89_bus_handle rhythm;
    goldie_audio89_bus_handle drums;
    goldie_audio89_bus_handle chain_parent;
    goldie_audio89_bus_handle chain_child;
    goldie_audio89_bus_handle bus_of_voice;
    rm_voice_handle bird_voice;
    rm_voice_handle drum_voice;
    short gain;
    short pan;
    short expected;
    int muted;
    int rc;
    unsigned int i;
    unsigned int created_extra;

    memset(test_pcm, 0, sizeof(test_pcm));
    for (i = 0U; i < 2048U; ++i) {
        test_pcm[i] = (short)((i & 1U) ? 2000 : -2000);
    }

    goldie_audio89_config_init(&cfg);
    cfg.backend = KNM_BACKEND_NULL;
    cfg.sample_rate = 48000UL;
    cfg.channels = 2U;
    cfg.frames_per_buffer = 256U;
    cfg.buffer_count = 4U;
    cfg.max_buses = 32U;

    rc = goldie_audio89_init(&g, &cfg);
    if (rc != GOLDIE_AUDIO89_OK) return fail("init");
    if (goldie_audio89_bus_count(&g) != 1U) return fail("MASTER count");
    if (goldie_audio89_bus_capacity(&g) != 32U) return fail("configured bus capacity");

    master = goldie_audio89_master_bus(&g);
    if (!goldie_audio89_bus_is_valid(&g, master)) return fail("MASTER handle");

    if (goldie_audio89_bus_create(&g, master, "Dialogue", &dialogue) != GOLDIE_AUDIO89_OK) return fail("Dialogue");
    if (goldie_audio89_bus_create(&g, master, "SFX", &sfx) != GOLDIE_AUDIO89_OK) return fail("SFX");
    if (goldie_audio89_bus_create(&g, master, "UI", &ui) != GOLDIE_AUDIO89_OK) return fail("UI");
    if (goldie_audio89_bus_create(&g, master, "Ambience", &ambience) != GOLDIE_AUDIO89_OK) return fail("Ambience");
    if (goldie_audio89_bus_create(&g, master, "Music", &music) != GOLDIE_AUDIO89_OK) return fail("Music");
    (void)dialogue;
    (void)ui;

    if (goldie_audio89_bus_create(&g, ambience, "Forest", &forest) != GOLDIE_AUDIO89_OK) return fail("Forest");
    if (goldie_audio89_bus_create(&g, forest, "Biofauna", &biofauna) != GOLDIE_AUDIO89_OK) return fail("Biofauna");
    if (goldie_audio89_bus_create(&g, biofauna, "Birds", &birds) != GOLDIE_AUDIO89_OK) return fail("Birds");
    if (goldie_audio89_bus_create(&g, music, "Score", &score) != GOLDIE_AUDIO89_OK) return fail("Score");
    if (goldie_audio89_bus_create(&g, score, "Rhythm", &rhythm) != GOLDIE_AUDIO89_OK) return fail("Rhythm");
    if (goldie_audio89_bus_create(&g, rhythm, "Drums", &drums) != GOLDIE_AUDIO89_OK) return fail("Drums");

    if (goldie_audio89_bus_set_gain_q15(&g, ambience, (short)16384) != GOLDIE_AUDIO89_OK) return fail("ambience gain");
    if (goldie_audio89_bus_set_gain_q15(&g, forest, (short)16384) != GOLDIE_AUDIO89_OK) return fail("forest gain");
    if (goldie_audio89_bus_set_pan_q15(&g, biofauna, (short)-5000) != GOLDIE_AUDIO89_OK) return fail("biofauna pan");
    if (goldie_audio89_bus_set_pan_q15(&g, birds, (short)1000) != GOLDIE_AUDIO89_OK) return fail("birds pan");

    if (goldie_audio89_bus_get_effective(&g, birds, &gain, &pan, &muted) != GOLDIE_AUDIO89_OK) return fail("effective birds");
    expected = mul_q15((short)16384, (short)16384);
    if (gain != expected || pan != (short)-4000 || muted) return fail("recursive gain/pan inheritance");

    pcm.samples = test_pcm;
    pcm.frame_count = 1024UL;
    pcm.sample_rate = 44100UL;
    pcm.channels = 2U;
    if (goldie_audio89_play_pcm_s16_on_bus(&g, &pcm, 1, birds, &bird_voice) != GOLDIE_AUDIO89_OK) return fail("bird voice");
    if (goldie_audio89_play_pcm_s16_on_bus(&g, &pcm, 1, drums, &drum_voice) != GOLDIE_AUDIO89_OK) return fail("drum voice");

    if (g.mixer.voices[bird_voice.slot].gain_q15 != expected) return fail("bird inherited gain applied");
    if (g.mixer.voices[bird_voice.slot].pan_q15 != (short)-4000) return fail("bird inherited pan applied");
    if (g.mixer.voices[drum_voice.slot].gain_q15 != (short)32767) return fail("music branch independence");

    if (goldie_audio89_get_voice_bus(&g, bird_voice, &bus_of_voice) != GOLDIE_AUDIO89_OK) return fail("voice bus query");
    if (bus_of_voice.slot != birds.slot || bus_of_voice.generation != birds.generation) return fail("voice bus identity");

    if (goldie_audio89_bus_set_mute(&g, ambience, 1) != GOLDIE_AUDIO89_OK) return fail("ambience mute");
    if (g.mixer.voices[bird_voice.slot].gain_q15 != (short)0) return fail("recursive mute");
    if (g.mixer.voices[drum_voice.slot].gain_q15 != (short)32767) return fail("mute leaked to sibling root");
    if (goldie_audio89_bus_set_mute(&g, ambience, 0) != GOLDIE_AUDIO89_OK) return fail("ambience unmute");

    if (goldie_audio89_set_voice_gain_q15(&g, bird_voice, (short)16384) != GOLDIE_AUDIO89_OK) return fail("local voice gain");
    expected = mul_q15(expected, (short)16384);
    if (g.mixer.voices[bird_voice.slot].gain_q15 != expected) return fail("local x inherited gain");

    if (goldie_audio89_bus_reparent(&g, forest, sfx) != GOLDIE_AUDIO89_OK) return fail("reparent forest to SFX");
    if (goldie_audio89_bus_set_gain_q15(&g, sfx, (short)24576) != GOLDIE_AUDIO89_OK) return fail("SFX gain");
    if (goldie_audio89_bus_get_effective(&g, birds, &gain, &pan, &muted) != GOLDIE_AUDIO89_OK) return fail("effective after reparent");
    expected = mul_q15((short)24576, (short)16384);
    if (gain != expected) return fail("reparent inheritance");

    if (goldie_audio89_bus_reparent(&g, sfx, birds) != GOLDIE_AUDIO89_ERR_CYCLE) return fail("cycle detection");
    if (goldie_audio89_bus_destroy(&g, sfx) != GOLDIE_AUDIO89_ERR_BUSY) return fail("destroy parent with child");

    chain_parent = drums;
    created_extra = 0U;
    for (;;) {
        rc = goldie_audio89_bus_create(&g, chain_parent, "Deep", &chain_child);
        if (rc == GOLDIE_AUDIO89_ERR_CAPACITY) break;
        if (rc != GOLDIE_AUDIO89_OK) return fail("deep chain create");
        chain_parent = chain_child;
        ++created_extra;
        if (created_extra > 64U) return fail("capacity guard");
    }
    if (goldie_audio89_bus_count(&g) != goldie_audio89_bus_capacity(&g)) return fail("pool exhaustion count");
    if (created_extra == 0U) return fail("deep recursion capacity");
    if (goldie_audio89_bus_get_effective(&g, chain_parent, &gain, &pan, &muted) != GOLDIE_AUDIO89_OK) return fail("deep chain resolve");

    if (goldie_audio89_stop_all(&g) != GOLDIE_AUDIO89_OK) return fail("stop all");
    goldie_audio89_shutdown(&g);

    printf("Goldie recursive bus smoke: roots=5 buses=32 deep_extra=%u inherited_gain=%d pan=%d cycle=blocked mute=isolated\n",
           created_extra,
           (int)gain,
           (int)pan);
    return 0;
}
