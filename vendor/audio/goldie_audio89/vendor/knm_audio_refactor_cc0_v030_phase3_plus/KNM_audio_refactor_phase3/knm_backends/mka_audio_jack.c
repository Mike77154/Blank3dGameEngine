#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_JACK && (defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__))

#include <jack/jack.h>
#include <stdio.h>

typedef struct mka_jack_state {
    jack_client_t *client;
    jack_port_t *in_ports[KNM_MAX_CHANNELS];
    jack_port_t *out_ports[KNM_MAX_CHANNELS];
    int activated;
} mka_jack_state;

static mka_jack_state *mka_jack(knm_device *dev)
{
    return (mka_jack_state *)(void *)dev->backend_state;
}

static int mka_jack_probe(void)
{
    return 1;
}


static void mka_jack_auto_connect_outputs(mka_jack_state *st, unsigned int channels);
static void mka_jack_auto_connect_inputs(mka_jack_state *st, unsigned int channels);
static int mka_jack_process(jack_nframes_t nframes, void *arg);
static int mka_jack_open(knm_device *dev);
static int mka_jack_start(knm_device *dev);
static int mka_jack_stop(knm_device *dev);
static void mka_jack_close(knm_device *dev);

static void mka_jack_auto_connect_outputs(mka_jack_state *st, unsigned int channels)
{
    const char **ports;
    unsigned int i;

    ports = jack_get_ports(st->client, (const char *)0, (const char *)0,
                           JackPortIsPhysical | JackPortIsInput);
    if (ports == (const char **)0) {
        return;
    }

    for (i = 0U; i < channels && ports[i] != (const char *)0; ++i) {
        jack_connect(st->client, jack_port_name(st->out_ports[i]), ports[i]);
    }
    jack_free((void *)ports);
}

static void mka_jack_auto_connect_inputs(mka_jack_state *st, unsigned int channels)
{
    const char **ports;
    unsigned int i;

    ports = jack_get_ports(st->client, (const char *)0, (const char *)0,
                           JackPortIsPhysical | JackPortIsOutput);
    if (ports == (const char **)0) {
        return;
    }

    for (i = 0U; i < channels && ports[i] != (const char *)0; ++i) {
        jack_connect(st->client, ports[i], jack_port_name(st->in_ports[i]));
    }
    jack_free((void *)ports);
}

static int mka_jack_process(jack_nframes_t nframes, void *arg)
{
    knm_device *dev;
    mka_jack_state *st;
    unsigned int channels;
    unsigned int total_frames;
    unsigned int offset;
    unsigned int chunk;
    unsigned int i;
    jack_default_audio_sample_t *src_buffers[KNM_MAX_CHANNELS];
    jack_default_audio_sample_t *dst_buffers[KNM_MAX_CHANNELS];
    unsigned int frame;
    unsigned int ch;

    dev = (knm_device *)arg;
    st = mka_jack(dev);
    channels = dev->cfg.channels;
    total_frames = (unsigned int)nframes;

    for (i = 0U; i < channels; ++i) {
        if (dev->cfg.enable_input && st->in_ports[i] != (jack_port_t *)0) {
            src_buffers[i] = (jack_default_audio_sample_t *)jack_port_get_buffer(st->in_ports[i], nframes);
        } else {
            src_buffers[i] = (jack_default_audio_sample_t *)0;
        }

        if (dev->cfg.enable_output && st->out_ports[i] != (jack_port_t *)0) {
            dst_buffers[i] = (jack_default_audio_sample_t *)jack_port_get_buffer(st->out_ports[i], nframes);
        } else {
            dst_buffers[i] = (jack_default_audio_sample_t *)0;
        }
    }

    offset = 0U;
    while (offset < total_frames) {
        chunk = total_frames - offset;
        if (chunk > KNM_MAX_FRAMES_PER_BUFFER) {
            chunk = KNM_MAX_FRAMES_PER_BUFFER;
        }

        if (dev->cfg.enable_input) {
            for (frame = 0U; frame < chunk; ++frame) {
                for (ch = 0U; ch < channels; ++ch) {
                    if (src_buffers[ch] != (jack_default_audio_sample_t *)0) {
                        dev->s16_input_scratch[(frame * channels) + ch] = (knm_s16)(src_buffers[ch][offset + frame] * 32767);
                    } else {
                        dev->s16_input_scratch[(frame * channels) + ch] = (knm_s16)0;
                    }
                }
            }
            mnk_capture_from_s16(dev, dev->s16_input_scratch, chunk);
        }

        if (dev->cfg.enable_output) {
            mnk_render_to_s16(dev, dev->s16_output_scratch, chunk);
            for (frame = 0U; frame < chunk; ++frame) {
                for (ch = 0U; ch < channels; ++ch) {
                    if (dst_buffers[ch] != (jack_default_audio_sample_t *)0) {
                        dst_buffers[ch][offset + frame] = (jack_default_audio_sample_t)((int)dev->s16_output_scratch[(frame * channels) + ch]  / (jack_default_audio_sample_t)32768);
                    }
                }
            }
        }

        offset += chunk;
    }

    return 0;
}

