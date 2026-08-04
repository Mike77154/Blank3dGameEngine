#ifndef CYCLER89_H
#define CYCLER89_H

#ifdef __cplusplus
extern "C" {
#endif

typedef long cycler89_item;

enum Cycler89ResultTag {
    CYCLER89_OK = 0,
    CYCLER89_BAD_ARG = -1,
    CYCLER89_EMPTY = -2,
    CYCLER89_NO_ACTIVE = -3,
    CYCLER89_SOURCE_ERROR = -4,
    CYCLER89_NOT_FOUND = -5
};

typedef int (*cycler89_count_fn)(void *context);
typedef int (*cycler89_read_fn)(void *context, int index,
                                cycler89_item *item_out);
typedef int (*cycler89_active_fn)(void *context, int index,
                                  cycler89_item item);

typedef struct Cycler89ListTag {
    void *context;
    cycler89_count_fn count;
    cycler89_read_fn read;
    cycler89_active_fn is_active;
} Cycler89List;

typedef struct Cycler89CursorTag {
    int has_item;
    int index;
    cycler89_item item;
} Cycler89Cursor;

void cycler89_cursor_init(Cycler89Cursor *cursor);

int cycler89_find(const Cycler89List *list,
                  cycler89_item item,
                  int *index_out);

int cycler89_step(const Cycler89List *list,
                  cycler89_item current_item,
                  int has_current,
                  int direction,
                  cycler89_item *item_out,
                  int *index_out);

int cycler89_next(const Cycler89List *list,
                  cycler89_item current_item,
                  int has_current,
                  cycler89_item *item_out,
                  int *index_out);

int cycler89_prev(const Cycler89List *list,
                  cycler89_item current_item,
                  int has_current,
                  cycler89_item *item_out,
                  int *index_out);

int cycler89_cursor_step(const Cycler89List *list,
                         Cycler89Cursor *cursor,
                         int direction);

int cycler89_cursor_next(const Cycler89List *list,
                         Cycler89Cursor *cursor);

int cycler89_cursor_prev(const Cycler89List *list,
                         Cycler89Cursor *cursor);

#ifdef __cplusplus
}
#endif

#endif
