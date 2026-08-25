/* Minimal host-side provider example. */
#include <stdio.h>
#include "gscopebundle89.h"

static int engine_draw(void *user, const gsp89_draw_cmd *cmd)
{
    (void)user;
    printf("engine draw kind=%d layer=%d part=%d\n",
           cmd->kind, cmd->layer, cmd->part_id);
    return GPR89_HANDLED;
}

int main(void)
{
    gscb89_ctx scope;
    gpr89_provider engine;
    gri89_doc recipe;

    gscb89_init(&scope);
    gpr89_provider_init(&engine, "host");
    engine.capabilities = GPR89_CAP_PAINT;
    engine.emit_draw_cmd = engine_draw;
    gscb89_register_provider(&scope, &engine);

    /* The recipe says paint=auto:host. All other domains can stay internal. */
    if (!gscb89_load_recipe(&scope, "config/hud/sniper_pso1.ini", &recipe))
        return 1;

    printf("provider registered; recipe scope=%s\n",
           gri89_get(&recipe, "scope", "use", "?"));
    return 0;
}
