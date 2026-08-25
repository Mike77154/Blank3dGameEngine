#include <stdio.h>
#include "gproj2d89.h"

#define DEMO_PATHS 2048
#define DEMO_POINTS 32768
#define CANVAS_W 1600
#define CANVAS_H 1080

static gp2d_path demo_paths[DEMO_PATHS];
static gp2d_vec2 demo_points[DEMO_POINTS];

static unsigned long magnitude_long(long value)
{
    if (value < 0L) {
        return (unsigned long)(-(value + 1L)) + 1UL;
    }
    return (unsigned long)value;
}

static void write_fx(FILE *out, gp2d_fx value, int negate)
{
    unsigned long magnitude;
    unsigned long whole;
    unsigned long fraction;
    int negative;

    negative = value < 0L;
    if (negate) {
        negative = !negative;
    }
    magnitude = magnitude_long(value);
    whole = magnitude / (unsigned long)GP2D_FX_ONE;
    fraction = ((magnitude % (unsigned long)GP2D_FX_ONE) * 1000UL) /
               (unsigned long)GP2D_FX_ONE;
    if (negative && magnitude != 0UL) {
        fputc('-', out);
    }
    fprintf(out, "%lu.%03lu", whole, fraction);
}

static void write_color(FILE *out, gp2d_color color)
{
    fprintf(out, "#%02X%02X%02X", (unsigned int)color.r,
            (unsigned int)color.g, (unsigned int)color.b);
}

static void write_scene_paths(FILE *out, const gp2d_scene *scene)
{
    unsigned short i;
    unsigned short j;
    const gp2d_path *path;
    const gp2d_vec2 *point;

    for (i = 0U; i < scene->path_count; ++i) {
        path = &scene->paths[i];
        fprintf(out, "<path d=\"");
        for (j = 0U; j < path->point_count; ++j) {
            point = &scene->points[path->first_point + j];
            fputc(j == 0U ? 'M' : 'L', out);
            write_fx(out, point->x, 0);
            fputc(' ', out);
            write_fx(out, point->y, 1);
            fputc(' ', out);
        }
        if (path->closed) {
            fputc('Z', out);
        }
        fprintf(out, "\" fill=\"");
        if (path->style.fill_enabled) {
            write_color(out, path->style.fill);
        } else {
            fprintf(out, "none");
        }
        fprintf(out, "\" stroke=\"");
        if (path->style.outline_enabled) {
            write_color(out, path->style.outline);
        } else {
            fprintf(out, "none");
        }
        fprintf(out, "\" stroke-width=\"");
        write_fx(out, path->style.outline_width, 0);
        fprintf(out, "\" stroke-linejoin=\"round\" ");
        fprintf(out, "stroke-linecap=\"round\"/>\n");
    }
}

static gp2d_fx profile_scale(int ammo_id)
{
    if (ammo_id == GP2D_AMMO_HAND_GRENADE) {
        return gp2d_fx_from_int(92L);
    }
    if (ammo_id == GP2D_AMMO_MISSILE) {
        return gp2d_fx_from_int(82L);
    }
    if (ammo_id == GP2D_AMMO_GRENADE_LAUNCHER) {
        return gp2d_fx_from_int(84L);
    }
    if (ammo_id == GP2D_AMMO_SNIPER) {
        return gp2d_fx_from_int(72L);
    }
    return gp2d_fx_from_int(78L);
}

static int build_mode(gp2d_scene *scene, int ammo_id,
                      int center_x, int center_y, int outline_enabled)
{
    gp2d_transform transform;
    gp2d_draw_options options;
    gp2d_fx scale;
    int has_shell;
    int status;
    int shell_x;
    int projectile_x;

    options = gp2d_draw_options_default();
    options.outline_enabled = outline_enabled ? GP2D_TRUE : GP2D_FALSE;
    options.outline_width = gp2d_fx_from_ratio(5L, 2L);
    options.detail_enabled = GP2D_TRUE;
    scale = profile_scale(ammo_id);
    has_shell = gp2d_profile_has_part(ammo_id, GP2D_PART_SHELL);
    shell_x = center_x - 43;
    projectile_x = center_x + 49;

    if (has_shell) {
        transform = gp2d_transform_identity();
        transform.position.x = gp2d_fx_from_int((long)shell_x);
        transform.position.y = gp2d_fx_from_int((long)-center_y);
        transform.scale.x = scale;
        transform.scale.y = scale;
        status = gp2d_build_part(scene, ammo_id, GP2D_PART_SHELL,
                                 &transform, &options);
        if (status != GP2D_OK) {
            return status;
        }
    } else {
        projectile_x = center_x;
    }

    transform = gp2d_transform_identity();
    transform.position.x = gp2d_fx_from_int((long)projectile_x);
    transform.position.y = gp2d_fx_from_int((long)-center_y);
    transform.scale.x = scale;
    transform.scale.y = scale;
    status = gp2d_build_part(scene, ammo_id, GP2D_PART_PROJECTILE,
                             &transform, &options);
    return status;
}

static const char *part_caption(int ammo_id)
{
    if (ammo_id == GP2D_AMMO_MISSILE) {
        return "proyectil completo";
    }
    if (ammo_id == GP2D_AMMO_HAND_GRENADE) {
        return "cuerpo + espoleta + palanca";
    }
    if (ammo_id == GP2D_AMMO_SHOTGUN_BUCKSHOT) {
        return "shell + grupo de 9 postas";
    }
    if (ammo_id == GP2D_AMMO_SHOTGUN_SLUG) {
        return "shell + slug separado";
    }
    return "shell + proyectil separados";
}

