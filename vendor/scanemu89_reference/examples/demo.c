/* demo.c - tiny smoke test (no real backend)
   compile: cc -std=c89 -I../include ../src/scanemu.c demo.c -o demo
*/
#include "scanemu/scanemu.h"
#include <stdio.h>
#include <string.h>

static void on_bound(void* u, const char* symbol, const scanemu_token* token)
{
    char buf[128];
    (void)u;
    buf[0] = '\0';
    scanemu_token_to_string(token, buf, (scanemu_i32)sizeof(buf));
    printf("[scanemu] bound %s = %s\n", symbol, buf);
}

int main(void)
{
    scanemu_binding storage[16];
    scanemu_ctx ctx;
    scanemu_event ev;
    scanemu_token t;

    scanemu_init(&ctx, storage, 16);
    scanemu_set_on_bound(&ctx, on_bound, 0);

    /* default binding: jumpbutton = KEY:0:44 */
    t.type = SCANEMU_T_KEY; t.device_id = 0; t.code = 44; t.value = 0.0f; t.extra = 0;
    scanemu_set(&ctx, "jumpbutton", &t);

    /* user selects rebind jumpbutton */
    scanemu_listen(&ctx, "jumpbutton", SCANEMU_LISTEN_ANY);

    /* simulate SPACE press */
    memset(&ev, 0, sizeof(ev));
    ev.kind = SCANEMU_EV_PRESS;
    ev.token.type = SCANEMU_T_KEY;
    ev.token.device_id = 0;
    ev.token.code = 57; /* pretend 57 is SPACE in this backend */
    scanemu_feed(&ctx, &ev);

    return 0;
}
