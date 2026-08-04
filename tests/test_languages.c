#include "blank3d_languages.h"

#include <stdio.h>
#include <string.h>

#define Q16_ONE 65536L

typedef struct TestHostTag {
    int w_down;
    int j_down;
    int f_down;
    int g_down;
    int m_down;
    int n_down;
    int ddsl_move;
    int ddsl_jump;
    int ddsl_up;
    int ddsl_down;
    int ddsl_cycle_next;
    int ddsl_cycle_prev;
    int rpyl_commands;
    int rpyl_enemies;
    int fpi_state;
    int fpi_actions;
    int fpi_movefore;
    int fpi_moveforeflat;
    int fpi_fireplayer;
    int fpi_moveup;
    int fpi_movedown;
    int fpi_jump;
    int fpi_far;
    int fpi_grounded;
    long fpi_height_q16;
    int fpi_flat_far;
    int fpi_below_target_height;
    int fpi_above_target_height;
    int fpi_height_above_enough;
    int fpi_return_far;
    int fpi_saveposition;
    int fpi_diveplayer;
    int fpi_returnposition;
    int fpi_lookhome;
    int fpi_hurtplayer;
    int fpi_motion_impact;
    int fpi_motion_finished;
    int fpi_airlungeplayer;
    int fpi_groundlanceplayer;
    int fpi_motion_reset;
    int fpi_motion_clear_impact;
} TestHost;

static int test_key(void *user, const char *name)
{
    TestHost *host;
    host = (TestHost *)user;
    if (!host) return 0;
    if (strcmp(name, "W") == 0) return host->w_down;
    if (strcmp(name, "J") == 0) return host->j_down;
    if (strcmp(name, "F") == 0) return host->f_down;
    if (strcmp(name, "G") == 0) return host->g_down;
    return 0;
}

static int test_input_query(void *user, const char *state, const char *name)
{
    TestHost *host;
    host = (TestHost *)user;
    if (!host || !state || !name) return 0;
    if (strcmp(name, "w") == 0) return host->w_down;
    if (strcmp(name, "up") == 0) return 0;
    if (strcmp(name, "j") == 0 &&
        (strcmp(state, "pressed") == 0 || strcmp(state, "hold") == 0))
        return host->j_down;
    if (strcmp(name, "f") == 0) return host->f_down;
    if (strcmp(name, "g") == 0) return host->g_down;
    if (strcmp(name, "m") == 0 && strcmp(state, "pressed") == 0)
        return host->m_down;
    if (strcmp(name, "n") == 0 && strcmp(state, "pressed") == 0)
        return host->n_down;
    return 0;
}

static void test_ddsl(void *user, const char *action,
                      long value_fixed, const char *value_text)
{
    TestHost *host;
    host = (TestHost *)user;
    if (!host || !action) return;
    if (strcmp(action, "list_cyclenext") == 0) {
        if (value_text && strcmp(value_text, "active_weapon") == 0)
            host->ddsl_cycle_next++;
        return;
    }
    if (strcmp(action, "list_cycleprev") == 0) {
        if (value_text && strcmp(value_text, "active_weapon") == 0)
            host->ddsl_cycle_prev++;
        return;
    }
    if (value_fixed == 0L) return;
    if (strcmp(action, "move_forward") == 0) host->ddsl_move++;
    if (strcmp(action, "jump") == 0) host->ddsl_jump++;
    if (strcmp(action, "move_up") == 0) host->ddsl_up++;
    if (strcmp(action, "move_down") == 0) host->ddsl_down++;
}

