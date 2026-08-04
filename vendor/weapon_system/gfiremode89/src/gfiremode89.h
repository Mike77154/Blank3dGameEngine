#ifndef GFIREMODE89_H
#define GFIREMODE89_H

/*
   gfiremode89
   C89 trigger automation and fire cadence.
   No dynamic allocation ownership and no floating-point types.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GFM89_OK                 0
#define GFM89_FIRE_REQUEST       1
#define GFM89_TRIGGER_LOCKED    -1
#define GFM89_COOLDOWN          -2
#define GFM89_PENDING           -3
#define GFM89_BAD_ARG           -4
#define GFM89_NO_PENDING        -5

#define GFM89_TRIGGER_NONE       0
#define GFM89_TRIGGER_DOWN       1
#define GFM89_TRIGGER_PRESSED    2
#define GFM89_TRIGGER_RELEASED   4

#define GFM89_FIRE_SEMI          0
#define GFM89_FIRE_AUTO          1
#define GFM89_FIRE_HOLD_ONCE     2
#define GFM89_FIRE_BURST         3

typedef struct GFM89_StateTag {
    int mode;
    int trigger_latched;
    int burst_left;
    int burst_count;
    int pending_request;
    unsigned short shot_interval_ms;
    unsigned short cooldown_ms_left;
} GFM89_State;

void gfm89_init(GFM89_State *state);
int  gfm89_configure(GFM89_State *state,
                     int mode,
                     int burst_count,
                     unsigned short shot_interval_ms);
void gfm89_reset(GFM89_State *state);
void gfm89_update(GFM89_State *state, unsigned short dt_ms);

/*
   Two-phase fire decision:
   1. request() says whether the trigger policy wants a shot.
   2. commit_fire() starts cadence only after ammo/reload checks succeed.
      reject_fire() clears the pending request without spending cadence.
*/
int  gfm89_request(GFM89_State *state,
                   int trigger_flags,
                   unsigned short dt_ms);
int  gfm89_commit_fire(GFM89_State *state);
int  gfm89_reject_fire(GFM89_State *state);

void gfm89_release_latch(GFM89_State *state);
void gfm89_cancel_burst(GFM89_State *state);
int  gfm89_is_ready(const GFM89_State *state);
int  gfm89_is_pending(const GFM89_State *state);
int  gfm89_burst_remaining(const GFM89_State *state);
unsigned short gfm89_cooldown_remaining(const GFM89_State *state);

int         gfm89_mode_from_name(const char *name);
const char *gfm89_mode_name(int mode);

#ifdef __cplusplus
}
#endif

#endif
