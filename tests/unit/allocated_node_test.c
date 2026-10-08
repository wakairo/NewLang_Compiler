#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t index_at, failure_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && index_at++ == failure_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && index_at++ == failure_at)
        return NULL;
    return __real_realloc(p, n);
}
static bool inspect(const NLCheckedFragment *f, size_t *trials, size_t *worlds)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_TRY_ALLOCATE_ONE) {
            ++*trials;
            CHECK(v->allocation_target != 0 && v->allocation_size == 24 &&
                  v->allocation_alignment == 8);
            if (v->result_count == 0) {
                CHECK(v->allocation_authority == 0 &&
                      v->storage_authority == 0 && v->backing == 0);
            } else {
                ++*worlds;
                const NLSemanticContext *c = nl_checked_context(f);
                if (v->allocation_success) {
                    CHECK(v->backing != 0 && v->allocation_authority != 0 &&
                          v->storage_authority != 0);
                    CHECK(c->values[v->allocation_authority - 1]
                              .allocation_region == v->backing);
                    CHECK(
                        c->values[v->storage_authority - 1].occupancy.region ==
                        v->backing);
                    CHECK(!c->regions[v->backing - 1].view.live);
                    CHECK(c->values[v->allocation_authority - 1].carrier ==
                          NL_CARRIER_ENDED);
                    CHECK(c->values[v->storage_authority - 1].carrier ==
                          NL_CARRIER_ENDED);
                    size_t starts = 0, ends = 0, refs = 0, deallocs = 0,
                           changes = 0;
                    NLCheckedNodeView start = {0};
                    NLPlaceId head = 0;
                    for (NLCheckedNodeId j = 1; j <= nl_checked_node_count(f);
                         ++j) {
                        const NLCheckedNodeView *op =
                            nl_checked_node_view(f, j);
                        if (op->kind == NL_CHECKED_INITIALIZE) {
                            ++starts;
                            start = *op;
                            CHECK(op->lifetime_domain != 0 &&
                                  op->backing == v->backing &&
                                  op->lifetime_range.start == 0 &&
                                  op->lifetime_range.length ==
                                      v->allocation_size);
                        }
                        if (op->kind == NL_CHECKED_DESTROY) {
                            ++ends;
                            CHECK(op->lifetime_domain ==
                                      start.lifetime_domain &&
                                  op->lifetime_place == start.lifetime_place &&
                                  op->lifetime_incarnation ==
                                      start.lifetime_incarnation &&
                                  op->lifetime_range.region ==
                                      start.lifetime_range.region &&
                                  op->lifetime_range.start ==
                                      start.lifetime_range.start &&
                                  op->lifetime_range.length ==
                                      start.lifetime_range.length);
                        }
                        if (op->kind == NL_CHECKED_DEALLOCATE) {
                            ++deallocs;
                            CHECK(ends == 1);
                        }
                        if (op->kind == NL_CHECKED_REPLACE) {
                            ++changes;
                            CHECK(op->field.present &&
                                  op->field.parent != start.lifetime_place);
                            if (head == 0)
                                head = op->field.parent;
                            CHECK(head == op->field.parent);
                        }
                        if (op->kind == NL_CHECKED_MATCH)
                            for (size_t a = 0; a < op->item_count; ++a) {
                                const NLCheckedFragment *arm =
                                    nl_checked_match_arm(f, j, a);
                                if (arm != NULL)
                                    for (NLCheckedNodeId k = 1;
                                         k <= nl_checked_node_count(arm); ++k)
                                        refs += nl_checked_node_view(arm, k)
                                                    ->kind ==
                                                NL_CHECKED_REF_FROM_PTR;
                            }
                    }
                    CHECK(starts == 1 && ends == 1 && deallocs == 1 &&
                          refs == 1 && changes == 2 && head != 0);
                    CHECK(nl_sem_validate((NLSemanticContext *)c) ==
                          NL_CHECK_OK);
                    CHECK(nl_raw_validate((NLSemanticContext *)c) ==
                          NL_CHECK_OK);
                } else {
                    CHECK(v->backing == 0 && v->allocation_authority == 0 &&
                          v->storage_authority == 0);
                    CHECK(c->region_count == 0 && c->domain_count == 0);
                }
            }
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body != NULL)
            CHECK(inspect(body, trials, worlds));
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a) {
                const NLCheckedFragment *arm = nl_checked_match_arm(f, i, a);
                if (arm != NULL)
                    CHECK(inspect(arm, trials, worlds));
            }
    }
    return true;
}
static bool evidence(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    size_t trials = 0, worlds = 0;
    CHECK(inspect(n.entry, &trials, &worlds));
    CHECK(trials == 3 && worlds == 2);
    CHECK(n.context->region_count == 0 && n.context->domain_count == 0);
    node_checked_destroy(&n);
    return true;
}
static bool failures(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    const NLSyntaxTree *inputs[] = {unit};
    bool done = false;
    for (failure_at = 0; failure_at < 30000; ++failure_at) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        size_t functions = c->function_count, initial_types = c->type_count;
        index_at = 0;
        injecting = true;
        NLFunctionUnitDiagnostic d = {0};
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, &d);
        injecting = false;
        if (status == NL_CHECK_OK) {
            done = true;
            nl_semantic_destroy(c);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY);
        CHECK(c->binding_count == 0 && c->region_count == 0 &&
              c->domain_count == 0 && c->function_count == functions &&
              c->type_count == initial_types);
        CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
              NL_CHECK_OK);
        nl_semantic_destroy(c);
    }
    CHECK(done);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    NLSource *entry = NULL;
    NLParser *ep = NULL;
    NLSyntaxTree *et = NULL;
    CHECK(nl_source_create("main()", 6, "allocated-entry", &entry) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(entry, &ep) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(ep, &et, NULL) == NL_PARSE_OK);
    size_t types = c->type_count, functions = c->function_count,
           values = c->value_count;
    done = false;
    for (failure_at = 0; failure_at < 30000; ++failure_at) {
        NLCheckedFragment *f = NULL;
        NLCheckDiagnostic d = {0};
        index_at = 0;
        injecting = true;
        NLCheckStatus status = nl_semantic_check_expression(c, et, &f, &d);
        injecting = false;
        if (status == NL_CHECK_OK) {
            done = true;
            nl_checked_destroy(f);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && f == NULL);
        CHECK(c->type_count == types && c->function_count == functions &&
              c->value_count == values);
        CHECK(c->region_count == 0 && c->domain_count == 0 &&
              c->binding_count == 0);
        CHECK(nl_sem_validate(c) == NL_CHECK_OK);
    }
    CHECK(done);
    nl_syntax_tree_destroy(et);
    nl_parser_destroy(ep);
    nl_source_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return EXIT_FAILURE;
    bool result = strcmp(argv[1], "evidence") == 0 ? evidence(argv[2])
                                                   : failures(argv[2]);
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
