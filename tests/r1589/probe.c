#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* R-owned read-only observation. Labels below are transport labels, not
 * semantic identities or complete call/result/world certificates. */
typedef struct {
    const NLCheckedFragment *fragment;
    size_t label;
    NLCheckedNodeId *seen;
    size_t used, capacity;
    unsigned depth;
    bool main_body, hypothetical;
} Walk;

static size_t next_label;
static bool scalar_conflict;

static const char *parse_name(NLParseStatus status)
{
    switch (status) {
    case NL_PARSE_OK:
        return "OK";
    case NL_PARSE_LEXICALLY_UNSUPPORTED:
        return "LEXICALLY_UNSUPPORTED";
    case NL_PARSE_SYNTAX_UNSUPPORTED:
        return "SYNTAX_UNSUPPORTED";
    case NL_PARSE_SYNTAX_ERROR:
        return "SYNTAX_ERROR";
    case NL_PARSE_OUT_OF_MEMORY:
        return "OUT_OF_MEMORY";
    case NL_PARSE_RESOURCE_LIMIT:
        return "RESOURCE_LIMIT";
    case NL_PARSE_INTERNAL_ERROR:
        return "INTERNAL_ERROR";
    }
    return "UNKNOWN_STATUS";
}

static const char *check_name(NLCheckStatus status)
{
    switch (status) {
    case NL_CHECK_OK:
        return "OK";
    case NL_CHECK_SEMANTIC_ERROR:
        return "SEMANTIC_ERROR";
    case NL_CHECK_SEMANTIC_UNSUPPORTED:
        return "SEMANTIC_UNSUPPORTED";
    case NL_CHECK_ANALYSIS_PRECISION_LIMIT:
        return "ANALYSIS_PRECISION_LIMIT";
    case NL_CHECK_OUT_OF_MEMORY:
        return "OUT_OF_MEMORY";
    case NL_CHECK_RESOURCE_LIMIT:
        return "RESOURCE_LIMIT";
    case NL_CHECK_INTERNAL_ERROR:
        return "INTERNAL_ERROR";
    }
    return "UNKNOWN_STATUS";
}

static int parse_failure_result(NLParseStatus status, bool adapter)
{
    switch (status) {
    case NL_PARSE_LEXICALLY_UNSUPPORTED:
    case NL_PARSE_SYNTAX_UNSUPPORTED:
        return adapter ? 30 : 20;
    case NL_PARSE_SYNTAX_ERROR:
        return adapter ? 71 : 20;
    case NL_PARSE_OUT_OF_MEMORY:
    case NL_PARSE_RESOURCE_LIMIT:
        return 70;
    case NL_PARSE_OK:
    case NL_PARSE_INTERNAL_ERROR:
        return 71;
    }
    return 71;
}

static int check_failure_result(NLCheckStatus status, bool adapter)
{
    switch (status) {
    case NL_CHECK_SEMANTIC_ERROR:
        return adapter ? 71 : 10;
    case NL_CHECK_SEMANTIC_UNSUPPORTED:
    case NL_CHECK_ANALYSIS_PRECISION_LIMIT:
        return adapter ? 30 : 10;
    case NL_CHECK_OUT_OF_MEMORY:
    case NL_CHECK_RESOURCE_LIMIT:
        return 70;
    case NL_CHECK_OK:
    case NL_CHECK_INTERNAL_ERROR:
        return 71;
    }
    return 71;
}

static bool failure(const char *reason)
{
    printf("STOP\t%s\n", reason);
    fflush(stdout);
    return false;
}

static void print_name(const NLSource *source, NLSourceSpan span)
{
    NLSourceView text;
    if (!nl_source_view(source, span, &text)) {
        printf("UNAVAILABLE");
        return;
    }
    for (size_t i = 0; i < text.length; ++i) {
        printf("%02x", (unsigned)text.bytes[i]);
    }
}

static bool scalar(const NLSemanticContext *context, NLCheckedResult result,
                   bool *known, size_t *number, int *kind)
{
    NLSemanticValueView value;
    NLSemanticTypeView type;
    *known = false;
    *number = 0;
    *kind = -1;
    if (!nl_semantic_type_view(context, result.type, &type) ||
        !nl_semantic_value_view(context, result.value, &value)) {
        return false;
    }
    if (value.type != result.type) {
        scalar_conflict = true;
        return false;
    }
    *kind = (int)type.kind;
    *known = value.scalar_known;
    if (*known) {
        *number = value.scalar_value;
    }
    return true;
}

static bool walk_fragment(const NLCheckedFragment *fragment, size_t parent,
                          NLCheckedNodeId owner, const char *relation,
                          unsigned depth, bool main_body, bool hypothetical);

