#include <stdio.h>
#include <string.h>
#include "gcrosshair_base89.h"

static FILE *g_expected;
static int expect_bytes(const unsigned char *b, unsigned long n)
{
    unsigned long i;
    int c;
    for (i = 0; i < n; ++i) {
        c = fgetc(g_expected);
        if (c == EOF || (unsigned char)c != b[i]) return 0;
    }
    return 1;
}
static int expect_u32(unsigned long v)
{
    unsigned char b[4];
    b[0]=(unsigned char)(v&255UL); b[1]=(unsigned char)((v>>8)&255UL);
    b[2]=(unsigned char)((v>>16)&255UL); b[3]=(unsigned char)((v>>24)&255UL);
    return expect_bytes(b,4UL);
}
static int expect_i32(long v) { return expect_u32((unsigned long)v); }
static int expect_str(const char *s)
{
    unsigned long n=(unsigned long)strlen(s);
    if (!expect_u32(n)) return 0;
    return expect_bytes((const unsigned char *)s,n);
}
static int expect_variant(const GC89_Variant *v)
{
#define EI(x) if(!expect_i32((long)(x))) return 0
#define EU(x) if(!expect_u32((unsigned long)(x))) return 0
    EI(v->draw_mode); EI(v->arm_mask); EI(v->dot_enabled);
    EI(v->gap_fx); EI(v->arm_length_fx); EI(v->thickness_fx); EI(v->dot_size_fx);
    EI(v->image_id); EI(v->image_width_fx); EI(v->image_height_fx);
    EU(v->color_rgba); EU(v->image_tint_rgba);
    EI(v->shape_type); EI(v->shape_segment_mask); EI(v->shape_direction_mask); EI(v->spread_mode);
    EI(v->shape_radius_x_fx); EI(v->shape_radius_y_fx); EI(v->shape_depth_fx);
    EI(v->shape_rotation_deg_fx); EI(v->shape_break_fx);
    EI(v->outline_enabled); EI(v->outline_width_fx); EU(v->outline_color_rgba);
#undef EI
#undef EU
    return 1;
}
int main(void)
{
    int i;
    GC89_Style s;
    GCB89_AnimationPreset a;
    g_expected=fopen("tests/golden/pre_ini_hardcoded_192.bin","rb");
    if(!g_expected) return 2;
    if(!expect_u32((unsigned long)gcb89_preset_count())) return 3;
    for(i=0;i<gcb89_preset_count();++i) {
        if(!gcb89_make_preset(i,&s)||!gcb89_make_preset_animation(i,&a)) return 4;
        if(!expect_i32((long)i)||!expect_str(gcb89_preset_name(i))||!expect_str(gcb89_preset_category(i))) return 5;
        if(!expect_variant(&s.normal)||!expect_variant(&s.aim)||!expect_variant(&s.fire)||!expect_variant(&s.hit)) return 6;
        if(!expect_i32(s.use_aim_variant)||!expect_i32(s.use_fire_variant)||!expect_i32(s.use_hit_variant)||!expect_i32(s.color_change_enabled)) return 7;
        if(!expect_i32(s.spread_multiplier_fx)||!expect_i32(s.center_offset_x_fx)||!expect_i32(s.center_offset_y_fx)) return 8;
        if(!expect_i32(a.micro_scale_fx)||!expect_i32(a.neutral_scale_fx)||!expect_i32(a.maxi_scale_fx)||!expect_i32(a.speed_fx_per_tick)) return 9;
    }
    if(fgetc(g_expected)!=EOF) return 10;
    fclose(g_expected);
    puts("pre-INI hardcoded 192 canonical bytes: IDENTICAL");
    return 0;
}
