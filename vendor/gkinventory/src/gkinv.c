#include "gkinv.h"

static gkinv_bool slot_is_empty(const gkinv_Slot *slot)
{
    if (slot == (const gkinv_Slot *)0) {
        return GKINV_TRUE;
    }
    return (slot->item_id == (gkinv_u16)GKINV_EMPTY_ITEM_ID || slot->quantity == 0u) ? GKINV_TRUE : GKINV_FALSE;
}

static void slot_from_stack(gkinv_Slot *slot, const gkinv_ItemStack *stack)
{
    slot->item_id = stack->item_id;
    slot->quantity = stack->quantity;
    slot->x = 0u;
    slot->y = 0u;
    slot->flags = stack->flags;
    slot->state0 = stack->state0;
    slot->state1 = stack->state1;
    slot->state2 = stack->state2;
    slot->state3 = stack->state3;
}

static void stack_from_slot(gkinv_ItemStack *stack, const gkinv_Slot *slot, gkinv_u16 quantity)
{
    stack->item_id = slot->item_id;
    stack->quantity = quantity;
    stack->flags = slot->flags;
    stack->state0 = slot->state0;
    stack->state1 = slot->state1;
    stack->state2 = slot->state2;
    stack->state3 = slot->state3;
}

static gkinv_bool slots_can_merge(const gkinv_Slot *slot, const gkinv_ItemStack *stack)
{
    if (slot->item_id != stack->item_id) {
        return GKINV_FALSE;
    }
    if (slot->flags != stack->flags) {
        return GKINV_FALSE;
    }
    if (slot->state0 != stack->state0) {
        return GKINV_FALSE;
    }
    if (slot->state1 != stack->state1) {
        return GKINV_FALSE;
    }
    if (slot->state2 != stack->state2) {
        return GKINV_FALSE;
    }
    if (slot->state3 != stack->state3) {
        return GKINV_FALSE;
    }
    return GKINV_TRUE;
}

static gkinv_u16 count_slots(const gkinv_Container *container)
{
    gkinv_u16 i;
    gkinv_u16 count;

    count = 0u;
    if (container == (const gkinv_Container *)0 || container->slots == (gkinv_Slot *)0) {
        return 0u;
    }
    i = 0u;
    while (i < container->slot_capacity) {
        if (!slot_is_empty(&container->slots[i])) {
            count = (gkinv_u16)(count + 1u);
        }
        i = (gkinv_u16)(i + 1u);
    }
    return count;
}

static void copy_slots(gkinv_Slot *dst, const gkinv_Slot *src, gkinv_u16 count)
{
    gkinv_u16 i;

    i = 0u;
    while (i < count) {
        dst[i] = src[i];
        i = (gkinv_u16)(i + 1u);
    }
}

static void emit_event(gkinv_Container *src,
                       gkinv_Container *dst,
                       int type,
                       int result,
                       gkinv_u16 item_id,
                       gkinv_u16 quantity,
                       gkinv_u16 src_index,
                       gkinv_u16 dst_index)
{
#if GKINV_ENABLE_HOOKS
    gkinv_Event event_data;
    const gkinv_Hooks *hooks;
    void *hook_user;

    hooks = (const gkinv_Hooks *)0;
    hook_user = (void *)0;
    if (dst != (gkinv_Container *)0 && dst->hooks != (const gkinv_Hooks *)0) {
        hooks = dst->hooks;
        hook_user = dst->hook_user;
    } else if (src != (gkinv_Container *)0 && src->hooks != (const gkinv_Hooks *)0) {
        hooks = src->hooks;
        hook_user = src->hook_user;
    }
    if (hooks != (const gkinv_Hooks *)0 && hooks->on_event != (gkinv_EventFn)0) {
        event_data.type = type;
        event_data.result = result;
        event_data.src = src;
        event_data.dst = dst;
        event_data.item_id = item_id;
        event_data.quantity = quantity;
        event_data.src_index = src_index;
        event_data.dst_index = dst_index;
        hooks->on_event(&event_data, hook_user);
    }
#else
    GKINV_UNUSED(src);
    GKINV_UNUSED(dst);
    GKINV_UNUSED(type);
    GKINV_UNUSED(result);
    GKINV_UNUSED(item_id);
    GKINV_UNUSED(quantity);
    GKINV_UNUSED(src_index);
    GKINV_UNUSED(dst_index);
#endif
}

static const gkinv_SlotRule *find_slot_rule(const gkinv_Container *container, gkinv_u16 index)
{
    gkinv_u16 i;

    if (container == (const gkinv_Container *)0 || container->slot_rules == (const gkinv_SlotRule *)0) {
        return (const gkinv_SlotRule *)0;
    }
    i = 0u;
    while (i < container->slot_rule_count) {
        if (container->slot_rules[i].slot_index == index) {
            return &container->slot_rules[i];
        }
        i = (gkinv_u16)(i + 1u);
    }
    return (const gkinv_SlotRule *)0;
}

static gkinv_u16 max_stack_for_slot(const gkinv_Container *container,
                                    const gkinv_ItemDef *def,
                                    gkinv_u16 index)
{
    const gkinv_SlotRule *rule;
    gkinv_u16 max_stack;

    max_stack = gkinv_item_max_stack(def);
    rule = find_slot_rule(container, index);
    if (rule != (const gkinv_SlotRule *)0 && rule->max_stack != 0u && rule->max_stack < max_stack) {
        max_stack = rule->max_stack;
    }
    if (max_stack == 0u) {
        max_stack = 1u;
    }
    return max_stack;
}

static int slot_rule_accepts(const gkinv_Container *container,
                             const gkinv_ItemDef *def,
                             gkinv_u16 index)
{
    const gkinv_SlotRule *rule;

    if (container == (const gkinv_Container *)0 || def == (const gkinv_ItemDef *)0) {
        return GKINV_ERR_NULL;
    }
    if (container->allowed_categories != 0UL) {
        if ((def->category_mask & container->allowed_categories) == 0UL) {
            return GKINV_ERR_CATEGORY;
        }
    }
    rule = find_slot_rule(container, index);
    if (rule != (const gkinv_SlotRule *)0) {
        if (rule->accept_categories != 0UL) {
            if ((def->category_mask & rule->accept_categories) == 0UL) {
                return GKINV_ERR_CATEGORY;
            }
        }
        if (rule->require_flags != 0UL) {
            if ((def->flags & rule->require_flags) != rule->require_flags) {
                return GKINV_ERR_FLAGS;
            }
        }
        if (rule->reject_flags != 0UL) {
            if ((def->flags & rule->reject_flags) != 0UL) {
                return GKINV_ERR_FLAGS;
            }
        }
    }
    return GKINV_OK;
}

static gkinv_fx stack_weight(const gkinv_ItemDef *def, gkinv_u16 quantity)
{
    if (def == (const gkinv_ItemDef *)0) {
        return GKINV_FX_ZERO;
    }
    return gkinv_fx_mul_int_sat(def->weight, quantity);
}

static int check_global_accept(const gkinv_ItemDB *db,
                               gkinv_Container *container,
                               const gkinv_ItemStack *stack,
                               gkinv_u16 dst_index,
                               gkinv_bool call_hook)
{
    const gkinv_ItemDef *def;
    gkinv_fx current_weight;
    gkinv_fx added_weight;
    gkinv_fx new_weight;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || container == (gkinv_Container *)0 || stack == (const gkinv_ItemStack *)0) {
        return GKINV_ERR_NULL;
    }
    if ((container->flags & GKINV_CONT_READ_ONLY) != 0UL) {
        return GKINV_ERR_READ_ONLY;
    }
    if (stack->item_id == (gkinv_u16)GKINV_EMPTY_ITEM_ID) {
        return GKINV_ERR_BAD_ITEM;
    }
    if (stack->quantity == 0u) {
        return GKINV_ERR_BAD_QUANTITY;
    }
    def = gkinv_db_find(db, stack->item_id);
    if (def == (const gkinv_ItemDef *)0) {
        return GKINV_ERR_BAD_ITEM;
    }
    rc = slot_rule_accepts(container, def, dst_index);
    if (rc != GKINV_OK && dst_index != (gkinv_u16)GKINV_NO_INDEX) {
        return rc;
    }
    if (container->allowed_categories != 0UL && (def->category_mask & container->allowed_categories) == 0UL) {
        return GKINV_ERR_CATEGORY;
    }
    if (container->max_weight > GKINV_FX_ZERO) {
        current_weight = gkinv_total_weight(db, container);
        added_weight = stack_weight(def, stack->quantity);
        new_weight = gkinv_fx_add_sat(current_weight, added_weight);
        if (new_weight > container->max_weight || new_weight == GKINV_FX_MAX) {
            return GKINV_ERR_WEIGHT;
        }
    }
