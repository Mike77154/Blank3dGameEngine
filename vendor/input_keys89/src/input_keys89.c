#include "input_keys89.h"

#include <ctype.h>
#include <string.h>

typedef struct InputKeys89NamedUsageTag {
    const char *name;
    unsigned char usage;
} InputKeys89NamedUsage;

static void ik89_compact(const char *source, char *out, unsigned int capacity)
{
    unsigned int i;
    unsigned int j;
    unsigned char c;
    if (!out || capacity == 0U) return;
    if (!source) source = "";
    j = 0U;
    for (i = 0U; source[i] != '\0' && j + 1U < capacity; ++i) {
        c = (unsigned char)source[i];
        if (isalnum(c)) out[j++] = (char)tolower(c);
    }
    out[j] = '\0';
}

static int ik89_parse_dec(const char *s, int *out_value)
{
    int value;
    int digits;
    if (!s || !out_value) return 0;
    value = 0;
    digits = 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        value = value * 10 + (*s - '0');
        ++digits;
        ++s;
    }
    if (!digits) return 0;
    *out_value = value;
    return 1;
}

static int ik89_hex_digit(char c)
{
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return 10 + (int)(c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (int)(c - 'A');
    return -1;
}

static int ik89_parse_hex_byte(const char *s, unsigned int *out_value)
{
    int hi;
    int lo;
    if (!s || !out_value || s[0] == '\0' || s[1] == '\0') return 0;
    hi = ik89_hex_digit(s[0]);
    lo = ik89_hex_digit(s[1]);
    if (hi < 0 || lo < 0) return 0;
    if (s[2] != '\0') return 0;
    *out_value = (unsigned int)((hi << 4) | lo);
    return 1;
}

static unsigned int ik89_letter_usage(char c)
{
    c = (char)tolower((unsigned char)c);
    if (c < 'a' || c > 'z') return 0U;
    return 0x04U + (unsigned int)(c - 'a');
}

static unsigned int ik89_digit_usage(char c)
{
    if (c >= '1' && c <= '9') return 0x1EU + (unsigned int)(c - '1');
    if (c == '0') return 0x27U;
    return 0U;
}

static unsigned int ik89_fkey_usage(int n)
{
    if (n >= 1 && n <= 12) return 0x3AU + (unsigned int)(n - 1);
    if (n >= 13 && n <= 24) return 0x68U + (unsigned int)(n - 13);
    return 0U;
}

static unsigned int ik89_keypad_usage(const char *compact)
{
    if (!compact) return 0U;
    if (strcmp(compact, "kp0") == 0 || strcmp(compact, "numpad0") == 0) return 0x62U;
    if (strcmp(compact, "kp1") == 0 || strcmp(compact, "numpad1") == 0) return 0x59U;
    if (strcmp(compact, "kp2") == 0 || strcmp(compact, "numpad2") == 0) return 0x5AU;
    if (strcmp(compact, "kp3") == 0 || strcmp(compact, "numpad3") == 0) return 0x5BU;
    if (strcmp(compact, "kp4") == 0 || strcmp(compact, "numpad4") == 0) return 0x5CU;
    if (strcmp(compact, "kp5") == 0 || strcmp(compact, "numpad5") == 0) return 0x5DU;
    if (strcmp(compact, "kp6") == 0 || strcmp(compact, "numpad6") == 0) return 0x5EU;
    if (strcmp(compact, "kp7") == 0 || strcmp(compact, "numpad7") == 0) return 0x5FU;
    if (strcmp(compact, "kp8") == 0 || strcmp(compact, "numpad8") == 0) return 0x60U;
    if (strcmp(compact, "kp9") == 0 || strcmp(compact, "numpad9") == 0) return 0x61U;
    if (strcmp(compact, "kpdiv") == 0 || strcmp(compact, "numpaddivide") == 0) return 0x54U;
    if (strcmp(compact, "kpmul") == 0 || strcmp(compact, "numpadmultiply") == 0) return 0x55U;
    if (strcmp(compact, "kpsub") == 0 || strcmp(compact, "numpadsubtract") == 0) return 0x56U;
    if (strcmp(compact, "kpadd") == 0 || strcmp(compact, "numpadadd") == 0) return 0x57U;
    if (strcmp(compact, "kpenter") == 0 || strcmp(compact, "numpadenter") == 0) return 0x58U;
    if (strcmp(compact, "kpdot") == 0 || strcmp(compact, "kpdecimal") == 0 ||
        strcmp(compact, "numpaddecimal") == 0) return 0x63U;
    if (strcmp(compact, "kpequal") == 0 || strcmp(compact, "numpadequal") == 0) return 0x67U;
    return 0U;
}

static const InputKeys89NamedUsage ik89_names[] = {
    {"enter", 0x28U}, {"return", 0x28U},
    {"escape", 0x29U}, {"esc", 0x29U},
    {"backspace", 0x2AU}, {"bksp", 0x2AU},
    {"tab", 0x2BU}, {"space", 0x2CU}, {"spacebar", 0x2CU},
    {"minus", 0x2DU}, {"dash", 0x2DU},
    {"equal", 0x2EU}, {"equals", 0x2EU},
    {"lbracket", 0x2FU}, {"leftbracket", 0x2FU},
    {"rbracket", 0x30U}, {"rightbracket", 0x30U},
    {"backslash", 0x31U}, {"bslash", 0x31U},
    {"nonushash", 0x32U}, {"hash", 0x32U},
    {"semicolon", 0x33U}, {"semi", 0x33U},
    {"apostrophe", 0x34U}, {"quote", 0x34U},
    {"grave", 0x35U}, {"tilde", 0x35U}, {"backtick", 0x35U},
    {"comma", 0x36U}, {"dot", 0x37U}, {"period", 0x37U},
    {"slash", 0x38U}, {"forwardslash", 0x38U},
    {"capslock", 0x39U},
    {"printscreen", 0x46U}, {"prtsc", 0x46U},
    {"scrolllock", 0x47U}, {"pause", 0x48U},
    {"insert", 0x49U}, {"ins", 0x49U},
    {"home", 0x4AU},
    {"pageup", 0x4BU}, {"pgup", 0x4BU},
    {"delete", 0x4CU}, {"del", 0x4CU},
    {"end", 0x4DU},
    {"pagedown", 0x4EU}, {"pgdn", 0x4EU},
    {"right", 0x4FU}, {"arrowright", 0x4FU}, {"cursorright", 0x4FU},
    {"left", 0x50U}, {"arrowleft", 0x50U}, {"cursorleft", 0x50U},
    {"down", 0x51U}, {"arrowdown", 0x51U}, {"cursordown", 0x51U},
    {"up", 0x52U}, {"arrowup", 0x52U}, {"cursorup", 0x52U},
    {"numlock", 0x53U},
    {"application", 0x65U}, {"menu", 0x65U}, {"app", 0x65U},
    {"power", 0x66U},
    {"lctrl", 0xE0U}, {"leftctrl", 0xE0U}, {"leftcontrol", 0xE0U}, {"ctrl", 0xE0U}, {"control", 0xE0U},
    {"lshift", 0xE1U}, {"leftshift", 0xE1U}, {"shift", 0xE1U},
    {"lalt", 0xE2U}, {"leftalt", 0xE2U}, {"alt", 0xE2U},
    {"lgui", 0xE3U}, {"leftgui", 0xE3U}, {"gui", 0xE3U}, {"win", 0xE3U}, {"windows", 0xE3U}, {"cmd", 0xE3U}, {"meta", 0xE3U},
    {"rctrl", 0xE4U}, {"rightctrl", 0xE4U}, {"rightcontrol", 0xE4U},
    {"rshift", 0xE5U}, {"rightshift", 0xE5U},
    {"ralt", 0xE6U}, {"rightalt", 0xE6U}, {"altgr", 0xE6U},
    {"rgui", 0xE7U}, {"rightgui", 0xE7U}
};

static const char *ik89_literal_name(unsigned int usage)
{
    switch (usage) {
    case 0x28U: return "enter";
    case 0x29U: return "escape";
    case 0x2AU: return "backspace";
    case 0x2BU: return "tab";
    case 0x2CU: return "space";
    case 0x2DU: return "minus";
    case 0x2EU: return "equal";
    case 0x2FU: return "lbracket";
    case 0x30U: return "rbracket";
    case 0x31U: return "backslash";
    case 0x32U: return "nonus_hash";
    case 0x33U: return "semicolon";
    case 0x34U: return "apostrophe";
    case 0x35U: return "grave";
    case 0x36U: return "comma";
    case 0x37U: return "dot";
    case 0x38U: return "slash";
    case 0x39U: return "capslock";
    case 0x46U: return "print_screen";
    case 0x47U: return "scroll_lock";
    case 0x48U: return "pause";
    case 0x49U: return "insert";
    case 0x4AU: return "home";
    case 0x4BU: return "page_up";
    case 0x4CU: return "delete";
    case 0x4DU: return "end";
    case 0x4EU: return "page_down";
    case 0x4FU: return "right";
    case 0x50U: return "left";
    case 0x51U: return "down";
    case 0x52U: return "up";
    case 0x53U: return "num_lock";
    case 0x54U: return "kp_div";
    case 0x55U: return "kp_mul";
    case 0x56U: return "kp_sub";
    case 0x57U: return "kp_add";
    case 0x58U: return "kp_enter";
    case 0x63U: return "kp_dot";
    case 0x65U: return "application";
    case 0x66U: return "power";
    case 0x67U: return "kp_equal";
    case 0xE0U: return "lctrl";
    case 0xE1U: return "lshift";
    case 0xE2U: return "lalt";
    case 0xE3U: return "lgui";
    case 0xE4U: return "rctrl";
    case 0xE5U: return "rshift";
    case 0xE6U: return "ralt";
    case 0xE7U: return "rgui";
    default: return 0;
    }
}

static void ik89_hex2(char *dst, unsigned int value)
{
    static const char hex[] = "0123456789abcdef";
    dst[0] = hex[(value >> 4) & 0x0FU];
    dst[1] = hex[value & 0x0FU];
}

input_key89 input_keys89_from_hid(unsigned int page, unsigned int usage)
{
    if (page > 0xFFU || usage > 0xFFU) return INPUT_KEY89_NONE;
    if (page == 0U && usage == 0U) return INPUT_KEY89_NONE;
    return INPUT_KEY89_MAKE(page, usage);
}

int input_keys89_to_hid(input_key89 key,
                        unsigned int *out_page,
                        unsigned int *out_usage)
{
    if (key == INPUT_KEY89_NONE) return 0;
    if (out_page) *out_page = INPUT_KEY89_PAGE(key);
    if (out_usage) *out_usage = INPUT_KEY89_USAGE(key);
    return 1;
}

int input_keys89_is_keyboard(input_key89 key)
{
    return key != INPUT_KEY89_NONE &&
           INPUT_KEY89_PAGE(key) == INPUT_KEY89_PAGE_KEYBOARD;
}

int input_keys89_keyboard_usage(input_key89 key, unsigned int *out_usage)
{
    if (!input_keys89_is_keyboard(key)) return 0;
    if (out_usage) *out_usage = INPUT_KEY89_USAGE(key);
    return 1;
}

input_key89 input_keys89_from_name(const char *name)
{
    char compact[64];
    unsigned int usage;
    unsigned int i;
    int n;
    if (!name) return INPUT_KEY89_NONE;
    ik89_compact(name, compact, (unsigned int)sizeof(compact));
    if (compact[0] == '\0' || strcmp(compact, "none") == 0) return INPUT_KEY89_NONE;

    if (compact[1] == '\0') {
        usage = ik89_letter_usage(compact[0]);
        if (!usage) usage = ik89_digit_usage(compact[0]);
        if (usage) return INPUT_KEY89_KB(usage);
    }

    if (compact[0] == 'f' && ik89_parse_dec(compact + 1, &n)) {
        usage = ik89_fkey_usage(n);
        if (usage) return INPUT_KEY89_KB(usage);
    }

    usage = ik89_keypad_usage(compact);
    if (usage) return INPUT_KEY89_KB(usage);

    for (i = 0U; i < (unsigned int)(sizeof(ik89_names) / sizeof(ik89_names[0])); ++i) {
        if (strcmp(compact, ik89_names[i].name) == 0)
            return INPUT_KEY89_KB((unsigned int)ik89_names[i].usage);
    }

    /* Portable explicit usage forms: 0x52 / kb0x52 / hid0752 after compaction. */
    if (strlen(compact) == 4U && strncmp(compact, "0x", 2U) == 0) {
        if (ik89_parse_hex_byte(compact + 2, &usage)) return INPUT_KEY89_KB(usage);
    }
    if (strlen(compact) == 6U && strncmp(compact, "kb0x", 4U) == 0) {
        if (ik89_parse_hex_byte(compact + 4, &usage)) return INPUT_KEY89_KB(usage);
    }
    if (strlen(compact) == 7U && strncmp(compact, "hid", 3U) == 0) {
        unsigned int page;
        char page_hex[3];
        char usage_hex[3];
        page_hex[0] = compact[3]; page_hex[1] = compact[4]; page_hex[2] = '\0';
        usage_hex[0] = compact[5]; usage_hex[1] = compact[6]; usage_hex[2] = '\0';
        if (ik89_parse_hex_byte(page_hex, &page) &&
            ik89_parse_hex_byte(usage_hex, &usage))
            return input_keys89_from_hid(page, usage);
    }

    return INPUT_KEY89_NONE;
}

const char *input_keys89_name(input_key89 key,
                              char *tmp,
                              unsigned int tmp_size)
{
    unsigned int page;
    unsigned int usage;
    const char *literal;
    int n;
    if (key == INPUT_KEY89_NONE) return "none";
    page = INPUT_KEY89_PAGE(key);
    usage = INPUT_KEY89_USAGE(key);
    if (page != INPUT_KEY89_PAGE_KEYBOARD) {
        if (tmp && tmp_size >= 10U) {
            tmp[0] = 'h'; tmp[1] = 'i'; tmp[2] = 'd'; tmp[3] = ':';
            ik89_hex2(tmp + 4, page);
            tmp[6] = ':';
            ik89_hex2(tmp + 7, usage);
            tmp[9] = '\0';
            return tmp;
        }
        return "unknown";
    }
    if (usage >= 0x04U && usage <= 0x1DU) {
        if (tmp && tmp_size >= 2U) {
            tmp[0] = (char)('a' + (char)(usage - 0x04U));
            tmp[1] = '\0';
            return tmp;
        }
        return "unknown";
    }
    if (usage >= 0x1EU && usage <= 0x26U) {
        if (tmp && tmp_size >= 2U) {
            tmp[0] = (char)('1' + (char)(usage - 0x1EU));
            tmp[1] = '\0';
            return tmp;
        }
        return "unknown";
    }
    if (usage == 0x27U) {
        if (tmp && tmp_size >= 2U) {
            tmp[0] = '0'; tmp[1] = '\0'; return tmp;
        }
        return "unknown";
    }
    if (usage >= 0x3AU && usage <= 0x45U) {
        n = (int)(usage - 0x3AU) + 1;
        if (tmp && tmp_size >= 4U) {
            tmp[0] = 'f';
            if (n >= 10) {
                tmp[1] = (char)('0' + n / 10);
                tmp[2] = (char)('0' + n % 10);
                tmp[3] = '\0';
            } else {
                tmp[1] = (char)('0' + n);
                tmp[2] = '\0';
            }
            return tmp;
        }
        return "unknown";
    }
    if (usage >= 0x68U && usage <= 0x73U) {
        n = (int)(usage - 0x68U) + 13;
        if (tmp && tmp_size >= 4U) {
            tmp[0] = 'f';
            tmp[1] = (char)('0' + n / 10);
            tmp[2] = (char)('0' + n % 10);
            tmp[3] = '\0';
            return tmp;
        }
        return "unknown";
    }
    if (usage >= 0x59U && usage <= 0x61U) {
        n = (int)(usage - 0x59U) + 1;
        if (tmp && tmp_size >= 5U) {
            tmp[0] = 'k'; tmp[1] = 'p'; tmp[2] = '_';
            tmp[3] = (char)('0' + n); tmp[4] = '\0'; return tmp;
        }
        return "unknown";
    }
    if (usage == 0x62U) {
        if (tmp && tmp_size >= 5U) {
            tmp[0] = 'k'; tmp[1] = 'p'; tmp[2] = '_'; tmp[3] = '0'; tmp[4] = '\0';
            return tmp;
        }
        return "unknown";
    }
    literal = ik89_literal_name(usage);
    if (literal) return literal;
    if (tmp && tmp_size >= 8U) {
        tmp[0] = 'k'; tmp[1] = 'b'; tmp[2] = '_'; tmp[3] = '0'; tmp[4] = 'x';
        ik89_hex2(tmp + 5, usage); tmp[7] = '\0'; return tmp;
    }
    return "unknown";
}
