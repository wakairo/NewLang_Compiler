#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>

static bool evidence(const NLCheckedFragment *f)
{
    const NLSemanticContext *c = nl_checked_context(f);
    NLPlaceId heap = 0, lexical = 0, child = 0;
    NLValueId sibling = 0;
    NLIncarnationId incarnation = 0, child_incarnation = 0;
    NLOccurrenceId occurrence = 0;
    size_t roots = 0, projections = 0, changes = 0, reads = 0, endings = 0;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_INITIALIZE) {
            heap = v->lifetime_place;
            incarnation = v->lifetime_incarnation;
            CHECK(heap != 0 && v->reference_result.writable &&
                  v->lifetime_range.region != 0 &&
                  v->lifetime_range.length == 24);
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, v->first_argument);
            a = nl_checked_node_view(f, a->next_argument);
            sibling = c->values[a->results[0].value - 1].fields[1];
        }
        if (v->kind == NL_CHECKED_PTR_FROM_REF)
            lexical = v->reference_result.place;
        if (v->kind == NL_CHECKED_REF_FROM_PTR) {
            ++roots;
            CHECK(v->lifetime_place == heap &&
                  v->lifetime_incarnation == incarnation);
            const NLSemanticTypeView t = c->types[v->type - 1].view;
            CHECK(t.kind == NL_TYPE_REF && !t.is_exclusive && t.is_copy &&
                  t.is_discardable);
            CHECK(v->reference_result.writable ==
                  (t.access == NL_ACCESS_WRITE));
            CHECK(v->reference_result.scope != 0 && v->lifetime_domain != 0);
            CHECK(v->argument_count == 2 && v->lifetime_range.region != 0);
            const NLCheckedNodeView *p =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *s =
                nl_checked_node_view(f, p->next_argument);
            CHECK(p->symbol != 0 && s != NULL && s->symbol != 0 &&
                  p->value_use == NL_VALUE_COPIED &&
                  s->value_use == NL_VALUE_COPIED);
        }
        if (v->kind == NL_CHECKED_FIELD_REF) {
            ++projections;
            const NLCheckedField p = v->field;
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, v->first_argument);
            CHECK(a != NULL && a->kind == NL_CHECKED_IDENTIFIER &&
                  a->value_use == NL_VALUE_COPIED && a->symbol == p.base);
            const NLSemanticTypeView rt = c->types[a->type - 1].view,
                                     ft = c->types[v->type - 1].view;
            CHECK(ft.kind == NL_TYPE_REF && !ft.is_exclusive &&
                  ft.target == p.type && ft.access == rt.access &&
                  p.access == rt.access && rt.target == p.nominal &&
                  p.index == 0);
            CHECK(p.parent == heap && p.parent_incarnation == incarnation &&
                  p.child != heap && p.child_incarnation != 0 &&
                  p.dependency_compatible && p.old_value != 0);
            if (child == 0) {
                child = p.child;
                child_incarnation = p.child_incarnation;
            }
            CHECK(p.child == child && p.child_incarnation == child_incarnation);
            const NLSemanticValueView parent =
                                          c->values[a->results[0].value - 1],
                                      result =
                                          c->values[v->results[0].value - 1];
            CHECK(result.reference.scope == parent.reference.scope &&
                  result.reference.provenance == parent.reference.provenance &&
                  result.reference.writable == parent.reference.writable &&
                  result.reference.occurrence_dependency ==
                      parent.reference.occurrence_dependency);
            CHECK(result.dependencies == parent.dependencies &&
                  result.value_dependency_count ==
                      parent.value_dependency_count &&
                  memcmp(result.value_dependencies, parent.value_dependencies,
                         sizeof(result.value_dependencies)) == 0);
            CHECK(c->places[p.child - 1].parent_aggregate == heap &&
                  c->places[p.child - 1].governing_domain ==
                      v->lifetime_domain);
        }
        if (v->kind == NL_CHECKED_REPLACE && v->field.present) {
            ++changes;
            const NLCheckedField p = v->field;
            /* The call node is reserved before its checked argument nodes;
             * follow recorded pre/post evidence, not allocation order. */
            if (child == 0) {
                child = p.child;
                child_incarnation = p.child_incarnation;
            }
            CHECK(p.parent == heap && p.child == child && p.base != 0 &&
                  p.parent_incarnation == incarnation &&
                  p.child_incarnation == child_incarnation &&
                  p.parent_fact != p.parent_post_fact &&
                  p.child_fact != p.child_post_fact);
            CHECK(v->results[0].value == p.old_value);
            CHECK(c->values[p.old_value - 1].variant ==
                      (changes == 1 ? 1u : 2u) &&
                  c->values[p.new_value - 1].variant ==
                      (changes == 1 ? 2u : 1u));
            if (changes == 1) {
                CHECK(p.payload_occurrence == 0 &&
                      p.post_payload_occurrence != 0);
                occurrence = p.post_payload_occurrence;
            } else {
                CHECK(changes == 2 && p.payload_occurrence == occurrence &&
                      p.post_payload_occurrence == 0);
                CHECK(!c->occurrences[occurrence - 1].live);
            }
            bool preserved = false;
            for (size_t n = 0; n < c->value_count; ++n)
                if (c->values[n].field_count == 2 &&
                    c->values[n].fields[0] == p.new_value &&
                    c->values[n].fields[1] == sibling)
                    preserved = true;
            CHECK(preserved);
        }
        if (v->kind == NL_CHECKED_LINK_READ) {
            ++reads;
            const NLSemanticValueView original =
                                          c->values[v->field.old_value - 1],
                                      copy = c->values[v->results[0].value - 1];
            CHECK(copy.variant == 2 &&
                  copy.sum_payload != original.sum_payload &&
                  v->results[0].value != v->field.old_value);
            const NLSemanticValueView old_ptr =
                                          c->values[original.sum_payload - 1],
                                      ptr = c->values[copy.sum_payload - 1];
            CHECK(ptr.reference.place == lexical && lexical != heap &&
                  ptr.reference.scope == 0 &&
                  ptr.reference.provenance == old_ptr.reference.provenance &&
                  ptr.reference.incarnation == old_ptr.reference.incarnation &&
                  ptr.dependencies == old_ptr.dependencies &&
                  copy.dependencies == original.dependencies);
        }
        if (v->kind == NL_CHECKED_DESTROY) {
            ++endings;
            CHECK(changes == 2 && reads == 1 && v->lifetime_place == heap &&
                  v->lifetime_incarnation == incarnation);
        }
    }
    CHECK(roots == 3 && projections == 3 && changes == 2 && reads == 1 &&
          endings == 1);
    CHECK(!c->places[heap - 1].live && c->region_count == 1 &&
          !c->regions[0].view.live && c->domain_count == 1 &&
          !c->domains[0].live);
    for (size_t i = 0; i < c->binding_count; ++i) {
        const NLSemanticTypeView t =
            c->types[c->bindings[i].view.type - 1].view;
        if (!t.is_copy && !t.is_discardable)
            CHECK(c->bindings[i].view.availability == NL_CONSUMED);
    }
    return true;
}
static bool run(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(body != NULL);
    bool some = false, none = false;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(body, i);
        if (v->kind != NL_CHECKED_MATCH)
            continue;
        for (size_t a = 0; a < v->item_count; ++a) {
            const NLCheckedFragment *arm = nl_checked_match_arm(body, i, a);
            const NLCheckedNodeView *root =
                nl_checked_node_view(arm, nl_checked_root(arm));
            CHECK(root->type == nl_semantic_unit_type(nl_checked_context(arm)));
            if (root->variant == 2) {
                CHECK(evidence(arm));
                some = true;
            } else {
                CHECK(nl_checked_context(arm)->region_count == 0 &&
                      nl_checked_context(arm)->domain_count == 0);
                none = true;
            }
        }
    }
    CHECK(some && none && n.context->region_count == 0 &&
          n.context->domain_count == 0);
    char *out = NULL;
    size_t length = 999;
    CHECK(nl_checked_c_node(n.entry, &out, &length) == NL_NODE_C_UNSUPPORTED &&
          out == NULL && length == 999);
    node_checked_destroy(&n);
    return true;
}
static bool rejection_rollback(const char *path)
{
    NLSource *good = NULL, *bad = NULL;
    CHECK(nl_source_load(path, &good) == NL_SOURCE_OK);
    NLSourceView contents;
    CHECK(nl_source_view(good, (NLSourceSpan){0, nl_source_length(good)},
                         &contents));
    char *text = malloc(contents.length + 1);
    CHECK(text != NULL);
    memcpy(text, contents.bytes, contents.length);
    text[contents.length] = 0;
    char *at = strstr(text, "ref_from_ptr(write, heap_head, stable)");
    CHECK(at != NULL);
    memcpy(at + strlen("ref_from_ptr("), "read ", 5);
    CHECK(nl_source_create(text, contents.length, "readonly-negative", &bad) ==
          NL_SOURCE_OK);
    free(text);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState before;
    CHECK(test_state(c, &before));
    NLCheckDiagnostic first = {0};
    for (size_t i = 0; i < 3; ++i) {
        NLParser *parser = NULL;
        NLSyntaxTree *tree = NULL;
        CHECK(nl_parser_create(i == 2 ? good : bad, &parser) == NL_PARSE_OK);
        CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) ==
              NL_PARSE_OK);
        const NLSyntaxTree *units[] = {tree};
        NLFunctionUnitDiagnostic d = {0};
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, units, 1, &d);
        if (i < 2) {
            CHECK(status == NL_CHECK_SEMANTIC_ERROR &&
                  test_unchanged(c, &before));
            CHECK(strcmp(d.diagnostic.diagnostic.code, "P3-TYPE-MISMATCH") ==
                  0);
            if (i == 0)
                first = d.diagnostic;
            else
                CHECK(first.span.start_byte == d.diagnostic.span.start_byte &&
                      first.span.end_byte == d.diagnostic.span.end_byte &&
                      strcmp(first.diagnostic.code,
                             d.diagnostic.diagnostic.code) == 0);
        } else
            CHECK(status == NL_CHECK_OK && nl_sem_validate(c) == NL_CHECK_OK);
        nl_syntax_tree_destroy(tree);
        nl_parser_destroy(parser);
    }
    nl_semantic_destroy(c);
    nl_source_destroy(good);
    nl_source_destroy(bad);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && run(argv[1]) && rejection_rollback(argv[1]) ? 0 : 1;
}
