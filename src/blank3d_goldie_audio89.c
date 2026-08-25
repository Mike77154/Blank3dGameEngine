#include "blank3d_goldie_audio89.h"

#include <string.h>

static void b3d_goldie_status(Blank3DGoldieAudio89 *host, const char *text)
{
    unsigned int i;
    if (!host) return;
    if (!text) text = "";
    i = 0U;
    while (text[i] && i + 1U < (unsigned int)sizeof(host->status)) {
        host->status[i] = text[i];
        ++i;
    }
    host->status[i] = '\0';
}

static rm_result b3d_goldie_weapon_stream_next(void *user,
                                                rm_s16 *out_frame,
                                                rm_u16 channels,
                                                rm_u16 *out_end_of_stream)
{
    Blank3DGoldieAudio89 *host;
    unsigned int at;
    host = (Blank3DGoldieAudio89 *)user;
    if (!host || !out_frame || !out_end_of_stream ||
        !host->weapon_stream_render) return RM_ERR_INVALID_ARG;
    if (channels == 0U || channels > 2U) return RM_ERR_UNSUPPORTED;

    if (host->weapon_stream_at >= host->weapon_stream_valid) {
        if (!host->weapon_stream_render(host->weapon_stream_user,
                                        host->weapon_stream_pcm,
                                        B3D_GOLDIE_STREAM_FRAMES)) {
            memset(host->weapon_stream_pcm, 0,
                   sizeof(host->weapon_stream_pcm));
        }
        host->weapon_stream_at = 0U;
        host->weapon_stream_valid = B3D_GOLDIE_STREAM_FRAMES;
    }

    at = host->weapon_stream_at * 2U;
    out_frame[0] = host->weapon_stream_pcm[at];
    if (channels > 1U) out_frame[1] = host->weapon_stream_pcm[at + 1U];
    host->weapon_stream_at++;
    *out_end_of_stream = 0U;
    return RM_OK;
}

static int b3d_goldie_make_bus(Blank3DGoldieAudio89 *host,
                               int bus,
                               int parent,
                               const char *name)
{
    int rc;
    if (!host || bus <= B3D_AUDIO_BUS_MASTER || bus >= B3D_AUDIO_BUS_COUNT)
        return 0;
    if (parent < B3D_AUDIO_BUS_MASTER || parent >= B3D_AUDIO_BUS_COUNT)
        return 0;
    rc = goldie_audio89_console_create(&host->runtime,
                                       host->buses[parent], name,
                                       &host->buses[bus]);
    return rc == GOLDIE_AUDIO89_OK ? 1 : 0;
}

static int b3d_goldie_attach_weapon_stream(Blank3DGoldieAudio89 *host)
{
    rm_stream_desc stream;
    rm_voice_params params;
    rm_result rr;
    goldie_audio89_console_handle bus;
    if (!host || !host->weapon_stream_render) return 0;
    bus = host->buses[B3D_AUDIO_BUS_WEAPONS];
    if (!goldie_audio89_console_is_valid(&host->runtime, bus)) return 0;

    memset(&stream, 0, sizeof(stream));
    stream.on_next = b3d_goldie_weapon_stream_next;
    stream.user = host;
    stream.sample_rate = (rm_u32)host->runtime.config.sample_rate;
    stream.channels = (rm_u16)host->runtime.config.channels;

    rm_voice_params_init(&params);
    params.gain_q15 = RM_Q15_ONE;
    params.pan_q15 = RM_PAN_CENTER;
    params.priority = (rm_u16)65535U;
    params.flags = (rm_u16)RM_VOICE_FLAG_PROTECTED;
    params.resampler = (rm_u16)RM_RESAMPLER_NEAREST;
    params.bus_id = RM_BUS_DEFAULT;

    host->weapon_stream_at = 0U;
    host->weapon_stream_valid = 0U;
    rr = rm_engine_play_stream(&host->runtime.consoles[bus.slot].mixer,
                               &stream, &params,
                               &host->weapon_stream_voice);
    return rr == RM_OK ? 1 : 0;
}

