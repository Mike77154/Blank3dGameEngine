#ifndef GRELOAD89_H
#define GRELOAD89_H

/*
   greload89
   C89 integer-millisecond reload timer and completion request.
   No dynamic allocation ownership and no floating-point types.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GREL89_OK                 0
#define GREL89_STARTED            1
#define GREL89_COMPLETED          2
#define GREL89_IDLE               3
#define GREL89_BAD_ARG           -1
#define GREL89_ALREADY_ACTIVE    -2
#define GREL89_NOT_NEEDED        -3
#define GREL89_COMPLETION_PENDING -4

typedef struct GREL89_StateTag {
    int active;
    int completion_pending;
    int rounds_requested;
    unsigned long duration_ms;
    unsigned long remaining_ms;
} GREL89_State;

void grel89_init(GREL89_State *reload);
int  grel89_begin(GREL89_State *reload,
                  unsigned long duration_ms,
                  int rounds_requested);
int  grel89_begin_full(GREL89_State *reload,
                       unsigned long duration_ms,
                       int current_rounds,
                       int capacity);
int  grel89_update(GREL89_State *reload, unsigned long dt_ms);
int  grel89_cancel(GREL89_State *reload);

int  grel89_is_active(const GREL89_State *reload);
int  grel89_has_completion(const GREL89_State *reload);
int  grel89_rounds_requested(const GREL89_State *reload);
int  grel89_take_completed_rounds(GREL89_State *reload);
unsigned long grel89_remaining_ms(const GREL89_State *reload);
unsigned long grel89_duration_ms(const GREL89_State *reload);
int  grel89_progress_permyriad(const GREL89_State *reload);

#ifdef __cplusplus
}
#endif

#endif
