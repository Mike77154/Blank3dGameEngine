#ifndef RPYL_SEMANTICS_H
#define RPYL_SEMANTICS_H

#include <stddef.h>
#include "rpyl_ast.h"
#include "rpyl_symtab.h"

#ifndef RPYL_SEMANTICS_MAX_ISSUES
#define RPYL_SEMANTICS_MAX_ISSUES 64
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RpylSemanticsIssueCode {
    RPYL_SEM_OK = 0,
    RPYL_SEM_EMPTY_ROOT = 1,
    RPYL_SEM_EMPTY_LABEL = 2,
    RPYL_SEM_DUPLICATE_LABEL = 3,
    RPYL_SEM_BAD_DEFINE = 4,
    RPYL_SEM_EMPTY_COMMAND = 5,
    RPYL_SEM_BAD_ARITY = 6,
    RPYL_SEM_UNKNOWN_TARGET = 7,
    RPYL_SEM_TABLE_OVERFLOW = 8
} RpylSemanticsIssueCode;

typedef struct RpylSemanticsIssue {
    RpylSemanticsIssueCode code;
    AstNodeType node_type;
    char name[RPYL_AST_MAX_NAME];
    int arg_count;
    char message[128];
} RpylSemanticsIssue;

typedef struct RpylSemanticsReport {
    RpylSymtab labels;
    RpylSymtab defines;
    RpylSemanticsIssue issues[RPYL_SEMANTICS_MAX_ISSUES];
    size_t issue_count;
    size_t dropped_issues;
    int block_count;
    int define_count;
    int command_count;
    int success;
} RpylSemanticsReport;

void rpyl_semantics_report_init(RpylSemanticsReport* report);
int rpyl_semantics_validate_ex(AstNode* root, RpylSemanticsReport* report);
int rpyl_semantics_validate(AstNode* root);
size_t rpyl_semantics_issue_count(const RpylSemanticsReport* report);
const RpylSemanticsIssue* rpyl_semantics_issue_at(const RpylSemanticsReport* report, size_t index);
const char* rpyl_semantics_issue_name(RpylSemanticsIssueCode code);

#ifdef __cplusplus
}
#endif

#endif
