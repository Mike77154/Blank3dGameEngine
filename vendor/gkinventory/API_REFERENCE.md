# GKINV API Reference

## Fixed point

```c
typedef gkinv_i32 gkinv_fx;

#define GKINV_FX_ONE
#define GKINV_FX_ZERO
#define GKINV_FX_FROM_INT(x)

gkinv_fx  gkinv_fx_from_int(gkinv_i32 value);
gkinv_i32 gkinv_fx_to_int_floor(gkinv_fx value);
gkinv_fx  gkinv_fx_from_ratio(gkinv_i32 numerator, gkinv_i32 denominator);
gkinv_fx  gkinv_fx_add_sat(gkinv_fx a, gkinv_fx b);
gkinv_fx  gkinv_fx_sub_sat(gkinv_fx a, gkinv_fx b);
gkinv_fx  gkinv_fx_mul_int_sat(gkinv_fx value, gkinv_u16 count);
```

## Container lifecycle

```c
void gkinv_container_init(gkinv_Container *container,
                          gkinv_Slot *slots,
                          gkinv_u16 slot_capacity,
                          gkinv_u32 flags);

void gkinv_container_clear(gkinv_Container *container);
void gkinv_container_set_grid(gkinv_Container *container, gkinv_u16 grid_w, gkinv_u16 grid_h);
void gkinv_container_set_limits(gkinv_Container *container, gkinv_u32 allowed_categories, gkinv_fx max_weight);
void gkinv_container_set_slot_rules(gkinv_Container *container, const gkinv_SlotRule *rules, gkinv_u16 rule_count);
void gkinv_container_set_hooks(gkinv_Container *container, const gkinv_Hooks *hooks, void *hook_user);
```

## Queries

```c
const gkinv_ItemDef *gkinv_db_find(const gkinv_ItemDB *db, gkinv_u16 item_id);
int gkinv_validate_container(const gkinv_ItemDB *db, const gkinv_Container *container);
gkinv_fx gkinv_total_weight(const gkinv_ItemDB *db, const gkinv_Container *container);
int gkinv_has_item(const gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity);
gkinv_u16 gkinv_count_item(const gkinv_Container *container, gkinv_u16 item_id);
int gkinv_find_item(const gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 *out_index);
```

## Add/remove

```c
int gkinv_add_item(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity, gkinv_u16 *out_index);
int gkinv_add_stack(const gkinv_ItemDB *db, gkinv_Container *container, const gkinv_ItemStack *stack, gkinv_u16 *out_index);
int gkinv_insert_stack_at(const gkinv_ItemDB *db, gkinv_Container *container, const gkinv_ItemStack *stack, gkinv_u16 index);
int gkinv_insert_stack_grid_at(const gkinv_ItemDB *db, gkinv_Container *container, const gkinv_ItemStack *stack, gkinv_u16 index, gkinv_u16 x, gkinv_u16 y);
int gkinv_remove_item(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 item_id, gkinv_u16 quantity);
int gkinv_remove_from_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index, gkinv_u16 quantity);
```

## Move/transfer/use/combine

```c
int gkinv_move_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 from_index, gkinv_u16 to_index);
int gkinv_split_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 from_index, gkinv_u16 to_index, gkinv_u16 quantity);
int gkinv_move_grid_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index, gkinv_u16 x, gkinv_u16 y);
int gkinv_transfer_item(const gkinv_ItemDB *db, gkinv_Container *src, gkinv_Container *dst, gkinv_u16 item_id, gkinv_u16 quantity);
int gkinv_transfer_slot(const gkinv_ItemDB *db, gkinv_Container *src, gkinv_Container *dst, gkinv_u16 src_index, gkinv_u16 quantity);
int gkinv_use_slot(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index, void *actor, void *target);
int gkinv_combine_slots(const gkinv_ItemDB *db, gkinv_Container *container, gkinv_u16 index_a, gkinv_u16 index_b, const gkinv_Recipe *recipes, gkinv_u16 recipe_count);
```

## Save/load

```c
int gkinv_save_text(const gkinv_Container *container, gkinv_TextWriter writer, void *user);
int gkinv_load_text(const gkinv_ItemDB *db, gkinv_Container *container, const char *text);
```
