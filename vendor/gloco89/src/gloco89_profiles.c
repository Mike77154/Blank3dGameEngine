#include "gloco89_profiles.h"

void gloco_make_tactical_profile(GLOCO_Profile *p)
{
    gloco_profile_defaults(p, GLOCO_PROFILE_TACTICAL);
}

void gloco_make_arcade_profile(GLOCO_Profile *p)
{
    gloco_profile_defaults(p, GLOCO_PROFILE_ARCADE);
}

void gloco_make_heavy_profile(GLOCO_Profile *p)
{
    gloco_profile_defaults(p, GLOCO_PROFILE_HEAVY);
}

void gloco_load_default_profile_bank(GLOCO_Context *ctx)
{
    GLOCO_Profile p;
    if (!ctx) return;

    gloco_profile_defaults(&p, GLOCO_PROFILE_DEFAULT);
    gloco_set_profile(ctx, GLOCO_PROFILE_DEFAULT, &p);

    gloco_make_tactical_profile(&p);
    gloco_set_profile(ctx, GLOCO_PROFILE_TACTICAL, &p);

    gloco_make_arcade_profile(&p);
    gloco_set_profile(ctx, GLOCO_PROFILE_ARCADE, &p);

    gloco_make_heavy_profile(&p);
    gloco_set_profile(ctx, GLOCO_PROFILE_HEAVY, &p);
}
