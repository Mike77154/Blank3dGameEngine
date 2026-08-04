#include "cycler89.h"
#include "blank3d_list_cycle.h"

#include <stdio.h>
#include <string.h>

typedef struct TestListTag {
    long items[6];
    int active[6];
    int count;
    long current;
    int has_current;
} TestList;

static int test_count(void *context)
{
    TestList *list;
    list = (TestList *)context;
    return list ? list->count : 0;
}

static int test_read(void *context, int index, cycler89_item *item_out)
{
    TestList *list;
    list = (TestList *)context;
    if (!list || !item_out || index < 0 || index >= list->count) return 0;
    *item_out = list->items[index];
    return 1;
}

static int test_active(void *context, int index, cycler89_item item)
{
    TestList *list;
    (void)item;
    list = (TestList *)context;
    if (!list || index < 0 || index >= list->count) return 0;
    return list->active[index];
}

static int test_current(void *context, cycler89_item *item_out)
{
    TestList *list;
    list = (TestList *)context;
    if (!list || !item_out || !list->has_current) return 0;
    *item_out = list->current;
    return 1;
}

static int test_activate(void *context, cycler89_item item)
{
    TestList *list;
    list = (TestList *)context;
    if (!list) return 0;
    list->current = item;
    list->has_current = 1;
    return 1;
}

int main(void)
{
    TestList source;
    Cycler89List list;
    Cycler89Cursor cursor;
    Blank3DListCycleRegistry registry;
    cycler89_item item;
    int index;

    memset(&source, 0, sizeof(source));
    source.items[0] = 10L;
    source.items[1] = 20L;
    source.items[2] = 30L;
    source.items[3] = 40L;
    source.active[0] = 1;
    source.active[2] = 1;
    source.count = 4;

    list.context = &source;
    list.count = test_count;
    list.read = test_read;
    list.is_active = test_active;

    if (cycler89_next(&list, 10L, 1, &item, &index) != CYCLER89_OK ||
        item != 30L || index != 2) return 1;
    if (cycler89_next(&list, 30L, 1, &item, &index) != CYCLER89_OK ||
        item != 10L || index != 0) return 2;
    if (cycler89_prev(&list, 10L, 1, &item, &index) != CYCLER89_OK ||
        item != 30L || index != 2) return 3;
    if (cycler89_next(&list, 999L, 1, &item, &index) != CYCLER89_OK ||
        item != 10L) return 4;
    if (cycler89_prev(&list, 999L, 1, &item, &index) != CYCLER89_OK ||
        item != 30L) return 5;

    cycler89_cursor_init(&cursor);
    if (cycler89_cursor_next(&list, &cursor) != CYCLER89_OK ||
        cursor.item != 10L) return 6;
    if (cycler89_cursor_next(&list, &cursor) != CYCLER89_OK ||
        cursor.item != 30L) return 7;
    if (cycler89_cursor_prev(&list, &cursor) != CYCLER89_OK ||
        cursor.item != 10L) return 8;

    blank3d_list_cycle_init(&registry);
    if (!blank3d_list_cycle_register(&registry, "Active Weapon", &list,
                                      &source, test_current, test_activate))
        return 9;
    source.current = 10L;
    source.has_current = 1;
    if (!blank3d_list_cycle_next(&registry, "active_weapon", &item) ||
        item != 30L || source.current != 30L) return 10;
    if (!blank3d_list_cycle_prev(&registry, "ACTIVE-WEAPON", &item) ||
        item != 10L || source.current != 10L) return 11;

    source.active[0] = 0;
    source.active[2] = 0;
    if (cycler89_next(&list, 10L, 1, &item, &index) != CYCLER89_NO_ACTIVE)
        return 12;

    puts("cycler89 + Blank3D named list registry: OK");
    return 0;
}
