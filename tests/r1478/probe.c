#include "copy_body_internal.h"
#include "h_origin_internal.h"
#include "newlang/checked_c_node.h"
#include "newlang/parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    NLSource *source;
    NLSource *call_source;
    NLParser *parser;
    NLParser *call_parser;
    NLSyntaxTree *unit;
    NLSyntaxTree *call;
    NLSemanticContext *context;
    NLCheckedFragment *owned;
} RProgram;

typedef struct {
    size_t trials;
    size_t terminals;
    size_t installers;
    size_t payload_reads;
} RCounts;

static void release_program(RProgram *p)
{
    nl_checked_destroy(p->owned);
    nl_syntax_tree_destroy(p->call);
    nl_parser_destroy(p->call_parser);
    nl_source_destroy(p->call_source);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
}

static bool read_source(const char *path, RProgram *p)
{
    return nl_source_load(path, &p->source) == NL_SOURCE_OK &&
           nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
           nl_parser_parse_function_unit(p->parser, &p->unit, NULL) ==
               NL_PARSE_OK &&
           nl_semantic_create(&p->context) == NL_CHECK_OK &&
           nl_source_create("main()", 6, "r1478-actual",
                            &p->call_source) == NL_SOURCE_OK &&
           nl_parser_create(p->call_source, &p->call_parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->call_parser, &p->call,
                                               NULL) == NL_PARSE_OK;
}

static bool has_code(const char *actual, const char *expected)
{
    return actual != NULL && strcmp(actual, expected) == 0;
}

static bool inspect(const NLCheckedFragment *f, RCounts *count,
                    unsigned depth)
{
    if (f == NULL || depth > 24)
        return false;
    const NLSemanticContext *context = nl_checked_context(f);
    const NLTypeId u8 = nl_semantic_core_type(context, NL_TYPE_U8);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n == NULL)
            return false;
        if (n->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
            ++count->trials;
        if (n->kind == NL_CHECKED_LINK_READ && n->type == u8 &&
            n->field.present && n->field.index == 1 &&
            n->field.access == NL_ACCESS_READ)
            ++count->payload_reads;
        if (n->kind == NL_CHECKED_MATCH && n->normal_arms == 1) {
            const NLCheckedNodeView *trial =
                nl_checked_node_view(f, n->initializer);
            if (trial != NULL && trial->kind == NL_CHECKED_TRY_ALLOCATE_ONE) {
                const NLCheckedFragment *arm =
                    nl_checked_match_arm(f, id, 0);
                const NLCheckedNodeView *end =
                    arm == NULL
                        ? NULL
                        : nl_checked_node_view(arm, nl_checked_root(arm));
                if (end == NULL || !end->terminates ||
                    end->result_count != 0)
                    return false;
                ++count->terminals;
            }
        }
        if (n->kind == NL_CHECKED_REGISTERED_CALL && n->type == u8) {
            if (n->function == 0 || n->function > context->function_count ||
                context->functions == NULL)
                return false;
            const NLFunctionEntry *fn =
                &context->functions[n->function - 1];
            if (strcmp(fn->name, "install_metric") == 0) {
                NLCopyBodyView view = {0};
                NLCopyBodyParameters p = {0};
                NLCopyBodyEventView event = {0};
                if (!nl_checked_copy_body_view(f, id, &view) ||
                    !nl_checked_copy_body_parameters(f, id, &p) ||
                    !nl_checked_copy_body_event(f, id, 0, &event) ||
                    nl_checked_copy_body_validate(f, id, &view) !=
                        NL_CHECK_OK ||
                    nl_checked_copy_body_parameters_validate(f, id, &p) !=
                        NL_CHECK_OK ||
                    nl_checked_copy_body_event_validate(f, id, 0, &event) !=
                        NL_CHECK_OK ||
                    view.event_count != 1 ||
                    event.kind != NL_COPY_BODY_CHANGE || !p.field.present ||
                    p.field.index != 0 ||
                    p.field.access != NL_ACCESS_WRITE ||
                    p.field.parent == 0 || p.field.child == 0)
                    return false;
                NLSemanticPlaceView before = {0}, after = {0};
                if (!nl_semantic_place_view(p.entry, p.field.child,
                                            &before) ||
                    !nl_semantic_place_view(p.handoff, p.field.child,
                                            &after) ||
                    !before.live || !after.live ||
                    before.incarnation != after.incarnation ||
                    before.current_fact == after.current_fact)
                    return false;
                ++count->installers;
            }
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body != NULL && !inspect(body, count, depth + 1))
            return false;
        if (n->kind == NL_CHECKED_MATCH || n->kind == NL_CHECKED_IF) {
            for (size_t i = 0; i < n->item_count; ++i) {
                const NLCheckedFragment *arm =
                    n->kind == NL_CHECKED_MATCH
                        ? nl_checked_match_arm(f, id, i)
                        : nl_checked_if_arm(f, id, i);
                if (arm == NULL || !inspect(arm, count, depth + 1))
                    return false;
            }
        }
    }
    return true;
}

