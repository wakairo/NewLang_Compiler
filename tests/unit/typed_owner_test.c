#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
typedef struct {
    size_t trials, grants, calls, ends, releases;
} Counts;
static bool inspect(const NLCheckedFragment *f, Counts *counts)
{
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v->kind == NL_CHECKED_TRY_ALLOCATE_ONE) {
            counts->trials += v->result_count == 0;
            counts->grants += v->allocation_success;
        }
        counts->ends += v->kind == NL_CHECKED_DESTROY;
        counts->releases += v->kind == NL_CHECKED_DEALLOCATE;
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (v->owner_call.entry_proved) {
            ++counts->calls;
            CHECK(body != NULL && v->body_backed && v->owner_call.post_proved);
            CHECK(v->owner_call.definition.requirements ==
                  NL_OWNER_ALL_REQUIREMENTS);
            CHECK(v->owner_call.definition.step_count == 4 ||
                  v->owner_call.definition.step_count == 5);
            const NLSemanticContext *before = nl_checked_owner_entry(f, id),
                                    *after = nl_checked_context(f);
            CHECK(before != NULL && before != after &&
                  nl_checked_context(body) == after);
            CHECK(before->region_count == 2 && before->domain_count == 2);
            const NLPlaceId root = v->owner_call.root;
            const NLBackingRegionId region = v->owner_call.range.region;
            const NLDomainId domain = v->owner_call.domain;
            CHECK(before->places[root - 1].live &&
                  before->regions[region - 1].view.live &&
                  before->domains[domain - 1].live);
            CHECK(before->places[root - 1].incarnation ==
                      v->owner_call.incarnation &&
                  before->places[root - 1].governing_domain == domain);
            CHECK(!after->places[root - 1].live &&
                  after->places[root - 1].placement.region == 0 &&
                  after->places[root - 1].current_value == 0);
            CHECK(!after->regions[region - 1].view.live &&
                  !after->domains[domain - 1].live);
            size_t other_live = 0;
            for (size_t p = 0; p < before->place_count; ++p)
                if (before->places[p].live &&
                    before->places[p].independent_root &&
                    before->places[p].placement.region != 0 && p + 1 != root) {
                    ++other_live;
                    CHECK(before->places[p].placement.region != region &&
                          before->places[p].governing_domain != domain);
                }
            CHECK(other_live == 1);
            for (size_t i = 0; i < 3; ++i) {
                const NLSymbolId donor = v->owner_call.donor[i],
                                 parameter = v->owner_call.parameters[i];
                CHECK(donor != parameter && donor != 0 && parameter != 0);
                CHECK(after->bindings[parameter - 1].view.value ==
                      v->owner_call.inputs[i]);
                if (i != 0) {
                    CHECK(before->values[v->owner_call.inputs[i] - 1].carrier ==
                          NL_CARRIER_LOOSE);
                    CHECK(before->bindings[donor - 1].view.availability ==
                              NL_CONSUMED &&
                          after->bindings[donor - 1].view.availability ==
                              NL_CONSUMED);
                    CHECK(after->bindings[parameter - 1].view.availability ==
                              NL_CONSUMED &&
                          after->values[v->owner_call.inputs[i] - 1].carrier ==
                              NL_CARRIER_ENDED);
                }
            }
            CHECK(
                before->values[v->owner_call.inputs[1] - 1].allocation_region ==
                region);
            CHECK(before->values[v->owner_call.inputs[2] - 1].domain == domain);
            size_t ends = 0, frees = 0;
            for (NLCheckedNodeId n = 1; n <= nl_checked_node_count(body); ++n) {
                const NLCheckedNodeView *op = nl_checked_node_view(body, n);
                if (op->kind == NL_CHECKED_DESTROY) {
                    ++ends;
                    CHECK(
                        op->lifetime_place == root &&
                        op->lifetime_incarnation == v->owner_call.incarnation &&
                        op->lifetime_domain == domain && op->backing == region);
                    CHECK(after->values[op->results[0].value - 1]
                              .occupancy.region == region);
                }
                if (op->kind == NL_CHECKED_ERASE_SLOT)
                    CHECK(after->values[op->results[0].value - 1]
                              .occupancy.region == region);
                if (op->kind == NL_CHECKED_DEALLOCATE) {
                    ++frees;
                    const NLCheckedNodeView *a =
                        nl_checked_node_view(body, op->first_argument);
                    const NLCheckedNodeView *raw =
                        nl_checked_node_view(body, a->next_argument);
                    CHECK(a->result_count == 1 && raw->result_count == 1);
                    CHECK(after->values[a->results[0].value - 1]
                              .allocation_region == region);
                    CHECK(after->values[raw->results[0].value - 1]
                              .occupancy.region == region);
                }
            }
            CHECK(ends == 1 && frees == 1);
        }
        if (body != NULL)
            CHECK(inspect(body, counts));
        if (v->kind == NL_CHECKED_MATCH) {
            const NLCheckedNodeView *trial =
                nl_checked_node_view(f, v->initializer);
            if (trial != NULL && trial->kind == NL_CHECKED_TRY_ALLOCATE_ONE) {
                const NLCheckedFragment *none = nl_checked_match_arm(f, id, 0),
                                        *some = nl_checked_match_arm(f, id, 1);
                Counts no = {0}, yes = {0};
                CHECK(inspect(none, &no) && inspect(some, &yes));
                CHECK(nl_checked_node_view(none, nl_checked_root(none))
                          ->variant == 1);
                CHECK(nl_checked_node_view(some, nl_checked_root(some))
                          ->variant == 2);
                if (v->captured_frame_closed) {
                    CHECK(no.calls == 0 && no.ends == 1 && no.releases == 1);
                    CHECK(yes.calls == 1 && yes.ends == 2 && yes.releases == 2);
                    CHECK(nl_checked_context(none)->region_count == 1 &&
                          nl_checked_context(some)->region_count == 2);
                    const NLSemanticContext *worlds[] = {
                        nl_checked_context(none), nl_checked_context(some)};
                    for (size_t w = 0; w < 2; ++w) {
                        for (size_t r = 0; r < worlds[w]->region_count; ++r)
                            CHECK(!worlds[w]->regions[r].view.live);
                        for (size_t d = 0; d < worlds[w]->domain_count; ++d)
                            CHECK(!worlds[w]->domains[d].live);
                    }
                } else {
                    CHECK(no.trials == 0 && no.grants == 0 && no.calls == 0 &&
                          no.ends == 0 && no.releases == 0);
                    CHECK(nl_checked_context(none)->region_count == 0 &&
                          nl_checked_context(none)->domain_count == 0);
                }
            }
            for (size_t a = 0; a < v->item_count; ++a)
                CHECK(inspect(nl_checked_match_arm(f, id, a), counts));
        }
    }
    return true;
}
static bool evidence(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n)); /* destroys original AST/source */
    Counts counts = {0};
    CHECK(inspect(n.entry, &counts));
    CHECK(counts.trials == 2 && counts.grants == 2 && counts.calls == 1 &&
          counts.ends == 3 && counts.releases == 3);
    char *c = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(n.entry, &c, &length) == NL_NODE_C_OK &&
          c != NULL && length == strlen(c));
    free(c);
    node_checked_destroy(&n);
    return true;
}
static bool definition(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {tree};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    /* Conditional registration produced no concrete grant/world, even though
     * main's separate definition validation checked hypothetical source paths.
     */
    CHECK(c->binding_count == 0 && c->value_count == 0 && c->place_count == 0 &&
          c->region_count == 0 && c->domain_count == 0);
    size_t definitions = 0;
    for (size_t f = 1; f <= c->function_count; ++f) {
        NLTypedOwnerDefinition d = {0};
        if (nl_semantic_function_applicability(c, f, &d)) {
            ++definitions;
            CHECK(d.definition_checked &&
                  d.requirements == NL_OWNER_ALL_REQUIREMENTS &&
                  d.step_count == 5);
            for (size_t s = 0; s < 5; ++s)
                CHECK(d.steps[s] == (NLTypedOwnerStep)s);
        }
    }
    CHECK(definitions == 1);
    nl_semantic_destroy(c);
    return true;
}
static bool precision_walk(const NLCheckedFragment *f, size_t *tested)
{
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v->owner_call.entry_proved) {
            const NLSemanticContext *entry = nl_checked_owner_entry(f, id);
            CHECK(entry != NULL);
            for (size_t mutation = 0; mutation < 11; ++mutation) {
                NLSemanticContext *c = NULL;
                CHECK(nl_sem_clone(entry, &c) == NL_CHECK_OK);
                NLSemanticValueView *p =
                                        &c->values[v->owner_call.inputs[0] - 1],
                                    *a =
                                        &c->values[v->owner_call.inputs[1] - 1],
                                    *d =
                                        &c->values[v->owner_call.inputs[2] - 1];
                NLCheckStatus expected = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
                switch (mutation) {
                case 0:
                    break; /* source-derived positive baseline */
                case 1:
                    p->reference.provenance = NL_PROVENANCE_UNKNOWN;
                    break;
                case 2:
                    p->reference_count = 2;
                    break;
                case 3:
                    p->dependencies = NL_DEPENDENCIES_UNKNOWN;
                    break;
                case 4:
                    c->places[v->owner_call.root - 1].placement.length -= 1;
                    break;
                case 5:
                    c->regions[v->owner_call.range.region - 1].view.size += 1;
                    break;
                case 6:
                    p->reference.incarnation += 1;
                    expected = NL_CHECK_SEMANTIC_ERROR;
                    break;
                case 7:
                    a->allocation_region = 1;
                    expected = NL_CHECK_SEMANTIC_ERROR;
                    break;
                case 8:
                    d->domain = 1;
                    expected = NL_CHECK_SEMANTIC_ERROR;
                    break;
                case 9:
                    c->places[v->owner_call.root - 1].live = false;
                    expected = NL_CHECK_SEMANTIC_ERROR;
                    break;
                case 10:
                    c->regions[v->owner_call.range.region - 1]
                        .view.ordinary_read = false;
                    break;
                }
                NLCheckDiagnostic diagnostic = {0};
                NLSemanticSnapshot before = {0}, after = {0};
                CHECK(nl_semantic_snapshot(c, &before));
                CHECK(nl_owner_relations(c, v->owner_call.inputs,
                                         &v->owner_call.definition,
                                         (NLSourceSpan){0}, &diagnostic) ==
                      (mutation == 0 ? NL_CHECK_OK : expected));
                CHECK(nl_semantic_snapshot(c, &after) &&
                      memcmp(&before, &after, sizeof(before)) == 0);
                nl_semantic_destroy(c);
                ++*tested;
            }
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body != NULL)
            CHECK(precision_walk(body, tested));
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a)
                CHECK(precision_walk(nl_checked_match_arm(f, id, a), tested));
    }
    return true;
}
static bool precision(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    size_t tested = 0;
    CHECK(precision_walk(n.entry, &tested) && tested == 11);
    node_checked_destroy(&n);
    return true;
}
static bool faults(const char *path)
{
    NLSource *source = NULL, *entry = NULL;
    NLParser *parser = NULL, *ep = NULL;
    NLSyntaxTree *tree = NULL, *et = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    const NLSyntaxTree *inputs[] = {tree};
    bool complete = false;
    for (fail_at = 0; fail_at < 30000; ++fail_at) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        NLSemanticSnapshot before = {0}, after = {0};
        CHECK(nl_semantic_snapshot(c, &before));
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            complete = true;
            nl_semantic_destroy(c);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY);
        CHECK(nl_semantic_snapshot(c, &after) &&
              memcmp(&before, &after, sizeof(before)) == 0);
        CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
              NL_CHECK_OK);
        nl_semantic_destroy(c);
    }
    CHECK(complete);
    printf("registration fault points: %zu\n", fail_at);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    CHECK(nl_source_create("main()", 6, "entry", &entry) == NL_SOURCE_OK);
    CHECK(nl_parser_create(entry, &ep) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(ep, &et, NULL) == NL_PARSE_OK);
    NLSemanticSnapshot before = {0}, after = {0};
    CHECK(nl_semantic_snapshot(c, &before));
    complete = false;
    for (fail_at = 0; fail_at < 30000; ++fail_at) {
        NLCheckedFragment *out = NULL;
        CHECK(nl_semantic_snapshot(c, &before));
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status = nl_semantic_check_expression(c, et, &out, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            complete = true;
            nl_checked_destroy(out);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && out == NULL);
        CHECK(nl_semantic_snapshot(c, &after) &&
              memcmp(&before, &after, sizeof(before)) == 0);
        CHECK(nl_semantic_check_expression(c, et, &out, NULL) == NL_CHECK_OK);
        nl_checked_destroy(out);
    }
    CHECK(complete);
    printf("call/branch/snapshot fault points: %zu\n", fail_at);
    nl_syntax_tree_destroy(et);
    nl_parser_destroy(ep);
    nl_source_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return EXIT_FAILURE;
    const bool ok = strcmp(argv[1], "definition") == 0 ? definition(argv[2])
                    : strcmp(argv[1], "evidence") == 0 ? evidence(argv[2])
                    : strcmp(argv[1], "precision") == 0
                        ? precision(argv[2])
                        : strcmp(argv[1], "faults") == 0 && faults(argv[2]);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
