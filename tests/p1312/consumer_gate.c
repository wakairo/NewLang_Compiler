#include "linear_break_internal.h"
#include "newlang/checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/parser.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const NLCheckedFragment *backend_entry;
static size_t fragments, producers, receivers, payload_reads,
    allocation_matches;
static size_t loops, closed_loans;
static void walk(const NLCheckedFragment *f, size_t depth)
{
    assert(f && depth < 12);
    ++fragments;
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        assert(n);
        if (n->kind == NL_CHECKED_LINK_READ && n->type == u8) {
            NLSemanticValueView value;
            assert(n->result_count == 1 &&
                   nl_semantic_value_view(c, n->results[0].value, &value));
            assert(value.type == u8 && value.scalar_known &&
                   value.scalar_value == 212);
            ++payload_reads;
            printf("payload world=%p node=%zu result=%zu known=%d value=%zu\n",
                   (const void *)c, id, n->results[0].value, value.scalar_known,
                   value.scalar_value);
        }
        if (n->kind == NL_CHECKED_LOAN_HEADER && n->loan.scope) {
            NLSemanticScopeView scope;
            assert(nl_semantic_scope_view(c, n->loan.scope, &scope) &&
                   !scope.active);
            ++closed_loans;
        }
        if (n->kind == NL_CHECKED_MATCH) {
            const NLCheckedNodeView *input =
                nl_checked_node_view(f, n->initializer);
            if (input && input->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
                ++allocation_matches;
            for (size_t a = 0; a < n->item_count; ++a)
                walk(nl_checked_match_arm(f, id, a), depth + 1);
        }
        if (n->kind == NL_CHECKED_IF) {
            walk(nl_checked_if_arm(f, id, 0), depth + 1);
            walk(nl_checked_if_arm(f, id, 1), depth + 1);
        }
        if (n->kind == NL_CHECKED_LOOP) {
            assert(n->header_inductive && n->continue_edges && n->break_edges);
            const bool direct_break_only =
                nl_checked_loop_break_provenance_valid(f, id);
            NLLinearBreakView current = {0};
            NLCheckStatus valid = nl_linear_break_validate(f, id, &current);
            printf("cyclic-current node=%zu private-status=%d public-C178=%d\n",
                   id, valid, direct_break_only);
            fflush(stdout);
            assert(!direct_break_only && valid == NL_CHECK_OK);
            assert(current.changing_current && current.terminal_old &&
                   current.initial_phase == 0 && current.limit == 2 &&
                   current.next_phase == 1 && current.entry_world &&
                   current.post_world && current.root && current.child &&
                   current.incarnation && current.child_incarnation &&
                   current.terminal_loan);
            const NLCheckedNodeView *edge =
                nl_checked_node_view(current.iteration, current.continue_node);
            const NLCheckedNodeView *add =
                edge
                    ? nl_checked_node_view(current.iteration, edge->initializer)
                    : NULL;
            const NLCheckedNodeView *left =
                add ? nl_checked_node_view(current.iteration,
                                           add->first_argument)
                    : NULL;
            const NLCheckedNodeView *right =
                left ? nl_checked_node_view(current.iteration,
                                            left->next_argument)
                     : NULL;
            printf("Continue node=%zu kind=%d initializer=%zu add-kind=%d "
                   "lhs-symbol=%zu phase=%zu rhs-kind=%d value=%zu\n",
                   current.continue_node, edge ? (int)edge->kind : -1,
                   edge ? edge->initializer : 0, add ? (int)add->kind : -1,
                   left ? left->symbol : 0, current.phase_symbol,
                   right ? (int)right->kind : -1,
                   right ? right->scalar_result.value : 0);
            assert(edge && edge->kind == NL_CHECKED_CONTINUE &&
                   edge->terminates && edge->initializer && add &&
                   add->kind == NL_CHECKED_USIZE_ADDITION &&
                   add->argument_count == 2 && left && right &&
                   left->kind == NL_CHECKED_IDENTIFIER &&
                   left->symbol == current.phase_symbol &&
                   right->kind == NL_CHECKED_USIZE_LITERAL &&
                   right->has_scalar_result && right->scalar_result.known &&
                   right->scalar_result.value == 1);
            ++loops;
            walk(nl_checked_loop_body(f, id), depth + 1);
        }
        if (n->kind != NL_CHECKED_REGISTERED_CALL || !n->body_backed)
            continue;
        if (n->producer.entry_proved) {
            assert(n->producer.return_proved &&
                   nl_checked_producer_valid(f, id));
            assert(nl_checked_producer_entry(f, id) &&
                   nl_checked_producer_return(f, id));
            ++producers;
        }
        if (n->owner_call.entry_proved) {
            const NLSemanticContext *at = nl_checked_owner_entry(f, id);
            NLSemanticPlaceView root;
            assert(n->owner_call.post_proved && at &&
                   nl_semantic_place_view(at, n->owner_call.root, &root));
            assert(root.live && root.incarnation == n->owner_call.incarnation &&
                   root.governing_domain == n->owner_call.domain &&
                   root.placement.region == n->owner_call.range.region);
            ++receivers;
        }
        walk(nl_checked_call_body(f, id), depth + 1);
    }
}
static void direct(const NLCheckedFragment *main_body)
{
    const NLSemanticContext *c = nl_checked_context(main_body);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    const NLCheckedNodeView *write = NULL, *current = NULL, *sibling = NULL;
    size_t typed_matches = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(main_body); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(main_body, id);
        if (n->kind == NL_CHECKED_MATCH && n->type == u8) {
            NLSemanticValueView v;
            assert(n->normal_frame_unchanged && n->normal_arms == 2 &&
                   n->result_count == 1 &&
                   nl_semantic_value_view(c, n->results[0].value, &v));
            assert(v.dependencies == NL_DEPENDENCY_FREE && !v.reference_count &&
                   !v.reference.place && !v.domain && !v.allocation_region);
            ++typed_matches;
            printf(
                "closed-match world=%p node=%zu result=%zu scalar-known=%d\n",
                (const void *)c, id, n->results[0].value, v.scalar_known);
        }
        if (!n->field.present || n->field.type != u8)
            continue;
        if (n->kind == NL_CHECKED_REPLACE && n->field.index == 0) {
            assert(!write);
            write = n;
        }
        if (n->kind == NL_CHECKED_FIELD_READ) {
            if (n->field.index == 0)
                current = n;
            if (n->field.index == 1)
                sibling = n;
        }
    }
    assert(typed_matches == 1 && write && current && sibling);
    NLSemanticValueView old, mirror;
    assert(nl_semantic_value_view(c, write->field.old_value, &old) &&
           old.scalar_known && old.scalar_value == 41);
    assert(nl_semantic_value_view(c, sibling->field.old_value, &mirror) &&
           mirror.scalar_known && mirror.scalar_value == 167);
    assert(
        write->field.parent == current->field.parent &&
        write->field.child == current->field.child &&
        write->field.parent_incarnation == current->field.parent_incarnation &&
        write->field.child_incarnation == current->field.child_incarnation &&
        write->field.parent_post_fact == current->field.parent_fact &&
        write->field.child_post_fact == current->field.child_fact &&
        write->field.new_value == current->field.old_value &&
        sibling->field.parent == write->field.parent &&
        sibling->field.parent_incarnation == write->field.parent_incarnation &&
        sibling->field.parent_fact == write->field.parent_post_fact &&
        sibling->field.child != write->field.child);
    printf("T-original world=%p root=%zu incarnation=%zu child=%zu old41 "
           "postcurrent-linked sibling167\n",
           (const void *)c, write->field.parent,
           write->field.parent_incarnation, write->field.child);
}

