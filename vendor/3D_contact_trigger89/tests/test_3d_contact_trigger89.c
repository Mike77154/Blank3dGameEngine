#include "3d_contact_trigger89.h"
#include <stdio.h>
#include <string.h>

typedef struct HarnessTag {
    CT89_Subject touching_owner;
    CT89_Subject touching_other;
    CT89_Subject near_owner;
    CT89_Subject near_other;
    int action_status;
    int action_calls;
    int enter_events;
    int stay_events;
    int exit_events;
    int activate_events;
    CT89_Subject destroyed[16];
    int destroyed_count;
} Harness;

static int gather_touch(void *user, const CT89_Probe *probe,
                        CT89_Candidate *out, int cap)
{
    Harness *h;
    h = (Harness *)user;
    if (!h || !probe || !out || cap <= 0) return -1;
    if (h->touching_owner != probe->owner ||
        h->touching_other == CT89_SUBJECT_INVALID) return 0;
    out[0].subject = h->touching_other;
    out[0].category_mask = 1UL;
    out[0].distance_fx = 0L;
    return 1;
}

static int gather_near(void *user, const CT89_Probe *probe,
                       CT89_Candidate *out, int cap)
{
    Harness *h;
    h = (Harness *)user;
    if (!h || !probe || !out || cap <= 0) return -1;
    if (h->near_owner != probe->owner ||
        h->near_other == CT89_SUBJECT_INVALID) return 0;
    out[0].subject = h->near_other;
    out[0].category_mask = 1UL;
    out[0].distance_fx = ct89_fx_from_int(1);
    return 1;
}

static int execute_action(void *user, const CT89_Event *event)
{
    Harness *h;
    h = (Harness *)user;
    if (!h || !event) return CT89_ACTION_UNHANDLED;
    h->action_calls += 1;
    return h->action_status;
}

static void emit_event(void *user, const CT89_Event *event)
{
    Harness *h;
    h = (Harness *)user;
    if (!h || !event) return;
    if (event->event_type == CT89_EVENT_ENTER) h->enter_events += 1;
    else if (event->event_type == CT89_EVENT_STAY) h->stay_events += 1;
    else if (event->event_type == CT89_EVENT_EXIT) h->exit_events += 1;
    else if (event->event_type == CT89_EVENT_ACTIVATE) h->activate_events += 1;
}

static int destroy_subject(void *user, CT89_Subject subject)
{
    Harness *h;
    h = (Harness *)user;
    if (!h || h->destroyed_count >= 16) return 0;
    h->destroyed[h->destroyed_count++] = subject;
    return 1;
}

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

