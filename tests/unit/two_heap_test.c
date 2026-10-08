#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdlib.h>

static const NLCheckedNodeView *find(const NLCheckedFragment *f,
                                     NLCheckedKind kind)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == kind)
            return nl_checked_node_view(f, i);
    return NULL;
}
static NLCheckedNodeId allocation_match(const NLCheckedFragment *f)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_MATCH && v->initializer != 0 &&
            nl_checked_node_view(f, v->initializer)->kind ==
                NL_CHECKED_TRY_ALLOCATE_ONE)
            return i;
    }
    return 0;
}
static bool pointer(NLSemanticValueView v, const NLCheckedNodeView *root)
{
    CHECK(v.reference_count == 0 && v.reference.place == root->lifetime_place &&
          v.reference.incarnation == root->lifetime_incarnation &&
          v.reference.provenance == NL_PROVENANCE_VALID &&
          v.reference.scope == 0 && v.dependencies == NL_DEPENDENCY_FREE &&
          v.value_dependency_count == 0);
    return true;
}
static bool selected_ref(const NLCheckedFragment *f, const NLCheckedNodeView *v,
                         const NLCheckedNodeView *root)
{
    const NLSemanticContext *c = nl_checked_context(f);
    CHECK(v->argument_count == 2 && v->has_reference_result &&
          v->lifetime_place == root->lifetime_place &&
          v->lifetime_incarnation == root->lifetime_incarnation &&
          v->lifetime_domain == root->lifetime_domain &&
          v->lifetime_range.region == root->backing &&
          v->lifetime_range.length == root->lifetime_range.length &&
          v->reference_result.provenance == NL_PROVENANCE_VALID);
    const NLCheckedNodeView *p = nl_checked_node_view(f, v->first_argument);
    const NLCheckedNodeView *s = nl_checked_node_view(f, p->next_argument);
    CHECK(p != NULL && s != NULL && p->kind == NL_CHECKED_IDENTIFIER &&
          s->kind == NL_CHECKED_IDENTIFIER && p->symbol != 0 &&
          s->symbol != 0 && p->value_use == NL_VALUE_COPIED &&
          s->value_use == NL_VALUE_COPIED);
    CHECK(pointer(c->values[p->results[0].value - 1], root));
    const NLSemanticValueView stable = c->values[s->results[0].value - 1];
    CHECK(stable.reference.place != 0);
    /* The selected source operand's target domain binding remains historical
     * after loan exit. Its reference place is the exact domain local carrier.
     */
    const NLPlaceId stable_place = stable.reference.place;
    bool matching = false;
    for (size_t b = 0; b < c->binding_count; ++b)
        if (c->bindings[b].view.place == stable_place)
            matching |= c->values[c->bindings[b].view.value - 1].domain ==
                        root->lifetime_domain;
    CHECK(matching && stable.reference.scope == v->reference_result.scope);
    CHECK(
        c->types[v->type - 1].view.kind == NL_TYPE_REF &&
        c->types[v->type - 1].view.access ==
            (v->reference_result.writable ? NL_ACCESS_WRITE : NL_ACCESS_READ));
    return true;
}
/* SUPPORTING mutations of an actual-source owned live snapshot. They are
 * not new source authorities or positive source-language evidence. */
