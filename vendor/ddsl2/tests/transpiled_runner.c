#include "common/ddsl.h"

#include <stdio.h>
#include <string.h>

int ddsl2_script_run(ddsl_vm *vm, ddsl_error *err);

int main(void) {
    ddsl_store store;
    ddsl_vm vm;
    ddsl_error err;
    const char *hit;
    const char *after;
    static const char expected_hit[] = "\303\251A";

    ddsl_store_init(&store);
    ddsl_vm_init(&vm, &store);
    if (!ddsl2_script_run(&vm, &err)) {
        fprintf(stderr, "transpiled runtime error: %s\n", err.message);
        return 1;
    }

    hit = ddsl_store_get(&store, "hit");
    after = ddsl_store_get(&store, "after");
    if (!hit || strcmp(hit, expected_hit) != 0) {
        fprintf(stderr, "unexpected hit value\n");
        return 1;
    }
    if (!after || strcmp(after, "1") != 0) {
        fprintf(stderr, "missing statement after if\n");
        return 1;
    }

    puts("PASS transpiled C execution");
    return 0;
}
