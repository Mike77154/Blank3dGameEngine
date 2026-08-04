#include "rpyl_semantics.h"
#include "rpyl_common.h"
#include "rpyl_builtins.h"

void rpyl_semantics_report_init(RpylSemanticsReport* report) {
    size_t i;
    if (!report) return;
    rpyl_symtab_init(&report->labels);
    rpyl_symtab_init(&report->defines);
    for (i = 0u; i < RPYL_SEMANTICS_MAX_ISSUES; i++) {
        report->issues[i].code = RPYL_SEM_OK;
        report->issues[i].node_type = AST_CALL;
        report->issues[i].name[0] = '\0';
        report->issues[i].arg_count = 0;
        report->issues[i].message[0] = '\0';
    }
    report->issue_count = 0u;
    report->dropped_issues = 0u;
    report->block_count = 0;
    report->define_count = 0;
    report->command_count = 0;
    report->success = 1;
}

const char* rpyl_semantics_issue_name(RpylSemanticsIssueCode code) {
    switch (code) {
    case RPYL_SEM_OK: return "ok";
    case RPYL_SEM_EMPTY_ROOT: return "empty_root";
    case RPYL_SEM_EMPTY_LABEL: return "empty_label";
    case RPYL_SEM_DUPLICATE_LABEL: return "duplicate_label";
    case RPYL_SEM_BAD_DEFINE: return "bad_define";
    case RPYL_SEM_EMPTY_COMMAND: return "empty_command";
    case RPYL_SEM_BAD_ARITY: return "bad_arity";
    case RPYL_SEM_UNKNOWN_TARGET: return "unknown_target";
    case RPYL_SEM_TABLE_OVERFLOW: return "table_overflow";
    default: return "unknown";
    }
}

static void sem_issue(RpylSemanticsReport* report, RpylSemanticsIssueCode code, AstNode* node, const char* name, int argc, const char* message) {
    RpylSemanticsIssue* issue;
    if (!report) return;
    report->success = 0;
    if (report->issue_count >= (size_t)RPYL_SEMANTICS_MAX_ISSUES) {
        report->dropped_issues++;
        return;
    }
    issue = &report->issues[report->issue_count];
    issue->code = code;
    issue->node_type = node ? node->type : AST_CALL;
    issue->arg_count = argc;
    (void)rpyl_common_copy(issue->name, sizeof(issue->name), name ? name : "");
    (void)rpyl_common_copy(issue->message, sizeof(issue->message), message ? message : rpyl_semantics_issue_name(code));
    report->issue_count++;
}

static int starts_with_dynamic_token(const char* s) {
    if (!s || !s[0]) return 0;
    return s[0] == '$' ? 1 : 0;
}

static void build_target(char out[RPYL_AST_MAX_NAME], AstNode* node) {
    int i;
    out[0] = '\0';
    if (!node) return;
    for (i = 0; i < node->arg_count; i++) {
        if (i > 0) (void)rpyl_common_append(out, RPYL_AST_MAX_NAME, " ");
        (void)rpyl_common_append(out, RPYL_AST_MAX_NAME, node->args[i]);
    }
}

static void collect_top_level(RpylSemanticsReport* report, AstNode* root) {
    AstNode* n;
    unsigned long index;
    if (!report || !root) return;
    n = root->children;
    index = 0UL;
    while (n != 0) {
        if (n->type == AST_BLOCK) {
            if (n->name[0] == '\0') sem_issue(report, RPYL_SEM_EMPTY_LABEL, n, n->name, 0, "label has no name");
            else if (rpyl_symtab_contains(&report->labels, n->name)) sem_issue(report, RPYL_SEM_DUPLICATE_LABEL, n, n->name, 0, "label already exists");
            else if (!rpyl_symtab_put_kind(&report->labels, n->name, index, RPYL_SYM_LABEL, 0UL)) sem_issue(report, RPYL_SEM_TABLE_OVERFLOW, n, n->name, 0, "label table capacity reached");
            report->block_count++;
            index++;
        } else if (n->type == AST_DEFINE) {
            if (n->name[0] == '\0') sem_issue(report, RPYL_SEM_BAD_DEFINE, n, n->name, 0, "define has no name");
            else if (!rpyl_symtab_put_kind(&report->defines, n->name, (unsigned long)report->define_count, RPYL_SYM_DEFINE, 0UL)) sem_issue(report, RPYL_SEM_TABLE_OVERFLOW, n, n->name, 0, "define table capacity reached");
            report->define_count++;
        }
        n = n->next;
    }
}

static void check_node(RpylSemanticsReport* report, AstNode* node);

static void check_children(RpylSemanticsReport* report, AstNode* first) {
    AstNode* n;
    n = first;
    while (n != 0) {
        check_node(report, n);
        n = n->next;
    }
}

static void check_call_target(RpylSemanticsReport* report, AstNode* node) {
    char target[RPYL_AST_MAX_NAME];
    unsigned long value;
    if (!report || !node) return;
    if (!(rpyl_common_streq(node->name, "jump") || rpyl_common_streq(node->name, "goto") || rpyl_common_streq(node->name, "call"))) return;
    if (node->arg_count <= 0) return;
    if (starts_with_dynamic_token(node->args[0])) return;
    build_target(target, node);
    if (!rpyl_symtab_get_kind(&report->labels, target, RPYL_SYM_LABEL, &value)) {
        sem_issue(report, RPYL_SEM_UNKNOWN_TARGET, node, target, node->arg_count, "static target label not found");
    }
}

static void check_node(RpylSemanticsReport* report, AstNode* node) {
    if (!report || !node) return;
    if (node->type == AST_CALL) {
        report->command_count++;
        if (node->name[0] == '\0') sem_issue(report, RPYL_SEM_EMPTY_COMMAND, node, node->name, node->arg_count, "command has no name");
        if (!rpyl_builtins_validate_arity(node->name, node->arg_count)) sem_issue(report, RPYL_SEM_BAD_ARITY, node, node->name, node->arg_count, "builtin called with wrong number of args");
        check_call_target(report, node);
        return;
    }
    if (node->type == AST_BLOCK) {
        if (node->name[0] == '\0') sem_issue(report, RPYL_SEM_EMPTY_LABEL, node, node->name, 0, "block has no name");
        check_children(report, node->children);
        return;
    }
    if (node->type == AST_DEFINE) {
        if (node->name[0] == '\0') sem_issue(report, RPYL_SEM_BAD_DEFINE, node, node->name, 0, "define has no name");
    }
}

int rpyl_semantics_validate_ex(AstNode* root, RpylSemanticsReport* report) {
    RpylSemanticsReport local_report;
    RpylSemanticsReport* r;
    if (report) r = report;
    else r = &local_report;
    rpyl_semantics_report_init(r);
    if (!root) {
        sem_issue(r, RPYL_SEM_EMPTY_ROOT, root, "", 0, "root is null");
        return 0;
    }
    collect_top_level(r, root);
    check_children(r, root->children);
    return r->success ? 1 : 0;
}

int rpyl_semantics_validate(AstNode* root) {
    return rpyl_semantics_validate_ex(root, (RpylSemanticsReport*)0);
}

size_t rpyl_semantics_issue_count(const RpylSemanticsReport* report) {
    if (!report) return 0u;
    return report->issue_count;
}

const RpylSemanticsIssue* rpyl_semantics_issue_at(const RpylSemanticsReport* report, size_t index) {
    if (!report || index >= report->issue_count) return 0;
    return &report->issues[index];
}