int blank3d_goldie_audio89_init(Blank3DGoldieAudio89 *host,
                                void *weapon_stream_user,
                                Blank3DGoldieAudioRenderFn89 weapon_stream_render,
                                unsigned long sample_rate)
{
    goldie_audio89_config cfg;
    int rc;
    if (!host || !weapon_stream_render || sample_rate == 0UL) return 0;
    memset(host, 0, sizeof(*host));
    host->weapon_stream_user = weapon_stream_user;
    host->weapon_stream_render = weapon_stream_render;

    goldie_audio89_config_init(&cfg);
    cfg.sample_rate = sample_rate;
    cfg.channels = 2U;
    cfg.frames_per_buffer = B3D_GOLDIE_STREAM_FRAMES;
    cfg.buffer_count = 4U;
    cfg.max_voices_per_console = 32U;
    cfg.max_consoles = 16U;
    /* Blank3D keeps its existing host-owned WinMM queue for now.  Goldie is
       therefore used as the authoritative mixer graph in deterministic
       offline/block mode, with KNM NULL serving only as its no-heap device
       provider.  This avoids a second hardware writer while making the
       hardware backend swappable later. */
    cfg.backend = KNM_BACKEND_NULL;
    cfg.prefer_low_latency = 1;
    rc = goldie_audio89_init(&host->runtime, &cfg);
    if (rc != GOLDIE_AUDIO89_OK) {
        b3d_goldie_status(host, goldie_audio89_result_string(rc));
        return 0;
    }

    host->buses[B3D_AUDIO_BUS_MASTER] =
        goldie_audio89_master_console(&host->runtime);
    if (!b3d_goldie_make_bus(host, B3D_AUDIO_BUS_DIALOGUE,
                             B3D_AUDIO_BUS_MASTER, "Dialogue") ||
        !b3d_goldie_make_bus(host, B3D_AUDIO_BUS_SFX,
                             B3D_AUDIO_BUS_MASTER, "SFX") ||
        !b3d_goldie_make_bus(host, B3D_AUDIO_BUS_WEAPONS,
                             B3D_AUDIO_BUS_SFX, "Weapons") ||
        !b3d_goldie_make_bus(host, B3D_AUDIO_BUS_UI,
                             B3D_AUDIO_BUS_MASTER, "UI") ||
        !b3d_goldie_make_bus(host, B3D_AUDIO_BUS_AMBIENCE,
                             B3D_AUDIO_BUS_MASTER, "Ambience") ||
        !b3d_goldie_make_bus(host, B3D_AUDIO_BUS_AIR,
                             B3D_AUDIO_BUS_AMBIENCE, "Air") ||
        !b3d_goldie_make_bus(host, B3D_AUDIO_BUS_BIOFAUNA,
                             B3D_AUDIO_BUS_AMBIENCE, "Biofauna") ||
        !b3d_goldie_make_bus(host, B3D_AUDIO_BUS_MUSIC,
                             B3D_AUDIO_BUS_MASTER, "Music") ||
        !b3d_goldie_make_bus(host, B3D_AUDIO_BUS_RHYTHM,
                             B3D_AUDIO_BUS_MUSIC, "Rhythm") ||
        !b3d_goldie_make_bus(host, B3D_AUDIO_BUS_HARMONY,
                             B3D_AUDIO_BUS_MUSIC, "Harmony")) {
        b3d_goldie_status(host, "Goldie console graph creation failed");
        goldie_audio89_shutdown(&host->runtime);
        return 0;
    }

    if (!b3d_goldie_attach_weapon_stream(host)) {
        b3d_goldie_status(host, "Goldie weapon synth stream attach failed");
        goldie_audio89_shutdown(&host->runtime);
        return 0;
    }

    (void)goldie_audio89_console_set_limiter(
        &host->runtime, host->buses[B3D_AUDIO_BUS_MASTER],
        (short)30000, 4U, 96U, GOLDIE_AUDIO89_Q15_UNITY, 32U);
    host->initialized = 1;
    b3d_goldie_status(host, "Goldie Matryoshka mixer online");
    return 1;
}

void blank3d_goldie_audio89_shutdown(Blank3DGoldieAudio89 *host)
{
    if (!host) return;
    if (host->initialized) goldie_audio89_shutdown(&host->runtime);
    host->initialized = 0;
    host->weapon_stream_user = (void *)0;
    host->weapon_stream_render = (Blank3DGoldieAudioRenderFn89)0;
    host->weapon_stream_at = 0U;
    host->weapon_stream_valid = 0U;
}

int blank3d_goldie_audio89_render(Blank3DGoldieAudio89 *host,
                                  short *dst_interleaved,
                                  unsigned int frames)
{
    int rc;
    if (!host || !host->initialized || !dst_interleaved || frames == 0U)
        return 0;
    rc = goldie_audio89_render_s16(&host->runtime, dst_interleaved, frames);
    if (rc != GOLDIE_AUDIO89_OK) {
        b3d_goldie_status(host, goldie_audio89_result_string(rc));
        return 0;
    }
    return 1;
}

int blank3d_goldie_audio89_set_bus_gain_q15(Blank3DGoldieAudio89 *host,
                                             int bus,
                                             short gain_q15)
{
    if (!host || !host->initialized || bus < 0 || bus >= B3D_AUDIO_BUS_COUNT)
        return 0;
    return goldie_audio89_console_set_gain_q15(&host->runtime,
                                                host->buses[bus], gain_q15)
           == GOLDIE_AUDIO89_OK ? 1 : 0;
}

int blank3d_goldie_audio89_set_bus_mute(Blank3DGoldieAudio89 *host,
                                         int bus,
                                         int mute_on)
{
    if (!host || !host->initialized || bus < 0 || bus >= B3D_AUDIO_BUS_COUNT)
        return 0;
    return goldie_audio89_console_set_mute(&host->runtime,
                                            host->buses[bus], mute_on)
           == GOLDIE_AUDIO89_OK ? 1 : 0;
}

int blank3d_goldie_audio89_play_pcm(Blank3DGoldieAudio89 *host,
                                     int bus,
                                     const goldie_audio89_pcm_view *pcm,
                                     int loop,
                                     goldie_audio89_voice_handle *out_voice)
{
    if (!host || !host->initialized || !pcm ||
        bus < 0 || bus >= B3D_AUDIO_BUS_COUNT)
        return 0;
    return goldie_audio89_console_play_pcm_s16(&host->runtime,
                                                host->buses[bus], pcm,
                                                loop, out_voice)
           == GOLDIE_AUDIO89_OK ? 1 : 0;
}

const char *blank3d_goldie_audio89_status(const Blank3DGoldieAudio89 *host)
{
    if (!host) return "Goldie host unavailable";
    return host->status;
}
