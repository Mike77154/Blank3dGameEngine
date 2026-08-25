#include "timeclocker.h"

static int expect_true(tc_i32 value, int code)
{
    if (!value) {
        return code;
    }
    return 0;
}

static int expect_false(tc_i32 value, int code)
{
    if (value) {
        return code;
    }
    return 0;
}

static int expect_eq(tc_i32 a, tc_i32 b, int code)
{
    if (a != b) {
        return code;
    }
    return 0;
}

int main(void)
{
    TimeClocker tc;
    TimeClockerCondition cond;
    TimeClockerAction act;
    TimeClockerEvent ev;
    int err;

    timeclocker_init(&tc, 60, 60);

    err = expect_eq(timeclocker_global_ticks(&tc), 0, 1);
    if (err) { return err; }
    err = expect_eq(timeclocker_global_frames(&tc), 0, 2);
    if (err) { return err; }

    err = expect_true(timeclocker_oneshot_seconds(&tc, "door", 2), 3);
    if (err) { return err; }
    timeclocker_update(&tc, 119, 1);
    err = expect_false(timeclocker_timer_done(&tc, "door"), 4);
    if (err) { return err; }
    timeclocker_update(&tc, 1, 1);
    err = expect_true(timeclocker_timer_done(&tc, "door"), 5);
    if (err) { return err; }
    err = expect_true(timeclocker_timer_fired(&tc, "door"), 6);
    if (err) { return err; }
    err = expect_eq(timeclocker_timer_remaining_units(&tc, "door"), 0, 7);
    if (err) { return err; }

    err = expect_true(timeclocker_cooldown_ready(&tc, "dash"), 8);
    if (err) { return err; }
    err = expect_true(timeclocker_cooldown_frames(&tc, "dash", 3), 9);
    if (err) { return err; }
    err = expect_false(timeclocker_cooldown_ready(&tc, "dash"), 10);
    if (err) { return err; }
    timeclocker_update(&tc, 0, 2);
    err = expect_false(timeclocker_cooldown_ready(&tc, "dash"), 11);
    if (err) { return err; }
    timeclocker_update(&tc, 0, 1);
    err = expect_true(timeclocker_cooldown_ready(&tc, "dash"), 12);
    if (err) { return err; }

    err = expect_true(timeclocker_loop_ticks(&tc, "spawn", 10, 0), 13);
    if (err) { return err; }
    timeclocker_update(&tc, 25, 1);
    err = expect_true(timeclocker_timer_fired(&tc, "spawn"), 14);
    if (err) { return err; }
    err = expect_eq(timeclocker_timer_fire_count(&tc, "spawn"), 2, 15);
    if (err) { return err; }
    err = expect_eq(timeclocker_timer_elapsed_units(&tc, "spawn"), 5, 16);
    if (err) { return err; }

    timeclocker_init(&tc, 60, 60);
    timeclocker_update(&tc, 60, 1);
    err = expect_true(timeclocker_every_seconds(&tc, 1), 17);
    if (err) { return err; }
    timeclocker_update(&tc, 30, 1);
    err = expect_false(timeclocker_every_seconds(&tc, 1), 18);
    if (err) { return err; }
    timeclocker_update(&tc, 30, 1);
    err = expect_true(timeclocker_every_seconds(&tc, 1), 19);
    if (err) { return err; }

    err = expect_true(timeclocker_frame_delay(&tc, "wait_frames", 2), 20);
    if (err) { return err; }
    timeclocker_update(&tc, 0, 1);
    err = expect_false(timeclocker_timer_done(&tc, "wait_frames"), 21);
    if (err) { return err; }
    timeclocker_update(&tc, 0, 1);
    err = expect_true(timeclocker_timer_done(&tc, "wait_frames"), 22);
    if (err) { return err; }

    err = expect_true(timeclocker_stopwatch_start(&tc, "life", TIME_CLOCKER_UNIT_TICKS), 23);
    if (err) { return err; }
    timeclocker_update(&tc, 7, 1);
    err = expect_eq(timeclocker_timer_elapsed_units(&tc, "life"), 7, 24);
    if (err) { return err; }
    err = expect_true(timeclocker_timer_active(&tc, "life"), 25);
    if (err) { return err; }

    err = expect_true(timeclocker_alarm_ticks(&tc, "door_alarm", 5, 42, "open_door", 1, 2, 3), 26);
    if (err) { return err; }
    timeclocker_update(&tc, 5, 1);
    err = expect_true(timeclocker_alarm_pending(&tc, "door_alarm"), 27);
    if (err) { return err; }
    timeclocker_event_init(&ev);
    err = expect_true(timeclocker_poll_alarm(&tc, &ev, 1), 28);
    if (err) { return err; }
    err = expect_eq(ev.valid, 1, 29);
    if (err) { return err; }
    err = expect_eq(ev.action_id, 42, 30);
    if (err) { return err; }
    err = expect_eq(ev.action_a, 1, 31);
    if (err) { return err; }
    err = expect_false(timeclocker_alarm_pending(&tc, "door_alarm"), 32);
    if (err) { return err; }

    timeclocker_action_init(&act, TIME_CLOCKER_ACT_TIMER_ONESHOT_TICKS, "bind_delay", 4, 0, 0);
    err = expect_true(timeclocker_action_run(&tc, &act), 33);
    if (err) { return err; }
    timeclocker_update(&tc, 4, 1);
    timeclocker_condition_init(&cond, TIME_CLOCKER_COND_TIMER_DONE, "bind_delay", 0, 0, 0);
    err = expect_true(timeclocker_condition_eval(&tc, &cond), 34);
    if (err) { return err; }

    return 0;
}
