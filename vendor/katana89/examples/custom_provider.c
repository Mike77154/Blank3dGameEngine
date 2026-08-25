#include "katana89.h"
#include <stdio.h>

static km89_vertex vertices[KM89_MAX_VERTICES];
static km89_triangle triangles[KM89_MAX_TRIANGLES];

int main(void)
{
    km89_mesh mesh;
    km89_build_info parts;
    km89_desc d;
    int result;

    d.name="My provider-built katana";
    d.blade_length=km89_from_millimetres(710);
    d.handle_length=km89_from_millimetres(270);
    d.sori=km89_from_millimetres(18);
    d.blade_width_base=km89_from_millimetres(32);
    d.blade_width_tip=km89_from_millimetres(20);
    d.blade_thickness=km89_from_millimetres(8);
    d.guard_radius=km89_from_millimetres(42);
    d.guard_thickness=km89_from_millimetres(5);
    d.blade_style=KM89_BLADE_SHINOGI;
    d.kissaki_style=KM89_KISSAKI_CHU;
    d.tsuba_style=KM89_TSUBA_MOKKO;
    d.tsuka_style=KM89_TSUKA_RIKKO;
    d.palette_id=3;
    d.flags=KM89_FLAG_HAMON|KM89_FLAG_BOHI|KM89_FLAG_MENUKI|KM89_FLAG_DOUBLE_WRAP|KM89_FLAG_SAYA;

    km89_mesh_init(&mesh,vertices,KM89_MAX_VERTICES,triangles,KM89_MAX_TRIANGLES);
    result=km89_build(&d,&mesh,&parts);
    if(result!=KM89_OK)return 1;
    printf("vertices=%u triangles=%u\n",(unsigned)mesh.vertex_count,(unsigned)mesh.triangle_count);
    return 0;
}
