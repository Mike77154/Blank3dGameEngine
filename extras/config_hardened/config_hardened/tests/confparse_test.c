#include <stdio.h>
#include <string.h>
#include "config/confparse.h"

static void print_entry(const conf_entry_t *e)
{
    printf("%.*s.%.*s=%.*s\n",
           e->section_len, e->section ? e->section : "",
           e->key_len, e->key ? e->key : "",
           e->value_len, e->value ? e->value : "");
}

int main(void)
{
    const char *ini =
        "; comment\n"
        "[ video ] ; inline comment\n"
        "width = 320 # inline\n"
        "height=200\n"
        "fullscreen = on\n"
        "scale = -1.5\n";
    const char *yaml =
        "# comment\n"
        "video: # section\n"
        "  width: 640\n"
        "  height: 480 ; inline\n"
        "  enabled: true\n"
        "  speed: 2.25\n";
    conf_parser_t p;
    conf_entry_t e;
    int v;

    conf_init(&p, ini, (int)strlen(ini));
    while (conf_next(&p, &e)) {
        print_entry(&e);
        if (conf_entry_key_eq(&e, "width")) {
            if (!conf_entry_parse_int(&e, &v) || v != 320) return 1;
        }
        if (conf_entry_key_eq(&e, "fullscreen")) {
            if (!conf_entry_parse_bool(&e, &v) || v != 1) return 2;
        }
        if (conf_entry_key_eq(&e, "scale")) {
            if (!conf_entry_parse_fixed(&e, 16, &v) || v != -98304) return 3;
        }
    }

    conf_init(&p, yaml, (int)strlen(yaml));
    while (conf_next(&p, &e)) {
        print_entry(&e);
        if (conf_entry_section_eq(&e, "video") && conf_entry_key_eq(&e, "speed")) {
            if (!conf_entry_parse_fixed(&e, 16, &v) || v != 147456) return 4;
        }
    }

    return 0;
}