static bool pressure(const NLSemanticContext *live,
                     const NLCheckedNodeView *head,
                     const NLCheckedNodeView *tail)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_sem_clone(live, &c) == NL_CHECK_OK);
    const NLPlaceId h = head->lifetime_place, t = tail->lifetime_place;
    const NLPlaceId link = c->places[h - 1].fixed_fields[0],
                    sibling = c->places[h - 1].fixed_fields[1];
    NLValueId ptr = 0;
    for (size_t i = 0; i < c->value_count; ++i)
        if (c->types[c->values[i].type - 1].view.kind == NL_TYPE_PTR &&
            c->values[i].reference.place == t)
            ptr = i + 1;
    CHECK(ptr != 0 && nl_allocated_write_access(c, ptr) == NL_CHECK_OK);
    c->values[ptr - 1].reference.provenance = NL_PROVENANCE_UNKNOWN;
    CHECK(nl_allocated_write_access(c, ptr) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    c->values[ptr - 1].reference.provenance = NL_PROVENANCE_VALID;
    c->values[ptr - 1].reference.writable = false;
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_SEMANTIC_ERROR);
    c->values[ptr - 1].reference.writable = true;
    c->regions[tail->backing - 1].view.ordinary_write = false;
    CHECK(nl_allocated_write_access(c, ptr) == NL_CHECK_SEMANTIC_ERROR);
    c->regions[tail->backing - 1].view.ordinary_write = true;
    const NLBackingRange placement = c->places[t - 1].placement;
    c->places[t - 1].placement = c->places[h - 1].placement;
    CHECK(nl_raw_validate(c) != NL_CHECK_OK); /* duplicated occupancy */
    c->places[t - 1].placement = placement;
    CHECK(nl_sem_validate(c) == NL_CHECK_OK);
    NLSemanticValueView blocker = {
        .type = nl_semantic_core_type(c, NL_TYPE_U8),
        .dependencies = NL_EXACT_VALUE_DEPENDENCIES,
        .value_dependency_count = 1,
        .value_dependencies = {{link, c->places[link - 1].current_fact}}};
    NLValueId dependent = 0;
    CHECK(nl_sem_new_value(c, blocker, &dependent) == NL_CHECK_OK);
    CHECK(nl_fixed_change_dependencies(c, link, 0) == NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_fixed_end_dependencies(c, h, SIZE_MAX) == NL_CHECK_SEMANTIC_ERROR);
    nl_sem_end_value(c, dependent);
    blocker.value_dependencies[0] =
        (NLValueDependency){sibling, c->places[sibling - 1].current_fact};
    CHECK(nl_sem_new_value(c, blocker, &dependent) == NL_CHECK_OK);
    CHECK(nl_fixed_change_dependencies(c, link, 0) == NL_CHECK_OK);
    nl_sem_end_value(c, dependent);
    /* Existing Reset conflict on this actual live head's Some occurrence.
     * Fresh fixture refs are SUPPORTING only, never source admission proof. */
    const NLOccurrenceId occurrence = c->places[link - 1].payload_occurrence;
    CHECK(occurrence != 0);
    const NLPlaceId payload = c->occurrences[occurrence - 1].payload_place;
    NLSymbolId w, r, replacement_binding;
    CHECK(test_reference(c, "fixture_writer", link, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &w, NULL));
    CHECK(test_reference(c, "fixture_payload", payload, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &r, NULL));
    c->values[c->bindings[r - 1].view.value - 1]
        .reference.occurrence_dependency = occurrence;
    NLValueId replacement = 0;
    CHECK(nl_sem_new_value(c,
                           (NLSemanticValueView){
                               .type = c->places[link - 1].type, .variant = 1},
                           &replacement) == NL_CHECK_OK);
    CHECK(nl_sem_bind(c, "fixture_none", replacement, &replacement_binding) ==
          NL_CHECK_OK);
    TestChecked rejected = {0};
    CHECK(test_run(c, "replace(fixture_writer,fixture_none)", TEST_EXPRESSION,
                   NL_CHECK_SEMANTIC_ERROR, "P6-OCCURRENCE-CONFLICT",
                   &rejected));
    test_checked_destroy(&rejected);
    nl_semantic_destroy(c);
    return true;
}

