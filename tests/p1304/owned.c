#include "newlang/copy_body.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include <assert.h>
#include <stdio.h>

typedef struct {
    const NLCheckedFragment *caller;
    size_t caller_function;
    NLCheckedNodeId id;
    NLCopyBodyView body;
    NLCopyBodyParameters parameters;
    NLCopyBodyEventView event;
} ObservedCall;
static ObservedCall calls[2];
static size_t count;

static NLSemanticValueView byte_value(const NLSemanticContext *c, NLValueId id)
{
    NLSemanticValueView v;
    assert(nl_semantic_value_view(c, id, &v));
    assert(v.type == nl_semantic_core_type(c, NL_TYPE_U8) &&
           v.dependencies == NL_DEPENDENCY_FREE && !v.reference_count &&
           !v.reference.place && !v.value_dependency_count && !v.owner_place &&
           !v.domain && !v.allocation_region);
    return v;
}

static int visit(const NLCheckedFragment *f, size_t function, size_t depth)
{
    assert(f && depth < 4);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->kind != NL_CHECKED_REGISTERED_CALL)
            continue;
        if (n->argument_count == 2) {
            assert(count < 2);
            ObservedCall *o = &calls[count++];
            o->caller = f;
            o->caller_function = function;
            o->id = id;
            bool p = nl_checked_copy_body_parameters(f, id, &o->parameters);
            bool b = nl_checked_copy_body_view(f, id, &o->body);
            bool e = nl_checked_copy_body_event(f, id, 0, &o->event);
            printf("GET caller=%zu call=%zu callee=%zu params=%d body=%d "
                   "event=%d\n",
                   function, id, n->function, p, b, e);
            if (!p || !b || !e)
                return 5;
            NLCheckStatus ps =
                nl_checked_copy_body_parameters_validate(f, id, &o->parameters);
            NLCheckStatus bs = nl_checked_copy_body_validate(f, id, &o->body);
            NLCheckStatus es =
                nl_checked_copy_body_event_validate(f, id, 0, &o->event);
            printf("VALIDATE caller=%zu params=%d body=%d event=%d\n", function,
                   ps, bs, es);
            if (ps != NL_CHECK_OK || bs != NL_CHECK_OK || es != NL_CHECK_OK)
                return 6;
            const size_t index = count == 1 ? 0 : 1;
            const size_t old_number = count == 1 ? 83 : 109;
            const size_t incoming = count == 1 ? 167 : 83;
            const size_t sibling_number = count == 1 ? 211 : 37;
            const NLCopyBodyParameters *x = &o->parameters;
            NLSemanticPlaceView parent, handoff, before, after, sb, sa;
            assert(o->body.event_count == 1 &&
                   o->event.kind == NL_COPY_BODY_CHANGE);
            assert(x->field.index == index);
            assert(nl_semantic_place_view(x->entry, x->field.parent, &parent));
            assert(
                nl_semantic_place_view(x->handoff, x->field.parent, &handoff));
            assert(parent.live && handoff.live && parent.independent_root &&
                   parent.implicit_local);
            assert(parent.incarnation == handoff.incarnation &&
                   parent.current_fact != handoff.current_fact);
            assert(parent.fixed_field_count == 2 &&
                   handoff.fixed_field_count == 2);
            assert(parent.fixed_fields[index] == x->field.child &&
                   handoff.fixed_fields[index] == x->field.child);
            assert(nl_semantic_place_view(x->entry, x->field.child, &before));
            assert(nl_semantic_place_view(x->handoff, x->field.child, &after));
            assert(before.incarnation == after.incarnation &&
                   before.current_fact != after.current_fact);
            assert(nl_semantic_place_view(x->entry,
                                          parent.fixed_fields[1 - index], &sb));
            assert(nl_semantic_place_view(
                x->handoff, handoff.fixed_fields[1 - index], &sa));
            assert(sb.current_fact == sa.current_fact &&
                   sb.current_value == sa.current_value);
            NLSemanticValueView old =
                byte_value(x->entry, before.current_value);
            NLSemanticValueView now =
                byte_value(x->handoff, after.current_value);
            NLSemanticValueView sibling =
                byte_value(x->handoff, sa.current_value);
            assert(old.scalar_known && old.scalar_value == old_number);
            assert(!now.scalar_known || now.scalar_value == incoming);
            assert(sibling.scalar_known &&
                   sibling.scalar_value == sibling_number);
            NLSemanticValueView result =
                byte_value(nl_checked_context(f), n->results[0].value);
            assert(result.scalar_known && result.scalar_value == old_number);
            const NLCheckedNodeView *loan = nl_checked_node_view(f, x->loan);
            NLSemanticScopeView scope;
            assert(loan && loan->kind == NL_CHECKED_LOAN_HEADER &&
                   loan->loan.body_nonescape_proved &&
                   loan->loan.normal_result_forwarded);
            assert(loan->loan.place == x->field.child &&
                   loan->loan.incarnation == x->field.child_incarnation);
            assert(nl_semantic_scope_view(nl_checked_context(f),
                                          loan->loan.scope, &scope) &&
                   !scope.active);
            printf("FACT caller=%zu world=%p parent=%zu child=%zu index=%zu "
                   "incarnation=%zu old=%zu current_known=%d current=%zu "
                   "sibling=%zu closed-loan=1\n",
                   function, (const void *)x->entry, x->field.parent,
                   x->field.child, index, before.incarnation, old_number,
                   now.scalar_known, now.scalar_value, sibling_number);
        }
        const NLCheckedFragment *child = nl_checked_call_body(f, id);
        if (child) {
            int status = visit(child, n->function, depth + 1);
            if (status)
                return status;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    NLSource *source = NULL, *call_source = NULL;
    NLParser *parser = NULL, *call_parser = NULL;
    NLSyntaxTree *unit = NULL, *call_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    NLParseDiagnostic pd = {0};
    NLFunctionUnitDiagnostic ud = {0};
    NLCheckDiagnostic cd = {0};
    assert(nl_source_load(argv[1], &source) == NL_SOURCE_OK);
    assert(nl_parser_create(source, &parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(parser, &unit, &pd) == NL_PARSE_OK);
    assert(nl_semantic_create(&context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    assert(nl_semantic_register_function_unit(context, inputs, 1, &ud) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "P1304 genuine main", &call_source) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(call_source, &call_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(call_parser, &call_tree, &pd) ==
           NL_PARSE_OK);
    assert(nl_semantic_check_expression(context, call_tree, &entry, &cd) ==
           NL_CHECK_OK);
    int status = visit(entry, 0, 0);
    if (!status) {
        assert(count == 2 && calls[0].caller != calls[1].caller);
        assert(calls[0].caller_function != calls[1].caller_function);
        assert(calls[0].body.function == calls[1].body.function);
        assert(calls[0].parameters.entry != calls[1].parameters.entry);
        for (size_t i = 0; i < 2; ++i) {
            ObservedCall *a = &calls[i], *b = &calls[1 - i];
            assert(nl_checked_copy_body_parameters_validate(
                       a->caller, a->id, &b->parameters) != NL_CHECK_OK);
            assert(nl_checked_copy_body_validate(a->caller, a->id, &b->body) !=
                   NL_CHECK_OK);
            assert(nl_checked_copy_body_event_validate(
                       a->caller, a->id, 0, &b->event) != NL_CHECK_OK);
            NLCopyBodyParameters wrong = a->parameters;
            wrong.field.index = 1 - wrong.field.index;
            assert(nl_checked_copy_body_parameters_validate(
                       a->caller, a->id, &wrong) != NL_CHECK_OK);
            assert(nl_checked_copy_body_parameters_validate(
                       a->caller, a->id, &a->parameters) == NL_CHECK_OK);
            assert(nl_checked_copy_body_validate(a->caller, a->id, &a->body) ==
                   NL_CHECK_OK);
        }
        puts(
            "P1304 two distinct named callers / one helper public "
            "owned-current PASS; eight cross-caller/wrong-field refusals PASS");
    }
    nl_checked_destroy(entry);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(call_tree);
    nl_parser_destroy(call_parser);
    nl_source_destroy(call_source);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return status;
}
