#include <stdio.h>
#include "var_dsl89.h"

static int print_emit(void *user, const vdsl89_command *cmd)
{
    const char *scope;
    const char *op;
    (void)user;

    scope = cmd->scope == VDSL89_SCOPE_LOCAL ? "local" :
            cmd->scope == VDSL89_SCOPE_GLOBAL ? "global" : "instance";
    op = cmd->opcode == VDSL89_OP_ADD ? "+=" :
         cmd->opcode == VDSL89_OP_SUB ? "-=" : "=";

    printf("%s %s %s ", scope, cmd->name, op);
    if (cmd->value.type == VDSL89_VALUE_FIXED) {
        printf("raw_q16=%ld", cmd->value.fixed_q16);
    } else if (cmd->value.type == VDSL89_VALUE_BOOL) {
        printf("%s", cmd->value.boolean ? "true" : "false");
    } else if (cmd->value.type == VDSL89_VALUE_STRING) {
        printf("\"%s\"", cmd->value.string_value);
    }
    printf("\n");
    return 1;
}

int main(void)
{
    vdsl89_provider p;
    p.emit = print_emit;
    p.user = NULL;

    vdsl89_execute_line("hp = 100;", &p);
    vdsl89_execute_line("var damage = 12.5;", &p);
    vdsl89_execute_line("global.score += 50;", &p);
    vdsl89_execute_line("name = \"Claire\";", &p);
    return 0;
}
