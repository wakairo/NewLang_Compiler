#include "newlang/checked.h"
#include "newlang/if_field.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"
#include "newlang/usize_addition.h"
#include "newlang/usize_subtraction.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t loops, returns, fields, arithmetic;

#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "R1163 PUBLIC STOP line=%d %s\n", __LINE__, #x);    \
            exit(2);                                                           \
        }                                                                      \
    } while (0)

static bool name_is(const NLCheckedFragment *f, const NLCheckedNodeView *n,
                    const char *name)
{
    NLSourceView v;
    return nl_source_view(nl_checked_source(f), n->name, &v) &&
           v.length == strlen(name) && memcmp(v.bytes, name, v.length) == 0;
}

static const NLCheckedNodeView *binding(const NLCheckedFragment *f,
                                        const char *name)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n && n->kind == NL_CHECKED_BINDING && name_is(f, n, name))
            return n;
    }
    return NULL;
}

static void lineage(const NLCheckedFragment *f)
{
    const NLCheckedNodeView *result = binding(f, "result");
    const NLCheckedNodeView *copy = binding(f, "result_copy");
    if (!result && !copy)
        return;
    REQUIRE(result && copy);
    const NLCheckedNodeView *loop =
        nl_checked_node_view(f, result->initializer);
    const NLCheckedNodeView *use = nl_checked_node_view(f, copy->initializer);
    REQUIRE(loop && loop->kind == NL_CHECKED_LOOP);
    REQUIRE(use && use->kind == NL_CHECKED_IDENTIFIER);
    REQUIRE(use->symbol == result->symbol && use->value_use == NL_VALUE_COPIED);
    size_t comparisons = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (!n || n->kind != NL_CHECKED_USIZE_EQUALITY)
            continue;
        const NLCheckedNodeView *left =
            nl_checked_node_view(f, n->first_argument);
        REQUIRE(left);
        const NLCheckedNodeView *right =
            nl_checked_node_view(f, left->next_argument);
        REQUIRE(right && right->kind == NL_CHECKED_USIZE_LITERAL);
        REQUIRE(left->kind == NL_CHECKED_IDENTIFIER);
        REQUIRE(left->symbol == copy->symbol);
        REQUIRE(left->value_use == NL_VALUE_COPIED);
        NLSemanticTypeView type;
        REQUIRE(nl_semantic_type_view(nl_checked_context(f), n->type, &type));
        REQUIRE(type.kind == NL_TYPE_BOOL);
        ++comparisons;
        printf("{\"event\":\"lineage\",\"result_symbol\":%zu,"
               "\"copy_symbol\":%zu,\"loop\":%zu,\"comparison\":%zu}\n",
               result->symbol, copy->symbol, result->initializer, i);
    }
    REQUIRE(comparisons == 1);
}

static void original(const NLCheckedFragment *f, NLCheckedNodeId id,
                     const NLIfReturnView *r)
{
    REQUIRE(nl_checked_if_return_validate(f, id, r) == NL_CHECK_OK);
    REQUIRE(r->normal_arm < 2);
    REQUIRE(r->replay_changes == 1);
    REQUIRE(r->field.arms[r->normal_arm].change_count == 1);
    REQUIRE(r->field.arms[1 - r->normal_arm].change_count == 0);
    NLSemanticPlaceView before, after, child_before, child_after;
    NLSemanticPlaceView sibling_before, sibling_after;
    REQUIRE(nl_semantic_place_view(r->field.entry, r->field.root, &before));
    REQUIRE(nl_semantic_place_view(r->field.post, r->field.root, &after));
    REQUIRE(nl_semantic_place_view(r->field.entry, r->field.child,
                                   &child_before));
    REQUIRE(nl_semantic_place_view(r->field.post, r->field.child, &child_after));
    REQUIRE(nl_semantic_place_view(r->field.entry, r->field.sibling,
                                   &sibling_before));
    REQUIRE(nl_semantic_place_view(r->field.post, r->field.sibling,
                                   &sibling_after));
    REQUIRE(before.live && after.live && before.independent_root);
    REQUIRE(before.incarnation == after.incarnation);
    REQUIRE(child_before.incarnation == child_after.incarnation);
    REQUIRE(child_before.parent_aggregate == r->field.root);
    REQUIRE(child_after.parent_aggregate == r->field.root);
    REQUIRE(!child_before.independent_root && !child_after.independent_root);
    REQUIRE(child_before.current_fact != child_after.current_fact);
    REQUIRE(before.current_fact != after.current_fact);
    REQUIRE(sibling_before.current_fact == sibling_after.current_fact);
    REQUIRE(sibling_before.incarnation == sibling_after.incarnation);
    NLSemanticValueView old, now, sibling;
    REQUIRE(nl_semantic_value_view(r->field.entry, child_before.current_value,
                                   &old));
    REQUIRE(nl_semantic_value_view(r->field.post, child_after.current_value,
                                   &now));
    REQUIRE(nl_semantic_value_view(r->field.post, sibling_after.current_value,
                                   &sibling));
    REQUIRE(old.scalar_known && now.scalar_known && sibling.scalar_known);
    printf("{\"event\":\"original\",\"normal_arm\":%zu,\"index\":%zu,"
           "\"root\":%zu,\"child\":%zu,\"sibling\":%zu,\"old\":%zu,"
           "\"current\":%zu,\"sibling_value\":%zu,\"replay_changes\":%zu}\n",
           r->normal_arm, r->field.index, r->field.root, r->field.child,
           r->field.sibling, old.scalar_value, now.scalar_value,
           sibling.scalar_value, r->replay_changes);
    for (size_t stream = 0; stream != 3; ++stream) {
        size_t count = stream == 2 ? r->replay_events
                                  : r->field.arms[stream].event_count;
        for (size_t i = 0; i < count; ++i) {
            NLIfFieldEvent e;
            REQUIRE(nl_checked_if_return_event(f, id, stream, i, &e));
            REQUIRE(nl_checked_if_return_event_validate(f, id, stream, i,
                                                         &e) == NL_CHECK_OK);
            REQUIRE(e.field.parent == r->field.root);
            if (e.change)
                REQUIRE(e.field.index == r->field.index);
            ++fields;
            printf("{\"event\":\"field_receipt\",\"stream\":%zu,"
                   "\"ordinal\":%zu,\"change\":%u,\"index\":%zu}\n",
                   stream, i, (unsigned)e.change, e.field.index);
        }
    }
}