static size_t allocation_count, fail_ordinal;
static bool counting;
static bool should_fail(void)
{
    if (!counting)
        return false;
    ++allocation_count;
    if (fail_ordinal && allocation_count == fail_ordinal) {
        errno = ENOMEM;
        return true;
    }
    return false;
}
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n)
{
    return should_fail() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return should_fail() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return should_fail() ? NULL : __real_realloc(p, n);
}

typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *tree;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
} Fixture;
static void prepare(Fixture *x, const char *path)
{
    NLParseDiagnostic pd = {0};
    NLFunctionUnitDiagnostic ud = {0};
    assert(nl_source_load(path, &x->source) == NL_SOURCE_OK);
    assert(nl_parser_create(x->source, &x->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(x->parser, &x->unit, &pd) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&x->context) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {x->unit};
    assert(nl_semantic_register_function_unit(x->context, units, 1, &ud) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "<p1310-focused>", &x->entry_source) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(x->entry_source, &x->entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(x->entry_parser, &x->tree,
                                               &pd) == NL_PARSE_OK);
}
static void release(Fixture *x)
{
    nl_checked_destroy(x->entry);
    nl_syntax_tree_destroy(x->tree);
    nl_parser_destroy(x->entry_parser);
    nl_source_destroy(x->entry_source);
    nl_semantic_destroy(x->context);
    nl_syntax_tree_destroy(x->unit);
    nl_parser_destroy(x->parser);
    nl_source_destroy(x->source);
}
static NLCheckedFragment *main_body(NLCheckedFragment *entry)
{
    return (NLCheckedFragment *)nl_checked_call_body(entry,
                                                     nl_checked_root(entry));
}
static NLCheckedNodeId loop_id(NLCheckedFragment *body)
{
    NLCheckedNodeId found = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id)
        if (nl_checked_node_view(body, id)->kind == NL_CHECKED_LOOP) {
            assert(!found);
            found = id;
        }
    assert(found);
    return found;
}
static void valid_cycle(NLCheckedFragment *entry)
{
    NLCheckedFragment *body = main_body(entry);
    NLLinearBreakView view = {0};
    assert(body &&
           nl_linear_break_validate(body, loop_id(body), &view) == NL_CHECK_OK);
    assert(view.changing_current && view.terminal_old && view.entry_world &&
           view.post_world);
}
static void rejected_cycle(NLCheckedFragment *body, NLCheckedNodeId loop,
                           const char *label)
{
    NLLinearBreakView view = {0};
    NLCheckStatus s = nl_linear_break_validate(body, loop, &view);
    printf("negative %s status=%d\n", label, s);
    assert(s != NL_CHECK_OK);
    char *output = NULL;
    size_t length = 98765;
    assert(nl_checked_c_node(backend_entry, &output, &length) ==
           NL_NODE_C_UNSUPPORTED);
    assert(!output && length == 98765);
}
static void focused(const char *path, NLCheckedFragment *entry)
{
    backend_entry = entry;
    NLCheckedFragment *a = main_body(entry);
    NLCheckedNodeId loop = loop_id(a), match = 0, copy = 0;
    for (NLCheckedNodeId id = 1; id <= a->count; ++id) {
        NLCheckedNodeView *n = &a->nodes[id - 1];
        if (n->kind == NL_CHECKED_MATCH) {
            assert(!match);
            match = id;
        }
        if (n->kind == NL_CHECKED_IDENTIFIER &&
            n->type == nl_semantic_core_type(a->context, NL_TYPE_U8) &&
            n->span.end_byte < a->nodes[loop - 1].span.start_byte)
            copy = id;
    }
    assert(match && copy);
    valid_cycle(entry);
    NLCheckedNodeView saved = a->nodes[match - 1];
    a->nodes[match - 1].normal_arms = 1;
    rejected_cycle(a, loop, "nonclosed-match");
    a->nodes[match - 1] = saved;
    valid_cycle(entry);
    saved = a->nodes[copy - 1];
    a->nodes[copy - 1].symbol = 0;
    rejected_cycle(a, loop, "unissued-copy");
    a->nodes[copy - 1] = saved;
    valid_cycle(entry);
    NLLinearBreakView view = {0};
    assert(nl_linear_break_validate(a, loop, &view) == NL_CHECK_OK);
    NLSemanticContext *post = (NLSemanticContext *)view.post_world;
    NLIncarnationId incarnation = post->places[view.child - 1].incarnation;
    ++post->places[view.child - 1].incarnation;
    rejected_cycle(a, loop, "stale-post-child");
    post->places[view.child - 1].incarnation = incarnation;
    valid_cycle(entry);
    Fixture foreign = {0};
    prepare(&foreign, path);
    NLCheckDiagnostic cd = {0};
    assert(nl_semantic_check_expression(foreign.context, foreign.tree,
                                        &foreign.entry, &cd) == NL_CHECK_OK);
    NLCheckedFragment *other = main_body(foreign.entry);
    NLCheckedNodeId foreign_match = 0;
    for (NLCheckedNodeId id = 1; id <= other->count; ++id)
        if (other->nodes[id - 1].kind == NL_CHECKED_MATCH)
            foreign_match = id;
    assert(foreign_match);
    size_t slot = 0;
    while (slot < a->arm_count && a->arms[slot].match != match)
        ++slot;
    assert(slot < a->arm_count);
    NLCheckedArm original = a->arms[slot];
    a->arms[slot].artifact =
        (NLCheckedFragment *)nl_checked_match_arm(other, foreign_match, 0);
    rejected_cycle(a, loop, "foreign-owned-H-arm");
    a->arms[slot] = original;
    valid_cycle(entry);
    NLCheckedFragment *h =
        (NLCheckedFragment *)nl_checked_match_arm(a, match, 1);
    NLCheckedNodeView arm = h->nodes[h->root - 1];
    NLValueId other_value = 0;
    for (NLCheckedNodeId id = 1; id <= h->count; ++id)
        if (h->nodes[id - 1].kind == NL_CHECKED_U8_LITERAL &&
            h->nodes[id - 1].result_count == 1 &&
            h->nodes[id - 1].results[0].value != arm.results[0].value)
            other_value = h->nodes[id - 1].results[0].value;
    assert(other_value && other_value <= h->context->value_count);
    h->nodes[h->root - 1].results[0].value = other_value;
    rejected_cycle(a, loop, "changed-issued-H-result");
    h->nodes[h->root - 1] = arm;
    valid_cycle(entry);
    /* Each actual allocation world has its own issued result. No success
     * arm's value may stand in for a missing H0/H1/H2 result package. */
    NLCheckedFragment *worlds[3] = {
        (NLCheckedFragment *)nl_checked_match_arm(a, match, 0), NULL, NULL};
    for (NLCheckedNodeId id = 1; id <= h->count; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(h, id);
        const NLCheckedNodeView *input =
            node ? nl_checked_node_view(h, node->initializer) : NULL;
        if (node && node->kind == NL_CHECKED_MATCH && input &&
            input->kind == NL_CHECKED_TRY_ALLOCATE_ONE) {
            assert(!worlds[1] && !worlds[2]);
            worlds[1] = (NLCheckedFragment *)nl_checked_match_arm(h, id, 0);
            worlds[2] = (NLCheckedFragment *)nl_checked_match_arm(h, id, 1);
        }
    }
    const char *labels[] = {"H0-unissued-result", "H1-unissued-result",
                            "H2-unissued-result"};
    for (size_t i = 0; i < 3; ++i) {
        assert(worlds[i]);
        NLCheckedNodeView *result = &worlds[i]->nodes[worlds[i]->root - 1];
        assert(result->result_count == 1 && result->results[0].value);
        const NLCheckedNodeView issued = *result;
        result->results[0].value = 0;
        rejected_cycle(a, loop, labels[i]);
        *result = issued;
        valid_cycle(entry);
    }
    release(&foreign);
    Fixture baseline = {0};
    prepare(&baseline, path);
    allocation_count = 0;
    fail_ordinal = 0;
    counting = true;
    NLCheckStatus s = nl_semantic_check_expression(
        baseline.context, baseline.tree, &baseline.entry, &cd);
    counting = false;
    size_t total = allocation_count;
    assert(s == NL_CHECK_OK && total > 4);
    valid_cycle(baseline.entry);
    release(&baseline);
    printf("normal CHECK allocation events=%zu\n", total);
    size_t selected[] = {1, 2, total / 2, total};
    for (size_t i = 0; i < 4; ++i) {
        Fixture x = {0};
        prepare(&x, path);
        NLSemanticContext before = *x.context;
        allocation_count = 0;
        fail_ordinal = selected[i];
        counting = true;
        s = nl_semantic_check_expression(x.context, x.tree, &x.entry, &cd);
        counting = false;
        fail_ordinal = 0;
        printf("OOM ordinal=%zu actual-events=%zu status=%d out=%p\n",
               selected[i], allocation_count, s, (void *)x.entry);
        assert(allocation_count >= selected[i] && s == NL_CHECK_OUT_OF_MEMORY &&
               !x.entry);
        assert(!memcmp(&before, x.context, sizeof(before)));
        assert(nl_semantic_check_expression(x.context, x.tree, &x.entry, &cd) ==
               NL_CHECK_OK);
        valid_cycle(x.entry);
        release(&x);
    }
    puts("FOCUSED CURRENT negative8 OOM4 rollback4 retry4 PASS");
}