static void test_rpyl(void *user, const char *command,
                      const char **args, int argc)
{
    TestHost *host;
    (void)args;
    (void)argc;
    host = (TestHost *)user;
    if (!host || !command) return;
    host->rpyl_commands++;
    if (strcmp(command, "enemy") == 0 ||
        strcmp(command, "zombie") == 0 ||
        strcmp(command, "gunner_enemy") == 0 ||
        strcmp(command, "armed_ally") == 0 ||
        strcmp(command, "ally") == 0 ||
        strcmp(command, "ally_gunner") == 0 ||
        strcmp(command, "hopper_enemy") == 0 ||
        strcmp(command, "dive_enemy") == 0 ||
        strcmp(command, "dive_bomber_enemy") == 0 ||
        strcmp(command, "air_lunger_enemy") == 0 ||
        strcmp(command, "airlunge_enemy") == 0 ||
        strcmp(command, "midair_lunger_enemy") == 0 ||
        strcmp(command, "ground_lancer_enemy") == 0 ||
        strcmp(command, "groundlance_enemy") == 0 ||
        strcmp(command, "pegasus_enemy") == 0)
        host->rpyl_enemies++;
}

static int test_fpi_condition(void *user, void *entity,
                              const char *condition,
                              long value_q16, const char *value_text,
                              int has_value)
{
    TestHost *host;
    (void)entity;
    (void)value_text;
    host = (TestHost *)user;
    if (!host || !condition) return 0;
    if (strcmp(condition, "always") == 0) return 1;
    if (strcmp(condition, "targetalive") == 0 ||
        strcmp(condition, "canattacktarget") == 0 ||
        strcmp(condition, "targethostile") == 0) return 1;
    if (strcmp(condition, "state") == 0)
        return has_value && host->fpi_state == (int)(value_q16 / Q16_ONE);
    if (strcmp(condition, "plrdistwithin") == 0 ||
        strcmp(condition, "targetdistwithin") == 0) return !host->fpi_far;
    if (strcmp(condition, "plrdistfurther") == 0 ||
        strcmp(condition, "targetdistfurther") == 0) return host->fpi_far;
    if (strcmp(condition, "plrflatdistwithin") == 0 ||
        strcmp(condition, "playerflatdistwithin") == 0)
        return !host->fpi_flat_far;
    if (strcmp(condition, "plrflatdistfurther") == 0 ||
        strcmp(condition, "playerflatdistfurther") == 0)
        return host->fpi_flat_far;
    if (strcmp(condition, "grounded") == 0 || strcmp(condition, "onground") == 0)
        return host->fpi_grounded;
    if (strcmp(condition, "airborne") == 0 || strcmp(condition, "offground") == 0)
        return !host->fpi_grounded;
    if (strcmp(condition, "heightbelow") == 0)
        return has_value && host->fpi_height_q16 < value_q16;
    if (strcmp(condition, "heightatleast") == 0)
        return has_value && host->fpi_height_q16 >= value_q16;
    if (strcmp(condition, "belowplayerheight") == 0 ||
        strcmp(condition, "belowplayeroffset") == 0)
        return host->fpi_below_target_height;
    if (strcmp(condition, "aboveplayerheight") == 0 ||
        strcmp(condition, "aboveplayeroffset") == 0)
        return host->fpi_above_target_height;
    if (strcmp(condition, "heightaboveplayeratleast") == 0 ||
        strcmp(condition, "playerbelowby") == 0)
        return host->fpi_height_above_enough;
    if (strcmp(condition, "returnpositionwithin") == 0 ||
        strcmp(condition, "homewithin") == 0)
        return !host->fpi_return_far;
    if (strcmp(condition, "returnpositionfurther") == 0 ||
        strcmp(condition, "homefurther") == 0)
        return host->fpi_return_far;
    if (strcmp(condition, "motionattackimpact") == 0 ||
        strcmp(condition, "attackmotionimpact") == 0 ||
        strcmp(condition, "attackimpact") == 0 ||
        strcmp(condition, "attackhit") == 0)
        return host->fpi_motion_impact;
    if (strcmp(condition, "motionattackfinished") == 0 ||
        strcmp(condition, "attackmotionfinished") == 0 ||
        strcmp(condition, "attackfinished") == 0 ||
        strcmp(condition, "attackdone") == 0)
        return host->fpi_motion_finished;
    if (strcmp(condition, "motionattackactive") == 0 ||
        strcmp(condition, "attackmotionactive") == 0 ||
        strcmp(condition, "airlungeactive") == 0 ||
        strcmp(condition, "groundlanceactive") == 0)
        return !host->fpi_motion_finished;
    return 0;
}

