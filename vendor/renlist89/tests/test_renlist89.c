#include "renlist89.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const char script[] =
        "image flame burn:\n"
        "    source sequence\n"
        "    billboard view\n"
        "    blend add\n"
        "    glow 1\n"
        "    fps 30\n"
        "    \"fire_001\"\n"
        "    pause 0.040\n"
        "    frame \"fire_002\" for 55\n"
        "    \"fire_003\"\n"
        "    repeat\n";
    RenList89 doc;
    rl89_id a;
    const RL89_Frame *f;
    rl89_init(&doc);
    if (!rl89_parse(&doc, script, (rl89_u32)(sizeof(script) - 1U))) {
        printf("parse fail line=%u err=%s\n", doc.error_line, rl89_error_string(doc.last_error));
        return 2;
    }
    a = rl89_find_animation(&doc, "flame", "burn");
    if (a == RL89_INVALID_ID) return 3;
    if (doc.animations[a].frame_count != 3U || doc.animations[a].loop_mode != RL89_LOOP_FORWARD) return 4;
    f = rl89_frame(&doc, a, 0U);
    if (!f || strcmp(f->request, "fire_001") != 0 || f->duration_ms != 40U) return 5;
    f = rl89_frame(&doc, a, 1U);
    if (!f || f->duration_ms != 55U) return 6;
    f = rl89_frame(&doc, a, 2U);
    if (!f || f->duration_ms != 33U) return 7;
    if (!rl89_property(&doc, a, "billboard") || strcmp(rl89_property(&doc, a, "billboard"), "view") != 0) return 8;
    if (!rl89_property(&doc, a, "blend") || strcmp(rl89_property(&doc, a, "blend"), "add") != 0) return 9;
    printf("RenList89 PASS animations=%u frames=%u properties=%u\n",
           (unsigned)doc.animation_count, (unsigned)doc.frame_count, (unsigned)doc.property_count);
    return 0;
}
