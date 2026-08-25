#include <stdio.h>
#include "gskybox89.h"
#include "gskybox89_assets.h"

int main(int argc, char **argv)
{
    Gskybox89_AssetSet set;
    int i;
    int rc;
    int face;
    int conv;
    int mask;
    char key[GSKYBOX89_ASSET_KEY_MAX];

    gskybox89_asset_set_clear(&set);
    if (argc <= 1) {
        printf("usage: demo_asset_mapper <sky files...>\n");
        printf("supported mapper names: Source FT/BK/LF/RT/UP/DN, Axis px/nx/py/ny/pz/nz, words front/back/right/left/up/down\n");
        return 0;
    }

    for (i = 1; i < argc; ++i) {
        rc = gskybox89_asset_face_from_name(argv[i], &face, &conv);
        if (rc == GSKYBOX89_ASSET_OK) {
            printf("map %-54s -> face=%s conv=%s\n", argv[i], gskybox89_face_name(face), gskybox89_asset_convention_name(conv));
        } else if (argv[i] != 0 && (argv[i][0] != '\0')) {
            printf("skip %-53s -> no sky face suffix\n", argv[i]);
        }
        rc = gskybox89_asset_add_path(&set, argv[i], GSKYBOX89_ASSET_FLAG_DEFAULT);
        (void)rc;
        if (argv[i] != 0 && argv[i][0] != '\0') {
            if (gskybox89_asset_vmf_extract_skyname(argv[i], key, (int)sizeof(key)) == GSKYBOX89_ASSET_OK) {
                printf("vmf skyname: %s\n", key);
            }
            if (gskybox89_asset_vmt_extract_basetexture(argv[i], key, (int)sizeof(key)) == GSKYBOX89_ASSET_OK) {
                printf("vmt basetexture: %s\n", key);
            }
        }
    }

    mask = gskybox89_asset_complete_mask(&set);
    printf("complete mask: 0x%02X present=%d/6\n", mask, set.present_count);
    for (i = 0; i < GSKYBOX89_CUBE_FACE_COUNT; ++i) {
        printf("face %-2s source_suffix=%s axis_suffix=%s present=%d path=%s\n",
            gskybox89_face_name(i),
            gskybox89_asset_source_suffix_for_face(i),
            gskybox89_asset_axis_suffix_for_face(i),
            set.present[i],
            set.face_path[i]);
    }
    return (set.present_count == 0) ? 1 : 0;
}