static int mka_jack_open(knm_device *dev)
{
    mka_jack_state *st;
    jack_status_t status;
    unsigned int i;
    char port_name[32];

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if ((unsigned long)sizeof(mka_jack_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_jack(dev);
    mnk_zero((void *)st, (unsigned long)sizeof(*st));
    status = 0;

    st->client = jack_client_open("KNM_audio", JackNoStartServer, &status);
    if (st->client == (jack_client_t *)0) {
        return KNM_DEVICE_ERROR;
    }

    jack_set_process_callback(st->client, mka_jack_process, (void *)dev);

    for (i = 0U; i < dev->cfg.channels; ++i) {
        if (dev->cfg.enable_input) {
            sprintf(port_name, "input_%u", i + 1U);
            st->in_ports[i] = jack_port_register(st->client,
                                                 port_name,
                                                 JACK_DEFAULT_AUDIO_TYPE,
                                                 JackPortIsInput,
                                                 0UL);
            if (st->in_ports[i] == (jack_port_t *)0) {
                mka_jack_close(dev);
                return KNM_DEVICE_ERROR;
            }
        }
        if (dev->cfg.enable_output) {
            sprintf(port_name, "output_%u", i + 1U);
            st->out_ports[i] = jack_port_register(st->client,
                                                  port_name,
                                                  JACK_DEFAULT_AUDIO_TYPE,
                                                  JackPortIsOutput,
                                                  0UL);
            if (st->out_ports[i] == (jack_port_t *)0) {
                mka_jack_close(dev);
                return KNM_DEVICE_ERROR;
            }
        }
    }

    return KNM_OK;
}

static int mka_jack_start(knm_device *dev)
{
    mka_jack_state *st;
    int rc;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_jack(dev);

    rc = jack_activate(st->client);
    if (rc != 0) {
        return KNM_DEVICE_ERROR;
    }

    st->activated = 1;
    dev->running = 1;

    if (dev->cfg.enable_output) {
        mka_jack_auto_connect_outputs(st, dev->cfg.channels);
    }
    if (dev->cfg.enable_input) {
        mka_jack_auto_connect_inputs(st, dev->cfg.channels);
    }

    return KNM_OK;
}

static int mka_jack_stop(knm_device *dev)
{
    mka_jack_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_jack(dev);
    dev->running = 0;

    if (st->activated) {
        jack_deactivate(st->client);
        st->activated = 0;
    }

    return KNM_OK;
}

static void mka_jack_close(knm_device *dev)
{
    mka_jack_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_jack(dev);

    if (st->client != (jack_client_t *)0) {
        if (st->activated) {
            jack_deactivate(st->client);
            st->activated = 0;
        }
        jack_client_close(st->client);
        st->client = (jack_client_t *)0;
    }
}

const knm_backend_vtbl knm_backend_jack_vtbl = {
    KNM_BACKEND_JACK,
    "jack",
    1,
    1,
    1,
    0,
    mka_jack_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_jack_open,
    mka_jack_start,
    mka_jack_stop,
    mka_jack_close
};

#else

static int mka_jack_probe(void)
{
    return 0;
}

static int mka_jack_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_jack_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_jack_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_jack_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_jack_vtbl = {
    KNM_BACKEND_JACK,
    "jack",
    1,
    1,
    1,
    0,
    mka_jack_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_jack_open,
    mka_jack_start,
    mka_jack_stop,
    mka_jack_close
};

#endif
