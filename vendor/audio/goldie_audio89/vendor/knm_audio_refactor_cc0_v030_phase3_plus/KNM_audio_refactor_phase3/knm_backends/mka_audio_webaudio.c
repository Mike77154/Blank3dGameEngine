#include "../KNM_audio/mnk_core_internal.h"

#if KNM_AUDIO_ENABLE_WEBAUDIO && defined(__EMSCRIPTEN__)

#include <emscripten/webaudio.h>
#include <emscripten/html5.h>
#include <stdbool.h>

typedef struct mka_webaudio_state {
    EMSCRIPTEN_WEBAUDIO_T context;
    EMSCRIPTEN_AUDIO_WORKLET_NODE_T node;
    int output_channel_counts[1];
    unsigned char worklet_stack[4096];
    int processor_ready;
} mka_webaudio_state;

static mka_webaudio_state *mka_webaudio(knm_device *dev)
{
    return (mka_webaudio_state *)(void *)dev->backend_state;
}

static int mka_webaudio_probe(void)
{
    return 1;
}


static bool mka_webaudio_process_audio(int numInputs,
                                       const AudioSampleFrame *inputs,
                                       int numOutputs,
                                       AudioSampleFrame *outputs,
                                       int numParams,
                                       const AudioParamFrame *params,
                                       void *userData);
static void mka_webaudio_processor_created(EMSCRIPTEN_WEBAUDIO_T audioContext,
                                           EMSCRIPTEN_AUDIO_WORKLET_NODE_T audioWorkletNode,
                                           void *userData);
static void mka_webaudio_thread_initialized(EMSCRIPTEN_WEBAUDIO_T audioContext,
                                            bool success,
                                            void *userData);
static int mka_webaudio_open(knm_device *dev);
static int mka_webaudio_start(knm_device *dev);
static int mka_webaudio_stop(knm_device *dev);
static void mka_webaudio_close(knm_device *dev);

EM_JS(void, mka_webaudio_install_toggle_button, (EMSCRIPTEN_WEBAUDIO_T audioContext), {
    var ctx = emscriptenGetAudioObject(audioContext);
    var button = document.getElementById('knm-audio-toggle');
    if (!button) {
      button = document.createElement('button');
      button.id = 'knm-audio-toggle';
      button.textContent = 'KNM audio start/stop';
      document.body.appendChild(button);
    }
    button.onclick = function() {
      if (ctx.state !== 'running') {
        ctx.resume();
      } else {
        ctx.suspend();
      }
    };
});

static bool mka_webaudio_process_audio(int numInputs,
                                       const AudioSampleFrame *inputs,
                                       int numOutputs,
                                       AudioSampleFrame *outputs,
                                       int numParams,
                                       const AudioParamFrame *params,
                                       void *userData)
{
    knm_device *dev;
    unsigned int channels;
    unsigned int frames;
    int i;

    (void)numInputs;
    (void)inputs;
    (void)numParams;
    (void)params;

    dev = (knm_device *)userData;
    if (!dev->running) {
        return false;
    }

    channels = dev->cfg.channels;

    {
        unsigned int sample_index;
        for (i = 0; i < numOutputs; ++i) {
            frames = (unsigned int)outputs[i].samplesPerChannel;
            mnk_render_to_s16(dev, dev->s16_output_scratch, frames);
            for (sample_index = 0U; sample_index < frames * channels; ++sample_index) {
                outputs[i].data[sample_index] = (int)dev->s16_output_scratch[sample_index] / 32768.0;
            }
        }
    }

    return true;
}

