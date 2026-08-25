#include "goldie_audio89.h"
#include "mwav89.h"
#include "mpcm89.h"
#include "mp3_frame89.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include "mp3_acm_codec89.h"
#endif

static void goldie_copy_text(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (dst == (char *)0 || cap == 0U) return;
    if (src == (const char *)0) src = "";
    i = 0U;
    while (src[i] != '\0' && i + 1U < cap) { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}

static void goldie_set_error(goldie_audio89 *g, const char *text)
{
    if (g != (goldie_audio89 *)0) goldie_copy_text(g->error, GOLDIE_AUDIO89_ERROR_CAP, text);
}

static void goldie_zero_s16(short *dst, unsigned int count)
{
    unsigned int i;
    for (i = 0U; i < count; ++i) dst[i] = 0;
}

static void goldie_zero_fix(knm_fix16 *dst, unsigned int count)
{
    unsigned int i;
    for (i = 0U; i < count; ++i) dst[i] = KNM_FIX_ZERO;
}

static int goldie_console_slot_valid(const goldie_audio89 *g, unsigned short slot, unsigned short generation)
{
    const goldie_audio89_console_state *c;
    if (g == (const goldie_audio89 *)0 || slot >= g->console_limit) return 0;
    c = &g->consoles[slot];
    return c->active && c->generation == generation;
}

static unsigned int goldie_direct_children(const goldie_audio89 *g, unsigned short slot, unsigned short generation)
{
    unsigned int i;
    unsigned int n;
    n = 0U;
    if (!goldie_console_slot_valid(g, slot, generation)) return 0U;
    for (i = 1U; i < g->console_limit; ++i) {
        if (g->consoles[i].active && g->consoles[i].parent_slot == slot &&
            g->consoles[i].parent_generation == generation) ++n;
    }
    return n;
}

static unsigned int goldie_local_active_voices(const goldie_audio89_console_state *c)
{
    unsigned int i;
    unsigned int n;
    n = 0U;
    if (c == (const goldie_audio89_console_state *)0 || !c->active) return 0U;
    for (i = 0U; i < (unsigned int)c->mixer.max_voices; ++i) {
        if (c->mixer.voices[i].active) ++n;
    }
    return n;
}

static int goldie_init_console_mixer(goldie_audio89 *g, goldie_audio89_console_state *c)
{
    rm_engine_config mc;
    if (g == (goldie_audio89 *)0 || c == (goldie_audio89_console_state *)0) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    rm_engine_config_init(&mc);
    mc.sample_rate = (rm_u32)g->config.sample_rate;
    mc.channels = (rm_u16)g->config.channels;
    mc.max_voices = (rm_u16)g->config.max_voices_per_console;
    mc.capture_enabled = 0U;
    mc.default_resampler = (rm_u16)RM_RESAMPLER_CUBIC;
    mc.master_gain_q15 = (rm_s16)RM_Q15_ONE;
    mc.headroom_q15 = (rm_s16)RM_Q15_ONE;
    if (rm_engine_init(&c->mixer, &mc) != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    if (rm_engine_set_bus(&c->mixer, RM_BUS_DEFAULT, RM_Q15_ONE, RM_PAN_CENTER) != RM_OK)
        return GOLDIE_AUDIO89_ERR_MIXER;
    return GOLDIE_AUDIO89_OK;
}

static int goldie_depth_for_slot(const goldie_audio89 *g, unsigned short slot, unsigned short *out_depth)
{
    unsigned int steps;
    unsigned short depth;
    unsigned short cur;
    unsigned short gen;
    const goldie_audio89_console_state *c;
    if (g == (const goldie_audio89 *)0 || out_depth == (unsigned short *)0 || slot >= g->console_limit)
        return GOLDIE_AUDIO89_ERR_ARGUMENT;
    if (!g->consoles[slot].active) return GOLDIE_AUDIO89_ERR_CONSOLE;
    cur = slot;
    gen = g->consoles[slot].generation;
    depth = 0U;
    steps = 0U;
    while (cur != 0U) {
        if (steps++ >= g->console_limit) return GOLDIE_AUDIO89_ERR_CYCLE;
        if (!goldie_console_slot_valid(g, cur, gen)) return GOLDIE_AUDIO89_ERR_CONSOLE;
        c = &g->consoles[cur];
        if (c->parent_slot == GOLDIE_AUDIO89_INVALID_SLOT) return GOLDIE_AUDIO89_ERR_CONSOLE;
        cur = c->parent_slot;
        gen = c->parent_generation;
        ++depth;
    }
    *out_depth = depth;
    return GOLDIE_AUDIO89_OK;
}

static int goldie_rebuild_render_order(goldie_audio89 *g)
{
    unsigned int i;
    unsigned int j;
    unsigned int n;
    unsigned short depth;
    unsigned short key;
    unsigned short key_depth;
    if (g == (goldie_audio89 *)0) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    n = 0U;
    for (i = 0U; i < g->console_limit; ++i) {
        if (!g->consoles[i].active) continue;
        if (goldie_depth_for_slot(g, (unsigned short)i, &depth) != GOLDIE_AUDIO89_OK)
            return GOLDIE_AUDIO89_ERR_CYCLE;
        g->consoles[i].depth = depth;
        g->render_order[n++] = (unsigned short)i;
    }
    for (i = 1U; i < n; ++i) {
        key = g->render_order[i];
        key_depth = g->consoles[key].depth;
        j = i;
        while (j > 0U && g->consoles[g->render_order[j - 1U]].depth < key_depth) {
            g->render_order[j] = g->render_order[j - 1U];
            --j;
        }
        g->render_order[j] = key;
    }
    g->render_count = n;
    return GOLDIE_AUDIO89_OK;
}

static int goldie_suspend_for_control(goldie_audio89 *g)
{
    int was_running;
    if (g == (goldie_audio89 *)0 || !g->initialized) return 0;
    was_running = g->running && !g->paused;
    if (was_running) {
        if (knm_audio_stop(g->device) != KNM_OK) return -1;
        g->running = 0;
    }
    return was_running;
}

static int goldie_restore_after_control(goldie_audio89 *g, int was_running)
{
    int rc;
    if (was_running > 0) {
        rc = knm_audio_start(g->device);
        if (rc != KNM_OK) {
            goldie_set_error(g, knm_result_string(rc));
            return GOLDIE_AUDIO89_ERR_HARDWARE;
        }
        g->running = 1;
    }
    return GOLDIE_AUDIO89_OK;
}

static int goldie_render_graph(goldie_audio89 *g, unsigned int frames)
{
    unsigned int oi;
    unsigned int ci;
    goldie_audio89_console_state *c;
    goldie_audio89_console_state *child;
    rm_buffer b;
    rm_voice_params p;
    rm_voice_handle transient;
    rm_result rr;
    if (g == (goldie_audio89 *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE;
    if (frames == 0U || frames > KNM_MAX_FRAMES_PER_BUFFER) return GOLDIE_AUDIO89_ERR_ARGUMENT;

    for (oi = 0U; oi < g->render_count; ++oi) {
        c = &g->consoles[g->render_order[oi]];
        if (!c->active) continue;

        for (ci = 1U; ci < g->console_limit; ++ci) {
            child = &g->consoles[ci];
            if (!child->active || child->parent_slot != g->render_order[oi] ||
                child->parent_generation != c->generation) continue;
            b.samples = child->output;
            b.frame_count = (rm_u32)frames;
            b.sample_rate = (rm_u32)g->config.sample_rate;
            b.channels = (rm_u16)g->config.channels;
            b.format = (rm_u16)RM_SAMPLE_S16;
            b.interleaved = 1U;
            b.reserved = 0U;
            rm_voice_params_init(&p);
            p.gain_q15 = RM_Q15_ONE;
            p.pan_q15 = RM_PAN_CENTER;
            p.priority = (rm_u16)65535U;
            p.flags = (rm_u16)RM_VOICE_FLAG_PROTECTED;
            p.resampler = (rm_u16)RM_RESAMPLER_NEAREST;
            p.bus_id = RM_BUS_DEFAULT;
            rr = rm_engine_play_buffer(&c->mixer, &b, &p, &transient);
            if (rr != RM_OK) {
                g->stats.child_channel_failures++;
                goldie_set_error(g, "parent console ran out of rawmix input voices");
                return GOLDIE_AUDIO89_ERR_CAPACITY;
            }
            g->stats.child_channels_injected++;
        }

        rr = rm_engine_render_s16(&c->mixer, c->output, (rm_u32)frames);
        if (rr != RM_OK) {
            goldie_zero_s16(c->output, frames * g->config.channels);
            goldie_set_error(g, "rawmix subconsole render failed");
            return GOLDIE_AUDIO89_ERR_MIXER;
        }
    }
    g->stats.graph_renders++;
    return GOLDIE_AUDIO89_OK;
}

static void goldie_knm_callback(void *user_data, const knm_fix16 *input,
                                knm_fix16 *output, unsigned int frames, unsigned int channels)
{
    goldie_audio89 *g;
    unsigned int samples;
    unsigned int i;
    int rc;
    (void)input;
    g = (goldie_audio89 *)user_data;
    if (g == (goldie_audio89 *)0 || output == (knm_fix16 *)0) return;
    samples = frames * channels;
    if (frames > KNM_MAX_FRAMES_PER_BUFFER || channels > KNM_MAX_CHANNELS || channels != g->config.channels) {
        goldie_zero_fix(output, samples);
        return;
    }
    rc = goldie_render_graph(g, frames);
    if (rc != GOLDIE_AUDIO89_OK) {
        goldie_zero_fix(output, samples);
        return;
    }
    for (i = 0U; i < samples; ++i)
        output[i] = (knm_fix16)((knm_int32)g->consoles[0].output[i] * (knm_int32)2);
}

void goldie_audio89_config_init(goldie_audio89_config *cfg)
{
    if (cfg == (goldie_audio89_config *)0) return;
    cfg->sample_rate = 48000UL;
    cfg->channels = 2U;
    cfg->frames_per_buffer = 1024U;
    cfg->buffer_count = 4U;
    cfg->max_voices_per_console = RAWMIX_MAX_VOICES;
    cfg->max_consoles = GOLDIE_AUDIO89_MAX_CONSOLES;
    cfg->backend = KNM_BACKEND_DEFAULT;
    cfg->prefer_low_latency = 0;
}

int goldie_audio89_init(goldie_audio89 *g, const goldie_audio89_config *cfg)
{
    knm_audio_config kc;
    knm_audio_config actual;
    goldie_audio89_console_state *m;
    int rc;
    if (g == (goldie_audio89 *)0 || cfg == (const goldie_audio89_config *)0) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    if (cfg->channels == 0U || cfg->channels > 2U || cfg->sample_rate == 0UL) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    if (cfg->max_voices_per_console == 0U || cfg->max_voices_per_console > RAWMIX_MAX_VOICES) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    if (cfg->max_consoles == 0U || cfg->max_consoles > GOLDIE_AUDIO89_MAX_CONSOLES) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    memset(g, 0, sizeof(*g));
    g->config = *cfg;
    g->console_limit = cfg->max_consoles;

    knm_audio_config_init(&kc);
    kc.sample_rate = cfg->sample_rate;
    kc.channels = cfg->channels;
    kc.frames_per_buffer = cfg->frames_per_buffer;
    kc.buffer_count = cfg->buffer_count;
    kc.capture_ring_frames = 0U;
    kc.enable_output = 1;
    kc.enable_input = 0;
    kc.backend = cfg->backend;
    kc.prefer_low_latency = cfg->prefer_low_latency;
    rc = knm_audio_open(&g->device, &kc, goldie_knm_callback, g);
    if (rc != KNM_OK) {
        goldie_set_error(g, knm_result_string(rc));
        return GOLDIE_AUDIO89_ERR_HARDWARE;
    }
    actual = kc;
    rc = knm_audio_get_config(g->device, &actual);
    if (rc != KNM_OK || actual.sample_rate == 0UL || actual.channels == 0U || actual.channels > 2U) {
        knm_audio_close(g->device);
        g->device = (knm_device *)0;
        return GOLDIE_AUDIO89_ERR_HARDWARE;
    }
    g->config.sample_rate = actual.sample_rate;
    g->config.channels = actual.channels;
    g->config.frames_per_buffer = actual.frames_per_buffer;
    g->config.buffer_count = actual.buffer_count;
    g->config.backend = actual.backend;

    m = &g->consoles[0];
    memset(m, 0, sizeof(*m));
    m->active = 1U;
    m->generation = 1U;
    m->parent_slot = GOLDIE_AUDIO89_INVALID_SLOT;
    m->parent_generation = 0U;
    goldie_copy_text(m->name, GOLDIE_AUDIO89_CONSOLE_NAME_CAP, "MASTER");
    rc = goldie_init_console_mixer(g, m);
    if (rc != GOLDIE_AUDIO89_OK) {
        knm_audio_close(g->device);
        g->device = (knm_device *)0;
        return rc;
    }
    g->console_count = 1U;
    g->initialized = 1;
    if (goldie_rebuild_render_order(g) != GOLDIE_AUDIO89_OK) {
        goldie_audio89_shutdown(g);
        return GOLDIE_AUDIO89_ERR_CYCLE;
    }
    g->error[0] = '\0';
    return GOLDIE_AUDIO89_OK;
}

int goldie_audio89_start(goldie_audio89 *g)
{
    int rc;
    if (g == (goldie_audio89 *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE;
    if (g->running && !g->paused) return GOLDIE_AUDIO89_OK;
    if (g->paused) return goldie_audio89_resume(g);
    rc = knm_audio_start(g->device);
    if (rc != KNM_OK) { goldie_set_error(g, knm_result_string(rc)); return GOLDIE_AUDIO89_ERR_HARDWARE; }
    g->running = 1; g->paused = 0; return GOLDIE_AUDIO89_OK;
}

int goldie_audio89_pause(goldie_audio89 *g)
{
    int rc;
    if (g == (goldie_audio89 *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE;
    if (g->paused) return GOLDIE_AUDIO89_OK;
    if (!g->running) { g->paused = 1; return GOLDIE_AUDIO89_OK; }
    rc = knm_audio_stop(g->device);
    if (rc != KNM_OK) return GOLDIE_AUDIO89_ERR_HARDWARE;
    g->running = 0; g->paused = 1; return GOLDIE_AUDIO89_OK;
}

int goldie_audio89_resume(goldie_audio89 *g)
{
    int rc;
    if (g == (goldie_audio89 *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE;
    rc = knm_audio_start(g->device);
    if (rc != KNM_OK) return GOLDIE_AUDIO89_ERR_HARDWARE;
    g->running = 1; g->paused = 0; return GOLDIE_AUDIO89_OK;
}

int goldie_audio89_stop_device(goldie_audio89 *g)
{
    int rc;
    if (g == (goldie_audio89 *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE;
    if (!g->running) { g->paused = 0; return GOLDIE_AUDIO89_OK; }
    rc = knm_audio_stop(g->device);
    if (rc != KNM_OK) return GOLDIE_AUDIO89_ERR_HARDWARE;
    g->running = 0; g->paused = 0; return GOLDIE_AUDIO89_OK;
}

void goldie_audio89_shutdown(goldie_audio89 *g)
{
    if (g == (goldie_audio89 *)0) return;
    if (g->initialized && g->device != (knm_device *)0) {
        if (g->running) (void)knm_audio_stop(g->device);
        knm_audio_close(g->device);
    }
    g->device = (knm_device *)0;
    g->initialized = 0; g->running = 0; g->paused = 0;
}

goldie_audio89_console_handle goldie_audio89_invalid_console(void)
{
    goldie_audio89_console_handle h; h.slot = GOLDIE_AUDIO89_INVALID_SLOT; h.generation = 0U; return h;
}

goldie_audio89_console_handle goldie_audio89_master_console(const goldie_audio89 *g)
{
    goldie_audio89_console_handle h;
    if (g == (const goldie_audio89 *)0 || !g->consoles[0].active) return goldie_audio89_invalid_console();
    h.slot = 0U; h.generation = g->consoles[0].generation; return h;
}

int goldie_audio89_console_is_valid(const goldie_audio89 *g, goldie_audio89_console_handle c)
{ return goldie_console_slot_valid(g, c.slot, c.generation); }

int goldie_audio89_console_create(goldie_audio89 *g, goldie_audio89_console_handle parent,
                                  const char *name, goldie_audio89_console_handle *out_console)
{
    unsigned int i;
    int was_running;
    int rc;
    int restore_rc;
    goldie_audio89_console_state *c;
    if (g == (goldie_audio89 *)0 || out_console == (goldie_audio89_console_handle *)0 || !g->initialized)
        return GOLDIE_AUDIO89_ERR_ARGUMENT;
    if (!goldie_audio89_console_is_valid(g, parent)) return GOLDIE_AUDIO89_ERR_CONSOLE;
    if (g->console_count >= g->console_limit) return GOLDIE_AUDIO89_ERR_CAPACITY;
    if (goldie_direct_children(g, parent.slot, parent.generation) >= g->config.max_voices_per_console)
        return GOLDIE_AUDIO89_ERR_CAPACITY;
    was_running = goldie_suspend_for_control(g);
    if (was_running < 0) return GOLDIE_AUDIO89_ERR_HARDWARE;
    for (i = 1U; i < g->console_limit; ++i) {
        if (!g->consoles[i].active) {
            c = &g->consoles[i];
            if (c->generation == 0U) c->generation = 1U;
            else { c->generation++; if (c->generation == 0U) c->generation = 1U; }
            c->active = 1U;
            c->parent_slot = parent.slot;
            c->parent_generation = parent.generation;
            goldie_copy_text(c->name, GOLDIE_AUDIO89_CONSOLE_NAME_CAP, name);
            rc = goldie_init_console_mixer(g, c);
            if (rc != GOLDIE_AUDIO89_OK) { c->active = 0U; (void)goldie_restore_after_control(g, was_running); return rc; }
            ++g->console_count;
            rc = goldie_rebuild_render_order(g);
            out_console->slot = (unsigned short)i;
            out_console->generation = c->generation;
            restore_rc = goldie_restore_after_control(g, was_running);
            if (rc != GOLDIE_AUDIO89_OK) return rc;
            return restore_rc;
        }
    }
    (void)goldie_restore_after_control(g, was_running);
    return GOLDIE_AUDIO89_ERR_CAPACITY;
}

int goldie_audio89_console_destroy(goldie_audio89 *g, goldie_audio89_console_handle c)
{
    unsigned int i;
    int was_running;
    int rc;
    goldie_audio89_console_state *s;
    if (g == (goldie_audio89 *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE;
    if (!goldie_audio89_console_is_valid(g, c)) return GOLDIE_AUDIO89_ERR_CONSOLE;
    if (c.slot == 0U) return GOLDIE_AUDIO89_ERR_BUSY;
    if (goldie_direct_children(g, c.slot, c.generation) != 0U) return GOLDIE_AUDIO89_ERR_BUSY;
    s = &g->consoles[c.slot];
    for (i = 0U; i < (unsigned int)s->mixer.max_voices; ++i) if (s->mixer.voices[i].active) return GOLDIE_AUDIO89_ERR_BUSY;
    was_running = goldie_suspend_for_control(g);
    if (was_running < 0) return GOLDIE_AUDIO89_ERR_HARDWARE;
    s->active = 0U;
    s->parent_slot = GOLDIE_AUDIO89_INVALID_SLOT;
    s->parent_generation = 0U;
    --g->console_count;
    rc = goldie_rebuild_render_order(g);
    if (goldie_restore_after_control(g, was_running) != GOLDIE_AUDIO89_OK) return GOLDIE_AUDIO89_ERR_HARDWARE;
    return rc;
}

int goldie_audio89_console_reparent(goldie_audio89 *g, goldie_audio89_console_handle c,
                                    goldie_audio89_console_handle new_parent)
{
    unsigned short old_parent;
    unsigned short old_gen;
    int was_running;
    int rc;
    int restore_rc;
    if (g == (goldie_audio89 *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE;
    if (!goldie_audio89_console_is_valid(g, c) || !goldie_audio89_console_is_valid(g, new_parent)) return GOLDIE_AUDIO89_ERR_CONSOLE;
    if (c.slot == 0U || c.slot == new_parent.slot) return GOLDIE_AUDIO89_ERR_CYCLE;
    if (goldie_direct_children(g, new_parent.slot, new_parent.generation) >= g->config.max_voices_per_console)
        return GOLDIE_AUDIO89_ERR_CAPACITY;
    was_running = goldie_suspend_for_control(g);
    if (was_running < 0) return GOLDIE_AUDIO89_ERR_HARDWARE;
    old_parent = g->consoles[c.slot].parent_slot;
    old_gen = g->consoles[c.slot].parent_generation;
    g->consoles[c.slot].parent_slot = new_parent.slot;
    g->consoles[c.slot].parent_generation = new_parent.generation;
    rc = goldie_rebuild_render_order(g);
    if (rc != GOLDIE_AUDIO89_OK) {
        g->consoles[c.slot].parent_slot = old_parent;
        g->consoles[c.slot].parent_generation = old_gen;
        (void)goldie_rebuild_render_order(g);
    }
    restore_rc = goldie_restore_after_control(g, was_running);
    if (rc != GOLDIE_AUDIO89_OK) return rc;
    return restore_rc;
}

int goldie_audio89_console_parent(const goldie_audio89 *g, goldie_audio89_console_handle c,
                                  goldie_audio89_console_handle *out_parent)
{
    const goldie_audio89_console_state *s;
    if (g == (const goldie_audio89 *)0 || out_parent == (goldie_audio89_console_handle *)0 ||
        !goldie_audio89_console_is_valid(g, c)) return GOLDIE_AUDIO89_ERR_CONSOLE;
    if (c.slot == 0U) { *out_parent = goldie_audio89_invalid_console(); return GOLDIE_AUDIO89_OK; }
    s = &g->consoles[c.slot]; out_parent->slot = s->parent_slot; out_parent->generation = s->parent_generation;
    return GOLDIE_AUDIO89_OK;
}

const char *goldie_audio89_console_name(const goldie_audio89 *g, goldie_audio89_console_handle c)
{ if (!goldie_audio89_console_is_valid(g, c)) return ""; return g->consoles[c.slot].name; }

unsigned int goldie_audio89_console_count(const goldie_audio89 *g)
{ return g == (const goldie_audio89 *)0 ? 0U : g->console_count; }
unsigned int goldie_audio89_console_capacity(const goldie_audio89 *g)
{ return g == (const goldie_audio89 *)0 ? 0U : g->console_limit; }
unsigned int goldie_audio89_console_child_count(const goldie_audio89 *g, goldie_audio89_console_handle c)
{ return goldie_audio89_console_is_valid(g, c) ? goldie_direct_children(g, c.slot, c.generation) : 0U; }
unsigned int goldie_audio89_console_active_voice_count(const goldie_audio89 *g, goldie_audio89_console_handle c)
{ return goldie_audio89_console_is_valid(g, c) ? goldie_local_active_voices(&g->consoles[c.slot]) : 0U; }

static int goldie_console_control_begin(goldie_audio89 *g, goldie_audio89_console_handle c, int *out_was)
{
    if (g == (goldie_audio89 *)0 || out_was == (int *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE;
    if (!goldie_audio89_console_is_valid(g, c)) return GOLDIE_AUDIO89_ERR_CONSOLE;
    *out_was = goldie_suspend_for_control(g);
    return *out_was < 0 ? GOLDIE_AUDIO89_ERR_HARDWARE : GOLDIE_AUDIO89_OK;
}

int goldie_audio89_console_set_gain_q15(goldie_audio89 *g, goldie_audio89_console_handle c, short gain_q15)
{
    int was; rm_result rr; int rc;
    if (gain_q15 < 0) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    rc = goldie_console_control_begin(g, c, &was); if (rc != GOLDIE_AUDIO89_OK) return rc;
    rr = rm_engine_set_bus(&g->consoles[c.slot].mixer, RM_BUS_DEFAULT, (rm_s16)gain_q15,
                           g->consoles[c.slot].mixer.buses[RM_BUS_DEFAULT].pan_q15);
    rc = goldie_restore_after_control(g, was);
    if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}

int goldie_audio89_console_set_pan_q15(goldie_audio89 *g, goldie_audio89_console_handle c, short pan_q15)
{
    int was; rm_result rr; int rc;
    rc = goldie_console_control_begin(g, c, &was); if (rc != GOLDIE_AUDIO89_OK) return rc;
    rr = rm_engine_set_bus(&g->consoles[c.slot].mixer, RM_BUS_DEFAULT,
                           g->consoles[c.slot].mixer.buses[RM_BUS_DEFAULT].gain_q15, (rm_s16)pan_q15);
    rc = goldie_restore_after_control(g, was);
    if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}

int goldie_audio89_console_set_mute(goldie_audio89 *g, goldie_audio89_console_handle c, int mute_on)
{
    int was; rm_result rr; int rc;
    rc = goldie_console_control_begin(g, c, &was); if (rc != GOLDIE_AUDIO89_OK) return rc;
    rr = rm_engine_set_bus_mute(&g->consoles[c.slot].mixer, RM_BUS_DEFAULT, mute_on ? 1U : 0U);
    rc = goldie_restore_after_control(g, was);
    if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}

int goldie_audio89_console_fx_clear(goldie_audio89 *g, goldie_audio89_console_handle c, unsigned int slot)
{
    int was; rm_result rr; int rc;
    rc = goldie_console_control_begin(g, c, &was); if (rc != GOLDIE_AUDIO89_OK) return rc;
    rr = rm_engine_bus_fx_clear(&g->consoles[c.slot].mixer, RM_BUS_DEFAULT, (rm_u16)slot);
    rc = goldie_restore_after_control(g, was); if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}

int goldie_audio89_console_fx_lowpass(goldie_audio89 *g, goldie_audio89_console_handle c, unsigned int slot,
                                      unsigned long cutoff_hz, short wet_q15, short output_gain_q15)
{
    int was; rm_result rr; int rc;
    rc = goldie_console_control_begin(g, c, &was); if (rc != GOLDIE_AUDIO89_OK) return rc;
    rr = rm_engine_bus_fx_set_lowpass(&g->consoles[c.slot].mixer, RM_BUS_DEFAULT, (rm_u16)slot,
                                      (rm_u32)cutoff_hz, (rm_s16)wet_q15, (rm_s16)output_gain_q15);
    rc = goldie_restore_after_control(g, was); if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}

int goldie_audio89_console_fx_drive(goldie_audio89 *g, goldie_audio89_console_handle c, unsigned int slot,
                                    unsigned int drive_q12, short threshold_q15, short wet_q15, short output_gain_q15)
{
    int was; rm_result rr; int rc;
    rc = goldie_console_control_begin(g, c, &was); if (rc != GOLDIE_AUDIO89_OK) return rc;
    rr = rm_engine_bus_fx_set_drive(&g->consoles[c.slot].mixer, RM_BUS_DEFAULT, (rm_u16)slot,
                                    (rm_u16)drive_q12, (rm_s16)threshold_q15, (rm_s16)wet_q15,
                                    (rm_s16)output_gain_q15);
    rc = goldie_restore_after_control(g, was); if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}

int goldie_audio89_console_set_limiter(goldie_audio89 *g, goldie_audio89_console_handle c,
                                       short threshold_q15, unsigned int attack_frames,
                                       unsigned int release_frames, short output_gain_q15,
                                       unsigned int lookahead_frames)
{
    int was; rm_result rr; int rc;
    rc = goldie_console_control_begin(g, c, &was); if (rc != GOLDIE_AUDIO89_OK) return rc;
    rr = rm_engine_set_limiter_ex(&g->consoles[c.slot].mixer, (rm_s16)threshold_q15,
                                  (rm_u16)attack_frames, (rm_u16)release_frames,
                                  (rm_s16)output_gain_q15, (rm_u16)lookahead_frames);
    rc = goldie_restore_after_control(g, was); if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}

int goldie_audio89_console_clear_limiter(goldie_audio89 *g, goldie_audio89_console_handle c)
{
    int was; rm_result rr; int rc;
    rc = goldie_console_control_begin(g, c, &was); if (rc != GOLDIE_AUDIO89_OK) return rc;
    rr = rm_engine_clear_limiter(&g->consoles[c.slot].mixer);
    rc = goldie_restore_after_control(g, was); if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}

int goldie_audio89_console_get_meter(const goldie_audio89 *g, goldie_audio89_console_handle c, rm_meter_state *out_meter)
{
    if (out_meter == (rm_meter_state *)0 || !goldie_audio89_console_is_valid(g, c)) return GOLDIE_AUDIO89_ERR_CONSOLE;
    return rm_engine_get_master_meter(&g->consoles[c.slot].mixer, out_meter) == RM_OK ? GOLDIE_AUDIO89_OK : GOLDIE_AUDIO89_ERR_MIXER;
}

int goldie_audio89_console_get_mixer_stats(const goldie_audio89 *g, goldie_audio89_console_handle c, rm_engine_stats *out_stats)
{
    if (out_stats == (rm_engine_stats *)0 || !goldie_audio89_console_is_valid(g, c)) return GOLDIE_AUDIO89_ERR_CONSOLE;
    rm_engine_get_stats(&g->consoles[c.slot].mixer, out_stats); return GOLDIE_AUDIO89_OK;
}

int goldie_audio89_console_play_pcm_s16(goldie_audio89 *g, goldie_audio89_console_handle c,
                                        const goldie_audio89_pcm_view *pcm, int loop,
                                        goldie_audio89_voice_handle *out_voice)
{
    rm_buffer b; rm_voice_params p; rm_voice_handle h; rm_result rr;
    int was; int rc; int restore_rc;
    unsigned int children; unsigned int locals;
    if (g == (goldie_audio89 *)0 || pcm == (const goldie_audio89_pcm_view *)0 ||
        out_voice == (goldie_audio89_voice_handle *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    if (!goldie_audio89_console_is_valid(g, c) || pcm->samples == (const short *)0 || pcm->frame_count == 0UL ||
        (pcm->channels != 1U && pcm->channels != 2U) || pcm->sample_rate == 0UL) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    was = goldie_suspend_for_control(g); if (was < 0) return GOLDIE_AUDIO89_ERR_HARDWARE;
    children = goldie_direct_children(g, c.slot, c.generation);
    locals = goldie_local_active_voices(&g->consoles[c.slot]);
    if (children + locals >= g->config.max_voices_per_console) {
        (void)goldie_restore_after_control(g, was); return GOLDIE_AUDIO89_ERR_CAPACITY;
    }
    b.samples = pcm->samples; b.frame_count = (rm_u32)pcm->frame_count; b.sample_rate = (rm_u32)pcm->sample_rate;
    b.channels = (rm_u16)pcm->channels; b.format = RM_SAMPLE_S16; b.interleaved = 1U; b.reserved = 0U;
    rm_voice_params_init(&p); p.flags = loop ? (rm_u16)RM_VOICE_FLAG_LOOP : 0U; p.bus_id = RM_BUS_DEFAULT;
    p.resampler = (rm_u16)RM_RESAMPLER_CUBIC;
    rr = rm_engine_play_buffer(&g->consoles[c.slot].mixer, &b, &p, &h);
    restore_rc = goldie_restore_after_control(g, was);
    if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    out_voice->console_slot = c.slot; out_voice->console_generation = c.generation; out_voice->raw = h;
    rc = restore_rc; return rc;
}

static goldie_audio89_console_state *goldie_voice_console(goldie_audio89 *g, goldie_audio89_voice_handle v)
{
    if (g == (goldie_audio89 *)0 || !goldie_console_slot_valid(g, v.console_slot, v.console_generation)) return (goldie_audio89_console_state *)0;
    return &g->consoles[v.console_slot];
}

int goldie_audio89_voice_stop(goldie_audio89 *g, goldie_audio89_voice_handle voice)
{
    goldie_audio89_console_state *c; int was; rm_result rr; int rc;
    c = goldie_voice_console(g, voice); if (c == (goldie_audio89_console_state *)0) return GOLDIE_AUDIO89_ERR_CONSOLE;
    was = goldie_suspend_for_control(g); if (was < 0) return GOLDIE_AUDIO89_ERR_HARDWARE;
    rr = rm_engine_stop_voice(&c->mixer, voice.raw); rc = goldie_restore_after_control(g, was);
    if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}
int goldie_audio89_voice_set_gain_q15(goldie_audio89 *g, goldie_audio89_voice_handle voice, short gain_q15)
{
    goldie_audio89_console_state *c; int was; rm_result rr; int rc;
    c = goldie_voice_console(g, voice); if (c == (goldie_audio89_console_state *)0 || gain_q15 < 0) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    was = goldie_suspend_for_control(g); if (was < 0) return GOLDIE_AUDIO89_ERR_HARDWARE;
    rr = rm_engine_set_voice_gain(&c->mixer, voice.raw, (rm_s16)gain_q15); rc = goldie_restore_after_control(g, was);
    if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}
int goldie_audio89_voice_set_pan_q15(goldie_audio89 *g, goldie_audio89_voice_handle voice, short pan_q15)
{
    goldie_audio89_console_state *c; int was; rm_result rr; int rc;
    c = goldie_voice_console(g, voice); if (c == (goldie_audio89_console_state *)0) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    was = goldie_suspend_for_control(g); if (was < 0) return GOLDIE_AUDIO89_ERR_HARDWARE;
    rr = rm_engine_set_voice_pan(&c->mixer, voice.raw, (rm_s16)pan_q15); rc = goldie_restore_after_control(g, was);
    if (rr != RM_OK) return GOLDIE_AUDIO89_ERR_MIXER;
    return rc;
}
int goldie_audio89_voice_is_active(const goldie_audio89 *g, goldie_audio89_voice_handle voice)
{
    if (g == (const goldie_audio89 *)0 || !goldie_console_slot_valid(g, voice.console_slot, voice.console_generation)) return 0;
    return rm_engine_is_voice_active(&g->consoles[voice.console_slot].mixer, voice.raw);
}

int goldie_audio89_stop_all(goldie_audio89 *g)
{
    unsigned int i; int was; int rc;
    if (g == (goldie_audio89 *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE;
    was = goldie_suspend_for_control(g); if (was < 0) return GOLDIE_AUDIO89_ERR_HARDWARE;
    for (i = 0U; i < g->console_limit; ++i) if (g->consoles[i].active) rm_engine_reset(&g->consoles[i].mixer);
    rc = goldie_restore_after_control(g, was); return rc;
}

int goldie_audio89_render_s16(goldie_audio89 *g, short *dst_interleaved, unsigned int frames)
{
    int rc; unsigned int samples; unsigned int i;
    if (g == (goldie_audio89 *)0 || dst_interleaved == (short *)0 || g->running) return GOLDIE_AUDIO89_ERR_STATE;
    rc = goldie_render_graph(g, frames); if (rc != GOLDIE_AUDIO89_OK) return rc;
    samples = frames * g->config.channels;
    for (i = 0U; i < samples; ++i) dst_interleaved[i] = g->consoles[0].output[i];
    return GOLDIE_AUDIO89_OK;
}
void goldie_audio89_get_stats(const goldie_audio89 *g, goldie_audio89_stats *out_stats)
{ if (g != (const goldie_audio89 *)0 && out_stats != (goldie_audio89_stats *)0) *out_stats = g->stats; }

/* MASTER-only compatibility surface. */
int goldie_audio89_play_pcm_s16(goldie_audio89 *g, const goldie_audio89_pcm_view *pcm, int loop, rm_voice_handle *out_voice)
{
    goldie_audio89_voice_handle v; int rc;
    if (out_voice == (rm_voice_handle *)0) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    rc = goldie_audio89_console_play_pcm_s16(g, goldie_audio89_master_console(g), pcm, loop, &v);
    if (rc == GOLDIE_AUDIO89_OK) *out_voice = v.raw;
    return rc;
}
int goldie_audio89_stop_voice(goldie_audio89 *g, rm_voice_handle voice)
{ goldie_audio89_voice_handle v; v.console_slot = 0U; v.console_generation = g->consoles[0].generation; v.raw = voice; return goldie_audio89_voice_stop(g, v); }
int goldie_audio89_set_master_gain_q15(goldie_audio89 *g, short gain_q15)
{ return goldie_audio89_console_set_gain_q15(g, goldie_audio89_master_console(g), gain_q15); }
int goldie_audio89_set_voice_gain_q15(goldie_audio89 *g, rm_voice_handle voice, short gain_q15)
{ goldie_audio89_voice_handle v; v.console_slot = 0U; v.console_generation = g->consoles[0].generation; v.raw = voice; return goldie_audio89_voice_set_gain_q15(g, v, gain_q15); }
int goldie_audio89_set_voice_pan_q15(goldie_audio89 *g, rm_voice_handle voice, short pan_q15)
{ goldie_audio89_voice_handle v; v.console_slot = 0U; v.console_generation = g->consoles[0].generation; v.raw = voice; return goldie_audio89_voice_set_pan_q15(g, v, pan_q15); }
int goldie_audio89_is_voice_active(const goldie_audio89 *g, rm_voice_handle voice)
{ return g != (const goldie_audio89 *)0 && g->initialized ? rm_engine_is_voice_active(&g->consoles[0].mixer, voice) : 0; }
int goldie_audio89_get_mixer_stats(const goldie_audio89 *g, rm_engine_stats *out_stats)
{ return goldie_audio89_console_get_mixer_stats(g, goldie_audio89_master_console(g), out_stats); }

int goldie_audio89_decode_wav_memory(const void *wav_bytes, unsigned long wav_byte_count,
                                     short *pcm_dst, unsigned long pcm_sample_capacity,
                                     goldie_audio89_pcm_view *out_pcm)
{
    mwav89_view w; mpcm89_format f; mpcm89_clip clip; unsigned long samples; unsigned long i; int value;
    if (wav_bytes == (const void *)0 || pcm_dst == (short *)0 || out_pcm == (goldie_audio89_pcm_view *)0) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    if (wav_byte_count > 0xFFFFFFFFUL) return GOLDIE_AUDIO89_ERR_CAPACITY;
    if (mwav89_parse_memory(wav_bytes, (mwav89_u32)wav_byte_count, &w) != MWAV89_OK) return GOLDIE_AUDIO89_ERR_WAV;
    f.channels = w.channels; f.sample_rate = w.sample_rate; f.byte_rate = w.byte_rate; f.block_align = w.block_align; f.bits_per_sample = w.bits_per_sample;
    if (mpcm89_clip_init(&clip, w.pcm_data, w.pcm_bytes, &f) != MPCM89_OK) return GOLDIE_AUDIO89_ERR_PCM;
    samples = (unsigned long)clip.frame_count * (unsigned long)clip.format.channels;
    if (samples > pcm_sample_capacity) return GOLDIE_AUDIO89_ERR_CAPACITY;
    if (clip.format.bits_per_sample == 16U) {
        for (i = 0UL; i < samples; ++i) { unsigned long bi; unsigned int u; bi = i * 2UL; u = (unsigned int)clip.bytes[bi] | ((unsigned int)clip.bytes[bi + 1UL] << 8); pcm_dst[i] = (short)u; }
    } else if (clip.format.bits_per_sample == 8U) {
        for (i = 0UL; i < samples; ++i) { value = (int)clip.bytes[i] - 128; pcm_dst[i] = (short)(value * 256); }
    } else return GOLDIE_AUDIO89_ERR_UNSUPPORTED;
    out_pcm->samples = pcm_dst; out_pcm->frame_count = (unsigned long)clip.frame_count; out_pcm->sample_rate = (unsigned long)clip.format.sample_rate; out_pcm->channels = (unsigned int)clip.format.channels;
    return GOLDIE_AUDIO89_OK;
}

int goldie_audio89_decode_wav_file(const char *path, unsigned char *file_workspace,
                                   unsigned long file_workspace_bytes, short *pcm_dst,
                                   unsigned long pcm_sample_capacity, goldie_audio89_pcm_view *out_pcm)
{
    FILE *fp; long size; size_t got;
    if (path == (const char *)0 || file_workspace == (unsigned char *)0 || file_workspace_bytes == 0UL) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    fp = fopen(path, "rb"); if (fp == (FILE *)0) return GOLDIE_AUDIO89_ERR_FILE;
    if (fseek(fp, 0L, SEEK_END) != 0) { fclose(fp); return GOLDIE_AUDIO89_ERR_FILE; }
    size = ftell(fp); if (size <= 0L || (unsigned long)size > file_workspace_bytes) { fclose(fp); return size <= 0L ? GOLDIE_AUDIO89_ERR_FILE : GOLDIE_AUDIO89_ERR_CAPACITY; }
    if (fseek(fp, 0L, SEEK_SET) != 0) { fclose(fp); return GOLDIE_AUDIO89_ERR_FILE; }
    got = fread(file_workspace, 1U, (size_t)size, fp); fclose(fp); if (got != (size_t)size) return GOLDIE_AUDIO89_ERR_FILE;
    return goldie_audio89_decode_wav_memory(file_workspace, (unsigned long)size, pcm_dst, pcm_sample_capacity, out_pcm);
}

int goldie_audio89_decode_mp3_file(const char *path, short *pcm_dst,
                                   unsigned long pcm_sample_capacity, goldie_audio89_pcm_view *out_pcm,
                                   char *error_text, unsigned int error_text_capacity)
{
#if defined(_WIN32) || defined(_WIN64)
    FILE *fp; unsigned long first_offset; mp3_frame89_info first_info; mp3_frame89_info info; mp3_acm_codec89 codec;
    unsigned char mp3_frame[MP3_FRAME89_MAX_FRAME_BYTES]; unsigned char pcm_temp[MP3_ACM_CODEC89_PCM_CAP];
    unsigned long sample_count; int frame_bytes; int pcm_bytes; int samples; int i;
    if (path == (const char *)0 || pcm_dst == (short *)0 || out_pcm == (goldie_audio89_pcm_view *)0) return GOLDIE_AUDIO89_ERR_ARGUMENT;
    if (error_text != (char *)0 && error_text_capacity > 0U) error_text[0] = '\0';
    fp = fopen(path, "rb"); if (fp == (FILE *)0) { goldie_copy_text(error_text, error_text_capacity, "cannot open MP3 file"); return GOLDIE_AUDIO89_ERR_FILE; }
    if (!mp3_frame89_find_first(fp, &first_offset, &first_info)) { fclose(fp); goldie_copy_text(error_text, error_text_capacity, "no valid MPEG Layer III frame found"); return GOLDIE_AUDIO89_ERR_MP3; }
    mp3_acm_codec89_init(&codec); if (!mp3_acm_codec89_open(&codec, &first_info)) { goldie_copy_text(error_text, error_text_capacity, mp3_acm_codec89_last_error(&codec)); fclose(fp); return GOLDIE_AUDIO89_ERR_MP3; }
    if (fseek(fp, (long)first_offset, SEEK_SET) != 0) { mp3_acm_codec89_close(&codec); fclose(fp); return GOLDIE_AUDIO89_ERR_FILE; }
    sample_count = 0UL;
    for (;;) {
        frame_bytes = mp3_frame89_read_next(fp, mp3_frame, (int)sizeof(mp3_frame), &info); if (frame_bytes <= 0) break;
        if (info.sample_rate != first_info.sample_rate || info.channels != first_info.channels || info.mpeg_version != first_info.mpeg_version) { mp3_acm_codec89_close(&codec); fclose(fp); return GOLDIE_AUDIO89_ERR_MP3; }
        pcm_bytes = 0;
        if (!mp3_acm_codec89_decode(&codec, mp3_frame, frame_bytes, pcm_temp, (int)sizeof(pcm_temp), &pcm_bytes, 0)) { goldie_copy_text(error_text, error_text_capacity, mp3_acm_codec89_last_error(&codec)); mp3_acm_codec89_close(&codec); fclose(fp); return GOLDIE_AUDIO89_ERR_MP3; }
        if ((pcm_bytes & 1) != 0) { mp3_acm_codec89_close(&codec); fclose(fp); return GOLDIE_AUDIO89_ERR_MP3; }
        samples = pcm_bytes / 2;
        if (sample_count + (unsigned long)samples > pcm_sample_capacity) { mp3_acm_codec89_close(&codec); fclose(fp); goldie_copy_text(error_text, error_text_capacity, "PCM workspace is too small for decoded MP3"); return GOLDIE_AUDIO89_ERR_CAPACITY; }
        for (i = 0; i < samples; ++i) { unsigned int u; u = (unsigned int)pcm_temp[i * 2] | ((unsigned int)pcm_temp[i * 2 + 1] << 8); pcm_dst[sample_count + (unsigned long)i] = (short)u; }
        sample_count += (unsigned long)samples;
    }
    mp3_acm_codec89_close(&codec); fclose(fp);
    if (sample_count == 0UL) return GOLDIE_AUDIO89_ERR_MP3;
    out_pcm->samples = pcm_dst; out_pcm->channels = (unsigned int)first_info.channels; out_pcm->sample_rate = (unsigned long)first_info.sample_rate; out_pcm->frame_count = sample_count / (unsigned long)first_info.channels;
    return GOLDIE_AUDIO89_OK;
#else
    (void)path; (void)pcm_dst; (void)pcm_sample_capacity; (void)out_pcm;
    goldie_copy_text(error_text, error_text_capacity, "MP3 ACM provider is Win32-only"); return GOLDIE_AUDIO89_ERR_UNSUPPORTED;
#endif
}

int goldie_audio89_service(goldie_audio89 *g, unsigned int frames)
{
    int rc; if (g == (goldie_audio89 *)0 || !g->initialized || g->device == (knm_device *)0) return GOLDIE_AUDIO89_ERR_STATE;
    rc = knm_audio_service(g->device, frames); if (rc != KNM_OK) { goldie_set_error(g, knm_result_string(rc)); return GOLDIE_AUDIO89_ERR_HARDWARE; } return GOLDIE_AUDIO89_OK;
}
int goldie_audio89_backend(const goldie_audio89 *g)
{ return g == (const goldie_audio89 *)0 || !g->initialized ? KNM_BACKEND_DEFAULT : knm_audio_backend(g->device); }
int goldie_audio89_get_hardware_config(const goldie_audio89 *g, goldie_audio89_config *out_cfg)
{ if (g == (const goldie_audio89 *)0 || out_cfg == (goldie_audio89_config *)0 || !g->initialized) return GOLDIE_AUDIO89_ERR_STATE; *out_cfg = g->config; return GOLDIE_AUDIO89_OK; }
unsigned long goldie_audio89_output_latency_frames(const goldie_audio89 *g)
{ return g == (const goldie_audio89 *)0 || !g->initialized ? 0UL : knm_audio_output_latency_frames(g->device); }
const char *goldie_audio89_last_error(const goldie_audio89 *g)
{ if (g == (const goldie_audio89 *)0) return "Goldie state is null"; return g->error[0] ? g->error : "no Goldie error"; }
const char *goldie_audio89_result_string(int result)
{
    switch (result) {
        case GOLDIE_AUDIO89_OK: return "ok"; case GOLDIE_AUDIO89_ERR_ARGUMENT: return "bad argument";
        case GOLDIE_AUDIO89_ERR_STATE: return "bad state"; case GOLDIE_AUDIO89_ERR_MIXER: return "rawmix error";
        case GOLDIE_AUDIO89_ERR_HARDWARE: return "KNM hardware error"; case GOLDIE_AUDIO89_ERR_FILE: return "file error";
        case GOLDIE_AUDIO89_ERR_WAV: return "WAV parse error"; case GOLDIE_AUDIO89_ERR_PCM: return "PCM validation error";
        case GOLDIE_AUDIO89_ERR_CAPACITY: return "capacity exhausted"; case GOLDIE_AUDIO89_ERR_MP3: return "MP3 decode error";
        case GOLDIE_AUDIO89_ERR_UNSUPPORTED: return "unsupported"; case GOLDIE_AUDIO89_ERR_CONSOLE: return "invalid console";
        case GOLDIE_AUDIO89_ERR_CYCLE: return "console cycle"; case GOLDIE_AUDIO89_ERR_BUSY: return "console busy";
        default: return "unknown Goldie result";
    }
}