static void test_fpi_action(void *user, void *entity,
                            const char *action,
                            long value_q16, const char *value_text,
                            int has_value)
{
    TestHost *host;
    (void)entity;
    (void)value_text;
    host = (TestHost *)user;
    if (!host || !action) return;
    host->fpi_actions++;
    if (strcmp(action, "movefore") == 0 ||
        strcmp(action, "moveforepattern") == 0 ||
        strcmp(action, "sineplayer") == 0 ||
        strcmp(action, "zigzagplayer") == 0 ||
        strcmp(action, "helixplayer") == 0) host->fpi_movefore++;
    if (strcmp(action, "moveforeflat") == 0 ||
        strcmp(action, "movetowardplayerflat") == 0 ||
        strcmp(action, "moveforeflatpattern") == 0 ||
        strcmp(action, "sineplayerflat") == 0 ||
        strcmp(action, "zigzagplayerflat") == 0 ||
        strcmp(action, "helixplayerflat") == 0 ||
        strcmp(action, "orbitplayerflat") == 0) host->fpi_moveforeflat++;
    if (strcmp(action, "moveup") == 0) host->fpi_moveup++;
    if (strcmp(action, "movedown") == 0) host->fpi_movedown++;
    if (strcmp(action, "jump") == 0 ||
        strcmp(action, "jumpstart") == 0 ||
        strcmp(action, "leap") == 0) host->fpi_jump++;
    if (strcmp(action, "fireplayer") == 0 ||
        strcmp(action, "shootplayer") == 0 ||
        strcmp(action, "firetarget") == 0 ||
        strcmp(action, "shoottarget") == 0) host->fpi_fireplayer++;
    if (strcmp(action, "saveposition") == 0 ||
        strcmp(action, "savereturnposition") == 0 ||
        strcmp(action, "savehome") == 0) host->fpi_saveposition++;
    if (strcmp(action, "diveplayer") == 0 ||
        strcmp(action, "ramplayer") == 0 ||
        strcmp(action, "movetowardplayer") == 0) host->fpi_diveplayer++;
    if (strcmp(action, "returntoposition") == 0 ||
        strcmp(action, "movetohome") == 0 ||
        strcmp(action, "returnhome") == 0) host->fpi_returnposition++;
    if (strcmp(action, "lookatreturnposition") == 0 ||
        strcmp(action, "lookathome") == 0) host->fpi_lookhome++;
    if (strcmp(action, "hurtplayer") == 0 ||
        strcmp(action, "hurttarget") == 0) host->fpi_hurtplayer++;
    if (strcmp(action, "airlungeplayer") == 0 ||
        strcmp(action, "midairlungeplayer") == 0 ||
        strcmp(action, "airramplayer") == 0 ||
        strcmp(action, "clawairplayer") == 0 ||
        strcmp(action, "pierceairplayer") == 0)
        host->fpi_airlungeplayer++;
    if (strcmp(action, "groundlanceplayer") == 0 ||
        strcmp(action, "groundchargeplayer") == 0 ||
        strcmp(action, "pegasusplayer") == 0 ||
        strcmp(action, "groundramplayer") == 0 ||
        strcmp(action, "skimhopplayer") == 0)
        host->fpi_groundlanceplayer++;
    if (strcmp(action, "motionattackreset") == 0 ||
        strcmp(action, "resetmotionattack") == 0 ||
        strcmp(action, "attackreset") == 0)
        host->fpi_motion_reset++;
    if (strcmp(action, "motionattackclearimpact") == 0 ||
        strcmp(action, "clearmotionattackimpact") == 0 ||
        (strcmp(action, "clearattackimpact") == 0 ||
         strcmp(action, "clearimpact") == 0)) {
        host->fpi_motion_clear_impact++;
        host->fpi_motion_impact = 0;
    }
    if ((strcmp(action, "setstate") == 0 ||
         strcmp(action, "state") == 0) && has_value)
        host->fpi_state = (int)(value_q16 / Q16_ONE);
}