int main(void)
{
    CT89_Context ctx;
    CT89_TriggerDesc desc;
    CT89_SensorProvider touch;
    CT89_SensorProvider nearp;
    CT89_ActionProvider action;
    CT89_EventProvider events;
    CT89_LifecycleProvider lifecycle;
    CT89_Trigger touch_trigger;
    CT89_Trigger proximity_trigger;
    CT89_Trigger rejected_trigger;
    Harness h;

    memset(&h, 0, sizeof(h));
    h.action_status = CT89_ACTION_ACCEPTED;
    ct89_init(&ctx);

    ct89_sensor_provider_init(&touch);
    touch.user = &h;
    touch.gather = gather_touch;
    ct89_set_contact_provider(&ctx, &touch);

    ct89_sensor_provider_init(&nearp);
    nearp.user = &h;
    nearp.gather = gather_near;
    ct89_set_proximity_provider(&ctx, &nearp);

    ct89_action_provider_init(&action);
    action.user = &h;
    action.execute = execute_action;
    ct89_set_action_provider(&ctx, &action);

    ct89_event_provider_init(&events);
    events.user = &h;
    events.emit = emit_event;
    ct89_set_event_provider(&ctx, &events);

    ct89_lifecycle_provider_init(&lifecycle);
    lifecycle.user = &h;
    lifecycle.destroy_subject = destroy_subject;
    ct89_set_lifecycle_provider(&ctx, &lifecycle);

    ct89_trigger_desc_defaults(&desc);
    desc.owner = 100UL;
    desc.sensor_mode = CT89_SENSOR_TOUCH;
    desc.action_event_mask = CT89_EVENT_MASK_ENTER;
    desc.consume_policy = CT89_CONSUME_DESTROY_OWNER;
    desc.action_id = 7;
    touch_trigger = ct89_trigger_create(&ctx, &desc);
    if (touch_trigger == CT89_TRIGGER_INVALID) return fail("touch trigger create");

    h.touching_owner = 100UL;
    h.touching_other = 1UL;
    if (!ct89_step(&ctx, 16UL)) return fail("touch step");
    if (h.action_calls != 1) return fail("touch action did not fire once");
    if (h.destroyed_count != 1 || h.destroyed[0] != 100UL)
        return fail("touch accepted did not destroy owner");
    if (ct89_trigger_is_enabled(&ctx, touch_trigger))
        return fail("destroy-owner trigger remained enabled");

    ct89_trigger_desc_defaults(&desc);
    desc.owner = 200UL;
    desc.sensor_mode = CT89_SENSOR_PROXIMITY;
    desc.radius_fx = ct89_fx_from_int(2);
    desc.action_event_mask = CT89_EVENT_MASK_ACTIVATE;
    desc.consume_policy = CT89_CONSUME_DESTROY_OWNER;
    desc.action_id = 11;
    proximity_trigger = ct89_trigger_create(&ctx, &desc);
    if (proximity_trigger == CT89_TRIGGER_INVALID)
        return fail("proximity trigger create");

    h.near_owner = 200UL;
    h.near_other = 1UL;
    if (!ct89_step(&ctx, 16UL)) return fail("proximity enter step");
    if (!ct89_subject_is_active(&ctx, proximity_trigger, 1UL))
        return fail("proximity subject not active");
    if (h.action_calls != 1)
        return fail("proximity enter executed action before activation");
    if (ct89_activate(&ctx, proximity_trigger, 1UL) != CT89_ACTION_ACCEPTED)
        return fail("proximity activation not accepted");
    if (h.action_calls != 2) return fail("activate action count");
    if (h.destroyed_count != 2 || h.destroyed[1] != 200UL)
        return fail("proximity accepted did not destroy owner");

    ct89_trigger_desc_defaults(&desc);
    desc.owner = 300UL;
    desc.sensor_mode = CT89_SENSOR_TOUCH;
    desc.action_event_mask = CT89_EVENT_MASK_ENTER;
    desc.consume_policy = CT89_CONSUME_DESTROY_OWNER;
    rejected_trigger = ct89_trigger_create(&ctx, &desc);
    if (rejected_trigger == CT89_TRIGGER_INVALID)
        return fail("rejected trigger create");
    h.action_status = CT89_ACTION_REJECTED;
    h.touching_owner = 300UL;
    h.touching_other = 1UL;
    if (!ct89_step(&ctx, 16UL)) return fail("rejected touch step");
    if (h.destroyed_count != 2)
        return fail("rejected action consumed object");
    if (!ct89_trigger_is_enabled(&ctx, rejected_trigger))
        return fail("rejected trigger disabled");

    h.touching_other = CT89_SUBJECT_INVALID;
    if (!ct89_step(&ctx, 16UL)) return fail("exit step");
    if (h.enter_events < 3) return fail("missing enter events");
    if (h.exit_events < 1) return fail("missing exit event");
    if (h.activate_events != 1) return fail("activate event count");

    printf("PASS: 3D_contact_trigger89 touch/proximity/activate/consume/reject; enters=%d stays=%d exits=%d actions=%d destroyed=%d\n",
           h.enter_events, h.stay_events, h.exit_events,
           h.action_calls, h.destroyed_count);
    return 0;
}