static int inspect_source(const char *kind, const char *path)
{
    const bool negative = strcmp(kind, "negative") == 0;
    RProgram p = {0};
    int result = 125;
    if (!read_source(path, &p)) {
        printf("R1478 SOURCE_NOT_REACHED (parse/registration input)\n");
        goto done;
    }
    NLFunctionUnitDiagnostic definition = {0};
    const NLSyntaxTree *units[] = {p.unit};
    NLCheckStatus status =
        nl_semantic_register_function_unit(p.context, units, 1,
                                            &definition);
    const char *definition_code = definition.diagnostic.diagnostic.code;
    printf("R1478 source registration=%d diagnostic=%s\n", (int)status,
           definition_code ? definition_code : "none");
    if (status != NL_CHECK_OK) {
        if (negative && status == NL_CHECK_SEMANTIC_ERROR &&
            has_code(definition_code, "ALLOCATED-DOMAIN-MISMATCH")) {
            result = 0;
            printf("R1478 WRONG_DOMAIN_CHECK_REJECT (definition)\n");
        }
        goto done;
    }

    NLCheckDiagnostic diagnostic = {0};
    status = nl_semantic_check_expression(p.context, p.call, &p.owned,
                                          &diagnostic);
    printf("R1478 actual CHECK=%d diagnostic=%s\n", (int)status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "none");
    if (negative) {
        if (status == NL_CHECK_OK) {
            result = 1;
            fprintf(stderr,
                    "R1478 STOP: wrong Domain accepted; guest prohibited\n");
        } else if (status == NL_CHECK_SEMANTIC_ERROR &&
                   has_code(diagnostic.diagnostic.code,
                            "ALLOCATED-DOMAIN-MISMATCH")) {
            result = 0;
            printf("R1478 WRONG_DOMAIN_CHECK_REJECT (actual)\n");
        }
        goto done;
    }
    if (status != NL_CHECK_OK || p.owned == NULL)
        goto done;
    if (nl_checked_proof_class(p.owned) != NL_CHECKED_MACHINE_PROVED ||
        !nl_h_custody_result_valid(p.owned, nl_checked_root(p.owned)) ||
        !nl_h_origin_validate(p.owned)) {
        fprintf(stderr,
                "R1478 STOP: positive reached CHECK but no valid MACHINE\n");
        result = 1;
        goto done;
    }
    RCounts count = {0};
    if (!inspect(p.owned, &count, 0) || count.trials != 2 ||
        count.terminals != 2 || count.installers != 2 ||
        count.payload_reads != 1) {
        fprintf(stderr,
                "R1478 STOP: public original/return/Copy trace mismatch: "
                "trials=%zu returns=%zu installers=%zu payload=%zu\n",
                count.trials, count.terminals, count.installers,
                count.payload_reads);
        result = 1;
        goto done;
    }
    char *text = NULL;
    size_t length = 0;
    NLNodeCStatus emitted = nl_checked_c_node(p.owned, &text, &length);
    if (emitted != NL_NODE_C_OK || text == NULL ||
        length != strlen(text)) {
        printf("R1478 C17_NOT_REACHED status=%d\n", (int)emitted);
        result = 125;
    } else {
        printf("R1478 CHECK+AUTHENTIC_MACHINE+C17_READY "
               "trials=%zu terminals=%zu installers=%zu payload=%zu\n",
               count.trials, count.terminals, count.installers,
               count.payload_reads);
        result = 0;
    }
    free(text);

done:
    release_program(&p);
    return result;
}

int main(int argc, char **argv)
{
    if (argc != 3 ||
        (strcmp(argv[1], "positive") != 0 &&
         strcmp(argv[1], "negative") != 0))
        return 2;
    return inspect_source(argv[1], argv[2]);
}
