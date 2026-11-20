#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *tree, *entry_tree;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
} Program;

typedef struct {
    const NLCheckedFragment *body;
    NLCheckedNodeId node, call;
    NLUsizeRemainderView remainder;
    NLUsizeDivisionView division;
} Event;

static bool require(bool condition, const char *message)
{
    if (!condition)
        fprintf(stderr, "R1557 STOP: %s\n", message);
    return condition;
}

static void release_program(Program *p)
{
    nl_checked_destroy(p->entry);
    nl_syntax_tree_destroy(p->entry_tree);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->tree);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    *p = (Program){0};
}

static bool register_program(Program *p, const char *path, const char *label,
                             const char *negative)
{
    NLParseDiagnostic pd = {0};
    NLFunctionUnitDiagnostic ud = {0};
    if (!require(nl_source_load(path, &p->source) == NL_SOURCE_OK,
                 "source load"))
        return false;
    if (!require(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK,
                 "parser create"))
        return false;
    NLParseStatus parse =
        nl_parser_parse_function_unit(p->parser, &p->tree, &pd);
    printf("R1557 PARSE label=%s status=%d code=%s\n", label, (int)parse,
           pd.diagnostic.code ? pd.diagnostic.code : "-");
    if (parse != NL_PARSE_OK) {
        (void)nl_parse_diagnostic_render(stderr, p->source, &pd);
        return require(false, "whole unit SOURCE refused");
    }
    if (!require(nl_semantic_create(&p->context) == NL_CHECK_OK,
                 "semantic context"))
        return false;
    const NLSyntaxTree *inputs[] = {p->tree};
    NLCheckStatus status =
        nl_semantic_register_function_unit(p->context, inputs, 1, &ud);
    printf("R1557 REGISTER label=%s status=%d code=%s input=%zu\n", label,
           (int)status, ud.diagnostic.diagnostic.code
                           ? ud.diagnostic.diagnostic.code
                           : "-",
           ud.input_index);
    if (negative) {
        if (ud.diagnostic.diagnostic.code)
            (void)nl_check_diagnostic_render(stdout, p->source,
                                             &ud.diagnostic);
        return require(status == NL_CHECK_SEMANTIC_ERROR,
                       "negative must fail semantic registration") &&
               require(ud.diagnostic.diagnostic.code != NULL &&
                           strcmp(ud.diagnostic.diagnostic.code, negative) ==
                               0,
                       "negative first semantic diagnostic");
    }
    if (status != NL_CHECK_OK) {
        if (ud.diagnostic.diagnostic.code)
            (void)nl_check_diagnostic_render(stderr, p->source,
                                             &ud.diagnostic);
        return require(false, "positive definition CHECK refused");
    }
    return true;
}

static bool enter_main(Program *p, const char *label)
{
    const char entry[] = "main()";
    NLParseDiagnostic pd = {0};
    NLCheckDiagnostic cd = {0};
    if (!require(nl_source_create(entry, sizeof(entry) - 1, "<r1557-entry>",
                                  &p->entry_source) == NL_SOURCE_OK,
                 "ordinary main entry source") ||
        !require(nl_parser_create(p->entry_source, &p->entry_parser) ==
                     NL_PARSE_OK,
                 "ordinary main entry parser") ||
        !require(nl_parser_parse_expression_fragment(
                     p->entry_parser, &p->entry_tree, &pd) == NL_PARSE_OK,
                 "ordinary main entry syntax"))
        return false;
    NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->entry_tree, &p->entry, &cd);
    printf("R1557 ENTRY label=%s status=%d code=%s\n", label, (int)status,
           cd.diagnostic.code ? cd.diagnostic.code : "-");
    if (status != NL_CHECK_OK) {
        if (cd.diagnostic.code)
            (void)nl_check_diagnostic_render(stderr, p->entry_source, &cd);
        return require(false, "ordinary main CHECK refused");
    }
    NLSemanticTypeView type = {0};
    NLTypeId usize = nl_semantic_core_type(p->context, NL_TYPE_USIZE);
    if (!require(nl_semantic_type_view(p->context, usize, &type),
                 "target type view"))
        return false;
    printf("R1557 TARGET label=%s bits=%zu max=%zu usize=%zu kind=%d "
           "layout=%d size=%zu alignment=%zu\n",
           label, sizeof(size_t) * CHAR_BIT, SIZE_MAX, usize, (int)type.kind,
           (int)type.layout_known, type.size, type.alignment);
    return require(sizeof(size_t) * CHAR_BIT == 64,
                   "frozen T64 target assumption") &&
           require(type.kind == NL_TYPE_USIZE && type.is_copy &&
                       type.is_discardable,
                   "genuine quantity core type") &&
           require(nl_checked_proof_class(p->entry) ==
                       NL_CHECKED_MACHINE_PROVED,
                   "ordinary machine-checked entry class");
}