static bool walk_node(Walk *walk, NLCheckedNodeId id, unsigned depth)
{
    if (id == 0) {
        return true;
    }
    if (depth > 128) {
        return failure("R_APPARATUS_TRAVERSAL_LIMIT");
    }
    for (size_t i = 0; i < walk->used; ++i) {
        if (walk->seen[i] == id) {
            return true;
        }
    }
    if (walk->used == walk->capacity) {
        return failure("R_APPARATUS_TRAVERSAL_LIMIT");
    }
    walk->seen[walk->used++] = id;
    const NLCheckedNodeView *node;
    node = nl_checked_node_view(walk->fragment, id);
    if (node == NULL) {
        return failure("PUBLIC_LINK_NODE_UNAVAILABLE");
    }
    const NLSemanticContext *context;
    context = nl_checked_context(walk->fragment);
    printf("NODE\t%zu\t%zu\t%d\t%zu\t%zu\t%zu\n", walk->label, id,
           (int)node->kind, node->span.start_byte, node->span.end_byte,
           node->result_count);
    if (node->result_count > 2) {
        return failure("PUBLIC_RESULT_COUNT_OUT_OF_CONTRACT");
    }
    for (size_t i = 0; i < node->result_count; ++i) {
        bool known;
        size_t number;
        int kind;
        bool visible =
            scalar(context, node->results[i], &known, &number, &kind);
        if (scalar_conflict) {
            return failure("RESULT_VALUE_TYPE_INCONSISTENCY");
        }
        printf("VALUE\t%zu\t%zu\t%zu\t%zu\t%d\t%d\t%d\t%zu\n", walk->label, id,
               i, node->results[i].value, kind, (int)visible, (int)known,
               number);
    }
    if (node->kind == NL_CHECKED_BINDING) {
        const NLCheckedNodeView *init;
        init = nl_checked_node_view(walk->fragment, node->initializer);
        bool known = false, visible = false;
        size_t number = 0, count = 0;
        int kind = -1;
        if (init != NULL) {
            count = init->result_count;
            if (count == 1) {
                visible =
                    scalar(context, init->results[0], &known, &number, &kind);
            }
        }
        if (scalar_conflict) {
            return failure("RESULT_VALUE_TYPE_INCONSISTENCY");
        }
        printf("BINDING\t%zu\t%zu\t%d\t%zu\t", walk->label, id,
               (int)walk->main_body, node->initializer);
        print_name(nl_checked_source(walk->fragment), node->name);
        printf("\t%zu\t%d\t%d\t%zu\t%d\n", count, (int)visible, (int)known,
               number, kind);
    }
    if (node->kind == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView view;
        bool valid = nl_checked_usize_division_valid(walk->fragment, id, NULL);
        bool visible =
            nl_checked_usize_division_view(walk->fragment, id, &view);
        printf("DIV_STATUS\t%zu\t%zu\t%d\t%d\n", walk->label, id, (int)valid,
               (int)visible);
        if (!valid || !visible) {
            return failure("GENUINE_DIVISION_VALIDATION_OR_VIEW_FAILED");
        }
        printf("DIV\t%zu\t%zu\t%d\t%d\t%zu\t%zu\t%zu\t%zu\n", walk->label, id,
               (int)view.numerator_known, (int)view.quotient_known,
               view.numbers[0], view.numbers[1], view.quotient,
               view.target_max);
        if (view.origin != walk->fragment || view.node != id) {
            return failure("DIVISION_ORIGIN_MISMATCH");
        }
        if (view.numbers[1] == 0) {
            return failure("ACCEPTED_ZERO_DIVISOR");
        }
        if (view.numerator_known && view.quotient_known &&
            view.quotient != view.numbers[0] / view.numbers[1]) {
            return failure("KNOWN_DIVISION_NUMBER_MISMATCH");
        }
    }
    if (node->kind == NL_CHECKED_USIZE_REMAINDER) {
        NLUsizeRemainderView view;
        bool valid = nl_checked_usize_remainder_valid(walk->fragment, id, NULL);
        bool visible =
            nl_checked_usize_remainder_view(walk->fragment, id, &view);
        printf("REM_STATUS\t%zu\t%zu\t%d\t%d\n", walk->label, id, (int)valid,
               (int)visible);
        if (!valid || !visible) {
            return failure("GENUINE_REMAINDER_VALIDATION_OR_VIEW_FAILED");
        }
        printf("REM\t%zu\t%zu\t%d\t%d\t%zu\t%zu\t%zu\t%zu\n", walk->label, id,
               (int)view.numerator_known, (int)view.remainder_known,
               view.numbers[0], view.numbers[1], view.remainder,
               view.target_max);
        if (view.origin != walk->fragment || view.node != id ||
            view.source != nl_checked_source(walk->fragment)) {
            return failure("REMAINDER_ORIGIN_MISMATCH");
        }
        if (view.numbers[1] == 0) {
            return failure("ACCEPTED_ZERO_REMAINDER_DIVISOR");
        }
        if (view.numerator_known && view.remainder_known &&
            view.remainder != view.numbers[0] % view.numbers[1]) {
            return failure("KNOWN_REMAINDER_NUMBER_MISMATCH");
        }
    }
    if (!walk_node(walk, node->initializer, depth + 1) ||
        !walk_node(walk, node->tail, depth + 1)) {
        return false;
    }
    NLCheckedNodeId child = node->first_item;
    for (size_t i = 0; i < node->item_count; ++i) {
        const NLCheckedNodeView *item;
        item = nl_checked_node_view(walk->fragment, child);
        if (item == NULL || !walk_node(walk, child, depth + 1)) {
            return failure("PUBLIC_ITEM_RELATION_UNAVAILABLE");
        }
        child = item->next_item;
    }
    child = node->first_argument;
    for (size_t i = 0; i < node->argument_count; ++i) {
        const NLCheckedNodeView *argument;
        argument = nl_checked_node_view(walk->fragment, child);
        if (argument == NULL || !walk_node(walk, child, depth + 1)) {
            return failure("PUBLIC_ARGUMENT_RELATION_UNAVAILABLE");
        }
        child = argument->next_argument;
    }
    if (node->kind == NL_CHECKED_REGISTERED_CALL && node->body_backed) {
        const NLCheckedFragment *body;
        body = nl_checked_call_body(walk->fragment, id);
        if (body == NULL) {
            printf("LIMIT\tCALL_BODY_NOT_OBSERVABLE\t%zu\t%zu\n", walk->label,
                   id);
        } else {
            if (nl_checked_context(body) != context) {
                return failure("CALL_BODY_CONTEXT_CONTRACT_MISMATCH");
            }
            if (!walk_fragment(body, walk->label, id, "call", walk->depth + 1,
                               false, walk->hypothetical)) {
                return false;
            }
        }
    }
    if (node->kind == NL_CHECKED_IF) {
        printf("IF\t%zu\t%zu\t%zu\n", walk->label, id, node->normal_arms);
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedFragment *arm;
            arm = nl_checked_if_arm(walk->fragment, id, i);
            if (arm == NULL) {
                printf("LIMIT\tIF_ARM_NOT_OBSERVABLE\t%zu\t%zu\t%zu\n",
                       walk->label, id, i);
            } else if (!walk_fragment(arm, walk->label, id,
                                      i == 0 ? "then" : "else", walk->depth + 1,
                                      false, true)) {
                return false;
            }
        }
    }
    return true;
}

