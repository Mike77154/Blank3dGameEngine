#include "fpi_vm.h"

#define FPI_VM_MATCH_BYTES ((FPI_MAX_RULES + 7) / 8)

static void vm_match_clear(unsigned char* bits) {
    int i;
    for (i = 0; i < FPI_VM_MATCH_BYTES; i++) bits[i] = 0;
}

static void vm_match_set(unsigned char* bits, int index) {
    bits[index >> 3] = (unsigned char)(bits[index >> 3] | (unsigned char)(1U << (index & 7)));
}

static int vm_match_get(const unsigned char* bits, int index) {
    return (bits[index >> 3] & (unsigned char)(1U << (index & 7))) != 0;
}

static int vm_eval_rule(void* user, const FPI_Program* program, const FPI_ProgramRule* rule, const FPI_Bindings* bindings, FPI_Error* error) {
    int ip;
    int last_condition;
    const FPI_Instruction* instruction;
    if (!bindings || !bindings->eval_cond) return FPI_ERR_ARGUMENT;
    ip = rule->condition_ip;
    last_condition = 0;
    while (ip < rule->action_ip) {
        instruction = &program->code[ip];
        if (instruction->opcode == FPI_OP_EVAL_COND) {
            int condition_result;
            condition_result = bindings->eval_cond(user, instruction->a,
                                                    &program->constants[instruction->b]);
            if (condition_result < 0) {
                fpi_error_set(error, condition_result,
                              fpi_span_make(0UL, 0UL, 0, 0),
                              "VM condition dispatch failed");
                return condition_result;
            }
            last_condition = condition_result > 0;
            ip++;
        } else if (instruction->opcode == FPI_OP_JUMP_IF_FALSE) {
            if (!last_condition) return 0;
            ip++;
        } else {
            fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, fpi_span_make(0UL, 0UL, 0, 0), "non-condition opcode inside VM condition range");
            return FPI_ERR_PROGRAM_INVALID;
        }
    }
    return 1;
}

static int vm_exec_rule(void* user, const FPI_Program* program, const FPI_ProgramRule* rule, const FPI_Bindings* bindings, FPI_Error* error) {
    int ip;
    const FPI_Instruction* instruction;
    ip = rule->action_ip;
    while (ip < rule->end_ip) {
        instruction = &program->code[ip];
        if (instruction->opcode == FPI_OP_EXEC_ACT) {
            if (bindings && bindings->exec_act)
                bindings->exec_act(user, instruction->a, &program->constants[instruction->b]);
        } else if (instruction->opcode != FPI_OP_RULE_FIRED) {
            fpi_error_set(error, FPI_ERR_PROGRAM_INVALID, fpi_span_make(0UL, 0UL, 0, 0), "non-action opcode inside VM action range");
            return FPI_ERR_PROGRAM_INVALID;
        }
        ip++;
    }
    return FPI_OK;
}

int fpi_vm_run_tick_ex(void* user, const FPI_Program* program, const FPI_Bindings* bindings, const FPI_RunOptions* options, int* out_fired, FPI_Error* error) {
    FPI_RunOptions defaults;
    const FPI_RunOptions* use_options;
    int rc;
    int i;
    int fired;
    unsigned char matched[FPI_VM_MATCH_BYTES];
    if (out_fired) *out_fired = 0;
    rc = fpi_program_validate(program, error);
    if (rc != FPI_OK) return rc;
    if (!bindings) return FPI_ERR_ARGUMENT;
    defaults.stop_on_first_match = 0;
    defaults.exec_mode = FPI_EXEC_SEQUENTIAL_IMMEDIATE;
    use_options = options ? options : &defaults;
    fired = 0;
    if (use_options->exec_mode == FPI_EXEC_SEQUENTIAL_IMMEDIATE) {
        for (i = 0; i < program->rule_count; i++) {
            rc = vm_eval_rule(user, program, &program->rules[i], bindings, error);
            if (rc < 0) return rc;
            if (rc == 0) continue;
            rc = vm_exec_rule(user, program, &program->rules[i], bindings, error);
            if (rc != FPI_OK) return rc;
            fired++;
            if (use_options->stop_on_first_match) break;
        }
    } else if (use_options->exec_mode == FPI_EXEC_TWO_PHASE) {
        vm_match_clear(matched);
        for (i = 0; i < program->rule_count; i++) {
            rc = vm_eval_rule(user, program, &program->rules[i], bindings, error);
            if (rc < 0) return rc;
            if (rc > 0) vm_match_set(matched, i);
        }
        for (i = 0; i < program->rule_count; i++) {
            if (!vm_match_get(matched, i)) continue;
            rc = vm_exec_rule(user, program, &program->rules[i], bindings, error);
            if (rc != FPI_OK) return rc;
            fired++;
            if (use_options->stop_on_first_match) break;
        }
    } else {
        return FPI_ERR_ARGUMENT;
    }
    if (out_fired) *out_fired = fired;
    return FPI_OK;
}

int fpi_vm_run_tick(void* user, const FPI_Program* program, const FPI_Bindings* bindings, const FPI_RunOptions* options) {
    int fired;
    if (fpi_vm_run_tick_ex(user, program, bindings, options, &fired, 0) != FPI_OK) return 0;
    return fired;
}
