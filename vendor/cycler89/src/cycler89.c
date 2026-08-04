#include "cycler89.h"

static int cycler89_valid_list(const Cycler89List *list)
{
    return list && list->count && list->read;
}

static int cycler89_item_active(const Cycler89List *list,
                                int index,
                                cycler89_item item)
{
    if (!list->is_active) return 1;
    return list->is_active(list->context, index, item) ? 1 : 0;
}

void cycler89_cursor_init(Cycler89Cursor *cursor)
{
    if (!cursor) return;
    cursor->has_item = 0;
    cursor->index = -1;
    cursor->item = 0L;
}

int cycler89_find(const Cycler89List *list,
                  cycler89_item item,
                  int *index_out)
{
    int count;
    int i;
    cycler89_item candidate;
    if (!cycler89_valid_list(list)) return CYCLER89_BAD_ARG;
    count = list->count(list->context);
    if (count <= 0) return CYCLER89_EMPTY;
    for (i = 0; i < count; ++i) {
        if (!list->read(list->context, i, &candidate))
            return CYCLER89_SOURCE_ERROR;
        if (candidate == item) {
            if (index_out) *index_out = i;
            return CYCLER89_OK;
        }
    }
    return CYCLER89_NOT_FOUND;
}

int cycler89_step(const Cycler89List *list,
                  cycler89_item current_item,
                  int has_current,
                  int direction,
                  cycler89_item *item_out,
                  int *index_out)
{
    int count;
    int current_index;
    int index;
    int visited;
    int found_current;
    cycler89_item candidate;
    if (!cycler89_valid_list(list) || !item_out)
        return CYCLER89_BAD_ARG;
    count = list->count(list->context);
    if (count <= 0) return CYCLER89_EMPTY;
    direction = direction >= 0 ? 1 : -1;
    current_index = direction > 0 ? -1 : 0;
    found_current = 0;
    if (has_current) {
        for (index = 0; index < count; ++index) {
            if (!list->read(list->context, index, &candidate))
                return CYCLER89_SOURCE_ERROR;
            if (candidate == current_item) {
                current_index = index;
                found_current = 1;
                break;
            }
        }
    }
    if (!found_current && direction < 0) current_index = 0;
    index = current_index;
    for (visited = 0; visited < count; ++visited) {
        index += direction;
        if (index >= count) index = 0;
        if (index < 0) index = count - 1;
        if (!list->read(list->context, index, &candidate))
            return CYCLER89_SOURCE_ERROR;
        if (cycler89_item_active(list, index, candidate)) {
            *item_out = candidate;
            if (index_out) *index_out = index;
            return CYCLER89_OK;
        }
    }
    return CYCLER89_NO_ACTIVE;
}

int cycler89_next(const Cycler89List *list,
                  cycler89_item current_item,
                  int has_current,
                  cycler89_item *item_out,
                  int *index_out)
{
    return cycler89_step(list, current_item, has_current, 1,
                         item_out, index_out);
}

int cycler89_prev(const Cycler89List *list,
                  cycler89_item current_item,
                  int has_current,
                  cycler89_item *item_out,
                  int *index_out)
{
    return cycler89_step(list, current_item, has_current, -1,
                         item_out, index_out);
}

int cycler89_cursor_step(const Cycler89List *list,
                         Cycler89Cursor *cursor,
                         int direction)
{
    int result;
    cycler89_item item;
    int index;
    if (!cursor) return CYCLER89_BAD_ARG;
    result = cycler89_step(list, cursor->item, cursor->has_item,
                           direction, &item, &index);
    if (result != CYCLER89_OK) return result;
    cursor->has_item = 1;
    cursor->item = item;
    cursor->index = index;
    return CYCLER89_OK;
}

int cycler89_cursor_next(const Cycler89List *list,
                         Cycler89Cursor *cursor)
{
    return cycler89_cursor_step(list, cursor, 1);
}

int cycler89_cursor_prev(const Cycler89List *list,
                         Cycler89Cursor *cursor)
{
    return cycler89_cursor_step(list, cursor, -1);
}