static bool scalar(const NLSemanticContext *context, NLValueId id,
                    size_t expected, const char *label)
{
    NLSemanticValueView value = {0};
    if (!require(nl_semantic_value_view(context, id, &value),
                 "actual scalar value view"))
        return false;
    printf("R1557 VALUE label=%s context=%p id=%zu type=%zu known=%d "
           "number=%zu carrier=%d\n",
           label, (const void *)context, id, value.type,
           (int)value.scalar_known, value.scalar_value, (int)value.carrier);
    return require(value.type ==
                       nl_semantic_core_type(context, NL_TYPE_USIZE),
                   "scalar core type identity") &&
           require(value.scalar_known,
                   "INCONCLUSIVE actual call scalar observation unavailable") &&
           require(value.scalar_value == expected,
                   "actual source scalar differs from oracle");
}

static bool name_is(const NLCheckedFragment *body, NLSourceSpan span,
                     const char *name)
{
    NLSourceView text = {0};
    return nl_source_view(nl_checked_source(body), span, &text) &&
           text.length == strlen(name) &&
           memcmp(text.bytes, name, text.length) == 0;
}

static bool inspect_bindings(const NLCheckedFragment *body, size_t first,
                              size_t second, bool caller)
{
    const NLSemanticContext *context = nl_checked_context(body);
    size_t matched = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, id);
        if (!require(node != NULL, "checked node inventory"))
            return false;
        if (node->kind != NL_CHECKED_BINDING)
            continue;
        size_t expected = first;
        bool selected = false;
        if (caller) {
            selected = name_is(body, node->name, "first_result") ||
                       name_is(body, node->name, "first_copy");
            if (name_is(body, node->name, "second_result") ||
                name_is(body, node->name, "second_copy")) {
                selected = true;
                expected = second;
            }
        } else {
            selected = name_is(body, node->name, "residual") ||
                       name_is(body, node->name, "quotient") ||
                       name_is(body, node->name, "forwarded");
        }
        if (!selected)
            continue;
        ++matched;
        const NLCheckedNodeView *init =
            nl_checked_node_view(body, node->initializer);
        printf("R1557 BINDING body=%p node=%zu symbol=%zu initializer=%zu "
               "span=%zu:%zu\n",
               (const void *)body, id, node->symbol, node->initializer,
               node->name.start_byte, node->name.end_byte);
        if (!require(init && init->result_count == 1,
                     "source binding actual initializer result") ||
            !scalar(context, init->results[0].value, expected,
                    caller ? "caller-binding" : "helper-binding"))
            return false;
        if (init->kind == NL_CHECKED_IDENTIFIER &&
            !require(init->value_use == NL_VALUE_COPIED,
                     "ordinary source Copy use"))
            return false;
    }
    return require(matched == (caller ? 4u : 2u),
                   "source-mapped binding count");
}

