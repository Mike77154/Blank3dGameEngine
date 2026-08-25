#include "glyphwifettf.h"
#include "common_io.h"
#include <stdio.h>

int main(int argc, char **argv){
    GWT_Font font;
    unsigned long size;
    int r, w, h;
    char name[128];
    const char *text;

    if(argc < 3){
        printf("usage: %s font.ttf text\n", argv[0]);
        return 1;
    }
    size = read_file_static(argv[1], g_font_buf, FONT_BUF_MAX);
    if(size == 0){ printf("could not read font\n"); return 1; }
    r = gwt_font_init(&font, g_font_buf, size);
    if(r != GWT_OK){ printf("font init: %s\n", gwt_error_string(r)); return 1; }
    if(gwt_get_name_ascii(&font, GWT_NAME_FULL, name, sizeof(name)) == GWT_OK){
        printf("font: %s\n", name);
    }
    text = argv[2];
    r = gwt_measure_utf8(&font, text, 24, &w, &h);
    if(r != GWT_OK){ printf("measure: %s\n", gwt_error_string(r)); return 1; }
    printf("measure px: %d x %d\n", w, h);
    return 0;
}