static bool fields(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n)); /* original source/AST disposed */
    const NLCheckedFragment *body = nl_checked_call_body(n.entry, 1);
    CHECK(body != NULL);
    const NLCheckedFragment *outer_some =
        nl_checked_match_arm(body, allocation_match(body), 1);
    CHECK(outer_some != NULL);
    const NLCheckedNodeView *head = find(outer_some, NL_CHECKED_INITIALIZE);
    NLCheckedNodeId inner_id = allocation_match(outer_some);
    const NLCheckedFragment *both =
        nl_checked_match_arm(outer_some, inner_id, 1);
    CHECK(head != NULL && both != NULL);
    const NLCheckedNodeView *tail = find(both, NL_CHECKED_INITIALIZE);
    CHECK(tail != NULL && tail->type == head->type &&
          tail->backing != head->backing &&
          tail->lifetime_domain != head->lifetime_domain &&
          tail->lifetime_place != head->lifetime_place &&
          tail->lifetime_incarnation != head->lifetime_incarnation);
    const NLSemanticContext *c = nl_checked_context(both);
    CHECK(c->region_count == 2 && c->domain_count == 2);
    NLPlaceId child = 0;
    NLIncarnationId child_inc = 0;
    NLOccurrenceId occurrence = 0;
    NLValueId sibling = 0;
    size_t root_refs = 0, projections = 0, changes = 0, reads = 0, ends = 0,
           releases = 0, option_arms = 0;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(both); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(both, i);
        if (v->kind == NL_CHECKED_REF_FROM_PTR) {
            CHECK(selected_ref(both, v, head));
            CHECK(v->reference_result.writable == (root_refs != 1));
            ++root_refs;
        }
        if (v->kind == NL_CHECKED_FIELD_REF) {
            ++projections;
            const NLCheckedField p = v->field;
            const NLCheckedNodeView *r =
                nl_checked_node_view(both, v->first_argument);
            CHECK(r != NULL && r->symbol == p.base &&
                  r->value_use == NL_VALUE_COPIED && p.present &&
                  p.parent == head->lifetime_place && p.index == 0 &&
                  p.parent_incarnation == head->lifetime_incarnation &&
                  p.dependency_compatible &&
                  v->lifetime_domain == head->lifetime_domain);
            const NLSemanticTypeView rt = c->types[r->type - 1].view,
                                     ft = c->types[v->type - 1].view;
            CHECK(rt.kind == NL_TYPE_REF && ft.kind == NL_TYPE_REF &&
                  rt.access == ft.access && p.access == ft.access &&
                  p.nominal == rt.target && ft.target == p.type);
            const NLSemanticValueView parent_ref =
                                          c->values[r->results[0].value - 1],
                                      field_ref =
                                          c->values[v->results[0].value - 1];
            CHECK(field_ref.reference.place == p.child &&
                  field_ref.reference.incarnation == p.child_incarnation &&
                  field_ref.reference.scope == parent_ref.reference.scope &&
                  field_ref.reference.provenance ==
                      parent_ref.reference.provenance &&
                  field_ref.reference.writable ==
                      parent_ref.reference.writable &&
                  field_ref.dependencies == parent_ref.dependencies &&
                  field_ref.value_dependency_count ==
                      parent_ref.value_dependency_count &&
                  memcmp(field_ref.value_dependencies,
                         parent_ref.value_dependencies,
                         sizeof(field_ref.value_dependencies)) == 0);
            if (child == 0) {
                child = p.child;
                child_inc = p.child_incarnation;
            }
            CHECK(p.child == child && p.child_incarnation == child_inc &&
                  c->places[child - 1].parent_aggregate ==
                      head->lifetime_place);
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            const NLCheckedField p = v->field;
            CHECK(p.present && p.parent == head->lifetime_place &&
                  p.parent_fact != p.parent_post_fact &&
                  p.child_fact != p.child_post_fact &&
                  p.parent_incarnation == head->lifetime_incarnation);
            CHECK(c->values[p.old_value - 1].variant ==
                      (changes == 0 ? 1u : 2u) &&
                  c->values[p.new_value - 1].variant ==
                      (changes == 0 ? 2u : 1u));
            if (changes == 0) {
                CHECK(p.post_payload_occurrence != 0 &&
                      p.payload_occurrence == 0);
                occurrence = p.post_payload_occurrence;
                CHECK(pointer(
                    c->values[c->values[p.new_value - 1].sum_payload - 1],
                    tail));
            } else {
                CHECK(changes == 1 && p.payload_occurrence == occurrence &&
                      p.post_payload_occurrence == 0 &&
                      !c->occurrences[occurrence - 1].live);
            }
            NLValueId kept = 0;
            for (size_t k = 0; k < c->value_count; ++k)
                if (c->values[k].field_count == 2 &&
                    c->values[k].fields[0] == p.new_value)
                    kept = c->values[k].fields[1];
            CHECK(kept != 0 && (sibling == 0 || sibling == kept));
            sibling = kept;
            ++changes;
        }
        if (v->kind == NL_CHECKED_LINK_READ) {
            ++reads;
            const NLSemanticValueView original =
                                          c->values[v->field.old_value - 1],
                                      copied =
                                          c->values[v->results[0].value - 1];
            CHECK(copied.variant == 2 &&
                  copied.sum_payload != original.sum_payload &&
                  v->results[0].value != v->field.old_value &&
                  v->value_use == NL_VALUE_COPIED);
            CHECK(pointer(c->values[copied.sum_payload - 1], tail));
            CHECK(pointer(c->values[original.sum_payload - 1], tail));
        }
        if (v->kind == NL_CHECKED_MATCH) {
            CHECK(v->normal_frame_unchanged && v->item_count == 2);
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *arm = nl_checked_match_arm(both, i, a);
                CHECK(arm != NULL);
                const NLSemanticContext *live = nl_checked_context(arm);
                CHECK(live->regions[head->backing - 1].view.live &&
                      live->regions[tail->backing - 1].view.live &&
                      live->domains[head->lifetime_domain - 1].live &&
                      live->domains[tail->lifetime_domain - 1].live &&
                      live->places[head->lifetime_place - 1].live &&
                      live->places[tail->lifetime_place - 1].live);
                const NLCheckedNodeView *roots[] = {head, tail};
                for (size_t nroot = 0; nroot < 2; ++nroot) {
                    size_t allocation_owners = 0, domain_owners = 0;
                    for (size_t b = 0; b < live->binding_count; ++b) {
                        const NLSemanticBindingView binding =
                            live->bindings[b].view;
                        if (binding.availability != NL_AVAILABLE)
                            continue;
                        const NLSemanticValueView value =
                            live->values[binding.value - 1];
                        allocation_owners +=
                            value.allocation_region == roots[nroot]->backing;
                        domain_owners +=
                            value.domain == roots[nroot]->lifetime_domain;
                    }
                    CHECK(allocation_owners == 1 && domain_owners == 1);
                }
                for (size_t p = 0; p < live->value_count; ++p) {
                    const NLSemanticValueView value = live->values[p];
                    const NLSemanticTypeKind kind =
                        live->types[value.type - 1].view.kind;
                    if (kind == NL_TYPE_STORAGE || kind == NL_TYPE_SLOT ||
                        live->types[value.type - 1].one_backing_target != 0)
                        CHECK(value.carrier == NL_CARRIER_ENDED);
                }
                CHECK(pressure(live, head, tail));
                const NLCheckedNodeView *r = find(arm, NL_CHECKED_REF_FROM_PTR);
                if (nl_checked_node_view(arm, nl_checked_root(arm))->variant ==
                    2) {
                    CHECK(r != NULL && !r->reference_result.writable);
                    CHECK(selected_ref(arm, r, tail));
                } else
                    CHECK(r == NULL);
                ++option_arms;
            }
        }
        if (v->kind == NL_CHECKED_DESTROY) {
            const NLCheckedNodeView *root = ends == 0 ? tail : head;
            CHECK(changes == 2 && reads == 1 &&
                  v->lifetime_place == root->lifetime_place &&
                  v->lifetime_incarnation == root->lifetime_incarnation &&
                  v->lifetime_domain == root->lifetime_domain &&
                  v->lifetime_range.region == root->backing);
            ++ends;
        }
        if (v->kind == NL_CHECKED_DEALLOCATE) {
            const NLCheckedNodeView *a =
                nl_checked_node_view(both, v->first_argument);
            const NLCheckedNodeView *s =
                nl_checked_node_view(both, a->next_argument);
            const NLCheckedNodeView *root = releases == 0 ? tail : head;
            CHECK(ends == releases + 1 &&
                  c->values[a->results[0].value - 1].allocation_region ==
                      root->backing &&
                  c->values[s->results[0].value - 1].occupancy.region ==
                      root->backing &&
                  c->values[s->results[0].value - 1].occupancy.length == 24);
            ++releases;
        }
    }
    CHECK(root_refs == 3 && projections == 3 && changes == 2 && reads == 1 &&
          ends == 2 && releases == 2 && option_arms == 2);
    for (size_t k = 0; k < c->value_count; ++k)
        if (!c->types[c->values[k].type - 1].view.is_discardable)
            CHECK(c->values[k].carrier == NL_CARRIER_ENDED);
    CHECK(!c->regions[0].view.live && !c->regions[1].view.live &&
          !c->domains[0].live && !c->domains[1].live);
    char *output = NULL;
    size_t length = 999;
    CHECK(nl_checked_c_node(n.entry, &output, &length) ==
              NL_NODE_C_UNSUPPORTED &&
          output == NULL && length == 999);
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
    char *at = strstr(text, "ref_from_ptr(write,");
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
    return argc == 2 && fields(argv[1]) && rejection_rollback(argv[1])
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