static bool inspect_result(const NLCheckedFragment *body, bool explicit_return,
                            size_t expected)
{
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    if (!require(root && root->kind == NL_CHECKED_BLOCK,
                 "helper body root block"))
        return false;
    size_t returns = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, id);
        if (node->kind != NL_CHECKED_RETURN)
            continue;
        ++returns;
        printf("R1557 RETURN body=%p node=%zu returned_type=%zu "
               "returned_value=%zu terminates=%d\n",
               (const void *)body, id, node->returned.type,
               node->returned.value, (int)node->terminates);
        if (!require(node->terminates, "explicit RETURN terminates") ||
            !scalar(nl_checked_context(body), node->returned.value, expected,
                    "explicit-return"))
            return false;
    }
    if (explicit_return)
        return require(returns == 1, "one actual RETURN edge");
    if (!require(returns == 0 && root->tail != 0,
                 "normal result tail identity"))
        return false;
    const NLCheckedNodeView *tail = nl_checked_node_view(body, root->tail);
    printf("R1557 TAIL body=%p node=%zu\n", (const void *)body, root->tail);
    return require(tail && tail->result_count == 1,
                   "normal tail scalar result") &&
           scalar(nl_checked_context(body), tail->results[0].value, expected,
                  "normal-tail");
}

static bool inspect_program(Program *p, const char *label, bool division,
                             bool explicit_return, const size_t inputs[2],
                             const size_t results[2], Event events[2])
{
    const NLCheckedNodeId entry_id = nl_checked_root(p->entry);
    const NLCheckedNodeView *entry =
        nl_checked_node_view(p->entry, entry_id);
    if (!require(entry && entry->kind == NL_CHECKED_REGISTERED_CALL &&
                     entry->body_backed && entry->result_count == 0,
                 "whole unit ordinary main call"))
        return false;
    const NLCheckedFragment *main_body =
        nl_checked_call_body(p->entry, entry_id);
    if (!require(main_body && nl_checked_context(main_body) == p->context,
                 "main owned body/world custody"))
        return false;
    size_t calls = 0;
    size_t function = 0;
    for (NLCheckedNodeId call = 1;
         call <= nl_checked_node_count(main_body); ++call) {
        const NLCheckedNodeView *node = nl_checked_node_view(main_body, call);
        if (node->kind != NL_CHECKED_REGISTERED_CALL)
            continue;
        if (!require(calls < 2 && node->body_backed &&
                         node->argument_count == 1 &&
                         node->result_count == 1,
                     "two unary actual helper calls"))
            return false;
        const NLCheckedFragment *body =
            nl_checked_call_body(main_body, call);
        if (!require(body && nl_checked_context(body) == p->context,
                     "actual child body same qualified context"))
            return false;
        if (calls == 0)
            function = node->function;
        if (!require(node->function == function, "same helper identity"))
            return false;
        const NLCheckedNodeView *argument =
            nl_checked_node_view(main_body, node->first_argument);
        printf("R1557 CALL label=%s index=%zu caller=%p call=%zu function=%zu "
               "body=%p context=%p source=%p result=%zu\n",
               label, calls, (const void *)main_body, call, node->function,
               (const void *)body, (const void *)p->context,
               (const void *)nl_checked_source(body), node->results[0].value);
        if (!require(argument && argument->result_count == 1,
                     "actual source call argument") ||
            !scalar(p->context, argument->results[0].value, inputs[calls],
                    "call-argument") ||
            !scalar(p->context, node->results[0].value, results[calls],
                    "call-result"))
            return false;
        size_t operations = 0;
        for (NLCheckedNodeId op = 1; op <= nl_checked_node_count(body); ++op) {
            const NLCheckedNodeView *operation =
                nl_checked_node_view(body, op);
            NLCheckedKind wanted = division ? NL_CHECKED_USIZE_DIVISION
                                            : NL_CHECKED_USIZE_REMAINDER;
            if (operation->kind != wanted)
                continue;
            ++operations;
            events[calls].body = body;
            events[calls].node = op;
            events[calls].call = call;
            if (division) {
                NLUsizeDivisionView *view = &events[calls].division;
                if (!require(nl_checked_usize_division_view(body, op, view) &&
                                 nl_checked_usize_division_valid(body, op,
                                                                 NULL) &&
                                 nl_checked_usize_division_valid(body, op,
                                                                 view),
                             "own actual division accessor/NULL/view"))
                    return false;
                printf("R1557 DIVISION label=%s call=%zu node=%zu result=%zu "
                       "num_known=%d numerator=%zu divisor=%zu "
                       "known=%d quotient=%zu max=%zu\n",
                       label, call, op, view->result,
                       (int)view->numerator_known, view->numbers[0],
                       view->numbers[1], (int)view->quotient_known,
                       view->quotient, view->target_max);
                if (!require(view->origin == body && view->node == op &&
                                 view->target_max == SIZE_MAX,
                             "division event origin/target") ||
                    !scalar(p->context, view->result, results[calls],
                            "division-result"))
                    return false;
            } else {
                NLUsizeRemainderView *view = &events[calls].remainder;
                if (!require(nl_checked_usize_remainder_view(body, op, view) &&
                                 nl_checked_usize_remainder_valid(body, op,
                                                                  NULL) &&
                                 nl_checked_usize_remainder_valid(body, op,
                                                                  view),
                             "own actual remainder accessor/NULL/view"))
                    return false;
                printf("R1557 REMAINDER label=%s call=%zu node=%zu result=%zu "
                       "source=%p num_known=%d numerator=%zu divisor=%zu "
                       "known=%d remainder=%zu max=%zu\n",
                       label, call, op, view->result, (const void *)view->source,
                       (int)view->numerator_known, view->numbers[0],
                       view->numbers[1], (int)view->remainder_known,
                       view->remainder, view->target_max);
                if (!require(view->origin == body && view->node == op &&
                                 view->source == nl_checked_source(body) &&
                                 view->target_max == SIZE_MAX,
                             "remainder event origin/source/target") ||
                    !scalar(p->context, view->result, results[calls],
                            "remainder-result"))
                    return false;
            }
        }
        if (!require(operations == 1, "one source arithmetic operation") ||
            !inspect_bindings(body, results[calls], results[calls], false) ||
            !inspect_result(body, explicit_return, results[calls]))
            return false;
        ++calls;
    }
    return require(calls == 2, "actual helper call count") &&
           inspect_bindings(main_body, results[0], results[1], true);
}

