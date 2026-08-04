/* rocketmeshes_example.c - minimal use/eject-hint example. */
#include <stdio.h>
#include "rocketmeshes.h"

static void draw_mesh_stub(const RM_Mesh *m)
{
    printf("draw %s: %u vertices, %u triangles\n", m->name, (unsigned int)m->vcount, (unsigned int)m->tcount);
}

static void show_eject_hint(int mesh_id)
{
    const RM_EjectHint *h;
    h = rm_get_eject_hint(mesh_id);
    if (!h) {
        return;
    }
    printf("eject hint for %s\n", rm_get_mesh_name(mesh_id));
    printf("  behavior=%u\n", (unsigned int)h->behavior);
    printf("  fire_fx_mesh=%d\n", (int)h->fire_fx_mesh);
    printf("  fire_debris_mesh=%d\n", (int)h->fire_debris_mesh);
    printf("  reload_eject_mesh=%d\n", (int)h->reload_eject_mesh);
    printf("  state_empty_mesh=%d\n", (int)h->state_empty_mesh);
}

static void show_behavior_profile(int mesh_id)
{
    const RM_BehaviorProfile *p;
    p = rm_get_behavior_profile(mesh_id);
    if (!p) {
        return;
    }
    printf("behavior profile for %s\n", rm_get_mesh_name(mesh_id));
    printf("  projectile=%d\n", rm_behavior_is_projectile(mesh_id));
    printf("  propulsion=%s\n", rm_get_propulsion_name((int)p->propulsion_model));
    printf("  fins=%s\n", rm_get_fin_rule_name((int)p->fin_rule));
    printf("  trail=%s\n", rm_get_trail_rule_name((int)p->trail_rule));
    printf("  ticks_to_fin_open=%d\n", (int)p->ticks_to_fin_open);
    printf("  ticks_to_sustainer_visual=%d\n", (int)p->ticks_to_sustainer_visual);
    printf("  visual_speed_q8=%d\n", (int)p->visual_speed_q8);
}

int main(void)
{
    const RM_Mesh *m;
    m = rm_get_mesh(RMESH_SURVIVAL_RPG7_CONE);
    if (m) {
        draw_mesh_stub(m);
        show_eject_hint(RMESH_SURVIVAL_RPG7_CONE);
        show_behavior_profile(RMESH_SURVIVAL_RPG7_CONE);
    }
    m = rm_get_mesh(RMESH_SHELL_40MM_SPENT_CASE);
    if (m) {
        draw_mesh_stub(m);
    }
    return 0;
}