static void mka_webaudio_processor_created(EMSCRIPTEN_WEBAUDIO_T audioContext,
                                           bool success,
                                           void *userData)
{
    knm_device *dev;
    mka_webaudio_state *st;
    EmscriptenAudioWorkletNodeCreateOptions options;

    dev = (knm_device *)userData;
    st = mka_webaudio(dev);

    if (!success) {
        return;
    }

    st->output_channel_counts[0] = (int)dev->cfg.channels;
    memset(&options, 0, sizeof(options));
    options.numberOfInputs = 0;
    options.numberOfOutputs = 1;
    options.outputChannelCounts = st->output_channel_counts;
    options.channelCount = dev->cfg.channels;
    options.channelCountMode = WEBAUDIO_CHANNEL_COUNT_MODE_EXPLICIT;
    options.channelInterpretation = WEBAUDIO_CHANNEL_INTERPRETATION_SPEAKERS;

    st->node = emscripten_create_wasm_audio_worklet_node(audioContext,
                                                         "knm-fixed-point",
                                                         &options,
                                                         mka_webaudio_process_audio,
                                                         (void *)dev);

    emscripten_audio_node_connect(st->node, audioContext, 0, 0);
    mka_webaudio_install_toggle_button(audioContext);
    st->processor_ready = 1;
}

static void mka_webaudio_thread_initialized(EMSCRIPTEN_WEBAUDIO_T audioContext,
                                            bool success,
                                            void *userData)
{
    WebAudioWorkletProcessorCreateOptions opts;

    if (!success) {
        return;
    }

    memset(&opts, 0, sizeof(opts));
    opts.name = "knm-fixed-point";
    emscripten_create_wasm_audio_worklet_processor_async(audioContext,
                                                         &opts,
                                                         mka_webaudio_processor_created,
                                                         userData);
}

static int mka_webaudio_open(knm_device *dev)
{
    mka_webaudio_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if (dev->cfg.enable_input) {
        return KNM_NOT_SUPPORTED;
    }
    if ((unsigned long)sizeof(mka_webaudio_state) > (unsigned long)KNM_BACKEND_STATE_BYTES) {
        return KNM_LIMIT_EXCEEDED;
    }

    st = mka_webaudio(dev);
    memset(st, 0, sizeof(*st));

    st->context = emscripten_create_audio_context(0);
    if (!st->context) {
        return KNM_DEVICE_ERROR;
    }

    return KNM_OK;
}

static int mka_webaudio_start(knm_device *dev)
{
    mka_webaudio_state *st;

    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }

    st = mka_webaudio(dev);
    dev->running = 1;

    emscripten_start_wasm_audio_worklet_thread_async(st->context,
                                                     st->worklet_stack,
                                                     sizeof(st->worklet_stack),
                                                     mka_webaudio_thread_initialized,
                                                     (void *)dev);
    return KNM_OK;
}

static int mka_webaudio_stop(knm_device *dev)
{
    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    dev->running = 0;
    return KNM_OK;
}

static void mka_webaudio_close(knm_device *dev)
{
    mka_webaudio_state *st;

    if (dev == (knm_device *)0) {
        return;
    }

    st = mka_webaudio(dev);
    dev->running = 0;

    if (st->context) {
        emscripten_destroy_audio_context(st->context);
        st->context = 0;
    }
}

const knm_backend_vtbl knm_backend_webaudio_vtbl = {
    KNM_BACKEND_WEBAUDIO,
    "webaudio",
    0,
    1,
    0,
    0,
    mka_webaudio_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_webaudio_open,
    mka_webaudio_start,
    mka_webaudio_stop,
    mka_webaudio_close
};

#else

static int mka_webaudio_probe(void)
{
    return 0;
}

static int mka_webaudio_open(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_webaudio_start(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static int mka_webaudio_stop(knm_device *dev)
{
    (void)dev;
    return KNM_BACKEND_UNAVAILABLE;
}

static void mka_webaudio_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_webaudio_vtbl = {
    KNM_BACKEND_WEBAUDIO,
    "webaudio",
    0,
    1,
    0,
    0,
    mka_webaudio_probe,
    (unsigned int (*)(void))0,
    (int (*)(unsigned int, knm_device_info *))0,
    mka_webaudio_open,
    mka_webaudio_start,
    mka_webaudio_stop,
    mka_webaudio_close
};

#endif
