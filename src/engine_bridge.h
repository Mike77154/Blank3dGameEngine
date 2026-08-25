#ifndef MONIKA_ENGINE_BRIDGE_H
#define MONIKA_ENGINE_BRIDGE_H

#include "../vendor/gamlib3d/gamlib3d_transform.h"
#include "../vendor/soquete3d/soquete3d.h"
#include "../vendor/giffany_shapes3d/g3d_shapes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Gamlib3D is Q20.12; Soquete3D and Shapes3D are Q16.16. */
soq3d_fx bridge_q20_to_q16(g3d_fix value);
g3d_fix bridge_q16_to_q20(soq3d_fx value);

void bridge_transform_to_pose(const Transform *transform, soq3d_pose *out_pose);
Vec3 bridge_pose_position_q20(const soq3d_pose *pose);

/* OpenGL is only the final renderer boundary. Simulation stays fixed-point. */
void bridge_gl_load_q20_matrix(const g3d_fix matrix[16]);
void bridge_gl_apply_pose(const soq3d_pose *pose);
void bridge_gl_apply_q16_matrix(const signed int matrix[4][4]);
void bridge_gl_draw_mesh(const g3d_mesh *mesh);
void bridge_gl_begin_frame(int width, int height);
void bridge_gl_set_muzzle_light(int enabled,
                                g3d_fix x_q12, g3d_fix y_q12, g3d_fix z_q12,
                                long intensity_q16, long radius_q16,
                                unsigned char r, unsigned char g,
                                unsigned char b);
void bridge_gl_set_flamethrower_light(int enabled,
                                      g3d_fix x_q12, g3d_fix y_q12,
                                      g3d_fix z_q12,
                                      long intensity_q16, long radius_q16,
                                      unsigned char r, unsigned char g,
                                      unsigned char b);
void bridge_gl_draw_grid(int half_extent);
void bridge_gl_draw_segment(const Vec3 *a, const Vec3 *b,
                            unsigned char r, unsigned char g,
                            unsigned char b_color);

#ifdef __cplusplus
}
#endif

#endif
