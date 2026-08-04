#ifndef GMAGAZINE89_H
#define GMAGAZINE89_H

/*
   gmagazine89
   C89 fixed-capacity magazine accounting.
   No dynamic allocation ownership and no floating-point types.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GMAG89_OK                 0
#define GMAG89_BYPASS             1
#define GMAG89_BAD_ARG           -1
#define GMAG89_EMPTY             -2
#define GMAG89_NOT_ENOUGH        -3

#define GMAG89_FLAG_ENABLED       1
#define GMAG89_FLAG_EMPTY         2
#define GMAG89_FLAG_FULL          4
#define GMAG89_FLAG_CAN_FIRE      8

typedef struct GMAG89_StateTag {
    int capacity;
    int rounds;
    int rounds_per_shot;
} GMAG89_State;

typedef struct GMAG89_ReportTag {
    int capacity;
    int rounds;
    int missing;
    int rounds_per_shot;
    int flags;
} GMAG89_Report;

void gmag89_init(GMAG89_State *magazine,
                 int capacity,
                 int initial_rounds,
                 int rounds_per_shot);
int  gmag89_configure(GMAG89_State *magazine,
                      int capacity,
                      int initial_rounds,
                      int rounds_per_shot);
int  gmag89_set_capacity(GMAG89_State *magazine, int capacity);
int  gmag89_set_rounds(GMAG89_State *magazine, int rounds);
int  gmag89_set_rounds_per_shot(GMAG89_State *magazine, int rounds_per_shot);

int  gmag89_load(GMAG89_State *magazine, int amount);
int  gmag89_fill(GMAG89_State *magazine);
int  gmag89_spend(GMAG89_State *magazine, int amount);
int  gmag89_spend_shot(GMAG89_State *magazine);

int  gmag89_is_enabled(const GMAG89_State *magazine);
int  gmag89_is_empty(const GMAG89_State *magazine);
int  gmag89_is_full(const GMAG89_State *magazine);
int  gmag89_can_spend(const GMAG89_State *magazine, int amount);
int  gmag89_can_fire(const GMAG89_State *magazine);
int  gmag89_rounds(const GMAG89_State *magazine);
int  gmag89_capacity(const GMAG89_State *magazine);
int  gmag89_missing(const GMAG89_State *magazine);
void gmag89_report(const GMAG89_State *magazine, GMAG89_Report *report);

#ifdef __cplusplus
}
#endif

#endif