#if GKINV_ENABLE_HOOKS
    if (call_hook != GKINV_FALSE && container->hooks != (const gkinv_Hooks *)0 && container->hooks->can_add != (gkinv_CanAddFn)0) {
        rc = container->hooks->can_add(container, db, def, stack, dst_index, container->hook_user);
        if (rc != GKINV_OK) {
            return rc;
        }
    }
#else
    GKINV_UNUSED(call_hook);
#endif
    return GKINV_OK;
}

#if GKINV_ENABLE_GRID
static gkinv_u32 grid_cell_count(const gkinv_Container *container)
{
    return (gkinv_u32)container->grid_w * (gkinv_u32)container->grid_h;
}

static int grid_check_size(const gkinv_Container *container)
{
    gkinv_u32 cell_count;

    if (container == (const gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if ((container->flags & GKINV_CONT_GRID) == 0UL) {
        return GKINV_OK;
    }
    if (container->grid_w == 0u || container->grid_h == 0u) {
        return GKINV_ERR_GRID;
    }
    cell_count = grid_cell_count(container);
    if (cell_count > (gkinv_u32)GKINV_GRID_MAX_CELLS) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    return GKINV_OK;
}

static void grid_clear_occ(gkinv_u8 *occ)
{
    gkinv_u16 i;

    i = 0u;
    while (i < (gkinv_u16)GKINV_GRID_MAX_CELLS) {
        occ[i] = 0u;
        i = (gkinv_u16)(i + 1u);
    }
}

static int grid_rect_can_mark(const gkinv_u8 *occ,
                              gkinv_u16 grid_w,
                              gkinv_u16 grid_h,
                              gkinv_u16 x,
                              gkinv_u16 y,
                              gkinv_u16 w,
                              gkinv_u16 h,
                              gkinv_u8 value)
{
    gkinv_u16 yy;
    gkinv_u16 xx;
    gkinv_u32 offset;

    if (w == 0u || h == 0u) {
        return GKINV_ERR_GRID;
    }
    if (x >= grid_w || y >= grid_h) {
        return GKINV_ERR_NO_ROOM;
    }
    if ((gkinv_u32)x + (gkinv_u32)w > (gkinv_u32)grid_w) {
        return GKINV_ERR_NO_ROOM;
    }
    if ((gkinv_u32)y + (gkinv_u32)h > (gkinv_u32)grid_h) {
        return GKINV_ERR_NO_ROOM;
    }
    yy = 0u;
    while (yy < h) {
        xx = 0u;
        while (xx < w) {
            offset = ((gkinv_u32)(y + yy) * (gkinv_u32)grid_w) + (gkinv_u32)(x + xx);
            if (offset >= (gkinv_u32)GKINV_GRID_MAX_CELLS) {
                return GKINV_ERR_TEMP_LIMIT;
            }
            if (value != 0u && occ[offset] != 0u) {
                return GKINV_ERR_NO_ROOM;
            }
            xx = (gkinv_u16)(xx + 1u);
        }
        yy = (gkinv_u16)(yy + 1u);
    }
    return GKINV_OK;
}

static int grid_mark_rect(gkinv_u8 *occ,
                          gkinv_u16 grid_w,
                          gkinv_u16 grid_h,
                          gkinv_u16 x,
                          gkinv_u16 y,
                          gkinv_u16 w,
                          gkinv_u16 h,
                          gkinv_u8 value)
{
    gkinv_u16 yy;
    gkinv_u16 xx;
    gkinv_u32 offset;
    int rc;

    rc = grid_rect_can_mark(occ, grid_w, grid_h, x, y, w, h, value);
    if (rc != GKINV_OK) {
        return rc;
    }
    yy = 0u;
    while (yy < h) {
        xx = 0u;
        while (xx < w) {
            offset = ((gkinv_u32)(y + yy) * (gkinv_u32)grid_w) + (gkinv_u32)(x + xx);
            occ[offset] = value;
            xx = (gkinv_u16)(xx + 1u);
        }
        yy = (gkinv_u16)(yy + 1u);
    }
    return GKINV_OK;
}

static int grid_build_occ(const gkinv_ItemDB *db,
                          const gkinv_Container *container,
                          gkinv_u16 skip_index,
                          gkinv_u8 *occ)
{
    gkinv_u16 i;
    const gkinv_ItemDef *def;
    gkinv_u16 w;
    gkinv_u16 h;
    int rc;

    rc = grid_check_size(container);
    if (rc != GKINV_OK) {
        return rc;
    }
    grid_clear_occ(occ);
    if ((container->flags & GKINV_CONT_GRID) == 0UL) {
        return GKINV_OK;
    }
    i = 0u;
    while (i < container->slot_capacity) {
        if (i != skip_index && !slot_is_empty(&container->slots[i])) {
            def = gkinv_db_find(db, container->slots[i].item_id);
            if (def == (const gkinv_ItemDef *)0) {
                return GKINV_ERR_BAD_ITEM;
            }
            w = gkinv_item_grid_w(def);
            h = gkinv_item_grid_h(def);
            rc = grid_mark_rect(occ, container->grid_w, container->grid_h,
                                container->slots[i].x, container->slots[i].y, w, h, 1u);
            if (rc != GKINV_OK) {
                return rc;
            }
        }
        i = (gkinv_u16)(i + 1u);
    }
    return GKINV_OK;
}

static int grid_rect_is_open(const gkinv_ItemDB *db,
                             const gkinv_Container *container,
                             gkinv_u16 skip_index,
                             gkinv_u16 x,
                             gkinv_u16 y,
                             gkinv_u16 w,
                             gkinv_u16 h)
{
    gkinv_u8 occ[GKINV_GRID_MAX_CELLS];
    int rc;

    rc = grid_build_occ(db, container, skip_index, occ);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = grid_mark_rect(occ, container->grid_w, container->grid_h, x, y, w, h, 1u);
    return rc;
}

static int grid_find_open_rect(const gkinv_ItemDB *db,
                               const gkinv_Container *container,
                               gkinv_u16 w,
                               gkinv_u16 h,
                               gkinv_u16 *out_x,
                               gkinv_u16 *out_y)
{
    gkinv_u8 occ[GKINV_GRID_MAX_CELLS];
    gkinv_u16 x;
    gkinv_u16 y;
    int rc;

    rc = grid_build_occ(db, container, (gkinv_u16)GKINV_NO_INDEX, occ);
    if (rc != GKINV_OK) {
        return rc;
    }
    y = 0u;
    while (y < container->grid_h) {
        x = 0u;
        while (x < container->grid_w) {
            rc = grid_mark_rect(occ, container->grid_w, container->grid_h, x, y, w, h, 1u);
            if (rc == GKINV_OK) {
                if (out_x != (gkinv_u16 *)0) {
                    *out_x = x;
                }
                if (out_y != (gkinv_u16 *)0) {
                    *out_y = y;
                }
                return GKINV_OK;
            }
            x = (gkinv_u16)(x + 1u);
        }
        y = (gkinv_u16)(y + 1u);
    }
    return GKINV_ERR_NO_ROOM;
}
#endif

static gkinv_bool container_can_simulate(const gkinv_Container *container)
{
    if (container == (const gkinv_Container *)0) {
        return GKINV_FALSE;
    }
    if (container->slot_capacity > (gkinv_u16)GKINV_SIM_MAX_SLOTS) {
        return GKINV_FALSE;
    }
    return GKINV_TRUE;
}

static void clone_container_with_slots(gkinv_Container *dst,
                                       const gkinv_Container *src,
                                       gkinv_Slot *slot_buffer)
{
    *dst = *src;
    dst->slots = slot_buffer;
    copy_slots(dst->slots, src->slots, src->slot_capacity);
    dst->hooks = (const gkinv_Hooks *)0;
    dst->hook_user = (void *)0;
}

static int add_stack_mutate(const gkinv_ItemDB *db,
                            gkinv_Container *container,
                            const gkinv_ItemStack *input_stack,
                            gkinv_u16 *out_index)
{
    const gkinv_ItemDef *def;
    gkinv_ItemStack stack;
    gkinv_u16 i;
    gkinv_u16 max_stack;
    gkinv_u16 room;
    gkinv_u16 add_qty;
    gkinv_u16 x;
    gkinv_u16 y;
    int rc;

    def = gkinv_db_find(db, input_stack->item_id);
    if (def == (const gkinv_ItemDef *)0) {
        return GKINV_ERR_BAD_ITEM;
    }
    stack = *input_stack;
    if (out_index != (gkinv_u16 *)0) {
        *out_index = (gkinv_u16)GKINV_NO_INDEX;
    }

    if ((container->flags & GKINV_CONT_STACKS) != 0UL && (def->flags & GKINV_ITEM_STACKABLE) != 0UL) {
        i = 0u;
        while (i < container->slot_capacity && stack.quantity > 0u) {
            if (!slot_is_empty(&container->slots[i]) && slots_can_merge(&container->slots[i], &stack)) {
                max_stack = max_stack_for_slot(container, def, i);
                if (container->slots[i].quantity < max_stack) {
                    room = (gkinv_u16)(max_stack - container->slots[i].quantity);
                    add_qty = (stack.quantity < room) ? stack.quantity : room;
                    container->slots[i].quantity = (gkinv_u16)(container->slots[i].quantity + add_qty);
                    stack.quantity = (gkinv_u16)(stack.quantity - add_qty);
                    if (out_index != (gkinv_u16 *)0 && *out_index == (gkinv_u16)GKINV_NO_INDEX) {
                        *out_index = i;
                    }
                }
            }
            i = (gkinv_u16)(i + 1u);
        }
    }

    while (stack.quantity > 0u) {
        i = 0u;
        while (i < container->slot_capacity && !slot_is_empty(&container->slots[i])) {
            i = (gkinv_u16)(i + 1u);
        }
        if (i >= container->slot_capacity) {
            return GKINV_ERR_FULL;
        }
        rc = slot_rule_accepts(container, def, i);
        if (rc != GKINV_OK) {
            i = (gkinv_u16)(i + 1u);
            while (i < container->slot_capacity) {
                if (slot_is_empty(&container->slots[i])) {
                    rc = slot_rule_accepts(container, def, i);
                    if (rc == GKINV_OK) {
                        break;
                    }
                }
                i = (gkinv_u16)(i + 1u);
            }
            if (i >= container->slot_capacity) {
                return rc;
            }
        }
        max_stack = max_stack_for_slot(container, def, i);
        add_qty = (stack.quantity < max_stack) ? stack.quantity : max_stack;
        if ((def->flags & GKINV_ITEM_STACKABLE) == 0UL) {
            add_qty = 1u;
        }
        if (add_qty == 0u) {
            return GKINV_ERR_STACK_LIMIT;
        }
        x = 0u;
        y = 0u;
#if GKINV_ENABLE_GRID
        if ((container->flags & GKINV_CONT_GRID) != 0UL) {
            rc = grid_find_open_rect(db, container, gkinv_item_grid_w(def), gkinv_item_grid_h(def), &x, &y);
            if (rc != GKINV_OK) {
                return rc;
            }
        }
#endif
        container->slots[i].item_id = stack.item_id;
        container->slots[i].quantity = add_qty;
        container->slots[i].flags = stack.flags;
        container->slots[i].state0 = stack.state0;
        container->slots[i].state1 = stack.state1;
        container->slots[i].state2 = stack.state2;
        container->slots[i].state3 = stack.state3;
        container->slots[i].x = x;
        container->slots[i].y = y;
        container->count = (gkinv_u16)(container->count + 1u);
        stack.quantity = (gkinv_u16)(stack.quantity - add_qty);
        if (out_index != (gkinv_u16 *)0 && *out_index == (gkinv_u16)GKINV_NO_INDEX) {
            *out_index = i;
        }
    }
    return GKINV_OK;
}

static int remove_from_slot_mutate(gkinv_Container *container, gkinv_u16 index, gkinv_u16 quantity)
{
    if (container == (gkinv_Container *)0 || container->slots == (gkinv_Slot *)0) {
        return GKINV_ERR_NULL;
    }
    if (index >= container->slot_capacity) {
        return GKINV_ERR_BAD_INDEX;
    }
    if (quantity == 0u) {
        return GKINV_ERR_BAD_QUANTITY;
    }
    if (slot_is_empty(&container->slots[index])) {
        return GKINV_ERR_NOT_FOUND;
    }
    if (container->slots[index].quantity < quantity) {
        return GKINV_ERR_BAD_QUANTITY;
    }
    container->slots[index].quantity = (gkinv_u16)(container->slots[index].quantity - quantity);
    if (container->slots[index].quantity == 0u) {
        gkinv_slot_clear(&container->slots[index]);
        container->count = count_slots(container);
    }
    return GKINV_OK;
}

static int remove_item_mutate(gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity)
{
    gkinv_u16 i;
    gkinv_u16 take;
    gkinv_u16 left;

    if (gkinv_count_item(container, item_id) < quantity) {
        return GKINV_ERR_NOT_FOUND;
    }
    left = quantity;
    i = container->slot_capacity;
    while (i > 0u && left > 0u) {
        i = (gkinv_u16)(i - 1u);
        if (!slot_is_empty(&container->slots[i]) && container->slots[i].item_id == item_id) {
            take = (container->slots[i].quantity < left) ? container->slots[i].quantity : left;
            container->slots[i].quantity = (gkinv_u16)(container->slots[i].quantity - take);
            left = (gkinv_u16)(left - take);
            if (container->slots[i].quantity == 0u) {
                gkinv_slot_clear(&container->slots[i]);
            }
        }
    }
    container->count = count_slots(container);
    return GKINV_OK;
}

static int insert_stack_at_mutate(const gkinv_ItemDB *db,
                                  gkinv_Container *container,
                                  const gkinv_ItemStack *stack,
                                  gkinv_u16 index,
                                  gkinv_u16 x,
                                  gkinv_u16 y,
                                  gkinv_bool use_grid_xy)
{
    const gkinv_ItemDef *def;
    gkinv_u16 max_stack;
    int rc;

    if (index >= container->slot_capacity) {
        return GKINV_ERR_BAD_INDEX;
    }
    if (!slot_is_empty(&container->slots[index])) {
        return GKINV_ERR_FULL;
    }
    def = gkinv_db_find(db, stack->item_id);
    if (def == (const gkinv_ItemDef *)0) {
        return GKINV_ERR_BAD_ITEM;
    }
    rc = slot_rule_accepts(container, def, index);
    if (rc != GKINV_OK) {
        return rc;
    }
    max_stack = max_stack_for_slot(container, def, index);
    if ((def->flags & GKINV_ITEM_STACKABLE) == 0UL && stack->quantity > 1u) {
        return GKINV_ERR_STACK_LIMIT;
    }
    if (stack->quantity > max_stack) {
        return GKINV_ERR_STACK_LIMIT;
    }
#if GKINV_ENABLE_GRID
    if ((container->flags & GKINV_CONT_GRID) != 0UL) {
        if (use_grid_xy == GKINV_FALSE) {
            rc = grid_find_open_rect(db, container, gkinv_item_grid_w(def), gkinv_item_grid_h(def), &x, &y);
        } else {
            rc = grid_rect_is_open(db, container, index, x, y, gkinv_item_grid_w(def), gkinv_item_grid_h(def));
        }
        if (rc != GKINV_OK) {
            return rc;
        }
    }
#else
    GKINV_UNUSED(x);
    GKINV_UNUSED(y);
    GKINV_UNUSED(use_grid_xy);
#endif
    slot_from_stack(&container->slots[index], stack);
    container->slots[index].x = x;
    container->slots[index].y = y;
    container->count = count_slots(container);
    return GKINV_OK;
}

static int call_remove_hook(const gkinv_ItemDB *db,
                            gkinv_Container *container,
                            gkinv_u16 index,
                            gkinv_u16 quantity)
{
#if GKINV_ENABLE_HOOKS
    const gkinv_ItemDef *def;
    int rc;

    if (container->hooks != (const gkinv_Hooks *)0 && container->hooks->can_remove != (gkinv_CanRemoveFn)0) {
        if (index >= container->slot_capacity || slot_is_empty(&container->slots[index])) {
            return GKINV_ERR_BAD_INDEX;
        }
        def = gkinv_db_find(db, container->slots[index].item_id);
        if (def == (const gkinv_ItemDef *)0) {
            return GKINV_ERR_BAD_ITEM;
        }
        rc = container->hooks->can_remove(container, db, def, index, quantity, container->hook_user);
        if (rc != GKINV_OK) {
            return rc;
        }
    }
#else
    GKINV_UNUSED(db);
    GKINV_UNUSED(container);
    GKINV_UNUSED(index);
    GKINV_UNUSED(quantity);
#endif
    return GKINV_OK;
}

void gkinv_slot_clear(gkinv_Slot *slot)
{
    if (slot == (gkinv_Slot *)0) {
        return;
    }
    slot->item_id = (gkinv_u16)GKINV_EMPTY_ITEM_ID;
    slot->quantity = 0u;
    slot->x = 0u;
    slot->y = 0u;
    slot->flags = 0UL;
    slot->state0 = 0L;
    slot->state1 = 0L;
    slot->state2 = 0L;
    slot->state3 = 0L;
}

void gkinv_stack_clear(gkinv_ItemStack *stack)
{
    if (stack == (gkinv_ItemStack *)0) {
        return;
    }
    stack->item_id = (gkinv_u16)GKINV_EMPTY_ITEM_ID;
    stack->quantity = 0u;
    stack->flags = 0UL;
    stack->state0 = 0L;
    stack->state1 = 0L;
    stack->state2 = 0L;
    stack->state3 = 0L;
}

int gkinv_stack_make(gkinv_ItemStack *stack, gkinv_u16 item_id, gkinv_u16 quantity)
{
    if (stack == (gkinv_ItemStack *)0) {
        return GKINV_ERR_NULL;
    }
    gkinv_stack_clear(stack);
    if (item_id == (gkinv_u16)GKINV_EMPTY_ITEM_ID) {
        return GKINV_ERR_BAD_ITEM;
    }
    if (quantity == 0u) {
        return GKINV_ERR_BAD_QUANTITY;
    }
    stack->item_id = item_id;
    stack->quantity = quantity;
    return GKINV_OK;
}

void gkinv_container_init(gkinv_Container *container,
                          gkinv_Slot *slots,
                          gkinv_u16 slot_capacity,
                          gkinv_u32 flags)
{
    if (container == (gkinv_Container *)0) {
        return;
    }
    container->slots = slots;
    container->slot_capacity = slot_capacity;
    container->count = 0u;
    container->grid_w = 0u;
    container->grid_h = 0u;
    container->flags = flags;
    container->allowed_categories = GKINV_CAT_ANY;
    container->max_weight = GKINV_FX_ZERO;
    container->slot_rules = (const gkinv_SlotRule *)0;
    container->slot_rule_count = 0u;
    container->hooks = (const gkinv_Hooks *)0;
    container->hook_user = (void *)0;
    container->user = (void *)0;
    gkinv_container_clear(container);
}

void gkinv_container_clear(gkinv_Container *container)
{
    gkinv_u16 i;

    if (container == (gkinv_Container *)0 || container->slots == (gkinv_Slot *)0) {
        return;
    }
    i = 0u;
    while (i < container->slot_capacity) {
        gkinv_slot_clear(&container->slots[i]);
        i = (gkinv_u16)(i + 1u);
    }
    container->count = 0u;
    emit_event(container, container, GKINV_EVENT_CLEARED, GKINV_OK, 0u, 0u,
               (gkinv_u16)GKINV_NO_INDEX, (gkinv_u16)GKINV_NO_INDEX);
}

void gkinv_container_set_grid(gkinv_Container *container, gkinv_u16 grid_w, gkinv_u16 grid_h)
{
    if (container == (gkinv_Container *)0) {
        return;
    }
    container->grid_w = grid_w;
    container->grid_h = grid_h;
    if (grid_w != 0u && grid_h != 0u) {
        container->flags |= GKINV_CONT_GRID;
    } else {
        container->flags &= ~GKINV_CONT_GRID;
    }
}

void gkinv_container_set_limits(gkinv_Container *container,
                                gkinv_u32 allowed_categories,
                                gkinv_fx max_weight)
{
    if (container == (gkinv_Container *)0) {
        return;
    }
    container->allowed_categories = allowed_categories;
    container->max_weight = max_weight;
}

void gkinv_container_set_slot_rules(gkinv_Container *container,
                                    const gkinv_SlotRule *rules,
                                    gkinv_u16 rule_count)
{
    if (container == (gkinv_Container *)0) {
        return;
    }
    container->slot_rules = rules;
    container->slot_rule_count = rule_count;
}

void gkinv_container_set_hooks(gkinv_Container *container,
                               const gkinv_Hooks *hooks,
                               void *hook_user)
{
    if (container == (gkinv_Container *)0) {
        return;
    }
    container->hooks = hooks;
    container->hook_user = hook_user;
}

const gkinv_ItemDef *gkinv_db_find(const gkinv_ItemDB *db, gkinv_u16 item_id)
{
    gkinv_u16 i;

    if (db == (const gkinv_ItemDB *)0 || db->items == (const gkinv_ItemDef *)0) {
        return (const gkinv_ItemDef *)0;
    }
    if (item_id == (gkinv_u16)GKINV_EMPTY_ITEM_ID) {
        return (const gkinv_ItemDef *)0;
    }
    i = 0u;
    while (i < db->count) {
        if (db->items[i].id == item_id) {
            return &db->items[i];
        }
        i = (gkinv_u16)(i + 1u);
    }
    return (const gkinv_ItemDef *)0;
}

gkinv_u16 gkinv_item_max_stack(const gkinv_ItemDef *def)
{
    if (def == (const gkinv_ItemDef *)0) {
        return 1u;
    }
    if ((def->flags & GKINV_ITEM_STACKABLE) == 0UL) {
        return 1u;
    }
    if (def->max_stack == 0u) {
        return 1u;
    }
    return def->max_stack;
}

gkinv_u16 gkinv_item_grid_w(const gkinv_ItemDef *def)
{
    if (def == (const gkinv_ItemDef *)0 || def->grid_w == 0u) {
        return 1u;
    }
    return def->grid_w;
}

gkinv_u16 gkinv_item_grid_h(const gkinv_ItemDef *def)
{
    if (def == (const gkinv_ItemDef *)0 || def->grid_h == 0u) {
        return 1u;
    }
    return def->grid_h;
}

gkinv_fx gkinv_total_weight(const gkinv_ItemDB *db, const gkinv_Container *container)
{
    gkinv_fx total;
    gkinv_u16 i;
    const gkinv_ItemDef *def;

    total = GKINV_FX_ZERO;
    if (db == (const gkinv_ItemDB *)0 || container == (const gkinv_Container *)0 || container->slots == (gkinv_Slot *)0) {
        return total;
    }
    i = 0u;
    while (i < container->slot_capacity) {
        if (!slot_is_empty(&container->slots[i])) {
            def = gkinv_db_find(db, container->slots[i].item_id);
            if (def != (const gkinv_ItemDef *)0) {
                total = gkinv_fx_add_sat(total, stack_weight(def, container->slots[i].quantity));
            }
        }
        i = (gkinv_u16)(i + 1u);
    }
    return total;
}

int gkinv_validate_container(const gkinv_ItemDB *db, const gkinv_Container *container)
{
    gkinv_u16 i;
    gkinv_u16 count;
    const gkinv_ItemDef *def;
    int rc;
#if GKINV_ENABLE_GRID
    gkinv_u8 occ[GKINV_GRID_MAX_CELLS];
#endif

    if (db == (const gkinv_ItemDB *)0 || container == (const gkinv_Container *)0 || container->slots == (gkinv_Slot *)0) {
        return GKINV_ERR_NULL;
    }
    if (container->slot_capacity > (gkinv_u16)GKINV_SIM_MAX_SLOTS) {
        return GKINV_ERR_TEMP_LIMIT;
    }
#if GKINV_ENABLE_GRID
    if ((container->flags & GKINV_CONT_GRID) != 0UL) {
        rc = grid_check_size(container);
        if (rc != GKINV_OK) {
            return rc;
        }
        grid_clear_occ(occ);
    }
#endif
    count = 0u;
    i = 0u;
    while (i < container->slot_capacity) {
        if (!slot_is_empty(&container->slots[i])) {
            count = (gkinv_u16)(count + 1u);
            def = gkinv_db_find(db, container->slots[i].item_id);
            if (def == (const gkinv_ItemDef *)0) {
                return GKINV_ERR_BAD_ITEM;
            }
            if (container->slots[i].quantity == 0u || container->slots[i].quantity > max_stack_for_slot(container, def, i)) {
                return GKINV_ERR_STACK_LIMIT;
            }
            rc = slot_rule_accepts(container, def, i);
            if (rc != GKINV_OK) {
                return rc;
            }
#if GKINV_ENABLE_GRID
            if ((container->flags & GKINV_CONT_GRID) != 0UL) {
                rc = grid_mark_rect(occ, container->grid_w, container->grid_h,
                                    container->slots[i].x, container->slots[i].y,
                                    gkinv_item_grid_w(def), gkinv_item_grid_h(def), 1u);
                if (rc != GKINV_OK) {
                    return rc;
                }
            }
#endif
        }
        i = (gkinv_u16)(i + 1u);
    }
    if (count != container->count) {
        return GKINV_ERR_RULE;
    }
    if (container->max_weight > GKINV_FX_ZERO && gkinv_total_weight(db, container) > container->max_weight) {
        return GKINV_ERR_WEIGHT;
    }
    return GKINV_OK;
}

int gkinv_has_item(const gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity)
{
    return (gkinv_count_item(container, item_id) >= quantity) ? GKINV_TRUE : GKINV_FALSE;
}

gkinv_u16 gkinv_count_item(const gkinv_Container *container, gkinv_u16 item_id)
{
    gkinv_u16 i;
    gkinv_u16 total;

    total = 0u;
    if (container == (const gkinv_Container *)0 || container->slots == (gkinv_Slot *)0) {
        return 0u;
    }
    i = 0u;
    while (i < container->slot_capacity) {
        if (!slot_is_empty(&container->slots[i]) && container->slots[i].item_id == item_id) {
            total = (gkinv_u16)(total + container->slots[i].quantity);
        }
        i = (gkinv_u16)(i + 1u);
    }
    return total;
}

int gkinv_find_item(const gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 *out_index)
{
    gkinv_u16 i;

    if (out_index != (gkinv_u16 *)0) {
        *out_index = (gkinv_u16)GKINV_NO_INDEX;
    }
    if (container == (const gkinv_Container *)0 || container->slots == (gkinv_Slot *)0) {
        return GKINV_ERR_NULL;
    }
    i = 0u;
    while (i < container->slot_capacity) {
        if (!slot_is_empty(&container->slots[i]) && container->slots[i].item_id == item_id) {
            if (out_index != (gkinv_u16 *)0) {
                *out_index = i;
            }
            return GKINV_OK;
        }
        i = (gkinv_u16)(i + 1u);
    }
    return GKINV_ERR_NOT_FOUND;
}

int gkinv_can_add_stack(const gkinv_ItemDB *db, const gkinv_Container *container, const gkinv_ItemStack *stack)
{
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_Container *mutable_container;
    int rc;

    if (container == (const gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    clone_container_with_slots(&temp_container, container, temp_slots);
    mutable_container = &temp_container;
    rc = check_global_accept(db, mutable_container, stack, (gkinv_u16)GKINV_NO_INDEX, GKINV_FALSE);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = add_stack_mutate(db, mutable_container, stack, (gkinv_u16 *)0);
    return rc;
}

int gkinv_add_item(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity, gkinv_u16 *out_index)
{
    gkinv_ItemStack stack;
    int rc;

    rc = gkinv_stack_make(&stack, item_id, quantity);
    if (rc != GKINV_OK) {
        return rc;
    }
    return gkinv_add_stack(db, container, &stack, out_index);
}

int gkinv_add_stack(const gkinv_ItemDB *db, gkinv_Container *container, const gkinv_ItemStack *stack, gkinv_u16 *out_index)
{
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_u16 index;
    int rc;

    if (out_index != (gkinv_u16 *)0) {
        *out_index = (gkinv_u16)GKINV_NO_INDEX;
    }
    if (container == (gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    rc = check_global_accept(db, container, stack, (gkinv_u16)GKINV_NO_INDEX, GKINV_TRUE);
    if (rc != GKINV_OK) {
        return rc;
    }
    clone_container_with_slots(&temp_container, container, temp_slots);
    rc = add_stack_mutate(db, &temp_container, stack, &index);
    if (rc != GKINV_OK) {
        return rc;
    }
    copy_slots(container->slots, temp_container.slots, container->slot_capacity);
    container->count = temp_container.count;
    if (out_index != (gkinv_u16 *)0) {
        *out_index = index;
    }
    emit_event(container, container, GKINV_EVENT_ADDED, GKINV_OK, stack->item_id, stack->quantity,
               (gkinv_u16)GKINV_NO_INDEX, index);
    return GKINV_OK;
}

int gkinv_insert_stack_at(const gkinv_ItemDB *db, gkinv_Container *container, const gkinv_ItemStack *stack, gkinv_u16 index)
{
    return gkinv_insert_stack_grid_at(db, container, stack, index, 0u, 0u);
}

int gkinv_insert_stack_grid_at(const gkinv_ItemDB *db,
                               gkinv_Container *container,
                               const gkinv_ItemStack *stack,
                               gkinv_u16 index,
                               gkinv_u16 x,
                               gkinv_u16 y)
{
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    int rc;

    if (container == (gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    rc = check_global_accept(db, container, stack, index, GKINV_TRUE);
    if (rc != GKINV_OK) {
        return rc;
    }
    clone_container_with_slots(&temp_container, container, temp_slots);
    rc = insert_stack_at_mutate(db, &temp_container, stack, index, x, y, GKINV_TRUE);
    if (rc != GKINV_OK) {
        return rc;
    }
    copy_slots(container->slots, temp_container.slots, container->slot_capacity);
    container->count = temp_container.count;
    emit_event(container, container, GKINV_EVENT_ADDED, GKINV_OK, stack->item_id, stack->quantity,
               (gkinv_u16)GKINV_NO_INDEX, index);
    return GKINV_OK;
}

int gkinv_remove_item(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity)
{
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_u16 i;
    gkinv_u16 left;
    gkinv_u16 take;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || container == (gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if ((container->flags & GKINV_CONT_READ_ONLY) != 0UL) {
        return GKINV_ERR_READ_ONLY;
    }
    if (quantity == 0u) {
        return GKINV_ERR_BAD_QUANTITY;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    if (gkinv_count_item(container, item_id) < quantity) {
        return GKINV_ERR_NOT_FOUND;
    }
    left = quantity;
    i = 0u;
    while (i < container->slot_capacity && left > 0u) {
        if (!slot_is_empty(&container->slots[i]) && container->slots[i].item_id == item_id) {
            take = (container->slots[i].quantity < left) ? container->slots[i].quantity : left;
            rc = call_remove_hook(db, container, i, take);
            if (rc != GKINV_OK) {
                return rc;
            }
            left = (gkinv_u16)(left - take);
        }
        i = (gkinv_u16)(i + 1u);
    }
    clone_container_with_slots(&temp_container, container, temp_slots);
    rc = remove_item_mutate(&temp_container, item_id, quantity);
    if (rc != GKINV_OK) {
        return rc;
    }
    copy_slots(container->slots, temp_container.slots, container->slot_capacity);
    container->count = temp_container.count;
    emit_event(container, container, GKINV_EVENT_REMOVED, GKINV_OK, item_id, quantity,
               (gkinv_u16)GKINV_NO_INDEX, (gkinv_u16)GKINV_NO_INDEX);
    return GKINV_OK;
}

int gkinv_remove_from_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index, gkinv_u16 quantity)
{
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_u16 item_id;
    int rc;

    if (container == (gkinv_Container *)0 || db == (const gkinv_ItemDB *)0) {
        return GKINV_ERR_NULL;
    }
    if ((container->flags & GKINV_CONT_READ_ONLY) != 0UL) {
        return GKINV_ERR_READ_ONLY;
    }
    if (index >= container->slot_capacity) {
        return GKINV_ERR_BAD_INDEX;
    }
    if (slot_is_empty(&container->slots[index])) {
        return GKINV_ERR_NOT_FOUND;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    rc = call_remove_hook(db, container, index, quantity);
    if (rc != GKINV_OK) {
        return rc;
    }
    item_id = container->slots[index].item_id;
    clone_container_with_slots(&temp_container, container, temp_slots);
    rc = remove_from_slot_mutate(&temp_container, index, quantity);
    if (rc != GKINV_OK) {
        return rc;
    }
    copy_slots(container->slots, temp_container.slots, container->slot_capacity);
    container->count = temp_container.count;
    emit_event(container, container, GKINV_EVENT_REMOVED, GKINV_OK, item_id, quantity,
               index, (gkinv_u16)GKINV_NO_INDEX);
    return GKINV_OK;
}

int gkinv_move_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 from_index, gkinv_u16 to_index)
{
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_Slot moving;
    gkinv_ItemStack moving_stack;
    gkinv_ItemStack dest_stack;
    const gkinv_ItemDef *def;
    const gkinv_ItemDef *dest_def;
    gkinv_u16 max_stack;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || container == (gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if ((container->flags & GKINV_CONT_READ_ONLY) != 0UL) {
        return GKINV_ERR_READ_ONLY;
    }
    if (from_index >= container->slot_capacity || to_index >= container->slot_capacity) {
        return GKINV_ERR_BAD_INDEX;
    }
    if (from_index == to_index) {
        return GKINV_OK;
    }
    if (slot_is_empty(&container->slots[from_index])) {
        return GKINV_ERR_NOT_FOUND;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    clone_container_with_slots(&temp_container, container, temp_slots);
    moving = temp_container.slots[from_index];
    def = gkinv_db_find(db, moving.item_id);
    if (def == (const gkinv_ItemDef *)0) {
        return GKINV_ERR_BAD_ITEM;
    }
    if (slot_is_empty(&temp_container.slots[to_index])) {
        rc = slot_rule_accepts(&temp_container, def, to_index);
        if (rc != GKINV_OK) {
            return rc;
        }
        temp_container.slots[to_index] = moving;
        gkinv_slot_clear(&temp_container.slots[from_index]);
    } else {
        stack_from_slot(&moving_stack, &moving, moving.quantity);
        if (slots_can_merge(&temp_container.slots[to_index], &moving_stack) && (def->flags & GKINV_ITEM_STACKABLE) != 0UL) {
            max_stack = max_stack_for_slot(&temp_container, def, to_index);
            if ((gkinv_u32)temp_container.slots[to_index].quantity + (gkinv_u32)moving.quantity > (gkinv_u32)max_stack) {
                return GKINV_ERR_STACK_LIMIT;
            }
            temp_container.slots[to_index].quantity = (gkinv_u16)(temp_container.slots[to_index].quantity + moving.quantity);
            gkinv_slot_clear(&temp_container.slots[from_index]);
        } else {
            if ((temp_container.flags & GKINV_CONT_ALLOW_SWAP) == 0UL) {
                return GKINV_ERR_FULL;
            }
            dest_def = gkinv_db_find(db, temp_container.slots[to_index].item_id);
            if (dest_def == (const gkinv_ItemDef *)0) {
                return GKINV_ERR_BAD_ITEM;
            }
            rc = slot_rule_accepts(&temp_container, def, to_index);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = slot_rule_accepts(&temp_container, dest_def, from_index);
            if (rc != GKINV_OK) {
                return rc;
            }
            dest_stack.item_id = temp_container.slots[to_index].item_id;
            dest_stack.quantity = temp_container.slots[to_index].quantity;
            dest_stack.flags = temp_container.slots[to_index].flags;
            dest_stack.state0 = temp_container.slots[to_index].state0;
            dest_stack.state1 = temp_container.slots[to_index].state1;
            dest_stack.state2 = temp_container.slots[to_index].state2;
            dest_stack.state3 = temp_container.slots[to_index].state3;
            GKINV_UNUSED(dest_stack);
            temp_container.slots[from_index] = temp_container.slots[to_index];
            temp_container.slots[to_index] = moving;
        }
    }
    temp_container.count = count_slots(&temp_container);
    copy_slots(container->slots, temp_container.slots, container->slot_capacity);
    container->count = temp_container.count;
    emit_event(container, container, GKINV_EVENT_MOVED, GKINV_OK, moving.item_id, moving.quantity,
               from_index, to_index);
    return GKINV_OK;
}

int gkinv_split_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 from_index, gkinv_u16 to_index, gkinv_u16 quantity)
{
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_ItemStack stack;
    const gkinv_ItemDef *def;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || container == (gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if ((container->flags & GKINV_CONT_READ_ONLY) != 0UL) {
        return GKINV_ERR_READ_ONLY;
    }
    if (from_index >= container->slot_capacity || to_index >= container->slot_capacity) {
        return GKINV_ERR_BAD_INDEX;
    }
    if (slot_is_empty(&container->slots[from_index]) || !slot_is_empty(&container->slots[to_index])) {
        return GKINV_ERR_FULL;
    }
    if (quantity == 0u || quantity >= container->slots[from_index].quantity) {
        return GKINV_ERR_BAD_QUANTITY;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    def = gkinv_db_find(db, container->slots[from_index].item_id);
    if (def == (const gkinv_ItemDef *)0) {
        return GKINV_ERR_BAD_ITEM;
    }
    if ((def->flags & GKINV_ITEM_STACKABLE) == 0UL) {
        return GKINV_ERR_STACK_LIMIT;
    }
    rc = slot_rule_accepts(container, def, to_index);
    if (rc != GKINV_OK) {
        return rc;
    }
    stack_from_slot(&stack, &container->slots[from_index], quantity);
    clone_container_with_slots(&temp_container, container, temp_slots);
    temp_container.slots[from_index].quantity = (gkinv_u16)(temp_container.slots[from_index].quantity - quantity);
    rc = insert_stack_at_mutate(db, &temp_container, &stack, to_index, 0u, 0u, GKINV_FALSE);
    if (rc != GKINV_OK) {
        return rc;
    }
    copy_slots(container->slots, temp_container.slots, container->slot_capacity);
    container->count = temp_container.count;
    emit_event(container, container, GKINV_EVENT_MOVED, GKINV_OK, stack.item_id, quantity,
               from_index, to_index);
    return GKINV_OK;
}

int gkinv_move_grid_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index, gkinv_u16 x, gkinv_u16 y)
{
#if GKINV_ENABLE_GRID
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    const gkinv_ItemDef *def;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || container == (gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if ((container->flags & GKINV_CONT_READ_ONLY) != 0UL) {
        return GKINV_ERR_READ_ONLY;
    }
    if ((container->flags & GKINV_CONT_GRID) == 0UL) {
        return GKINV_ERR_GRID;
    }
    if (index >= container->slot_capacity || slot_is_empty(&container->slots[index])) {
        return GKINV_ERR_BAD_INDEX;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    def = gkinv_db_find(db, container->slots[index].item_id);
    if (def == (const gkinv_ItemDef *)0) {
        return GKINV_ERR_BAD_ITEM;
    }
    clone_container_with_slots(&temp_container, container, temp_slots);
    rc = grid_rect_is_open(db, &temp_container, index, x, y, gkinv_item_grid_w(def), gkinv_item_grid_h(def));
    if (rc != GKINV_OK) {
        return rc;
    }
    temp_container.slots[index].x = x;
    temp_container.slots[index].y = y;
    copy_slots(container->slots, temp_container.slots, container->slot_capacity);
    emit_event(container, container, GKINV_EVENT_MOVED, GKINV_OK, container->slots[index].item_id,
               container->slots[index].quantity, index, index);
    return GKINV_OK;
#else
    GKINV_UNUSED(db);
    GKINV_UNUSED(container);
    GKINV_UNUSED(index);
    GKINV_UNUSED(x);
    GKINV_UNUSED(y);
    return GKINV_ERR_GRID;
#endif
}

int gkinv_transfer_item(const gkinv_ItemDB *db, gkinv_Container *src, gkinv_Container *dst, gkinv_u16 item_id, gkinv_u16 quantity)
{
    gkinv_Container temp_src;
    gkinv_Container temp_dst;
    gkinv_Slot temp_src_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_Slot temp_dst_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_ItemStack stack;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || src == (gkinv_Container *)0 || dst == (gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if ((src->flags & GKINV_CONT_READ_ONLY) != 0UL || (dst->flags & GKINV_CONT_READ_ONLY) != 0UL) {
        return GKINV_ERR_READ_ONLY;
    }
    if (quantity == 0u) {
        return GKINV_ERR_BAD_QUANTITY;
    }
    if (!container_can_simulate(src) || !container_can_simulate(dst)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    if (gkinv_count_item(src, item_id) < quantity) {
        return GKINV_ERR_NOT_FOUND;
    }
    rc = gkinv_stack_make(&stack, item_id, quantity);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = check_global_accept(db, dst, &stack, (gkinv_u16)GKINV_NO_INDEX, GKINV_TRUE);
    if (rc != GKINV_OK) {
        return rc;
    }
    clone_container_with_slots(&temp_src, src, temp_src_slots);
    clone_container_with_slots(&temp_dst, dst, temp_dst_slots);
    rc = remove_item_mutate(&temp_src, item_id, quantity);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = add_stack_mutate(db, &temp_dst, &stack, (gkinv_u16 *)0);
    if (rc != GKINV_OK) {
        return rc;
    }
    copy_slots(src->slots, temp_src.slots, src->slot_capacity);
    copy_slots(dst->slots, temp_dst.slots, dst->slot_capacity);
    src->count = temp_src.count;
    dst->count = temp_dst.count;
    emit_event(src, dst, GKINV_EVENT_TRANSFERRED, GKINV_OK, item_id, quantity,
               (gkinv_u16)GKINV_NO_INDEX, (gkinv_u16)GKINV_NO_INDEX);
    return GKINV_OK;
}

int gkinv_transfer_slot(const gkinv_ItemDB *db, gkinv_Container *src, gkinv_Container *dst, gkinv_u16 src_index, gkinv_u16 quantity)
{
    gkinv_Container temp_src;
    gkinv_Container temp_dst;
    gkinv_Slot temp_src_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_Slot temp_dst_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_ItemStack stack;
    gkinv_u16 item_id;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || src == (gkinv_Container *)0 || dst == (gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if ((src->flags & GKINV_CONT_READ_ONLY) != 0UL || (dst->flags & GKINV_CONT_READ_ONLY) != 0UL) {
        return GKINV_ERR_READ_ONLY;
    }
    if (src_index >= src->slot_capacity || slot_is_empty(&src->slots[src_index])) {
        return GKINV_ERR_BAD_INDEX;
    }
    if (quantity == 0u || quantity > src->slots[src_index].quantity) {
        return GKINV_ERR_BAD_QUANTITY;
    }
    if (!container_can_simulate(src) || !container_can_simulate(dst)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    item_id = src->slots[src_index].item_id;
    stack_from_slot(&stack, &src->slots[src_index], quantity);
    rc = call_remove_hook(db, src, src_index, quantity);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = check_global_accept(db, dst, &stack, (gkinv_u16)GKINV_NO_INDEX, GKINV_TRUE);
    if (rc != GKINV_OK) {
        return rc;
    }
    clone_container_with_slots(&temp_src, src, temp_src_slots);
    clone_container_with_slots(&temp_dst, dst, temp_dst_slots);
    rc = remove_from_slot_mutate(&temp_src, src_index, quantity);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = add_stack_mutate(db, &temp_dst, &stack, (gkinv_u16 *)0);
    if (rc != GKINV_OK) {
        return rc;
    }
    copy_slots(src->slots, temp_src.slots, src->slot_capacity);
    copy_slots(dst->slots, temp_dst.slots, dst->slot_capacity);
    src->count = temp_src.count;
    dst->count = temp_dst.count;
    emit_event(src, dst, GKINV_EVENT_TRANSFERRED, GKINV_OK, item_id, quantity,
               src_index, (gkinv_u16)GKINV_NO_INDEX);
    return GKINV_OK;
}

int gkinv_use_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index, void *actor, void *target)
{
    const gkinv_ItemDef *def;
    gkinv_u16 item_id;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || container == (gkinv_Container *)0) {
        return GKINV_ERR_NULL;
    }
    if (index >= container->slot_capacity || slot_is_empty(&container->slots[index])) {
        return GKINV_ERR_BAD_INDEX;
    }
    def = gkinv_db_find(db, container->slots[index].item_id);
    if (def == (const gkinv_ItemDef *)0) {
        return GKINV_ERR_BAD_ITEM;
    }
    if ((def->flags & GKINV_ITEM_USABLE) == 0UL && (def->flags & GKINV_ITEM_CONSUMABLE) == 0UL) {
        return GKINV_ERR_RULE;
    }
#if GKINV_ENABLE_HOOKS
    if (container->hooks == (const gkinv_Hooks *)0 || container->hooks->on_use == (gkinv_OnUseFn)0) {
        return GKINV_ERR_NO_HOOK;
    }
    rc = container->hooks->on_use(container, db, index, actor, target, container->hook_user);
    if (rc != GKINV_OK) {
        return rc;
    }
#else
    GKINV_UNUSED(actor);
    GKINV_UNUSED(target);
    return GKINV_ERR_NO_HOOK;
#endif
    item_id = container->slots[index].item_id;
    if ((def->flags & GKINV_ITEM_CONSUMABLE) != 0UL) {
        rc = gkinv_remove_from_slot(db, container, index, 1u);
        if (rc != GKINV_OK) {
            return rc;
        }
    }
    emit_event(container, container, GKINV_EVENT_USED, GKINV_OK, item_id, 1u, index, index);
    return GKINV_OK;
}

int gkinv_combine_slots(const gkinv_ItemDB *db,
                        gkinv_Container *container,
                        gkinv_u16 index_a,
                        gkinv_u16 index_b,
                        const gkinv_Recipe *recipes,
                        gkinv_u16 recipe_count)
{
#if GKINV_ENABLE_RECIPES
    gkinv_u16 i;
    const gkinv_Recipe *recipe;
    gkinv_bool swapped;
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    gkinv_ItemStack result_stack;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || container == (gkinv_Container *)0 || recipes == (const gkinv_Recipe *)0) {
        return GKINV_ERR_NULL;
    }
    if ((container->flags & GKINV_CONT_READ_ONLY) != 0UL) {
        return GKINV_ERR_READ_ONLY;
    }
    if (index_a >= container->slot_capacity || index_b >= container->slot_capacity || index_a == index_b) {
        return GKINV_ERR_BAD_INDEX;
    }
    if (slot_is_empty(&container->slots[index_a]) || slot_is_empty(&container->slots[index_b])) {
        return GKINV_ERR_NOT_FOUND;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }

    recipe = (const gkinv_Recipe *)0;
    swapped = GKINV_FALSE;
    i = 0u;
    while (i < recipe_count) {
        if (recipes[i].item_a == container->slots[index_a].item_id && recipes[i].item_b == container->slots[index_b].item_id) {
            recipe = &recipes[i];
            swapped = GKINV_FALSE;
            break;
        }
        if ((recipes[i].flags & GKINV_RECIPE_ORDERED) == 0UL) {
            if (recipes[i].item_a == container->slots[index_b].item_id && recipes[i].item_b == container->slots[index_a].item_id) {
                recipe = &recipes[i];
                swapped = GKINV_TRUE;
                break;
            }
        }
        i = (gkinv_u16)(i + 1u);
    }
    if (recipe == (const gkinv_Recipe *)0) {
        return GKINV_ERR_NOT_FOUND;
    }
    if (swapped == GKINV_FALSE) {
        if (container->slots[index_a].quantity < recipe->qty_a || container->slots[index_b].quantity < recipe->qty_b) {
            return GKINV_ERR_BAD_QUANTITY;
        }
    } else {
        if (container->slots[index_a].quantity < recipe->qty_b || container->slots[index_b].quantity < recipe->qty_a) {
            return GKINV_ERR_BAD_QUANTITY;
        }
    }
    rc = gkinv_stack_make(&result_stack, recipe->result_item, recipe->result_qty);
    if (rc != GKINV_OK) {
        return rc;
    }
    if ((recipe->flags & GKINV_RECIPE_KEEP_A_STATE) != 0UL) {
        result_stack.flags = container->slots[index_a].flags;
        result_stack.state0 = container->slots[index_a].state0;
        result_stack.state1 = container->slots[index_a].state1;
        result_stack.state2 = container->slots[index_a].state2;
        result_stack.state3 = container->slots[index_a].state3;
    } else if ((recipe->flags & GKINV_RECIPE_KEEP_B_STATE) != 0UL) {
        result_stack.flags = container->slots[index_b].flags;
        result_stack.state0 = container->slots[index_b].state0;
        result_stack.state1 = container->slots[index_b].state1;
        result_stack.state2 = container->slots[index_b].state2;
        result_stack.state3 = container->slots[index_b].state3;
    }
    rc = check_global_accept(db, container, &result_stack, (gkinv_u16)GKINV_NO_INDEX, GKINV_TRUE);
    if (rc != GKINV_OK && rc != GKINV_ERR_WEIGHT) {
        return rc;
    }
    clone_container_with_slots(&temp_container, container, temp_slots);
    if (swapped == GKINV_FALSE) {
        rc = remove_from_slot_mutate(&temp_container, index_a, recipe->qty_a);
        if (rc == GKINV_OK) {
            rc = remove_from_slot_mutate(&temp_container, index_b, recipe->qty_b);
        }
    } else {
        rc = remove_from_slot_mutate(&temp_container, index_a, recipe->qty_b);
        if (rc == GKINV_OK) {
            rc = remove_from_slot_mutate(&temp_container, index_b, recipe->qty_a);
        }
    }
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = add_stack_mutate(db, &temp_container, &result_stack, (gkinv_u16 *)0);
    if (rc != GKINV_OK) {
        return rc;
    }
    if (container->max_weight > GKINV_FX_ZERO && gkinv_total_weight(db, &temp_container) > container->max_weight) {
        return GKINV_ERR_WEIGHT;
    }
    copy_slots(container->slots, temp_container.slots, container->slot_capacity);
    container->count = temp_container.count;
    emit_event(container, container, GKINV_EVENT_COMBINED, GKINV_OK, result_stack.item_id, result_stack.quantity,
               index_a, index_b);
    return GKINV_OK;
#else
    GKINV_UNUSED(db);
    GKINV_UNUSED(container);
    GKINV_UNUSED(index_a);
    GKINV_UNUSED(index_b);
    GKINV_UNUSED(recipes);
    GKINV_UNUSED(recipe_count);
    return GKINV_ERR_RULE;
#endif
}

#if GKINV_ENABLE_TEXT_IO
static int writer_puts(gkinv_TextWriter writer, void *user, const char *s)
{
    const char *p;
    gkinv_u16 len;

    if (writer == (gkinv_TextWriter)0 || s == (const char *)0) {
        return GKINV_ERR_NULL;
    }
    p = s;
    len = 0u;
    while (*p != '\0') {
        len = (gkinv_u16)(len + 1u);
        p++;
    }
    return writer(user, s, len);
}

static int writer_put_u32(gkinv_TextWriter writer, void *user, gkinv_u32 value)
{
    char buf[16];
    char out[16];
    gkinv_u16 i;
    gkinv_u16 j;

    i = 0u;
    if (value == 0UL) {
        buf[i] = '0';
        i = (gkinv_u16)(i + 1u);
    } else {
        while (value > 0UL && i < 15u) {
            buf[i] = (char)('0' + (char)(value % 10UL));
            value = value / 10UL;
            i = (gkinv_u16)(i + 1u);
        }
    }
    j = 0u;
    while (i > 0u) {
        i = (gkinv_u16)(i - 1u);
        out[j] = buf[i];
        j = (gkinv_u16)(j + 1u);
    }
    out[j] = '\0';
    return writer_puts(writer, user, out);
}

static int writer_put_i32(gkinv_TextWriter writer, void *user, gkinv_i32 value)
{
    gkinv_u32 u;
    int rc;

    if (value < 0L) {
        rc = writer_puts(writer, user, "-");
        if (rc != GKINV_OK) {
            return rc;
        }
        u = (gkinv_u32)(0UL - (gkinv_u32)value);
    } else {
        u = (gkinv_u32)value;
    }
    return writer_put_u32(writer, user, u);
}

static int save_space(gkinv_TextWriter writer, void *user)
{
    return writer_puts(writer, user, " ");
}

static int save_newline(gkinv_TextWriter writer, void *user)
{
    return writer_puts(writer, user, "\n");
}

int gkinv_save_text(const gkinv_Container *container, gkinv_TextWriter writer, void *user)
{
    gkinv_u16 i;
    int rc;

    if (container == (const gkinv_Container *)0 || writer == (gkinv_TextWriter)0) {
        return GKINV_ERR_NULL;
    }
    rc = writer_puts(writer, user, "GKINV1 ");
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = writer_put_u32(writer, user, (gkinv_u32)container->slot_capacity);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = save_space(writer, user);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = writer_put_u32(writer, user, (gkinv_u32)container->flags);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = save_space(writer, user);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = writer_put_u32(writer, user, (gkinv_u32)container->grid_w);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = save_space(writer, user);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = writer_put_u32(writer, user, (gkinv_u32)container->grid_h);
    if (rc != GKINV_OK) {
        return rc;
    }
    rc = save_newline(writer, user);
    if (rc != GKINV_OK) {
        return rc;
    }

    i = 0u;
    while (i < container->slot_capacity) {
        if (!slot_is_empty(&container->slots[i])) {
            rc = writer_puts(writer, user, "S ");
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_u32(writer, user, (gkinv_u32)i);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_space(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_u32(writer, user, (gkinv_u32)container->slots[i].item_id);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_space(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_u32(writer, user, (gkinv_u32)container->slots[i].quantity);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_space(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_u32(writer, user, (gkinv_u32)container->slots[i].x);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_space(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_u32(writer, user, (gkinv_u32)container->slots[i].y);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_space(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_u32(writer, user, container->slots[i].flags);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_space(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_i32(writer, user, container->slots[i].state0);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_space(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_i32(writer, user, container->slots[i].state1);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_space(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_i32(writer, user, container->slots[i].state2);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_space(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = writer_put_i32(writer, user, container->slots[i].state3);
            if (rc != GKINV_OK) {
                return rc;
            }
            rc = save_newline(writer, user);
            if (rc != GKINV_OK) {
                return rc;
            }
        }
        i = (gkinv_u16)(i + 1u);
    }
    return writer_puts(writer, user, "END\n");
}

static const char *skip_ws(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        p++;
    }
    return p;
}

static const char *skip_line(const char *p)
{
    while (*p != '\0' && *p != '\n') {
        p++;
    }
    if (*p == '\n') {
        p++;
    }
    return p;
}

static int parse_u32(const char **pp, gkinv_u32 *out)
{
    const char *p;
    gkinv_u32 value;
    gkinv_bool any;

    p = skip_ws(*pp);
    value = 0UL;
    any = GKINV_FALSE;
    while (*p >= '0' && *p <= '9') {
        value = (value * 10UL) + (gkinv_u32)(*p - '0');
        any = GKINV_TRUE;
        p++;
    }
    if (any == GKINV_FALSE) {
        return GKINV_ERR_PARSE;
    }
    *out = value;
    *pp = p;
    return GKINV_OK;
}

static int parse_i32(const char **pp, gkinv_i32 *out)
{
    const char *p;
    gkinv_u32 u;
    int sign;
    int rc;

    p = skip_ws(*pp);
    sign = 1;
    if (*p == '-') {
        sign = -1;
        p++;
    }
    rc = parse_u32(&p, &u);
    if (rc != GKINV_OK) {
        return rc;
    }
    if (sign < 0) {
        *out = (gkinv_i32)(0L - (gkinv_i32)u);
    } else {
        *out = (gkinv_i32)u;
    }
    *pp = p;
    return GKINV_OK;
}

int gkinv_load_text(const gkinv_ItemDB *db, gkinv_Container *container, const char *text)
{
    gkinv_Container temp_container;
    gkinv_Slot temp_slots[GKINV_SIM_MAX_SLOTS];
    const char *p;
    gkinv_u32 u;
    gkinv_i32 s;
    gkinv_u16 index;
    int rc;

    if (db == (const gkinv_ItemDB *)0 || container == (gkinv_Container *)0 || text == (const char *)0) {
        return GKINV_ERR_NULL;
    }
    if (!container_can_simulate(container)) {
        return GKINV_ERR_TEMP_LIMIT;
    }
    clone_container_with_slots(&temp_container, container, temp_slots);
    gkinv_container_clear(&temp_container);
    p = text;
    while (*p != '\0') {
        p = skip_ws(p);
        if (*p == 'S') {
            p++;
            rc = parse_u32(&p, &u);
            if (rc != GKINV_OK || u > 65535UL) {
                return GKINV_ERR_PARSE;
            }
            index = (gkinv_u16)u;
            if (index >= temp_container.slot_capacity) {
                return GKINV_ERR_BAD_INDEX;
            }
            rc = parse_u32(&p, &u);
            if (rc != GKINV_OK || u > 65535UL) {
                return GKINV_ERR_PARSE;
            }
            temp_container.slots[index].item_id = (gkinv_u16)u;
            rc = parse_u32(&p, &u);
            if (rc != GKINV_OK || u > 65535UL) {
                return GKINV_ERR_PARSE;
            }
            temp_container.slots[index].quantity = (gkinv_u16)u;
            rc = parse_u32(&p, &u);
            if (rc != GKINV_OK || u > 65535UL) {
                return GKINV_ERR_PARSE;
            }
            temp_container.slots[index].x = (gkinv_u16)u;
            rc = parse_u32(&p, &u);
            if (rc != GKINV_OK || u > 65535UL) {
                return GKINV_ERR_PARSE;
            }
            temp_container.slots[index].y = (gkinv_u16)u;
            rc = parse_u32(&p, &u);
            if (rc != GKINV_OK) {
                return GKINV_ERR_PARSE;
            }
            temp_container.slots[index].flags = u;
            rc = parse_i32(&p, &s);
            if (rc != GKINV_OK) {
                return GKINV_ERR_PARSE;
            }
            temp_container.slots[index].state0 = s;
            rc = parse_i32(&p, &s);
            if (rc != GKINV_OK) {
                return GKINV_ERR_PARSE;
            }
            temp_container.slots[index].state1 = s;
            rc = parse_i32(&p, &s);
            if (rc != GKINV_OK) {
                return GKINV_ERR_PARSE;
            }
            temp_container.slots[index].state2 = s;
            rc = parse_i32(&p, &s);
            if (rc != GKINV_OK) {
                return GKINV_ERR_PARSE;
            }
            temp_container.slots[index].state3 = s;
            p = skip_line(p);
        } else if (*p == 'E') {
            break;
        } else {
            p = skip_line(p);
        }
    }
    temp_container.count = count_slots(&temp_container);
    rc = gkinv_validate_container(db, &temp_container);
    if (rc != GKINV_OK) {
        return rc;
    }
    copy_slots(container->slots, temp_container.slots, container->slot_capacity);
    container->count = temp_container.count;
    emit_event(container, container, GKINV_EVENT_LOADED, GKINV_OK, 0u, 0u,
               (gkinv_u16)GKINV_NO_INDEX, (gkinv_u16)GKINV_NO_INDEX);
    return GKINV_OK;
}
#endif

const char *gkinv_result_name(int result)
{
    switch (result) {
    case GKINV_OK:
        return "GKINV_OK";
    case GKINV_ERR_NULL:
        return "GKINV_ERR_NULL";
    case GKINV_ERR_BAD_ITEM:
        return "GKINV_ERR_BAD_ITEM";
    case GKINV_ERR_BAD_QUANTITY:
        return "GKINV_ERR_BAD_QUANTITY";
    case GKINV_ERR_NOT_FOUND:
        return "GKINV_ERR_NOT_FOUND";
    case GKINV_ERR_FULL:
        return "GKINV_ERR_FULL";
    case GKINV_ERR_NO_ROOM:
        return "GKINV_ERR_NO_ROOM";
    case GKINV_ERR_BAD_INDEX:
        return "GKINV_ERR_BAD_INDEX";
    case GKINV_ERR_STACK_LIMIT:
        return "GKINV_ERR_STACK_LIMIT";
    case GKINV_ERR_CATEGORY:
        return "GKINV_ERR_CATEGORY";
    case GKINV_ERR_FLAGS:
        return "GKINV_ERR_FLAGS";
    case GKINV_ERR_WEIGHT:
        return "GKINV_ERR_WEIGHT";
    case GKINV_ERR_RULE:
        return "GKINV_ERR_RULE";
    case GKINV_ERR_GRID:
        return "GKINV_ERR_GRID";
    case GKINV_ERR_TEMP_LIMIT:
        return "GKINV_ERR_TEMP_LIMIT";
    case GKINV_ERR_READ_ONLY:
        return "GKINV_ERR_READ_ONLY";
    case GKINV_ERR_NO_HOOK:
        return "GKINV_ERR_NO_HOOK";
    case GKINV_ERR_PARSE:
        return "GKINV_ERR_PARSE";
    default:
        return "GKINV_ERR_UNKNOWN";
    }
}
