#include "blank3d_npc_inventory.h"
#include "cycler89.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void b3d_npc_status(char *dst, size_t cap, const char *src)
{
    size_t i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (i + 1U < cap && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static char *b3d_npc_trim(char *text)
{
    char *end;
    if (!text) return text;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

static int b3d_npc_int(const char *text, int fallback)
{
    char *end;
    long value;
    if (!text || !*text) return fallback;
    value = strtol(text, &end, 10);
    while (*end && isspace((unsigned char)*end)) ++end;
    if (*end != '\0') return fallback;
    if (value < -2147483647L) value = -2147483647L;
    if (value > 2147483647L) value = 2147483647L;
    return (int)value;
}

static int b3d_npc_bool(const char *text, int fallback)
{
    char lower[16];
    size_t i;
    if (!text) return fallback;
    i = 0U;
    while (i + 1U < sizeof(lower) && text[i] != '\0') {
        lower[i] = (char)tolower((unsigned char)text[i]);
        ++i;
    }
    lower[i] = '\0';
    if (strcmp(lower, "1") == 0 || strcmp(lower, "true") == 0 ||
        strcmp(lower, "yes") == 0 || strcmp(lower, "on") == 0)
        return 1;
    if (strcmp(lower, "0") == 0 || strcmp(lower, "false") == 0 ||
        strcmp(lower, "no") == 0 || strcmp(lower, "off") == 0)
        return 0;
    return fallback;
}

static int b3d_npc_find_weapon_index(const Blank3DNpcInventory *inventory,
                                     int weapon_id)
{
    int i;
    if (!inventory || weapon_id <= 0) return -1;
    for (i = 0; i < inventory->weapon_count; ++i) {
        if (inventory->weapon_ids[i] == weapon_id) return i;
    }
    return -1;
}

static int b3d_npc_profile_id(const GWP89_Manager *manager,
                              const char *text)
{
    int slot;
    int id;
    const GWP89_WeaponProfile *profile;
    if (!manager || !text || !*text) return 0;
    id = b3d_npc_int(text, 0);
    if (id > 0 && gwp89_find_weapon_slot_by_id(manager, id) >= 0)
        return id;
    slot = gwp89_find_weapon_slot(manager, text);
    if (slot < 0) return 0;
    profile = gwp89_get_weapon(manager, slot);
    return profile ? profile->weapon_id : 0;
}

static const GWP89_WeaponProfile *b3d_npc_profile_id_ptr(
    const GWP89_Manager *manager, int weapon_id)
{
    int slot;
    if (!manager) return 0;
    slot = gwp89_find_weapon_slot_by_id(manager, weapon_id);
    return slot < 0 ? 0 : gwp89_get_weapon(manager, slot);
}

void blank3d_npc_inventory_bank_init(Blank3DNpcInventoryBank *bank)
{
    if (!bank) return;
    memset(bank, 0, sizeof(*bank));
}

Blank3DNpcInventory *blank3d_npc_inventory_find(
    Blank3DNpcInventoryBank *bank, int actor_id)
{
    int i;
    if (!bank) return 0;
    for (i = 0; i < B3D_NPC_INV_MAX_ACTORS; ++i) {
        if (bank->actors[i].active && bank->actors[i].actor_id == actor_id)
            return &bank->actors[i];
    }
    return 0;
}

const Blank3DNpcInventory *blank3d_npc_inventory_find_const(
    const Blank3DNpcInventoryBank *bank, int actor_id)
{
    int i;
    if (!bank) return 0;
    for (i = 0; i < B3D_NPC_INV_MAX_ACTORS; ++i) {
        if (bank->actors[i].active && bank->actors[i].actor_id == actor_id)
            return &bank->actors[i];
    }
    return 0;
}

Blank3DNpcInventory *blank3d_npc_inventory_bind(
    Blank3DNpcInventoryBank *bank, int actor_id)
{
    Blank3DNpcInventory *inventory;
    int i;
    if (!bank || actor_id <= 0) return 0;
    inventory = blank3d_npc_inventory_find(bank, actor_id);
    if (inventory) return inventory;
    for (i = 0; i < B3D_NPC_INV_MAX_ACTORS; ++i) {
        if (!bank->actors[i].active) {
            memset(&bank->actors[i], 0, sizeof(bank->actors[i]));
            bank->actors[i].active = 1;
            bank->actors[i].actor_id = actor_id;
            return &bank->actors[i];
        }
    }
    return 0;
}

int blank3d_npc_inventory_give_id(Blank3DNpcInventoryBank *bank,
                                  GWP89_Manager *manager,
                                  int actor_id,
                                  int weapon_id,
                                  int fill_clip)
{
    Blank3DNpcInventory *inventory;
    const GWP89_WeaponProfile *profile;
    int index;
    if (!bank || !manager || weapon_id <= 0) return GWP89_BAD_ARG;
    profile = b3d_npc_profile_id_ptr(manager, weapon_id);
    if (!profile) return GWP89_NOT_FOUND;
    inventory = blank3d_npc_inventory_bind(bank, actor_id);
    if (!inventory) return GWP89_FULL;
    index = b3d_npc_find_weapon_index(inventory, weapon_id);
    if (index < 0) {
        if (inventory->weapon_count >= B3D_NPC_INV_MAX_WEAPONS)
            return GWP89_FULL;
        index = inventory->weapon_count++;
        inventory->weapon_ids[index] = weapon_id;
        inventory->weapon_clips[index] = 0;
    }
    if (fill_clip)
        inventory->weapon_clips[index] = profile->clip_size > 0
                                      ? profile->clip_size : 0;
    return GWP89_OK;
}

int blank3d_npc_inventory_give_name(Blank3DNpcInventoryBank *bank,
                                    GWP89_Manager *manager,
                                    int actor_id,
                                    const char *weapon_name,
                                    int fill_clip)
{
    int weapon_id;
    weapon_id = b3d_npc_profile_id(manager, weapon_name);
    if (weapon_id <= 0) return GWP89_NOT_FOUND;
    return blank3d_npc_inventory_give_id(bank, manager, actor_id,
                                         weapon_id, fill_clip);
}

int blank3d_npc_inventory_take_id(Blank3DNpcInventoryBank *bank,
                                  int actor_id,
                                  int weapon_id)
{
    Blank3DNpcInventory *inventory;
    int index;
    int i;
    inventory = blank3d_npc_inventory_find(bank, actor_id);
    if (!inventory) return GWP89_NOT_FOUND;
    index = b3d_npc_find_weapon_index(inventory, weapon_id);
    if (index < 0) return GWP89_NOT_FOUND;
    for (i = index; i + 1 < inventory->weapon_count; ++i) {
        inventory->weapon_ids[i] = inventory->weapon_ids[i + 1];
        inventory->weapon_clips[i] = inventory->weapon_clips[i + 1];
    }
    inventory->weapon_count--;
    if (inventory->weapon_count >= 0 &&
        inventory->weapon_count < B3D_NPC_INV_MAX_WEAPONS) {
        inventory->weapon_ids[inventory->weapon_count] = 0;
        inventory->weapon_clips[inventory->weapon_count] = 0;
    }
    if (inventory->equipped_weapon_id == weapon_id)
        inventory->equipped_weapon_id = 0;
    return GWP89_OK;
}

int blank3d_npc_inventory_has_id(const Blank3DNpcInventoryBank *bank,
                                 int actor_id,
                                 int weapon_id)
{
    const Blank3DNpcInventory *inventory;
    inventory = blank3d_npc_inventory_find_const(bank, actor_id);
    return inventory && b3d_npc_find_weapon_index(inventory, weapon_id) >= 0;
}

int blank3d_npc_inventory_has_name(const Blank3DNpcInventoryBank *bank,
                                   const GWP89_Manager *manager,
                                   int actor_id,
                                   const char *weapon_name)
{
    int weapon_id;
    weapon_id = b3d_npc_profile_id(manager, weapon_name);
    return weapon_id > 0 &&
           blank3d_npc_inventory_has_id(bank, actor_id, weapon_id);
}

int blank3d_npc_inventory_equip_id(Blank3DNpcInventoryBank *bank,
                                   GWP89_Manager *manager,
                                   int actor_id,
                                   int weapon_id)
{
    Blank3DNpcInventory *inventory;
    int slot;
    int result;
    inventory = blank3d_npc_inventory_find(bank, actor_id);
    if (!inventory || b3d_npc_find_weapon_index(inventory, weapon_id) < 0)
        return GWP89_NOT_FOUND;
    slot = gwp89_find_weapon_slot_by_id(manager, weapon_id);
    if (slot < 0) return slot;
    result = gwp89_equip_slot(manager, actor_id, slot, 0);
    if (result == GWP89_OK) inventory->equipped_weapon_id = weapon_id;
    return result;
}

int blank3d_npc_inventory_equip_name(Blank3DNpcInventoryBank *bank,
                                     GWP89_Manager *manager,
                                     int actor_id,
                                     const char *weapon_name)
{
    int weapon_id;
    weapon_id = b3d_npc_profile_id(manager, weapon_name);
    if (weapon_id <= 0) return GWP89_NOT_FOUND;
    return blank3d_npc_inventory_equip_id(bank, manager, actor_id, weapon_id);
}

typedef struct B3DNpcCycleSourceTag {
    Blank3DNpcInventory *inventory;
} B3DNpcCycleSource;

static int b3d_npc_cycle_count(void *context)
{
    B3DNpcCycleSource *source;
    source = (B3DNpcCycleSource *)context;
    return source && source->inventory ? source->inventory->weapon_count : 0;
}

static int b3d_npc_cycle_read(void *context, int index,
                              cycler89_item *item_out)
{
    B3DNpcCycleSource *source;
    source = (B3DNpcCycleSource *)context;
    if (!source || !source->inventory || !item_out || index < 0 ||
        index >= source->inventory->weapon_count)
        return 0;
    *item_out = (cycler89_item)source->inventory->weapon_ids[index];
    return 1;
}

static int b3d_npc_cycle(Blank3DNpcInventoryBank *bank,
                         GWP89_Manager *manager,
                         int actor_id,
                         int direction)
{
    Blank3DNpcInventory *inventory;
    B3DNpcCycleSource source;
    Cycler89List list;
    cycler89_item selected;
    int result;
    inventory = blank3d_npc_inventory_find(bank, actor_id);
    if (!inventory || inventory->weapon_count <= 0) return GWP89_NOT_FOUND;
    source.inventory = inventory;
    list.context = &source;
    list.count = b3d_npc_cycle_count;
    list.read = b3d_npc_cycle_read;
    list.is_active = 0;
    result = cycler89_step(&list,
                           (cycler89_item)inventory->equipped_weapon_id,
                           inventory->equipped_weapon_id > 0, direction,
                           &selected, 0);
    if (result != CYCLER89_OK) return GWP89_NOT_FOUND;
    return blank3d_npc_inventory_equip_id(bank, manager, actor_id,
                                          (int)selected);
}

int blank3d_npc_inventory_cycle_next(Blank3DNpcInventoryBank *bank,
                                     GWP89_Manager *manager,
                                     int actor_id)
{
    return b3d_npc_cycle(bank, manager, actor_id, 1);
}

int blank3d_npc_inventory_cycle_prev(Blank3DNpcInventoryBank *bank,
                                     GWP89_Manager *manager,
                                     int actor_id)
{
    return b3d_npc_cycle(bank, manager, actor_id, -1);
}

int blank3d_npc_inventory_set_ammo(Blank3DNpcInventoryBank *bank,
                                   int actor_id,
                                   int ammo_id,
                                   int amount)
{
    Blank3DNpcInventory *inventory;
    if (!bank || ammo_id < 0 || ammo_id >= GWP89_MAX_AMMO_TYPES)
        return GWP89_BAD_ARG;
    inventory = blank3d_npc_inventory_bind(bank, actor_id);
    if (!inventory) return GWP89_FULL;
    if (amount < 0) amount = 0;
    inventory->ammo_amount[ammo_id] = amount;
    return amount;
}

int blank3d_npc_inventory_add_ammo(Blank3DNpcInventoryBank *bank,
                                   int actor_id,
                                   int ammo_id,
                                   int amount)
{
    Blank3DNpcInventory *inventory;
    long total;
    if (!bank || ammo_id < 0 || ammo_id >= GWP89_MAX_AMMO_TYPES)
        return GWP89_BAD_ARG;
    inventory = blank3d_npc_inventory_bind(bank, actor_id);
    if (!inventory) return GWP89_FULL;
    total = (long)inventory->ammo_amount[ammo_id] + (long)amount;
    if (total < 0L) total = 0L;
    if (total > 2147483647L) total = 2147483647L;
    inventory->ammo_amount[ammo_id] = (int)total;
    return inventory->ammo_amount[ammo_id];
}

int blank3d_npc_inventory_ammo(const Blank3DNpcInventoryBank *bank,
                               int actor_id,
                               int ammo_id)
{
    const Blank3DNpcInventory *inventory;
    if (ammo_id < 0 || ammo_id >= GWP89_MAX_AMMO_TYPES) return 0;
    inventory = blank3d_npc_inventory_find_const(bank, actor_id);
    return inventory ? inventory->ammo_amount[ammo_id] : 0;
}

int blank3d_npc_inventory_clip(const Blank3DNpcInventoryBank *bank,
                               int actor_id,
                               int weapon_id)
{
    const Blank3DNpcInventory *inventory;
    int index;
    inventory = blank3d_npc_inventory_find_const(bank, actor_id);
    if (!inventory) return 0;
    index = b3d_npc_find_weapon_index(inventory, weapon_id);
    return index < 0 ? 0 : inventory->weapon_clips[index];
}

int blank3d_npc_inventory_weapon_id(const Blank3DNpcInventoryBank *bank,
                                    int actor_id)
{
    const Blank3DNpcInventory *inventory;
    inventory = blank3d_npc_inventory_find_const(bank, actor_id);
    return inventory ? inventory->equipped_weapon_id : 0;
}

static int b3d_npc_key_weapon_id(const GWP89_Manager *manager,
                                 const char *key)
{
    const char *suffix;
    if (!key) return 0;
    suffix = key;
    if (strncmp(key, "weapon_", 7U) == 0) suffix = key + 7;
    return b3d_npc_profile_id(manager, suffix);
}

static int b3d_npc_key_ammo_id(const GWP89_Manager *manager,
                               const char *key)
{
    int id;
    int weapon_id;
    const GWP89_WeaponProfile *profile;
    if (!key) return -1;
    if (strncmp(key, "ammo_", 5U) == 0) {
        id = b3d_npc_int(key + 5, -1);
        if (id >= 0 && id < GWP89_MAX_AMMO_TYPES) return id;
    }
    weapon_id = b3d_npc_profile_id(manager, key);
    profile = b3d_npc_profile_id_ptr(manager, weapon_id);
    return profile ? profile->ammo_id : -1;
}

int blank3d_npc_inventory_load_ini(Blank3DNpcInventoryBank *bank,
                                   GWP89_Manager *manager,
                                   int actor_id,
                                   const char *path,
                                   char *status,
                                   size_t status_capacity)
{
    FILE *file;
    char line[B3D_NPC_INV_LINE_CAP];
    char section[32];
    char equipped[B3D_NPC_INV_NAME_CAP];
    Blank3DNpcInventory *inventory;
    int loaded;
    int weapon_id;
    int ammo_id;
    int amount;
    int index;
    if (!bank || !manager || !path) return 0;
    inventory = blank3d_npc_inventory_bind(bank, actor_id);
    if (!inventory) {
        b3d_npc_status(status, status_capacity, "NPC inventory bank full");
        return 0;
    }
    memset(inventory->weapon_ids, 0, sizeof(inventory->weapon_ids));
    memset(inventory->weapon_clips, 0, sizeof(inventory->weapon_clips));
    memset(inventory->ammo_amount, 0, sizeof(inventory->ammo_amount));
    inventory->weapon_count = 0;
    inventory->equipped_weapon_id = 0;
    file = fopen(path, "rb");
    if (!file) {
        b3d_npc_status(status, status_capacity, "NPC inventory INI missing");
        return 0;
    }
    section[0] = '\0';
    equipped[0] = '\0';
    loaded = 0;
    while (fgets(line, sizeof(line), file)) {
        char *text;
        char *end;
        char *equals;
        char *key;
        char *value;
        text = b3d_npc_trim(line);
        if (*text == '\0' || *text == '#' || *text == ';') continue;
        if (*text == '[') {
            end = strchr(text + 1, ']');
            if (!end) continue;
            *end = '\0';
            b3d_npc_status(section, sizeof(section), b3d_npc_trim(text + 1));
            continue;
        }
        equals = strchr(text, '=');
        if (!equals) continue;
        *equals = '\0';
        key = b3d_npc_trim(text);
        value = b3d_npc_trim(equals + 1);
        if ((strcmp(section, "npc") == 0 ||
             strcmp(section, "loadout") == 0) &&
            (strcmp(key, "equipped") == 0 ||
             strcmp(key, "equipped_weapon") == 0 ||
             strcmp(key, "equipped_weapon_id") == 0)) {
            b3d_npc_status(equipped, sizeof(equipped), value);
            ++loaded;
        } else if (strcmp(section, "weapons") == 0) {
            weapon_id = b3d_npc_key_weapon_id(manager, key);
            if (weapon_id > 0 && b3d_npc_bool(value, 0)) {
                if (blank3d_npc_inventory_give_id(bank, manager, actor_id,
                                                  weapon_id, 1) == GWP89_OK)
                    ++loaded;
            }
        } else if (strcmp(section, "ammo") == 0) {
            ammo_id = b3d_npc_key_ammo_id(manager, key);
            amount = b3d_npc_int(value, 0);
            if (ammo_id >= 0 && ammo_id < GWP89_MAX_AMMO_TYPES) {
                (void)blank3d_npc_inventory_set_ammo(bank, actor_id,
                                                     ammo_id, amount);
                ++loaded;
            }
        } else if (strcmp(section, "clips") == 0) {
            weapon_id = b3d_npc_key_weapon_id(manager, key);
            index = b3d_npc_find_weapon_index(inventory, weapon_id);
            if (index >= 0) {
                amount = b3d_npc_int(value, inventory->weapon_clips[index]);
                if (amount < 0) amount = 0;
                inventory->weapon_clips[index] = amount;
                ++loaded;
            }
        }
    }
    fclose(file);
    if (inventory->weapon_count <= 0) {
        b3d_npc_status(status, status_capacity,
                       "NPC inventory INI contained no registered weapons");
        return 0;
    }
    if (equipped[0] != '\0')
        weapon_id = b3d_npc_profile_id(manager, equipped);
    else
        weapon_id = inventory->weapon_ids[0];
    if (!blank3d_npc_inventory_has_id(bank, actor_id, weapon_id))
        weapon_id = inventory->weapon_ids[0];
    if (blank3d_npc_inventory_equip_id(bank, manager, actor_id,
                                      weapon_id) != GWP89_OK) {
        b3d_npc_status(status, status_capacity,
                       "NPC inventory loaded but initial equip failed");
        return 0;
    }
    b3d_npc_status(status, status_capacity, "NPC inventory INI loaded");
    return loaded;
}

int blank3d_npc_inventory_provider(void *context,
                                   GWP89_ProviderPacket *packet)
{
    Blank3DNpcInventoryBank *bank;
    Blank3DNpcInventory *inventory;
    int index;
    int current;
    if (!context || !packet || packet->phase != GWP89_PHASE_PRE)
        return GWP89_PROVIDER_PASS;
    bank = (Blank3DNpcInventoryBank *)context;
    inventory = blank3d_npc_inventory_find(bank, packet->actor_id);
    if (!inventory) return GWP89_PROVIDER_PASS;

    if (packet->operation == GWP89_OP_EQUIP) {
        if (b3d_npc_find_weapon_index(inventory, packet->weapon_id) < 0) {
            packet->result_code = GWP89_NOT_FOUND;
            return GWP89_PROVIDER_CANCEL;
        }
        return GWP89_PROVIDER_PASS;
    }

    if (packet->operation == GWP89_OP_CLIP_QUERY ||
        packet->operation == GWP89_OP_CLIP_SET) {
        index = b3d_npc_find_weapon_index(inventory, packet->weapon_id);
        if (index < 0) {
            packet->i_value = 0;
            packet->result_code = GWP89_NOT_FOUND;
            return GWP89_PROVIDER_HANDLED;
        }
        if (packet->operation == GWP89_OP_CLIP_SET) {
            if (packet->i_value < 0) packet->i_value = 0;
            inventory->weapon_clips[index] = packet->i_value;
        } else {
            packet->i_value = inventory->weapon_clips[index];
        }
        packet->result_code = GWP89_OK;
        return GWP89_PROVIDER_HANDLED;
    }

    if (packet->ammo_id < 0 || packet->ammo_id >= GWP89_MAX_AMMO_TYPES)
        return GWP89_PROVIDER_PASS;
    if (packet->operation == GWP89_OP_AMMO_QUERY) {
        packet->i_value = inventory->ammo_amount[packet->ammo_id];
        packet->result_code = GWP89_OK;
        return GWP89_PROVIDER_HANDLED;
    }
    if (packet->operation == GWP89_OP_AMMO_SET) {
        if (packet->i_value < 0) packet->i_value = 0;
        inventory->ammo_amount[packet->ammo_id] = packet->i_value;
        packet->result_code = GWP89_OK;
        return GWP89_PROVIDER_HANDLED;
    }
    if (packet->operation == GWP89_OP_AMMO_ADD) {
        packet->i_value = blank3d_npc_inventory_add_ammo(
            bank, packet->actor_id, packet->ammo_id, packet->amount);
        packet->result_code = packet->i_value < 0
                            ? GWP89_PROVIDER_ERROR : GWP89_OK;
        return GWP89_PROVIDER_HANDLED;
    }
    if (packet->operation == GWP89_OP_AMMO_CONSUME) {
        current = inventory->ammo_amount[packet->ammo_id];
        if (packet->amount <= 0 || current < packet->amount) {
            packet->result_code = GWP89_NO_AMMO;
        } else {
            inventory->ammo_amount[packet->ammo_id] = current - packet->amount;
            packet->i_value = inventory->ammo_amount[packet->ammo_id];
            packet->result_code = GWP89_OK;
        }
        return GWP89_PROVIDER_HANDLED;
    }
    return GWP89_PROVIDER_PASS;
}