static bool authenticate(Event events[2], Event duplicate[2], Event division[2],
                          Program *p2, Program *copy)
{
    Event *a = &events[0], *b = &events[1], *foreign = &duplicate[0];
    if (!require(p2->context != copy->context &&
                     a->remainder.source != foreign->remainder.source,
                 "observed distinct original source/context"))
        return false;
    bool rejected_source = !nl_checked_usize_remainder_valid(
        a->body, a->node, &foreign->remainder);
    bool rejected_call =
        !nl_checked_usize_remainder_valid(a->body, a->node, &b->remainder);
    printf("R1557 AUTH foreign_source_rejected=%d foreign_call_rejected=%d "
           "source_a=%p source_b=%p call_a=%zu call_b=%zu\n",
           (int)rejected_source, (int)rejected_call,
           (const void *)a->remainder.source,
           (const void *)foreign->remainder.source, a->call, b->call);
    if (!require(a->body != b->body && a->call != b->call,
                 "distinct actual call event") ||
        !require(rejected_source && rejected_call, "foreign view rejection"))
        return false;
    NLSemanticValueView va = {0}, vb = {0};
    if (!require(nl_semantic_value_view(p2->context, a->remainder.result,
                                        &va) &&
                     nl_semantic_value_view(p2->context, b->remainder.result,
                                             &vb),
                 "two genuine result observations") ||
        !require(a->remainder.result != b->remainder.result &&
                     va.scalar_known && vb.scalar_known &&
                     va.scalar_value == vb.scalar_value,
                 "INCONCLUSIVE distinct same-valued results unavailable"))
        return false;
    NLUsizeRemainderView mixed = a->remainder;
    mixed.result = b->remainder.result;
    bool rejected_result =
        !nl_checked_usize_remainder_valid(a->body, a->node, &mixed);
    printf("R1557 AUTH mixed_result_rejected=%d id_a=%zu id_b=%zu "
           "observed_a=%zu observed_b=%zu\n",
           (int)rejected_result, a->remainder.result, b->remainder.result,
           va.scalar_value, vb.scalar_value);
    if (!require(rejected_result, "same-valued foreign result must reject"))
        return false;
    NLUsizeRemainderView output = {0};
    bool accessor_rejects = !nl_checked_usize_remainder_view(
        division[0].body, division[0].node, &output);
    bool validator_rejects = !nl_checked_usize_remainder_valid(
        division[0].body, division[0].node, NULL);
    const NLUsizeDivisionView *dv = &division[0].division;
    if (!require(dv->numerator_known && dv->quotient_known &&
                     a->remainder.numerator_known &&
                     a->remainder.remainder_known &&
                     dv->numbers[0] == a->remainder.numbers[0] &&
                     dv->numbers[1] == a->remainder.numbers[1] &&
                     dv->quotient == a->remainder.remainder,
                 "actual equal operands and numeric bits for both operations"))
        return false;
    printf("R1557 AUTH division_own_valid=1 division_as_remainder_view=%d "
           "division_as_remainder_null=%d numerator=%zu divisor=%zu "
           "division_number=%zu remainder_number=%zu\n",
           (int)!accessor_rejects, (int)!validator_rejects, dv->numbers[0],
           dv->numbers[1], dv->quotient, a->remainder.remainder);
    return require(accessor_rejects && validator_rejects,
                   "division cannot authenticate as remainder");
}