static bool walk_fragment(const NLCheckedFragment *fragment, size_t parent,
                          NLCheckedNodeId owner, const char *relation,
                          unsigned depth, bool main_body, bool hypothetical)
{
    size_t count = nl_checked_node_count(fragment);
    if (depth > 64 || count > 16384 || count == 0) {
        return failure("R_APPARATUS_FRAGMENT_LIMIT");
    }
    Walk walk = {0};
    walk.fragment = fragment;
    walk.label = ++next_label;
    walk.capacity = count;
    walk.depth = depth;
    walk.main_body = main_body;
    walk.hypothetical = hypothetical;
    walk.seen = calloc(count, sizeof(*walk.seen));
    if (walk.seen == NULL) {
        return failure("R_APPARATUS_OUT_OF_MEMORY");
    }
    printf("FRAGMENT\t%zu\t%zu\t%zu\t%s\t%d\t%zu\n", walk.label, parent, owner,
           relation, (int)hypothetical, count);
    bool ok = walk_node(&walk, nl_checked_root(fragment), 0);
    free(walk.seen);
    return ok;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: r1589_public_probe SOURCE lawful|invalid\n");
        return 70;
    }
    printf("TYPE_KIND\tUSIZE\t%d\n", (int)NL_TYPE_USIZE);
    int result = 70;
    NLSource *source = NULL, *adapter_source = NULL;
    NLParser *parser = NULL, *adapter_parser = NULL;
    NLSyntaxTree *tree = NULL, *adapter_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic parse_diagnostic = {0};
    NLFunctionUnitDiagnostic unit_diagnostic = {0};
    NLCheckDiagnostic check_diagnostic = {0};
    NLSourceStatus source_status = nl_source_load(argv[1], &source);
    printf("STAGE\tsource_load\t%d\n", (int)source_status);
    fflush(stdout);
    if (source_status != NL_SOURCE_OK) {
        goto done;
    }
    printf("INPUT_LENGTH\t%zu\n", nl_source_length(source));
    NLParseStatus parse_status = nl_parser_create(source, &parser);
    printf("STAGE\tparser_create\t%s\n", parse_name(parse_status));
    if (parse_status != NL_PARSE_OK) {
        result = parse_status == NL_PARSE_OUT_OF_MEMORY ? 70 : 71;
        goto done;
    }
    parse_status =
        nl_parser_parse_function_unit(parser, &tree, &parse_diagnostic);
    printf("STAGE\tparse_unit\t%s\n", parse_name(parse_status));
    fflush(stdout);
    if (parse_status != NL_PARSE_OK) {
        (void)nl_parse_diagnostic_render(stderr, source, &parse_diagnostic);
        result = parse_failure_result(parse_status, false);
        goto done;
    }
    NLCheckStatus check_status = nl_semantic_create(&context);
    printf("STAGE\tcontext_create\t%s\n", check_name(check_status));
    if (check_status != NL_CHECK_OK) {
        result = check_status == NL_CHECK_OUT_OF_MEMORY ? 70 : 71;
        goto done;
    }
    const NLSyntaxTree *inputs[] = {tree};
    check_status = nl_semantic_register_function_unit(context, inputs, 1,
                                                      &unit_diagnostic);
    printf("STAGE\tregister_unit\t%s\n", check_name(check_status));
    fflush(stdout);
    if (check_status != NL_CHECK_OK) {
        printf("DIAGNOSTIC_INPUT\t%zu\n", unit_diagnostic.input_index);
        (void)nl_check_diagnostic_render(stderr, source,
                                         &unit_diagnostic.diagnostic);
        result = check_failure_result(check_status, false);
        goto done;
    }
    if (strcmp(argv[2], "invalid") == 0) {
        (void)failure("TRUE_INVALID_ACCEPTED_AT_DEFINITION_CHECK");
        result = 81;
        goto done;
    }
    const char adapter[] = "main()\n";
    source_status =
        nl_source_create(adapter, sizeof(adapter) - 1,
                         "R1589_SYNTHETIC_MAIN_ADAPTER", &adapter_source);
    printf("STAGE\tadapter_source\t%d\n", (int)source_status);
    if (source_status != NL_SOURCE_OK) {
        goto done;
    }
    parse_status = nl_parser_create(adapter_source, &adapter_parser);
    printf("STAGE\tadapter_parser_create\t%s\n", parse_name(parse_status));
    if (parse_status != NL_PARSE_OK) {
        result = parse_status == NL_PARSE_OUT_OF_MEMORY ? 70 : 71;
        goto done;
    }
    parse_status = nl_parser_parse_expression_fragment(
        adapter_parser, &adapter_tree, &parse_diagnostic);
    printf("STAGE\tadapter_parse\t%s\n", parse_name(parse_status));
    if (parse_status != NL_PARSE_OK) {
        (void)nl_parse_diagnostic_render(stderr, adapter_source,
                                         &parse_diagnostic);
        result = parse_failure_result(parse_status, true);
        goto done;
    }
    check_status = nl_semantic_check_expression(context, adapter_tree, &checked,
                                                &check_diagnostic);
    printf("STAGE\tadapter_check\t%s\n", check_name(check_status));
    fflush(stdout);
    if (check_status != NL_CHECK_OK) {
        (void)nl_check_diagnostic_render(stderr, adapter_source,
                                         &check_diagnostic);
        result = check_failure_result(check_status, true);
        goto done;
    }
    NLCheckedNodeId root = nl_checked_root(checked);
    const NLCheckedNodeView *call = nl_checked_node_view(checked, root);
    if (call == NULL || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed) {
        (void)failure("SYNTHETIC_MAIN_BODY_NOT_OBSERVABLE");
        result = 31;
        goto done;
    }
    const NLCheckedFragment *body = nl_checked_call_body(checked, root);
    if (body == NULL) {
        (void)failure("SYNTHETIC_MAIN_BODY_NOT_OBSERVABLE");
        result = 31;
        goto done;
    }
    printf("ADAPTER\tmain()\t%zu\tNOT_ORIGINAL_NATIVE_SOURCE\n", root);
    if (nl_checked_context(body) != nl_checked_context(checked)) {
        (void)failure("MAIN_BODY_CONTEXT_CONTRACT_MISMATCH");
        result = 71;
        goto done;
    }
    result = walk_fragment(body, 0, root, "registered_main", 0, true, false)
                 ? 0
                 : 71;
    printf("LIMIT\tWHOLE_CALL_RESULT_WORLD_CUSTODY\tNOT_OBSERVABLE\n");
    printf("LIMIT\tSELECTED_RUNTIME_ARM_AND_QUANTITY\tNOT_OBSERVABLE\n");
done:
    fflush(stdout);
    fflush(stderr);
    nl_checked_destroy(checked);
    nl_syntax_tree_destroy(adapter_tree);
    nl_parser_destroy(adapter_parser);
    nl_source_destroy(adapter_source);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_semantic_destroy(context);
    nl_source_destroy(source);
    return result;
}