int main(void)
{
    Blank3DLanguageHost callbacks;
    TestHost host;
    memset(&host, 0, sizeof(host));
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.user = &host;
    callbacks.key_down = test_key;
    callbacks.input_query = test_input_query;
    callbacks.ddsl_action = test_ddsl;
    callbacks.rpyl_command = test_rpyl;
    callbacks.fpil_condition = test_fpi_condition;
    callbacks.fpil_action = test_fpi_action;

    blank3d_languages_init(&callbacks);
    if (!blank3d_languages_reload_ddsl2("scripts/player.ddsl2")) {
        printf("DDSL2 load failed: %s\n", blank3d_languages_status());
        return 1;
    }
    host.w_down = 1;
    if (!blank3d_languages_tick_ddsl2() || host.ddsl_move != 1) {
        printf("DDSL2 execution failed: %s (%d)\n",
               blank3d_languages_status(), host.ddsl_move);
        return 2;
    }
    host.w_down = 0;
    host.j_down = 1;
    if (!blank3d_languages_tick_ddsl2() || host.ddsl_jump != 1) {
        printf("DDSL2 jump-J failed: %s (%d)\n",
               blank3d_languages_status(), host.ddsl_jump);
        return 8;
    }
    host.j_down = 0;
    host.f_down = 1;
    if (!blank3d_languages_tick_ddsl2() || host.ddsl_up != 1) {
        printf("DDSL2 fly-up-F failed: %s (%d)\n",
               blank3d_languages_status(), host.ddsl_up);
        return 9;
    }
    host.f_down = 0;
    host.g_down = 1;
    if (!blank3d_languages_tick_ddsl2() || host.ddsl_down != 1) {
        printf("DDSL2 fly-down-G failed: %s (%d)\n",
               blank3d_languages_status(), host.ddsl_down);
        return 18;
    }
    host.g_down = 0;
    host.m_down = 1;
    if (!blank3d_languages_tick_ddsl2() || host.ddsl_cycle_next != 1) {
        printf("DDSL2 named list next failed: %s (%d)\n",
               blank3d_languages_status(), host.ddsl_cycle_next);
        return 22;
    }
    host.m_down = 0;
    host.n_down = 1;
    if (!blank3d_languages_tick_ddsl2() || host.ddsl_cycle_prev != 1) {
        printf("DDSL2 named list prev failed: %s (%d)\n",
               blank3d_languages_status(), host.ddsl_cycle_prev);
        return 23;
    }
    host.n_down = 0;

    if (!blank3d_languages_reload_fpil("scripts/enemy.fpi")) {
        printf("FPIL load failed: %s\n", blank3d_languages_status());
        return 3;
    }
    if (!blank3d_languages_tick_fpil(&host) ||
        host.fpi_state != 1 || host.fpi_actions < 1) {
        printf("FPIL execution failed: %s state=%d actions=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_actions);
        return 4;
    }


    host.fpi_state = 0;
    host.fpi_movefore = 0;
    host.fpi_fireplayer = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/gunner_enemy.fpi", &host) ||
        host.fpi_fireplayer < 1 || host.fpi_movefore != 0) {
        printf("FPIL per-script routing failed: %s fire=%d move=%d\n",
               blank3d_languages_status(), host.fpi_fireplayer,
               host.fpi_movefore);
        return 5;
    }

    host.fpi_state = 0;
    host.fpi_far = 1;
    host.fpi_fireplayer = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/armed_ally.fpi", &host) ||
        !blank3d_languages_tick_fpil_path("scripts/armed_ally.fpi", &host) ||
        host.fpi_fireplayer < 1) {
        printf("allied target-first FPIL failed: %s fire=%d state=%d\n",
               blank3d_languages_status(), host.fpi_fireplayer,
               host.fpi_state);
        return 31;
    }

    host.fpi_state = 0;
    host.fpi_far = 1;
    host.fpi_flat_far = 1;
    host.fpi_grounded = 1;
    host.fpi_height_q16 = 0L;
    host.fpi_moveup = 0;
    host.fpi_movedown = 0;
    host.fpi_jump = 0;
    host.fpi_moveforeflat = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/hopper_enemy.fpi", &host) ||
        host.fpi_state != 1 || host.fpi_jump != 1 ||
        host.fpi_moveforeflat != 1) {
        printf("hopper jump takeoff failed: %s state=%d jump=%d flat=%d\n",
               blank3d_languages_status(), host.fpi_state, host.fpi_jump,
               host.fpi_moveforeflat);
        return 10;
    }
    host.fpi_grounded = 0;
    host.fpi_height_q16 = 2L * Q16_ONE;
    if (!blank3d_languages_tick_fpil_path("scripts/hopper_enemy.fpi", &host) ||
        host.fpi_state != 1 || host.fpi_jump != 1 ||
        host.fpi_moveforeflat != 2) {
        printf("hopper airborne pursuit failed: %s state=%d jump=%d flat=%d\n",
               blank3d_languages_status(), host.fpi_state, host.fpi_jump,
               host.fpi_moveforeflat);
        return 11;
    }
    host.fpi_far = 0;
    host.fpi_flat_far = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/hopper_enemy.fpi", &host) ||
        host.fpi_state != 1 || host.fpi_moveforeflat != 2) {
        printf("hopper close air brake failed: %s state=%d flat=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_moveforeflat);
        return 12;
    }
    host.fpi_grounded = 1;
    host.fpi_height_q16 = 0L;
    if (!blank3d_languages_tick_fpil_path("scripts/hopper_enemy.fpi", &host) ||
        host.fpi_state != 0) {
        printf("hopper landing reset failed: %s state=%d\n",
               blank3d_languages_status(), host.fpi_state);
        return 13;
    }
    (void)blank3d_languages_tick_fpil_path("scripts/hopper_enemy.fpi", &host);
    if (host.fpi_state != 0 || host.fpi_jump != 1 ||
        host.fpi_moveforeflat != 2) {
        printf("hopper close idle failed: %s state=%d jump=%d flat=%d\n",
               blank3d_languages_status(), host.fpi_state, host.fpi_jump,
               host.fpi_moveforeflat);
        return 14;
    }

    host.fpi_state = 0;
    host.fpi_far = 1;
    host.fpi_flat_far = 1;
    host.fpi_below_target_height = 0;
    host.fpi_above_target_height = 0;
    host.fpi_height_above_enough = 1;
    host.fpi_return_far = 1;
    host.fpi_saveposition = 0;
    host.fpi_diveplayer = 0;
    host.fpi_returnposition = 0;
    host.fpi_lookhome = 0;
    host.fpi_hurtplayer = 0;
    host.fpi_moveforeflat = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/dive_enemy.fpi", &host) ||
        host.fpi_state != 0 || host.fpi_moveforeflat != 1) {
        printf("dive enemy hover pursuit failed: %s state=%d flat=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_moveforeflat);
        return 16;
    }
    host.fpi_flat_far = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/dive_enemy.fpi", &host) ||
        host.fpi_state != 1 || host.fpi_saveposition != 1 ||
        host.fpi_diveplayer != 1) {
        printf("dive enemy launch failed: %s state=%d save=%d dive=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_saveposition, host.fpi_diveplayer);
        return 17;
    }
    if (!blank3d_languages_tick_fpil_path("scripts/dive_enemy.fpi", &host) ||
        host.fpi_state != 1 || host.fpi_diveplayer != 2) {
        printf("dive enemy descent failed: %s state=%d dive=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_diveplayer);
        return 18;
    }
    host.fpi_far = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/dive_enemy.fpi", &host) ||
        host.fpi_state != 2 || host.fpi_hurtplayer != 1 ||
        host.fpi_returnposition != 1 || host.fpi_lookhome != 1) {
        printf("dive enemy impact failed: %s state=%d hurt=%d return=%d home=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_hurtplayer, host.fpi_returnposition,
               host.fpi_lookhome);
        return 19;
    }
    if (!blank3d_languages_tick_fpil_path("scripts/dive_enemy.fpi", &host) ||
        host.fpi_state != 2 || host.fpi_returnposition != 2) {
        printf("dive enemy return failed: %s state=%d return=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_returnposition);
        return 20;
    }
    host.fpi_return_far = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/dive_enemy.fpi", &host) ||
        host.fpi_state != 0) {
        printf("dive enemy reset failed: %s state=%d\n",
               blank3d_languages_status(), host.fpi_state);
        return 21;
    }

    host.fpi_state = 0;
    host.fpi_far = 1;
    host.fpi_flat_far = 1;
    host.fpi_grounded = 1;
    host.fpi_motion_impact = 0;
    host.fpi_motion_finished = 0;
    host.fpi_airlungeplayer = 0;
    host.fpi_motion_reset = 0;
    host.fpi_motion_clear_impact = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/air_lunger_enemy.fpi", &host) ||
        host.fpi_state != 1 || host.fpi_jump < 2) {
        printf("air lunger takeoff failed: %s state=%d jump=%d\n",
               blank3d_languages_status(), host.fpi_state, host.fpi_jump);
        return 24;
    }
    host.fpi_grounded = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/air_lunger_enemy.fpi", &host) ||
        host.fpi_airlungeplayer < 1) {
        printf("air lunger request failed: %s lunge=%d\n",
               blank3d_languages_status(), host.fpi_airlungeplayer);
        return 25;
    }
    host.fpi_motion_impact = 1;
    if (!blank3d_languages_tick_fpil_path("scripts/air_lunger_enemy.fpi", &host) ||
        host.fpi_state != 2 || host.fpi_hurtplayer < 2 ||
        host.fpi_motion_clear_impact < 1) {
        printf("air lunger impact failed: %s state=%d hurt=%d clear=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_hurtplayer, host.fpi_motion_clear_impact);
        return 26;
    }
    host.fpi_grounded = 1;
    if (!blank3d_languages_tick_fpil_path("scripts/air_lunger_enemy.fpi", &host) ||
        host.fpi_state != 0 || host.fpi_motion_reset < 1) {
        printf("air lunger reset failed: %s state=%d reset=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_motion_reset);
        return 27;
    }

    host.fpi_state = 0;
    host.fpi_far = 1;
    host.fpi_flat_far = 1;
    host.fpi_grounded = 1;
    host.fpi_motion_impact = 0;
    host.fpi_motion_finished = 0;
    host.fpi_groundlanceplayer = 0;
    host.fpi_motion_reset = 0;
    host.fpi_motion_clear_impact = 0;
    if (!blank3d_languages_tick_fpil_path("scripts/ground_lancer_enemy.fpi", &host) ||
        host.fpi_state != 1 || host.fpi_groundlanceplayer != 1) {
        printf("ground lancer request failed: %s state=%d lance=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_groundlanceplayer);
        return 28;
    }
    host.fpi_motion_impact = 1;
    if (!blank3d_languages_tick_fpil_path("scripts/ground_lancer_enemy.fpi", &host) ||
        host.fpi_state != 2 || host.fpi_motion_clear_impact != 1) {
        printf("ground lancer impact failed: %s state=%d clear=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_motion_clear_impact);
        return 29;
    }
    host.fpi_motion_finished = 1;
    if (!blank3d_languages_tick_fpil_path("scripts/ground_lancer_enemy.fpi", &host) ||
        host.fpi_state != 0 || host.fpi_motion_reset < 1) {
        printf("ground lancer reset failed: %s state=%d reset=%d\n",
               blank3d_languages_status(), host.fpi_state,
               host.fpi_motion_reset);
        return 30;
    }

    if (!blank3d_languages_run_rpyl("scripts/startup.rpy")) {
        printf("RPYL run failed: %s\n", blank3d_languages_status());
        return 6;
    }
    if (host.rpyl_commands < 15 || host.rpyl_enemies != 8) {
        printf("RPYL command dispatch failed: commands=%d enemies=%d\n",
               host.rpyl_commands, host.rpyl_enemies);
        return 7;
    }

    puts("Blank3D vendored DDSL2 + FPIL + RPYL test: OK");
    return 0;
}