static void emitter_oom(NLCheckedFragment *entry)
{
    char *baseline = NULL;
    size_t length = 98765;
    allocation_count = 0;
    fail_ordinal = 0;
    counting = true;
    NLNodeCStatus status = nl_checked_c_node(entry, &baseline, &length);
    counting = false;
    const size_t events = allocation_count;
    assert(status == NL_NODE_C_OK && baseline && events && length);
    printf("normal C17 allocation events=%zu bytes=%zu\n", events, length);
    const size_t choices[] = {1, 2, events / 2, events};
    const NLCheckedNodeId root = nl_checked_root(entry);
    const NLSemanticContext *context = nl_checked_context(entry);
    const NLSemanticContext before = *context;
    size_t checked[4] = {0}, count = 0;
    for (size_t i = 0; i < 4; ++i) {
        const size_t at = choices[i];
        bool duplicate = false;
        for (size_t j = 0; j < count; ++j)
            if (checked[j] == at)
                duplicate = true;
        if (!at || at > events || duplicate)
            continue;
        checked[count++] = at;
        char *output = NULL;
        size_t bytes = 98765;
        allocation_count = 0;
        fail_ordinal = at;
        counting = true;
        status = nl_checked_c_node(entry, &output, &bytes);
        counting = false;
        fail_ordinal = 0;
        printf("emit OOM ordinal=%zu actual=%zu status=%d out=%p length=%zu\n",
               at, allocation_count, status, (void *)output, bytes);
        assert(allocation_count >= at && status == NL_NODE_C_OUT_OF_MEMORY);
        assert(!output && bytes == 98765 && nl_checked_root(entry) == root);
        assert(!memcmp(&before, context, sizeof(before)));
        valid_cycle(entry);
        assert(nl_checked_c_node(entry, &output, &bytes) == NL_NODE_C_OK);
        assert(output && bytes == length && !memcmp(output, baseline, length));
        free(output);
    }
    free(baseline);
    printf("EMITTER OOM%zu no-clobber rollback retry PASS\n", count);
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    setvbuf(stdout, NULL, _IONBF, 0);
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    NLParseDiagnostic pd = {0};
    NLFunctionUnitDiagnostic ud = {0};
    NLCheckDiagnostic cd = {0};
    assert(nl_source_load(argv[2], &source) == NL_SOURCE_OK);
    assert(nl_parser_create(source, &parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(parser, &unit, &pd) == NL_PARSE_OK);
    assert(nl_semantic_create(&context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    NLCheckStatus status =
        nl_semantic_register_function_unit(context, inputs, 1, &ud);
    printf("register-status=%d\n", status);
    if (status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, source, &ud.diagnostic);
        return 10;
    }
    assert(nl_source_create("main()", 6, "<p1310-public-entry>",
                            &entry_source) == NL_SOURCE_OK);
    assert(nl_parser_create(entry_source, &entry_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(entry_parser, &tree, &pd) ==
           NL_PARSE_OK);
    status = nl_semantic_check_expression(context, tree, &entry, &cd);
    printf("check-status=%d\n", status);
    if (status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, entry_source, &cd);
        return 11;
    }
    const NLCheckedFragment *main_body =
        nl_checked_call_body(entry, nl_checked_root(entry));
    assert(main_body);
    walk(entry, 0);
    assert(allocation_matches == 2 && payload_reads == 1 && producers == 1 &&
           receivers == 1);
    if (!strcmp(argv[1], "control")) {
        assert(!loops);
        direct(main_body);
    } else {
        assert(!strcmp(argv[1], "positive") && loops == 1);
    }
    printf("SOURCE-OWNED VIEWS + PRIVATE CURRENT PASS fragments=%zu "
           "allocation-matches=%zu "
           "payload-reads=%zu producer=%zu receiver=%zu closed-loans=%zu "
           "loops=%zu\n",
           fragments, allocation_matches, payload_reads, producers, receivers,
           closed_loans, loops);
    if (!strcmp(argv[1], "positive")) {
        focused(argv[2], entry);
        emitter_oom(entry);
    }
    nl_checked_destroy(entry);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return 0;
}
