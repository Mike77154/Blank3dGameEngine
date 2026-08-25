#include <stdio.h>
#include "var_manager89.h"

int main(void)
{
    vm89_manager manager;
    vm89_value value;
    vm89_value out;
    const char *runtime_names[4];
    int i;

    runtime_names[0] = "hp_creado_en_runtime";
    runtime_names[1] = "quest_state_93";
    runtime_names[2] = "variable_inventada";
    runtime_names[3] = "otra_que_no_existia";

    vm89_init(&manager);
    vm89_instance_create(&manager, 77UL);
    vm89_begin_event(&manager, 77UL, 5UL);

    for (i = 0; i < 4; ++i) {
        vm89_value_fixed_int(&value, (vm89_i32)(i + 1) * 10L);

        if (i == 0) {
            vm89_set(&manager, VM89_SCOPE_INSTANCE, 77UL,
                     runtime_names[i], &value);
        } else if (i == 1) {
            vm89_set(&manager, VM89_SCOPE_GLOBAL, 0UL,
                     runtime_names[i], &value);
        } else {
            vm89_set(&manager, VM89_SCOPE_LOCAL, 0UL,
                     runtime_names[i], &value);
        }
    }

    vm89_get(&manager, VM89_SCOPE_GLOBAL, 0UL, "quest_state_93", &out);
    printf("runtime global raw=%ld\n", out.fixed_q16);

    printf("unknown_before_set=%d\n",
           vm89_exists(&manager, VM89_SCOPE_GLOBAL, 0UL, "never_created"));

    vm89_value_fixed_int(&value, 123L);
    vm89_set(&manager, VM89_SCOPE_GLOBAL, 0UL, "never_created", &value);

    printf("unknown_after_set=%d\n",
           vm89_exists(&manager, VM89_SCOPE_GLOBAL, 0UL, "never_created"));

    vm89_unset(&manager, VM89_SCOPE_GLOBAL, 0UL, "never_created");

    printf("after_unset=%d\n",
           vm89_exists(&manager, VM89_SCOPE_GLOBAL, 0UL, "never_created"));

    vm89_end_event(&manager);
    return 0;
}
