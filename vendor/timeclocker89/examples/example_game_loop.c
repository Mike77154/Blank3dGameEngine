#include "timeclocker.h"

#define GAME_ACT_NONE      0
#define GAME_ACT_OPEN_DOOR 1

static void game_spawn_enemy(void)
{
    /* Engine hook placeholder. */
}

static void game_player_dash(void)
{
    /* Engine hook placeholder. */
}

static void game_open_door(void)
{
    /* Engine hook placeholder. */
}

static int input_dash_pressed(void)
{
    return 1;
}

int main(void)
{
    TimeClocker clocker;
    TimeClockerEvent event_value;
    tc_i32 delta_ticks;

    timeclocker_init(&clocker, 60, 60);

    timeclocker_loop_seconds(&clocker, "enemy_spawn", 5, 0);
    timeclocker_alarm_seconds(&clocker, "door_alarm", 2, GAME_ACT_OPEN_DOOR, "open_door", 0, 0, 0);
    timeclocker_stopwatch_start(&clocker, "player_life", TIME_CLOCKER_UNIT_TICKS);

    delta_ticks = 1;

    while (timeclocker_global_frames(&clocker) < 600) {
        timeclocker_update(&clocker, delta_ticks, 1);

        if (timeclocker_timer_fired(&clocker, "enemy_spawn")) {
            game_spawn_enemy();
        }

        if (input_dash_pressed() && timeclocker_cooldown_ready(&clocker, "dash")) {
            game_player_dash();
            timeclocker_cooldown_frames(&clocker, "dash", 45);
        }

        if (timeclocker_poll_alarm(&clocker, &event_value, 1)) {
            if (event_value.action_id == GAME_ACT_OPEN_DOOR) {
                game_open_door();
            }
        }
    }

    return 0;
}