int main(int argc, char **argv)
{
    if (argc != 8) {
        fprintf(stderr, "R1557 requires p1 p2 auth n1 n2 n3 n4 paths\n");
        return 2;
    }
    Program p1 = {0}, p2 = {0}, copy = {0}, division = {0};
    Event first[2] = {0}, second[2] = {0}, duplicate[2] = {0}, control[2] = {0};
    const size_t first_inputs[] = {143, 153}, first_results[] = {7, 0};
    const size_t second_inputs[] = {0, 65535}, second_results[] = {0, 0};
    const size_t division_inputs[] = {0, 7}, division_results[] = {0, 7};
    const char *negative_codes[] = {
        "USIZE-REMAINDER-ZERO", "USIZE-REMAINDER-TYPE",
        "USIZE-LITERAL-RANGE", "P8-BODY-RESULT"};
    bool ok = register_program(&p1, argv[1], "p1", NULL) &&
              enter_main(&p1, "p1") &&
              inspect_program(&p1, "p1", false, false, first_inputs,
                              first_results, first) &&
              register_program(&p2, argv[2], "p2", NULL) &&
              enter_main(&p2, "p2") &&
              inspect_program(&p2, "p2", false, true, second_inputs,
                              second_results, second) &&
              register_program(&copy, argv[2], "p2-duplicate", NULL) &&
              enter_main(&copy, "p2-duplicate") &&
              inspect_program(&copy, "p2-duplicate", false, true, second_inputs,
                              second_results, duplicate) &&
              register_program(&division, argv[3], "auth", NULL) &&
              enter_main(&division, "auth") &&
              inspect_program(&division, "auth", true, true, division_inputs,
                              division_results, control) &&
              authenticate(second, duplicate, control, &p2, &copy);
    for (size_t i = 0; ok && i < 4; ++i) {
        Program negative = {0};
        ok = register_program(&negative, argv[4 + i], negative_codes[i],
                              negative_codes[i]);
        release_program(&negative);
    }
    printf("R1557 PUBLIC_FINAL passed=%d guest_processes=0\n", (int)ok);
    release_program(&division);
    release_program(&copy);
    release_program(&p2);
    release_program(&p1);
    return ok ? 0 : 1;
}
