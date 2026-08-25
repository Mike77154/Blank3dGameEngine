#include "gveh_profile_bank.h"
#include <string.h>

typedef struct gveh_profile_bank_row_s {
    const char *name;
    gveh_profile_make_fn make;
} gveh_profile_bank_row;

static const gveh_profile_bank_row gveh_profile_bank_rows[] = {
    { "warthog89", gveh_profile_warthog89 },
    { "gt_sport89", gveh_profile_gt_sport89 },
    { "rally89", gveh_profile_rally89 },
    { "daytona89", gveh_profile_daytona89 },
    { "wave_jetski89", gveh_profile_wave_jetski89 },
    { "tank_lite89", gveh_profile_tank_lite89 },
    { "thunder_chopper89", gveh_profile_thunder_chopper89 },
    { "strike_chopper89", gveh_profile_strike_chopper89 },
    { "sim_heli89", gveh_profile_sim_heli89 },
    { "ace_fighter89", gveh_profile_ace_fighter89 },
    { "fs_lightplane89", gveh_profile_fs_lightplane89 },
    { "comanche_voxel89", gveh_profile_comanche_voxel89 },
    { "longbow_campaign89", gveh_profile_longbow_campaign89 },
    { "gunship_flight89", gveh_profile_gunship_flight89 },
    { "rogue_xwing89", gveh_profile_rogue_xwing89 },
    { "tie_interceptor89", gveh_profile_tie_interceptor89 },
    { "battlefront_bomber89", gveh_profile_battlefront_bomber89 },
    { "tank4_m1a1_89", gveh_profile_tank4_m1a1_89 },
    { "tank4_t72_89", gveh_profile_tank4_t72_89 },
    { "tank4_tiger_89", gveh_profile_tank4_tiger_89 },
    { "tank4_sherman_89", gveh_profile_tank4_sherman_89 },
    { "tank4_destroyer_89", gveh_profile_tank4_destroyer_89 },
    { "tank4_scout_89", gveh_profile_tank4_scout_89 }
};

gveh_i16 gveh_profile_bank_count(void)
{
    return (gveh_i16)(sizeof(gveh_profile_bank_rows) / sizeof(gveh_profile_bank_rows[0]));
}

const char *gveh_profile_bank_name(gveh_i16 index)
{
    if (index < 0 || index >= gveh_profile_bank_count()) return "";
    return gveh_profile_bank_rows[index].name;
}

gveh_i32 gveh_profile_bank_make_by_index(gveh_i16 index, gveh_profile *out_profile)
{
    if (out_profile == 0) return 0;
    if (index < 0 || index >= gveh_profile_bank_count()) return 0;
    gveh_profile_bank_rows[index].make(out_profile);
    gveh_profile_refresh_module_flags(out_profile);
    return 1;
}

gveh_i32 gveh_profile_bank_make_by_name(const char *name, gveh_profile *out_profile)
{
    gveh_i16 i;
    if (name == 0 || out_profile == 0) return 0;
    i = 0;
    while (i < gveh_profile_bank_count()) {
        if (strcmp(name, gveh_profile_bank_rows[i].name) == 0) {
            return gveh_profile_bank_make_by_index(i, out_profile);
        }
        i++;
    }
    return 0;
}
