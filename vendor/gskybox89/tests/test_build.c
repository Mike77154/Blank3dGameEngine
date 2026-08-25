#include "gskybox89.h"
#include "gskybox89_assets.h"

static void t_tri(void *user, const Gskybox89_Vertex *a, const Gskybox89_Vertex *b, const Gskybox89_Vertex *c)
{
    int *n;
    (void)a;
    (void)b;
    (void)c;
    n = (int *)user;
    *n += 1;
}

static int count_render_tris(const Gskybox89_Config *in_cfg)
{
    Gskybox89_Context ctx;
    Gskybox89_Backend be;
    Gskybox89_Mat3 rot;
    int tris;
    int rc;

    tris = 0;
    be.user = &tris;
    be.begin_pass = 0;
    be.end_pass = 0;
    be.set_state = 0;
    be.bind_face = 0;
    be.emit_tri = t_tri;
    gskybox89_mat3_identity(&rot);
    gskybox89_init(&ctx, in_cfg, &be);
    rc = gskybox89_render(&ctx, &rot);
    if (rc != GSKYBOX89_OK) return -1000 + rc;
    return tris;
}

static int test_asset_mapper(void)
{
    Gskybox89_AssetSet set;
    int face;
    int conv;
    int rc;

    gskybox89_asset_set_clear(&set);
    rc = gskybox89_asset_face_from_name("Materials/skybox/Sky_Night01FT.tga", &face, &conv);
    if (rc != GSKYBOX89_ASSET_OK || face != GSKYBOX89_FACE_POS_Z || conv != GSKYBOX89_ASSET_CONV_SOURCE6) return 11;
    rc = gskybox89_asset_face_from_name("sky_105_cubemap_2k/nx.png", &face, &conv);
    if (rc != GSKYBOX89_ASSET_OK || face != GSKYBOX89_FACE_NEG_X || conv != GSKYBOX89_ASSET_CONV_AXIS6) return 12;
    rc = gskybox89_asset_face_from_name("my_sky_right.bmp", &face, &conv);
    if (rc != GSKYBOX89_ASSET_OK || face != GSKYBOX89_FACE_POS_X || conv != GSKYBOX89_ASSET_CONV_WORD6) return 13;

    if (gskybox89_asset_add_path(&set, "Sky_Night01FT.tga", GSKYBOX89_ASSET_FLAG_DEFAULT) != GSKYBOX89_FACE_POS_Z) return 14;
    if (gskybox89_asset_add_path(&set, "Sky_Night01BK.tga", GSKYBOX89_ASSET_FLAG_DEFAULT) != GSKYBOX89_FACE_NEG_Z) return 15;
    if (gskybox89_asset_add_path(&set, "Sky_Night01LF.tga", GSKYBOX89_ASSET_FLAG_DEFAULT) != GSKYBOX89_FACE_NEG_X) return 16;
    if (gskybox89_asset_add_path(&set, "Sky_Night01RT.tga", GSKYBOX89_ASSET_FLAG_DEFAULT) != GSKYBOX89_FACE_POS_X) return 17;
    if (gskybox89_asset_add_path(&set, "Sky_Night01UP.tga", GSKYBOX89_ASSET_FLAG_DEFAULT) != GSKYBOX89_FACE_POS_Y) return 18;
    if (gskybox89_asset_add_path(&set, "Sky_Night01DN.tga", GSKYBOX89_ASSET_FLAG_DEFAULT) != GSKYBOX89_FACE_NEG_Y) return 19;
    if (set.present_count != 6) return 20;
    if (gskybox89_asset_complete_mask(&set) != GSKYBOX89_CUBE_ALL_FACES) return 21;
    return 0;
}

int main(void)
{
    Gskybox89_Config cfg;
    int tris;
    int arc;

    gskybox89_default_config(&cfg);
    cfg.layer_mask = GSKYBOX89_LAYER_HYBRID;
    tris = count_render_tris(&cfg);
    if (tris <= 0) return 3;

    gskybox89_default_config(&cfg);
    cfg.layer_mask = GSKYBOX89_LAYER_CUBE6;
    cfg.cube_fast_shared_vertices = 1;
    cfg.cube_face_mask = GSKYBOX89_CUBE_ALL_FACES;
    tris = count_render_tris(&cfg);
    if (tris != GSKYBOX89_CUBE_TRI_COUNT) return 4;

    gskybox89_default_config(&cfg);
    cfg.layer_mask = GSKYBOX89_LAYER_CUBE6;
    cfg.cube_fast_shared_vertices = 0;
    cfg.cube_face_mask = GSKYBOX89_CUBE_ALL_FACES;
    tris = count_render_tris(&cfg);
    if (tris != GSKYBOX89_CUBE_TRI_COUNT) return 5;

    gskybox89_default_config(&cfg);
    cfg.layer_mask = GSKYBOX89_LAYER_CUBE6;
    cfg.cube_fast_shared_vertices = 1;
    cfg.cube_face_mask = (1 << GSKYBOX89_FACE_POS_X) | (1 << GSKYBOX89_FACE_POS_Z);
    tris = count_render_tris(&cfg);
    if (tris != 4) return 6;

    arc = test_asset_mapper();
    if (arc != 0) return arc;

    return 0;
}
