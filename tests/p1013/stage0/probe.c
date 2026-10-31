#include "newlang/checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/copy_body.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Only the unchanged emitter translation unit is compiler-instrumented.
 * Numeric addresses identify a debug event; no graph/proof is fabricated. */
static uintptr_t reject_address;
static size_t rejection_count;
void __cyg_profile_func_enter(void *function, void *site)
{
    if ((uintptr_t)function == reject_address && rejection_count++ == 0)
        printf("{\"stage\":\"first_private_reject\",\"return_address\":"
               "\"0x%" PRIxPTR "\"}\n",
               (uintptr_t)site);
}
void __cyg_profile_func_exit(void *function, void *site)
{
    (void)function;
    (void)site;
}
typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *tree, *entry_tree;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
} Run;
typedef struct {
    const NLCheckedFragment *caller;
    NLCheckedNodeId call;
    NLCopyBodyView body;
    NLCopyBodyParameters parameters;
} Observation;
static void destroy(Run *r)
{
    nl_checked_destroy(r->entry);
    nl_syntax_tree_destroy(r->entry_tree);
    nl_parser_destroy(r->entry_parser);
    nl_source_destroy(r->entry_source);
    nl_semantic_destroy(r->context);
    nl_syntax_tree_destroy(r->tree);
    nl_parser_destroy(r->parser);
    nl_source_destroy(r->source);
    *r = (Run){0};
}
static int load(const char *path, Run *r)
{
    if (nl_source_load(path, &r->source) != NL_SOURCE_OK ||
        nl_parser_create(r->source, &r->parser) != NL_PARSE_OK)
        return 10;
    NLParseDiagnostic parse = {0};
    NLParseStatus ps =
        nl_parser_parse_function_unit(r->parser, &r->tree, &parse);
    printf("{\"stage\":\"parse\",\"status\":%d,\"code\":\"%s\"}\n", (int)ps,
           parse.diagnostic.code ? parse.diagnostic.code : "");
    if (ps != NL_PARSE_OK)
        return 11;
    if (nl_semantic_create(&r->context) != NL_CHECK_OK)
        return 12;
    const NLSyntaxTree *units[] = {r->tree};
    NLFunctionUnitDiagnostic definition = {0};
    NLCheckStatus s =
        nl_semantic_register_function_unit(r->context, units, 1, &definition);
    printf("{\"stage\":\"register\",\"status\":%d,\"code\":\"%s\"}\n", (int)s,
           definition.diagnostic.diagnostic.code
               ? definition.diagnostic.diagnostic.code
               : "");
    if (s != NL_CHECK_OK)
        return 13;
    if (nl_source_create("main()", 6, "<p1013-entry>", &r->entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(r->entry_source, &r->entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(r->entry_parser, &r->entry_tree,
                                            NULL) != NL_PARSE_OK)
        return 14;
    NLCheckDiagnostic checked = {0};
    s = nl_semantic_check_expression(r->context, r->entry_tree, &r->entry,
                                     &checked);
    printf("{\"stage\":\"CHECK\",\"status\":%d,\"code\":\"%s\"}\n", (int)s,
           checked.diagnostic.code ? checked.diagnostic.code : "");
    return s == NL_CHECK_OK ? 0 : 15;
}
static bool scalar(const NLSemanticContext *context, NLValueId id, size_t *out)
{
    NLSemanticValueView value;
    if (!nl_semantic_value_view(context, id, &value) || !value.scalar_known)
        return false;
    *out = value.scalar_value;
    return true;
}
static bool observe(const Run *run, Observation result[2], bool report)
{
    const NLCheckedFragment *caller =
        nl_checked_call_body(run->entry, nl_checked_root(run->entry));
    if (!caller)
        return false;
    size_t count = 0;
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(caller, id);
        if (!node || node->kind != NL_CHECKED_REGISTERED_CALL)
            continue;
        NLCopyBodyView body;
        NLCopyBodyParameters parameters;
        bool get_body = nl_checked_copy_body_view(caller, id, &body);
        bool get_parameters =
            nl_checked_copy_body_parameters(caller, id, &parameters);
        if (!get_body || !get_parameters)
            continue;
        if (count >= 2)
            return false;
        NLCheckStatus bs = nl_checked_copy_body_validate(caller, id, &body);
        NLCheckStatus ps =
            nl_checked_copy_body_parameters_validate(caller, id, &parameters);
        NLCopyBodyEventView event;
        bool eg = nl_checked_copy_body_event(caller, id, 0, &event);
        NLCheckStatus es =
            eg ? nl_checked_copy_body_event_validate(caller, id, 0, &event)
               : NL_CHECK_INTERNAL_ERROR;
        if (report)
            printf("{\"stage\":\"public\",\"ordinal\":%zu,\"call\":%zu,"
                   "\"function\":%zu,\"body_get\":true,\"parameters_get\":true,"
                   "\"body_validate\":%d,\"parameters_validate\":%d,"
                   "\"event_validate\":%d}\n",
                   count, id, body.function, (int)bs, (int)ps, (int)es);
        if (bs != NL_CHECK_OK || ps != NL_CHECK_OK || es != NL_CHECK_OK ||
            body.event_count != 1 || event.kind != NL_COPY_BODY_CHANGE)
            return false;
        NLSemanticPlaceView before, after, root, sibling0, sibling1;
        size_t old = 0, next = 0, old_field = 0, sibling = 0;
        if (!nl_semantic_place_view(parameters.entry, parameters.field.child,
                                    &before) ||
            !nl_semantic_place_view(parameters.handoff, parameters.field.child,
                                    &after) ||
            !nl_semantic_place_view(parameters.entry, parameters.field.parent,
                                    &root) ||
            root.fixed_field_count != 2 || parameters.field.index != 0 ||
            !nl_semantic_place_view(parameters.entry, root.fixed_fields[1],
                                    &sibling0) ||
            !nl_semantic_place_view(parameters.handoff, root.fixed_fields[1],
                                    &sibling1) ||
            !scalar(parameters.entry, before.current_value, &old_field) ||
            !scalar(parameters.handoff, body.result.value, &old) ||
            !scalar(parameters.handoff, after.current_value, &next) ||
            !scalar(parameters.handoff, sibling1.current_value, &sibling))
            return false;
        if (report)
            printf("{\"stage\":\"values\",\"ordinal\":%zu,\"old\":%zu,"
                   "\"before\":%zu,\"new\":%zu,\"sibling\":%zu,\"parent\":%zu,"
                   "\"child\":%zu,\"incarnation\":%zu,\"loan\":%zu,"
                   "\"sibling_fact_unchanged\":%s}\n",
                   count, old, old_field, next, sibling,
                   parameters.field.parent, parameters.field.child,
                   parameters.field.parent_incarnation, parameters.loan,
                   sibling0.current_fact == sibling1.current_fact ? "true"
                                                                  : "false");
        if (old != (count ? 67u : 41u) || old_field != old ||
            next != (count ? 109u : 67u) || sibling != 173 ||
            sibling0.current_fact != sibling1.current_fact ||
            sibling0.incarnation != sibling1.incarnation)
            return false;
        result[count++] = (Observation){caller, id, body, parameters};
    }
    if (report)
        printf("{\"stage\":\"count\",\"actual_current_calls\":%zu}\n", count);
    return count == 2 &&
           result[0].parameters.field.parent ==
               result[1].parameters.field.parent &&
           result[0].parameters.field.child ==
               result[1].parameters.field.child &&
           result[0].parameters.field.parent_incarnation ==
               result[1].parameters.field.parent_incarnation;
}
int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4)
        return 2;
    if (argc == 4)
        reject_address = (uintptr_t)strtoull(argv[3], NULL, 16);
    Run a = {0}, b = {0};
    Observation first[2], foreign[2];
    int code = load(argv[1], &a);
    if (code)
        goto cleanup;
    if (!observe(&a, first, true)) {
        code = 20;
        goto cleanup;
    }
    code = load(argv[1], &b);
    if (code)
        goto cleanup;
    if (!observe(&b, foreign, false)) {
        code = 21;
        goto cleanup;
    }
    for (size_t i = 0; i < 2; ++i) {
        NLCopyBodyParameters wrong = first[i].parameters;
        wrong.field.index ^= 1u;
        NLCheckStatus s0 = nl_checked_copy_body_validate(
            first[i].caller, first[i].call, &foreign[i].body);
        NLCheckStatus s1 = nl_checked_copy_body_parameters_validate(
            first[i].caller, first[i].call, &foreign[i].parameters);
        NLCheckStatus s2 = nl_checked_copy_body_validate(
            first[i].caller, first[i].call, &first[1 - i].body);
        NLCheckStatus s3 = nl_checked_copy_body_parameters_validate(
            first[i].caller, first[i].call, &wrong);
        printf("{\"stage\":\"negative\",\"ordinal\":%zu,"
               "\"foreign_body\":%d,\"foreign_parameters\":%d,"
               "\"other_actual_call_body\":%d,\"wrong_field\":%d}\n",
               i, (int)s0, (int)s1, (int)s2, (int)s3);
        if (s0 == NL_CHECK_OK || s1 == NL_CHECK_OK || s2 == NL_CHECK_OK ||
            s3 == NL_CHECK_OK) {
            code = 22;
            goto cleanup;
        }
    }
    if (strcmp(argv[2], "mixed") == 0) {
        char *text = NULL;
        size_t bytes = 0;
        NLNodeCStatus status = nl_checked_c_node(a.entry, &text, &bytes);
        printf("{\"stage\":\"node-C17\",\"status\":%d,\"bytes\":%zu,"
               "\"published\":%s}\n",
               (int)status, bytes, text ? "true" : "false");
        if (status != NL_NODE_C_OK && (text || bytes))
            code = 23;
        free(text);
    }
    destroy(&a);
    if (!observe(&b, foreign, false))
        code = 24;
    printf("{\"stage\":\"foreign_owner_after_A_destroy\",\"B_healthy\":%s}\n",
           code ? "false" : "true");
cleanup:
    destroy(&a);
    destroy(&b);
    return code;
}
