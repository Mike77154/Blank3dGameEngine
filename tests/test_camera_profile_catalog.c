#include <stdio.h>
#include <string.h>

#include "blank3d_camera_profiles.h"

static int fail(const char *message)
{
    printf("FAIL: %s\n", message);
    return 1;
}

int main(void)
{
    Blank3DCameraCatalog catalog;
    const Blank3DCameraProfile *profile;
    int loaded;

    blank3d_camera_catalog_init(&catalog);
    loaded = blank3d_camera_catalog_load(
        &catalog, "tests/data/camera_catalog");
    if (loaded != 4) return fail("four INI files were not discovered");
    if (catalog.count != 4) return fail("catalog count is not four");

    profile = blank3d_camera_catalog_at(&catalog, 0);
    if (!profile || strcmp(profile->id, "tps_centred") != 0)
        return fail("order 10 TPS profile missing");
    profile = blank3d_camera_catalog_at(&catalog, 1);
    if (!profile || strcmp(profile->id, "ots") != 0)
        return fail("order 20 OTS profile missing");
    profile = blank3d_camera_catalog_at(&catalog, 2);
    if (!profile || strcmp(profile->id, "fps") != 0)
        return fail("order 30 FPS profile missing");
    profile = blank3d_camera_catalog_at(&catalog, 3);
    if (!profile || strcmp(profile->id, "isometric_test") != 0)
        return fail("new INI profile was not appended without code changes");

    if (!blank3d_camera_catalog_select_id(&catalog, "isometric_test"))
        return fail("select by profile id failed");
    profile = blank3d_camera_catalog_current(&catalog);
    if (!profile || profile->distance != G3D_FIX_FROM_INT(13))
        return fail("custom camera values were not parsed");
    if (!blank3d_camera_catalog_next(&catalog))
        return fail("catalog cycle failed");
    profile = blank3d_camera_catalog_current(&catalog);
    if (!profile || strcmp(profile->id, "tps_centred") != 0)
        return fail("catalog did not wrap by INI order");

    printf("PASS: add-only camera INI catalog (%d profiles)\n", catalog.count);
    return 0;
}
