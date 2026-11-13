#include "../../src/h_origin_internal.h"
#include "../support/semantic_check.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdlib.h>

/* Test-only observation of actual allocation loss INSIDE the original source
 * capture. The wrapper neither produces semantic values nor returns fake OOM.
 */
static size_t wanted_event, wanted_side, wanted_pass, wanted_offset;
static size_t matched, allocation_index;
static bool armed, in_capture, injected;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool lose(void)
{
    if (!in_capture)
        return false;
    if (allocation_index++ != wanted_offset)
        return false;
    injected = true;
    return true;
}
void *__wrap_malloc(size_t n)
{
    return lose() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return lose() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return lose() ? NULL : __real_realloc(p, n);
}
NLCheckStatus __real_nl_captured_change_begin(NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __real_nl_captured_change_end(NLCheckedFragment *,
                                            const NLSemanticContext *);
static bool select_capture(size_t event, size_t side)
{
    if (!armed || event != wanted_event || side != wanted_side)
        return false;
    ++matched;
    if (matched != wanted_pass)
        return false;
    allocation_index = 0;
    return true;
}
NLCheckStatus __wrap_nl_captured_change_begin(NLCheckedFragment *f,
                                              NLCheckedNodeId id,
                                              const NLSemanticContext *world)
{
    in_capture = select_capture(f->field_change_count + 1, 0);
    NLCheckStatus s = __real_nl_captured_change_begin(f, id, world);
    in_capture = false;
    return s;
}
NLCheckStatus __wrap_nl_captured_change_end(NLCheckedFragment *f,
                                            const NLSemanticContext *world)
{
    in_capture = select_capture(f->field_change_count, 1);
    NLCheckStatus s = __real_nl_captured_change_end(f, world);
    in_capture = false;
    return s;
}

typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *entry_syntax;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
} Program;
static void destroy(Program *p)
{
    nl_checked_destroy(p->entry);
    nl_syntax_tree_destroy(p->entry_syntax);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    *p = (Program){0};
}
static NLCheckStatus prepare(const char *path, Program *p)
{
    if (nl_source_load(path, &p->source) != NL_SOURCE_OK ||
        nl_parser_create(p->source, &p->parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(p->parser, &p->unit, NULL) !=
            NL_PARSE_OK ||
        nl_semantic_create(&p->context) != NL_CHECK_OK)
        return NL_CHECK_INTERNAL_ERROR;
    NLSemanticSnapshot before, after;
    if (!nl_semantic_snapshot(p->context, &before))
        return NL_CHECK_INTERNAL_ERROR;
    const NLSyntaxTree *units[] = {p->unit};
    NLCheckStatus s =
        nl_semantic_register_function_unit(p->context, units, 1, NULL);
    if (s != NL_CHECK_OK) {
        if (!nl_semantic_snapshot(p->context, &after) ||
            memcmp(&before, &after, sizeof(before)))
            return NL_CHECK_INTERNAL_ERROR;
        return s;
    }
    if (nl_source_create("main()", 6, "p1387-actual-entry", &p->entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(p->entry_source, &p->entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(p->entry_parser, &p->entry_syntax,
                                            NULL) != NL_PARSE_OK ||
        !nl_semantic_snapshot(p->context, &before))
        return NL_CHECK_INTERNAL_ERROR;
    s = nl_semantic_check_expression(p->context, p->entry_syntax, &p->entry,
                                     NULL);
    if (s != NL_CHECK_OK && (!nl_semantic_snapshot(p->context, &after) ||
                             memcmp(&before, &after, sizeof(before))))
        return NL_CHECK_INTERNAL_ERROR;
    return s;
}
static NLCheckedNodeId match(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_MATCH)
            return i;
    return 0;
}
static bool option_target(const NLSemanticContext *c, NLValueId id,
                          NLPlaceId root)
{
    NLSemanticValueView v;
    if (!nl_semantic_value_view(c, id, &v))
        return false;
    if (!root)
        return v.variant == 1 && !v.sum_payload;
    NLSemanticValueView ptr;
    NLSemanticPlaceView p;
    return v.variant == 2 && nl_semantic_value_view(c, v.sum_payload, &ptr) &&
           nl_semantic_place_view(c, root, &p) && p.live &&
           ptr.reference.place == root &&
           ptr.reference.incarnation == p.incarnation &&
           ptr.reference.provenance == NL_PROVENANCE_VALID &&
           ptr.reference.scope == 0;
}
static bool graph(const NLSemanticContext *c, const NLPlaceId *roots, bool post)
{
    /* Original source policy is an oracle, not a compiler authority input. */
    const size_t targets[5][3] = {{5, 5, 1},
                                  {2, post ? 2 : 3, 5},
                                  {post ? 5 : 3, 1, 5},
                                  {5, post ? 5 : 2, 5},
                                  {5, 5, 5}};
    for (size_t i = 0; i < 5; ++i) {
        NLSemanticPlaceView p;
        CHECK(nl_semantic_place_view(c, roots[i], &p) && p.live &&
              p.independent_root && p.fixed_field_count == 4 &&
              p.governing_domain && c->domains[p.governing_domain - 1].live);
        for (size_t j = 0; j < i; ++j) {
            NLSemanticPlaceView q;
            CHECK(nl_semantic_place_view(c, roots[j], &q) &&
                  p.placement.region != q.placement.region &&
                  p.governing_domain != q.governing_domain);
        }
        for (size_t j = 0; j < 3; ++j) {
            NLSemanticPlaceView f;
            CHECK(nl_semantic_place_view(c, p.fixed_fields[j], &f) && f.live &&
                  !f.independent_root && f.parent_aggregate == roots[i] &&
                  f.parent_incarnation == p.incarnation &&
                  f.parent_field_index == j &&
                  f.governing_domain == p.governing_domain &&
                  option_target(c, f.current_value,
                                targets[i][j] == 5 ? 0 : roots[targets[i][j]]));
        }
    }
    return true;
}
static bool reject(const NLCheckedFragment *top)
{
    return nl_checked_captured_closure_validate(top, match(top)) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT;
}
static bool owned(Program *p, bool nine, bool attacks)
{
    CHECK(nl_h_custody_validate(p->entry));
    const NLCheckedFragment *top =
        nl_checked_call_body(p->entry, nl_checked_root(p->entry));
    NLCapturedClosureView proof;
    CHECK(top && nl_checked_captured_closure_view(top, match(top), &proof) &&
          proof.count == 0 &&
          nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    for (size_t i = 0; i <= 5; ++i)
        CHECK(proof.release_worlds[i] == 1);
    const NLCheckedFragment *parent = top;
    for (size_t i = 0; i < 4; ++i)
        parent = nl_checked_match_arm(parent, match(parent), 1);
    CHECK(parent &&
          nl_checked_captured_closure_view(parent, match(parent), &proof) &&
          proof.count == 4);
    NLCheckedFragment *leaf =
        (NLCheckedFragment *)nl_checked_match_arm(parent, match(parent), 1);
    CHECK(leaf && nl_checked_captured_change_count(leaf) == (nine ? 9u : 6u));
    NLPlaceId roots[5] = {0};
    for (size_t i = 0; i < 4; ++i)
        roots[i] = proof.originals[i].root;
    for (size_t i = 1; i <= nl_checked_node_count(leaf); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(leaf, i);
        if (v->kind == NL_CHECKED_INITIALIZE)
            roots[4] = v->lifetime_place;
        if (v->kind == NL_CHECKED_LOAN_HEADER)
            CHECK(v->loan.body_nonescape_proved && v->loan.scope &&
                  !leaf->context->scopes[v->loan.scope - 1].active);
    }
    CHECK(roots[4]);
    NLCapturedChangeView setup;
    CHECK(nl_checked_captured_change_view(leaf, 5, &setup) &&
          graph(setup.after, roots, false));
    if (!nine)
        return true;
    NLCapturedChangeView last;
    CHECK(nl_checked_captured_change_view(leaf, 8, &last) &&
          graph(last.after, roots, true));
    for (size_t i = 0; i < 5; ++i) {
        const NLSemanticPlaceView a = setup.after->places[roots[i] - 1];
        const NLSemanticPlaceView b = last.after->places[roots[i] - 1];
        CHECK(a.incarnation == b.incarnation &&
              a.governing_domain == b.governing_domain &&
              !memcmp(a.fixed_fields, b.fixed_fields, sizeof(a.fixed_fields)));
    }
    const size_t owners[] = {2, 1, 3}, fields[] = {0, 1, 1}, old[] = {3, 3, 2};
    for (size_t i = 0; i < 3; ++i) {
        NLCapturedChangeView c;
        CHECK(nl_checked_captured_change_view(leaf, i + 6, &c));
        NLCheckedNodeView *v = &leaf->nodes[c.operation - 1];
        CHECK(
            v->field.parent == roots[owners[i]] &&
            v->field.index == fields[i] &&
            v->results[0].value == v->field.old_value &&
            option_target(c.before, v->field.old_value, roots[old[i]]) &&
            option_target(c.after, v->field.new_value, i == 1 ? roots[2] : 0) &&
            v->field.payload_occurrence &&
            v->field.payload_occurrence != v->field.post_payload_occurrence);
        if (!attacks)
            continue;
        const NLCheckedNodeView saved = *v;
        v->field.child_fact = 0;
        CHECK(reject(top));
        *v = saved;
        v->field.post_payload_occurrence = v->field.payload_occurrence;
        CHECK(reject(top));
        *v = saved;
        v->field.index = (v->field.index + 1) % 3;
        CHECK(reject(top));
        *v = saved;
        NLCapturedChange *record = leaf->field_changes[i + 6];
        const NLSemanticContext *before = record->before;
        record->before = leaf->field_changes[0]->before;
        CHECK(reject(top));
        record->before = (NLSemanticContext *)before;
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_OK);
    }
    if (attacks)
        puts(
            "P1387 genuine owned nine Changes: real old/current, five O/R/A/D, "
            "six release worlds, closed loans, 12 poisons/restores PASS");
    return true;
}
static bool check(const char *path, bool nine)
{
    Program p = {0};
    CHECK(prepare(path, &p) == NL_CHECK_OK);
    CHECK(owned(&p, nine, true));
    destroy(&p);
    return true;
}
static bool oom(const char *path)
{
    size_t cases = 0;
    for (wanted_event = 7; wanted_event <= 9; ++wanted_event)
        for (wanted_side = 0; wanted_side < 2; ++wanted_side)
            for (wanted_pass = 1; wanted_pass <= 2; ++wanted_pass)
                for (wanted_offset = 0; wanted_offset < 2; ++wanted_offset) {
                    Program p = {0};
                    matched = 0;
                    injected = false;
                    armed = true;
                    NLCheckStatus s = prepare(path, &p);
                    armed = false;
                    CHECK(injected && s == NL_CHECK_OUT_OF_MEMORY && !p.entry);
                    destroy(&p);
                    CHECK(prepare(path, &p) == NL_CHECK_OK &&
                          owned(&p, true, false));
                    destroy(&p);
                    ++cases;
                    printf("OOM event%zu %s %s allocation%zu: reached, "
                           "no publication/context clobber, retry PASS\n",
                           wanted_event, wanted_side ? "after" : "before",
                           wanted_pass == 1 ? "definition" : "actual",
                           wanted_offset);
                }
    CHECK(cases == 24);
    return true;
}
static bool resource_call(Program *p, bool emit, char **out, size_t *length,
                          bool failure)
{
    if (emit) {
        NLNodeCStatus s = nl_checked_c_node(p->entry, out, length);
        return s == (failure ? NL_NODE_C_OUT_OF_MEMORY : NL_NODE_C_OK);
    }
    const NLCheckedFragment *body =
        nl_checked_call_body(p->entry, nl_checked_root(p->entry));
    NLCheckStatus s = nl_checked_captured_closure_validate(body, match(body));
    return s == (failure ? NL_CHECK_OUT_OF_MEMORY : NL_CHECK_OK);
}
static bool resource_oom(const char *path, bool emit)
{
    Program p = {0};
    CHECK(prepare(path, &p) == NL_CHECK_OK && owned(&p, true, false));
    char *baseline = NULL;
    size_t size = 98765;
    wanted_offset = SIZE_MAX;
    allocation_index = 0;
    in_capture = true;
    bool ok = resource_call(&p, emit, &baseline, &size, false);
    in_capture = false;
    CHECK(ok);
    const size_t total = allocation_index;
    CHECK(total > 16);
    if (emit)
        CHECK(baseline && size == strlen(baseline));
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(p.context, &before));
    size_t targets[13];
    for (size_t i = 0; i < 8; ++i)
        targets[i] = i;
    targets[8] = total / 2;
    for (size_t i = 0; i < 4; ++i)
        targets[9 + i] = total - 4 + i;
    for (size_t i = 0; i < 13; ++i) {
        char *out = NULL;
        size_t length = 98765;
        wanted_offset = targets[i];
        allocation_index = 0;
        injected = false;
        in_capture = true;
        ok = resource_call(&p, emit, &out, &length, true);
        in_capture = false;
        CHECK(injected && ok && !out && length == 98765);
        CHECK(nl_semantic_snapshot(p.context, &after) &&
              !memcmp(&before, &after, sizeof(before)));
        CHECK(resource_call(&p, emit, &out, &length, false));
        if (emit)
            CHECK(length == size && !memcmp(out, baseline, size + 1));
        free(out);
        printf("%s OOM actual allocator %zu/%zu: no-clobber/retry PASS\n",
               emit ? "emitter" : "validator", targets[i], total);
    }
    free(baseline);
    destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    bool ok = !strcmp(argv[1], "emit-oom")       ? resource_oom(argv[2], true)
              : !strcmp(argv[1], "validate-oom") ? resource_oom(argv[2], false)
              : !strcmp(argv[1], "oom")          ? oom(argv[2])
              : !strcmp(argv[1], "six")          ? check(argv[2], false)
                                                 : check(argv[2], true);
    return ok ? 0 : 1;
}