int main(int argc, char **argv)
{
    gp2d_scene scene;
    FILE *out;
    const char *output_name;
    const gp2d_profile *profile;
    int card_w;
    int card_h;
    int gap_x;
    int gap_y;
    int margin_x;
    int margin_y;
    int col;
    int row;
    int x;
    int y;
    int mode_y;
    int status;
    unsigned short i;

    output_name = argc > 1 ? argv[1] : "preview/gproj2d89_preview.svg";
    gp2d_scene_init(&scene, demo_paths, DEMO_PATHS,
                    demo_points, DEMO_POINTS);
    card_w = 500;
    card_h = 310;
    gap_x = 15;
    gap_y = 18;
    margin_x = 35;
    margin_y = 92;

    for (i = 0U; i < gp2d_profile_count(); ++i) {
        profile = gp2d_profile_at(i);
        if (profile == 0) {
            return 2;
        }
        col = (int)(i % 3U);
        row = (int)(i / 3U);
        x = margin_x + col * (card_w + gap_x);
        y = margin_y + row * (card_h + gap_y);
        mode_y = y + 176;
        status = build_mode(&scene, profile->ammo_id, x + 128,
                            mode_y, GP2D_TRUE);
        if (status != GP2D_OK) {
            fprintf(stderr, "outline build failed for %s: %d\n",
                    profile->name, status);
            return 3;
        }
        status = build_mode(&scene, profile->ammo_id, x + 372,
                            mode_y, GP2D_FALSE);
        if (status != GP2D_OK) {
            fprintf(stderr, "flat build failed for %s: %d\n",
                    profile->name, status);
            return 4;
        }
    }

    if (gp2d_scene_validate(&scene) != GP2D_OK) {
        fprintf(stderr, "scene validation failed\n");
        return 5;
    }

    out = fopen(output_name, "wb");
    if (out == 0) {
        fprintf(stderr, "cannot open %s\n", output_name);
        return 6;
    }
    fprintf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(out, "<svg xmlns=\"http://www.w3.org/2000/svg\" ");
    fprintf(out, "width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n",
            CANVAS_W, CANVAS_H, CANVAS_W, CANVAS_H);
    fprintf(out, "<defs><linearGradient id=\"bg\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"1\">");
    fprintf(out, "<stop offset=\"0\" stop-color=\"#10141A\"/><stop offset=\"1\" stop-color=\"#202832\"/></linearGradient></defs>\n");
    fprintf(out, "<rect width=\"1600\" height=\"1080\" fill=\"url(#bg)\"/>\n");
    fprintf(out, "<text x=\"42\" y=\"46\" fill=\"#F4F6F8\" font-family=\"sans-serif\" font-size=\"28\" font-weight=\"700\">GProj2D89 — catálogo vectorial de munición 2D</text>\n");
    fprintf(out, "<text x=\"43\" y=\"72\" fill=\"#AAB5C2\" font-family=\"sans-serif\" font-size=\"14\">C89 · Q16.16 · buffers del caller · shell/proyectil desacoplados · recolor y outline por path</text>\n");

    for (i = 0U; i < gp2d_profile_count(); ++i) {
        profile = gp2d_profile_at(i);
        col = (int)(i % 3U);
        row = (int)(i / 3U);
        x = margin_x + col * (card_w + gap_x);
        y = margin_y + row * (card_h + gap_y);
        fprintf(out, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" rx=\"18\" fill=\"#F7F8FA\" fill-opacity=\"0.96\" stroke=\"#4B5968\" stroke-width=\"1\"/>\n",
                x, y, card_w, card_h);
        fprintf(out, "<text x=\"%d\" y=\"%d\" fill=\"#18202A\" font-family=\"sans-serif\" font-size=\"20\" font-weight=\"700\">%s</text>\n",
                x + 18, y + 30, profile->label);
        fprintf(out, "<text x=\"%d\" y=\"%d\" fill=\"#65717E\" font-family=\"monospace\" font-size=\"12\">%s</text>\n",
                x + 18, y + 49, profile->name);
        fprintf(out, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"#D7DDE3\"/>\n",
                x + 250, y + 62, x + 250, y + 258);
        fprintf(out, "<text x=\"%d\" y=\"%d\" text-anchor=\"middle\" fill=\"#3A4652\" font-family=\"sans-serif\" font-size=\"13\" font-weight=\"700\">CON OUTLINE</text>\n",
                x + 128, y + 72);
        fprintf(out, "<text x=\"%d\" y=\"%d\" text-anchor=\"middle\" fill=\"#3A4652\" font-family=\"sans-serif\" font-size=\"13\" font-weight=\"700\">SIN OUTLINE</text>\n",
                x + 372, y + 72);
        fprintf(out, "<text x=\"%d\" y=\"%d\" text-anchor=\"middle\" fill=\"#697581\" font-family=\"sans-serif\" font-size=\"12\">%s</text>\n",
                x + 250, y + 288, part_caption(profile->ammo_id));
    }

    write_scene_paths(out, &scene);
    fprintf(out, "<text x=\"1555\" y=\"1060\" text-anchor=\"end\" fill=\"#92A0AE\" font-family=\"monospace\" font-size=\"12\">preview generado desde la API de GProj2D89</text>\n");
    fprintf(out, "</svg>\n");
    fclose(out);
    printf("wrote %s with %u paths and %u points\n", output_name,
           (unsigned int)scene.path_count,
           (unsigned int)scene.point_count);
    return 0;
}