static void inspect(const NLCheckedFragment *f, size_t depth)
{
    REQUIRE(f && depth < 16);
    lineage(f);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (!n)
            continue;
        if (n->kind == NL_CHECKED_LOOP) {
            REQUIRE(n->header_inductive && n->continue_edges > 0);
            REQUIRE(n->break_edges > 0 && n->return_edges == 0);
            ++loops;
            printf("{\"event\":\"loop\",\"continue_edges\":%zu,"
                   "\"break_edges\":%zu,\"return_edges\":%zu,"
                   "\"single_direct_break_validator\":%u}\n",
                   n->continue_edges, n->break_edges, n->return_edges,
                   (unsigned)nl_checked_loop_break_provenance_valid(f, i));
            inspect(nl_checked_loop_body(f, i), depth + 1);
        }
        if (n->kind == NL_CHECKED_IF) {
            NLIfReturnView r;
            if (nl_checked_if_return_view(f, i, &r)) {
                original(f, i, &r);
                ++returns;
            }
            for (size_t arm = 0; arm < 2; ++arm) {
                const NLCheckedFragment *a = nl_checked_if_arm(f, i, arm);
                if (a)
                    inspect(a, depth + 1);
            }
        }
        if (n->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *b = nl_checked_call_body(f, i);
            if (b)
                inspect(b, depth + 1);
        }
        if (n->kind == NL_CHECKED_USIZE_ADDITION) {
            NLUsizeAdditionView a;
            bool available = nl_checked_usize_addition_view(f, i, &a);
            if (available) {
                REQUIRE(nl_checked_usize_addition_valid(f, i, &a));
                ++arithmetic;
            }
            printf("{\"event\":\"addition_receipt\",\"applicable\":%u}\n",
                   (unsigned)available);
        }
        if (n->kind == NL_CHECKED_USIZE_SUBTRACTION) {
            NLUsizeSubtractionView a;
            bool available = nl_checked_usize_subtraction_view(f, i, &a);
            if (available) {
                REQUIRE(nl_checked_usize_subtraction_valid(f, i, &a));
                ++arithmetic;
            }
            printf("{\"event\":\"subtraction_receipt\",\"applicable\":%u}\n",
                   (unsigned)available);
        }
    }
}

int main(int argc, char **argv)
{
    REQUIRE(argc == 2);
    NLSource *source = NULL, *entry = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *call = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *checked = NULL;
    NLParseDiagnostic pd;
    NLFunctionUnitDiagnostic ud;
    NLCheckDiagnostic cd;
    REQUIRE(nl_source_load(argv[1], &source) == NL_SOURCE_OK);
    REQUIRE(nl_parser_create(source, &parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_function_unit(parser, &unit, &pd) == NL_PARSE_OK);
    REQUIRE(nl_semantic_create(&context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    REQUIRE(nl_semantic_register_function_unit(context, inputs, 1, &ud) ==
            NL_CHECK_OK);
    REQUIRE(nl_source_create("main()", 6, "r1163-entry", &entry) == NL_SOURCE_OK);
    REQUIRE(nl_parser_create(entry, &entry_parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_source_fragment(entry_parser, &call, &pd) ==
            NL_PARSE_OK);
    REQUIRE(nl_semantic_check_source_fragment(context, call, &checked, &cd) ==
            NL_CHECK_OK);
    inspect(checked, 0);
    REQUIRE(loops == 1 && returns == 1 && fields >= 2);
    printf("{\"event\":\"public_complete\",\"loops\":%zu,\"returns\":%zu,"
           "\"validated_field_events\":%zu,\"arithmetic_receipts\":%zu}\n",
           loops, returns, fields, arithmetic);
    nl_checked_destroy(checked);
    nl_syntax_tree_destroy(call);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return 0;
}
