#include <stdio.h>
#include <string.h>
#include "nationalmecanicanimal89.h"

#define DEMO_SOCKET_HAND 1
#define DEMO_SOURCE_RECOIL 2
#define DEMO_RESOURCE_PISTOL 10
#define DEMO_GROUP_FRAME 0
#define DEMO_GROUP_SLIDE 1
#define DEMO_ACTION_FIRE 100

typedef struct demo_engine_s {
    nm89_fx recoil;
    nm89_transform hand_socket;
    int draw_calls;
} demo_engine;

static int demo_sample_transform(
    void *user, const nm89_rig *rig, nm89_i16 part_id,
    nm89_i16 source_id, nm89_transform_sample *out_sample)
{
    demo_engine *engine;
    (void)rig;
    (void)part_id;
    engine = (demo_engine *)user;
    if (source_id != DEMO_SOURCE_RECOIL) {
        return NM89_ERR_NOT_FOUND;
    }
    nm89_transform_sample_identity(out_sample);
    out_sample->transform.move.z = engine->recoil;
    out_sample->channels = NM89_CHANNEL_MOVE_Z;
    return NM89_OK;
}

static int demo_sample_socket(
    void *user, const nm89_rig *rig, nm89_i16 part_id,
    nm89_i16 socket_id, nm89_transform_sample *out_sample)
{
    demo_engine *engine;
    (void)rig;
    (void)part_id;
    engine = (demo_engine *)user;
    if (socket_id != DEMO_SOCKET_HAND) {
        return NM89_ERR_NOT_FOUND;
    }
    nm89_transform_sample_identity(out_sample);
    out_sample->transform = engine->hand_socket;
    out_sample->channels = NM89_CHANNEL_TRANSFORM;
    return NM89_OK;
}

static void demo_draw_geometry(
    void *user, const nm89_rig *rig,
    const nm89_geometry_packet *packet)
{
    demo_engine *engine;
    (void)rig;
    engine = (demo_engine *)user;
    engine->draw_calls += 1;
    printf("draw binding=%d resource=%d selector=%d "
           "world=(%d,%d,%d) visible=%d\n",
           (int)packet->binding_id,
           (int)packet->resource_id,
           (int)packet->selector_id,
           (int)NM89_FX_TO_INT(packet->world.m[0][3]),
           (int)NM89_FX_TO_INT(packet->world.m[1][3]),
           (int)NM89_FX_TO_INT(packet->world.m[2][3]),
           (int)packet->visible);
}

static void demo_event(
    void *user, const nm89_rig *rig,
    const nm89_clip_event *event_value)
{
    (void)user;
    (void)rig;
    printf("event type=%d code=%d tick=%u\n",
           (int)event_value->type,
           (int)event_value->code,
           (unsigned int)event_value->tick);
}

static void demo_provider_identity(nm89_provider *provider)
{
    memset(provider, 0, sizeof(*provider));
}

int main(void)
{
    nm89_rig rig;
    nm89_provider provider;
    demo_engine engine;
    nm89_transform home;
    nm89_constraint slide_constraint;
    nm89_clip_event event_value;
    nm89_i16 provider_id;
    nm89_i16 constraint_id;
    nm89_i16 root;
    nm89_i16 slide;
    nm89_i16 clip;
    nm89_i16 track;
    int tick;

    memset(&engine, 0, sizeof(engine));
    nm89_transform_identity(&engine.hand_socket);
    engine.hand_socket.move.x = NM89_FX_FROM_INT(100);
    engine.hand_socket.move.y = NM89_FX_FROM_INT(20);

    demo_provider_identity(&provider);
    provider.user = &engine;
    provider.sample_transform = demo_sample_transform;
    provider.sample_socket = demo_sample_socket;
    provider.apply_geometry = demo_draw_geometry;
    provider.emit_event = demo_event;

    nm89_rig_init(&rig, 1);
    nm89_provider_add(&rig, &provider, &provider_id);

    nm89_transform_identity(&home);
    nm89_part_add(&rig, "weapon", NM89_ROOT_PART, 0,
                  &home, NM89_INVALID_ID, NM89_INVALID_ID,
                  1U, &root);
    nm89_part_bind_socket_provider(&rig, root, provider_id,
                                   DEMO_SOCKET_HAND,
                                   NM89_SOCKET_WORLD_SPACE);

    nm89_constraint_lock_to_transform(&slide_constraint, &home);
    nm89_constraint_set(&slide_constraint, NM89_PROP_MOVE_Z,
                        NM89_CONSTRAINT_CLAMPED,
                        NM89_FX_FROM_INT(-4), 0, NM89_FX_ONE);
    nm89_constraint_add(&rig, &slide_constraint, &constraint_id);

    nm89_part_add(&rig, "slide", root, 1,
                  &home, constraint_id, NM89_INVALID_ID,
                  1U, &slide);
    nm89_part_bind_transform_provider(
        &rig, slide, provider_id, DEMO_SOURCE_RECOIL,
        NM89_CHANNEL_MOVE_Z, NM89_PROVIDER_ADDITIVE);

    nm89_binding_add(&rig, root, DEMO_RESOURCE_PISTOL,
                     NM89_SELECTOR_GROUP, DEMO_GROUP_FRAME, 0,
                     NM89_INVALID_ID, NM89_INVALID_ID, 0, 0);
    nm89_binding_add(&rig, slide, DEMO_RESOURCE_PISTOL,
                     NM89_SELECTOR_GROUP, DEMO_GROUP_SLIDE, 1,
                     NM89_INVALID_ID, NM89_INVALID_ID, 1, 0);

    nm89_clip_begin(&rig, 0, 8U, 0U, &clip);
    nm89_clip_add_track(&rig, slide, NM89_PROP_MOVE_Z,
                        NM89_INTERP_EASE_OUT, &track);
    nm89_track_add_key(&rig, track, 0U, 0);
    nm89_track_add_key(&rig, track, 2U, NM89_FX_FROM_INT(-4));
    nm89_track_add_key(&rig, track, 8U, 0);

    event_value.tick = 1U;
    event_value.part_id = slide;
    event_value.code = 900;
    event_value.value = 0;
    event_value.type = NM89_EVENT_SOUND;
    nm89_clip_add_event(&rig, &event_value);
    nm89_clip_end(&rig);
    nm89_action_bind(&rig, DEMO_ACTION_FIRE, clip,
                     NM89_ACTION_RESTART);

    nm89_trigger_action(&rig, DEMO_ACTION_FIRE);
    for (tick = 0; tick < 10; ++tick) {
        engine.recoil = tick == 1 ? NM89_FX_FROM_INT(-1) : 0;
        nm89_step(&rig, 1U, 1U);
    }

    printf("draw calls: %d\n", engine.draw_calls);
    return 0;
}
