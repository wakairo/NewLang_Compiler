/* Stage1 tests: actual registered source only; no seeded guest state. */
#include "../../src/semantic_internal.h" /* read-only function-plan observations */
#include "../support/semantic_check.h"
#include "newlang/copy_body.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t calls, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && calls++ == fail_at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_realloc(p, n);
}
static bool load(const char *path, NLSource **source, NLSyntaxTree **tree,
                 NLParseStatus expected)
{
    CHECK(nl_source_load(path, source) == NL_SOURCE_OK);
    NLParser *p = NULL;
    CHECK(nl_parser_create(*source, &p) == NL_PARSE_OK);
    NLParseDiagnostic d = {0};
    CHECK(nl_parser_parse_function_unit(p, tree, &d) == expected);
    if (expected != NL_PARSE_OK)
        CHECK(*tree == NULL && nl_source_span_valid(*source, d.span));
    nl_parser_destroy(p);
    return true;
}
static bool type_id(NLSemanticContext *c, const char *name, NLTypeId *id)
{
    TestChecked a = {0};
    CHECK(test_run(c, name, TEST_TYPE, NL_CHECK_OK, NULL, &a));
    *id = test_root(&a)->type;
    CHECK(*id);
    test_checked_destroy(&a);
    return true;
}
static size_t function(const NLSemanticContext *c, const char *name)
{
    for (size_t i = 0; i < c->function_count; ++i)
        if (strcmp(c->functions[i].name, name) == 0)
            return i + 1;
    return 0;
}
static size_t copy_calls, initializations;
static NLTypeId t_type, h_type;
static bool heap_hook;
static NLPlaceId heap_roots[2];
static NLDomainId heap_domains[2];
static NLBackingRegionId heap_regions[2];
static NLIncarnationId heap_incarnations[2];
static bool walk(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_parameters(f, i, &p)) {
            NLCopyBodyView b;
            CHECK(p.field.nominal == t_type &&
                  nl_checked_copy_body_view(f, i, &b));
            NLCheckStatus bs = nl_checked_copy_body_validate(f, i, &b),
                          ps = nl_checked_copy_body_parameters_validate(f, i,
                                                                        &p);
            if (heap_hook) {
                /* Existing scalar replay cannot certify the later heap worlds.
                 * Stage1 preserves this precision fence; no joined proof is
                 * synthesized. The independent Copy-only mixed case below
                 * still requires successful existing validators. */
                CHECK(bs == NL_CHECK_ANALYSIS_PRECISION_LIMIT && bs == ps);
                printf("COPY existing_view=present validators=PRECISION_LIMIT "
                       "same_body_caller_world=%u Stage2_NOT_IMPLEMENTED\n",
                       nl_checked_context(b.body) == nl_checked_context(f));
                NLSemanticTypeView earlier_h, later_h;
                CHECK(nl_semantic_type_view(b.live_exit, h_type, &earlier_h) &&
                      nl_semantic_type_view(nl_checked_context(f), h_type,
                                            &later_h));
                printf("H layout_known=%u->%u size=%zu->%zu "
                       "alignment=%zu->%zu existing_type_epoch_fence\n",
                       earlier_h.layout_known, later_h.layout_known,
                       earlier_h.size, later_h.size, earlier_h.alignment,
                       later_h.alignment);
                ++copy_calls;
                continue;
            }
            CHECK(bs == NL_CHECK_OK && ps == NL_CHECK_OK);
            NLSemanticPlaceView before, after, sibling_before, sibling_after;
            CHECK(nl_semantic_place_view(p.entry, p.field.parent, &before) &&
                  nl_semantic_place_view(p.handoff, p.field.parent, &after) &&
                  before.incarnation == after.incarnation &&
                  before.current_fact != after.current_fact &&
                  nl_semantic_place_view(p.entry,
                                         before.fixed_fields[1 - p.field.index],
                                         &sibling_before) &&
                  nl_semantic_place_view(p.handoff,
                                         before.fixed_fields[1 - p.field.index],
                                         &sibling_after) &&
                  sibling_before.current_fact == sibling_after.current_fact &&
                  sibling_before.current_value == sibling_after.current_value);
            const NLCheckedNodeView *loan = nl_checked_node_view(f, p.loan);
            NLSemanticScopeView during, after_scope;
            CHECK(
                loan &&
                nl_semantic_scope_view(p.handoff, loan->loan.scope, &during) &&
                during.active &&
                nl_semantic_scope_view(nl_checked_context(f), loan->loan.scope,
                                       &after_scope) &&
                !after_scope.active);
            NLSemanticValueView old_copy, new_copy, sibling_copy;
            NLSemanticPlaceView changed_child;
            CHECK(nl_semantic_value_view(b.live_exit, b.result.value,
                                         &old_copy) &&
                  old_copy.scalar_known && old_copy.scalar_value == 28 &&
                  nl_semantic_place_view(p.handoff, p.field.child,
                                         &changed_child) &&
                  nl_semantic_value_view(p.handoff, changed_child.current_value,
                                         &new_copy) &&
                  new_copy.scalar_known && new_copy.scalar_value == 93 &&
                  nl_semantic_value_view(p.handoff, sibling_after.current_value,
                                         &sibling_copy) &&
                  sibling_copy.scalar_known &&
                  sibling_copy.scalar_value == 144);
            printf("COPY original=%zu child=%zu nominal=%zu old_fact=%zu "
                   "post_fact=%zu old_Copy=%zu existing_validators=PASS\n",
                   p.field.parent, p.field.child, p.field.nominal,
                   before.current_fact, after.current_fact, b.result.value);
            ++copy_calls;
        }
        if (n->kind == NL_CHECKED_INITIALIZE) {
            CHECK(initializations < 2 && n->type != t_type);
            heap_roots[initializations] = n->lifetime_place;
            heap_domains[initializations] = n->lifetime_domain;
            heap_regions[initializations] = n->backing;
            heap_incarnations[initializations++] = n->lifetime_incarnation;
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body)
            CHECK(walk(body));
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t arm = 0; arm < n->item_count; ++arm)
                CHECK(walk(nl_checked_match_arm(f, i, arm)));
    }
    return true;
}
static bool positive(const char *path, bool hook)
{
    NLSource *s = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(load(path, &s, &tree, NL_PARSE_OK));
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status = nl_semantic_register_function_unit(c, inputs, 1, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "registration %d %s\n", status,
                d.diagnostic.diagnostic.code);
    CHECK(status == NL_CHECK_OK);
    CHECK(type_id(c, "Packet", &h_type) && type_id(c, "Controls", &t_type));
    NLTypeId ptr;
    NLSemanticTypeView h, t, p;
    CHECK(h_type != t_type && nl_semantic_type_view(c, h_type, &h) &&
          nl_semantic_type_view(c, t_type, &t) && t.is_copy &&
          t.is_discardable && !nl_semantic_recursive_local_type(c, t_type) &&
          nl_semantic_recursive_local_type(c, h_type) &&
          nl_semantic_recursive_link_types(c, h_type, &ptr) &&
          nl_semantic_type_view(c, ptr, &p) && p.target == h_type);
    const size_t requested = function(c, "requested"),
                 retune = function(c, "retune"), main_id = function(c, "main");
    CHECK(
        requested && retune && main_id && c->functions[requested - 1].body &&
        c->functions[requested - 1].count == 0 &&
        c->functions[requested - 1].result ==
            nl_semantic_core_type(c, NL_TYPE_U8) &&
        c->functions[retune - 1].body && c->functions[retune - 1].count == 2 &&
        c->functions[main_id - 1].body && c->functions[main_id - 1].count == 0);
    /* Original input can be destroyed: registry owns retained source plans. */
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(s);
    TestChecked a = {0};
    CHECK(test_run(c, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL, &a));
    copy_calls = initializations = 0;
    heap_hook = hook;
    CHECK(walk(a.artifact) && copy_calls == 1 &&
          initializations == (hook ? 2u : 0u));
    if (hook)
        CHECK(heap_roots[0] != heap_roots[1] &&
              heap_domains[0] != heap_domains[1] &&
              heap_regions[0] != heap_regions[1] &&
              heap_incarnations[0] != heap_incarnations[1]);
    printf("H=%zu T=%zu self=%zu signatures=PASS source_owned=PASS "
           "heap_originals=%zu NO_DUAL_CARRIER_PROOF\n",
           h_type, t_type, p.target, initializations);
    test_checked_destroy(&a);
    nl_semantic_destroy(c);
    return true;
}
static bool failure(const char *bad, const char *good)
{
    NLSource *s = NULL, *gs = NULL;
    NLSyntaxTree *tree = NULL, *gt = NULL;
    CHECK(load(bad, &s, &tree, NL_PARSE_OK));
    CHECK(load(good, &gs, &gt, NL_PARSE_OK));
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLSource *preserved_source = NULL;
    NLSyntaxTree *preserved_tree = NULL;
    CHECK(nl_source_create("fn intact()->u8{u8(41)}",
                           strlen("fn intact()->u8{u8(41)}"), "preserved",
                           &preserved_source) == NL_SOURCE_OK);
    NLParser *preserved_parser = NULL;
    CHECK(nl_parser_create(preserved_source, &preserved_parser) ==
              NL_PARSE_OK &&
          nl_parser_parse_function_unit(preserved_parser, &preserved_tree,
                                        NULL) == NL_PARSE_OK);
    const NLSyntaxTree *preserved_inputs[] = {preserved_tree};
    CHECK(nl_semantic_register_function_unit(c, preserved_inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_parser_destroy(preserved_parser);
    nl_syntax_tree_destroy(preserved_tree);
    nl_source_destroy(preserved_source);
    NLFunctionBody *preserved_body =
        c->functions[function(c, "intact") - 1].body;
    TestState before;
    CHECK(test_state(c, &before));
    const NLSyntaxTree *inputs[] = {tree};
    for (size_t repeat = 0; repeat < 2; ++repeat) {
        NLFunctionUnitDiagnostic d = {0};
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, &d);
        CHECK(status != NL_CHECK_OK && status != NL_CHECK_INTERNAL_ERROR &&
              status != NL_CHECK_OUT_OF_MEMORY && d.input_index == 0 &&
              nl_source_span_valid(s, d.diagnostic.span) &&
              (test_unchanged(c, &before) &&
               c->functions[function(c, "intact") - 1].body == preserved_body));
        printf("REJECT %s status=%d code=%s span=%zu:%zu rollback=PASS\n", bad,
               status, d.diagnostic.diagnostic.code,
               d.diagnostic.span.start_byte, d.diagnostic.span.end_byte);
    }
    inputs[0] = gt;
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(tree);
    nl_syntax_tree_destroy(gt);
    nl_source_destroy(s);
    nl_source_destroy(gs);
    nl_semantic_destroy(c);
    return true;
}
static bool oom(const char *path)
{
    NLSource *s = NULL;
    CHECK(nl_source_load(path, &s) == NL_SOURCE_OK);
    NLParser *p = NULL;
    CHECK(nl_parser_create(s, &p) == NL_PARSE_OK);
    NLSyntaxTree *tree = NULL;
    bool done = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLParseStatus status = nl_parser_parse_function_unit(p, &tree, NULL);
        injecting = false;
        if (status == NL_PARSE_OK) {
            done = true;
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
    }
    CHECK(done && fail_at > 0);
    printf("OOM parser=%zu partial_tree=0 retry=PASS\n", fail_at);
    nl_parser_destroy(p);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLSource *preserved_source = NULL;
    NLSyntaxTree *preserved_tree = NULL;
    CHECK(nl_source_create("fn intact()->u8{u8(41)}",
                           strlen("fn intact()->u8{u8(41)}"), "preserved",
                           &preserved_source) == NL_SOURCE_OK);
    NLParser *preserved_parser = NULL;
    CHECK(nl_parser_create(preserved_source, &preserved_parser) ==
              NL_PARSE_OK &&
          nl_parser_parse_function_unit(preserved_parser, &preserved_tree,
                                        NULL) == NL_PARSE_OK);
    const NLSyntaxTree *preserved_inputs[] = {preserved_tree};
    CHECK(nl_semantic_register_function_unit(c, preserved_inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_parser_destroy(preserved_parser);
    nl_syntax_tree_destroy(preserved_tree);
    nl_source_destroy(preserved_source);
    NLFunctionBody *preserved_body =
        c->functions[function(c, "intact") - 1].body;
    TestState before;
    CHECK(test_state(c, &before));
    const NLSyntaxTree *inputs[] = {tree};
    done = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            done = true;
            break;
        }
        if (status != NL_CHECK_OUT_OF_MEMORY)
            fprintf(stderr, "OOM at=%zu status=%d\n", fail_at, status);
        CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
              (test_unchanged(c, &before) &&
               c->functions[function(c, "intact") - 1].body == preserved_body));
    }
    CHECK(done && fail_at > 0);
    printf("OOM registration=%zu clone/types/completion/signatures/late-body "
           "rollback=PASS\n",
           fail_at);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(s);
    return true;
}
static bool multi(const char *path)
{
    NLSource *s = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(load(path, &s, &t, NL_PARSE_OK));
    const NLSyntaxTree *inputs[] = {t, t};
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState before;
    CHECK(test_state(c, &before));
    NLFunctionUnitDiagnostic d = {0};
    CHECK(nl_semantic_register_function_unit(c, inputs, 2, &d) != NL_CHECK_OK &&
          test_unchanged(c, &before));
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(t);
    nl_source_destroy(s);
    puts("multi-input refused; rollback=PASS");
    return true;
}
static bool text_tree(const char *text, NLSource **s, NLSyntaxTree **t)
{
    CHECK(nl_source_create(text, strlen(text), "mixed-placement", s) ==
          NL_SOURCE_OK);
    NLParser *p = NULL;
    CHECK(nl_parser_create(*s, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p, t, NULL) == NL_PARSE_OK);
    nl_parser_destroy(p);
    return true;
}
static bool placement(void)
{
    const char *parts[] = {"struct H{link:Option<ptr<H>>,value:u8}",
                           "struct T{a:u8,b:u8}", "fn step()->u8{u8(13)}"};
    const size_t orders[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                 {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    for (size_t k = 0; k < 6; ++k) {
        char text[512];
        int size = snprintf(
            text, sizeof(text),
            "%s%s%sfn main()->unit{let v=T{a:step(),b:u8(19)};v;unit}",
            parts[orders[k][0]], parts[orders[k][1]], parts[orders[k][2]]);
        CHECK(size > 0 && (size_t)size < sizeof(text));
        NLSource *s = NULL;
        NLSyntaxTree *t = NULL;
        NLSemanticContext *c = NULL;
        CHECK(text_tree(text, &s, &t) && nl_semantic_create(&c) == NL_CHECK_OK);
        const NLSyntaxTree *inputs[] = {t};
        CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
              NL_CHECK_OK);
        NLTypeId h, copy;
        CHECK(type_id(c, "H", &h) && type_id(c, "T", &copy) && h != copy);
        TestChecked call = {0};
        CHECK(test_run(c, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL, &call));
        test_checked_destroy(&call);
        nl_semantic_destroy(c);
        nl_syntax_tree_destroy(t);
        nl_source_destroy(s);
    }
    /* Separate physical inputs cannot be used to combine the categories. */
    NLSource *ss[2] = {0};
    NLSyntaxTree *tt[2] = {0};
    CHECK(
        text_tree("struct H{link:Option<ptr<H>>,value:u8}fn main()->unit{unit}",
                  &ss[0], &tt[0]));
    CHECK(text_tree("struct T{a:u8,b:u8}fn helper()->unit{unit}", &ss[1],
                    &tt[1]));
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState state;
    CHECK(test_state(c, &state));
    const NLSyntaxTree *inputs[] = {tt[0], tt[1]};
    NLFunctionUnitDiagnostic d = {0};
    CHECK(nl_semantic_register_function_unit(c, inputs, 2, &d) ==
              NL_CHECK_SEMANTIC_UNSUPPORTED &&
          d.input_index == 1 &&
          strcmp(d.diagnostic.diagnostic.code, "AVS-UNIT-PROFILE") == 0 &&
          test_unchanged(c, &state));
    nl_semantic_destroy(c);
    for (size_t i = 0; i < 2; ++i) {
        nl_syntax_tree_destroy(tt[i]);
        nl_source_destroy(ss[i]);
    }
    puts("six bounded fn/declaration placements and two distinct physical "
         "inputs PASS");
    return true;
}
int main(int argc, char **argv)
{
    bool ok = false;
    if (argc == 2 && strcmp(argv[1], "placement") == 0)
        ok = placement();
    if (argc == 3 && strcmp(argv[1], "positive") == 0)
        ok = positive(argv[2], strstr(argv[2], "two_original") != NULL);
    if (argc == 4 && strcmp(argv[1], "negative") == 0)
        ok = failure(argv[2], argv[3]);
    if (argc == 3 && strcmp(argv[1], "oom") == 0)
        ok = oom(argv[2]);
    if (argc == 3 && strcmp(argv[1], "multi") == 0)
        ok = multi(argv[2]);
    return ok ? 0 : 1;
}
