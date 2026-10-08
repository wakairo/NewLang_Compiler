#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdio.h>
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t at, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && at++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && at++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
static void lifecycle_counts(const NLCheckedFragment *f, size_t *ends,
                             size_t *frees, size_t *producers)
{
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        *ends += v->kind == NL_CHECKED_DESTROY;
        *frees += v->kind == NL_CHECKED_DEALLOCATE;
        *producers += v->producer.entry_proved;
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body != NULL)
            lifecycle_counts(body, ends, frees, producers);
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t i = 0; i < v->item_count; ++i)
                lifecycle_counts(nl_checked_match_arm(f, id, i), ends, frees,
                                 producers);
    }
}
static bool inspect(NLCheckedFragment *f, size_t *producers, size_t *receivers,
                    size_t *destructures)
{
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        NLCheckedNodeView *v = &f->nodes[id - 1];
        if (v->producer.entry_proved) {
            ++*producers;
            CHECK(nl_checked_producer_valid(f, id));
            const NLSemanticContext *a = nl_checked_producer_entry(f, id),
                                    *b = nl_checked_producer_return(f, id);
            CHECK(a != b && a != nl_checked_context(f) &&
                  b != nl_checked_context(f));
            CHECK(a->region_count == 2 && b->region_count == 2 &&
                  a->domain_count == 2 && b->domain_count == 2);
            CHECK(b->places[v->producer.root - 1].live);
            CHECK(b->regions[v->producer.range.region - 1].view.live &&
                  b->domains[v->producer.domain - 1].live);
            CHECK(!b->types[v->results[0].type - 1].view.is_copy &&
                  !b->types[v->results[0].type - 1].view.is_discardable);
            CHECK(v->producer.root !=
                  b->bindings[v->producer.parameters[1] - 1].view.place);
            CHECK(v->producer.range.region !=
                  a->places[v->producer.head.parent - 1].placement.region);
            CHECK(v->producer.domain != v->producer.head_domain);
            CHECK(v->producer.head_before_fact != v->producer.head_after_fact);
            const NLCheckedFragment *body = nl_checked_call_body(f, id);
            size_t changes = 0, returns = 0, constructors = 0;
            for (NLCheckedNodeId n = 1; n <= nl_checked_node_count(body); ++n) {
                const NLCheckedNodeView *op = nl_checked_node_view(body, n);
                CHECK(op->kind != NL_CHECKED_DESTROY &&
                      op->kind != NL_CHECKED_DEALLOCATE &&
                      op->kind != NL_CHECKED_TRY_ALLOCATE_ONE);
                changes += op->kind == NL_CHECKED_REPLACE;
                returns += op->kind == NL_CHECKED_RETURN;
                constructors += op->kind == NL_CHECKED_AGGREGATE;
                if (op->kind == NL_CHECKED_REPLACE)
                    CHECK(op->field.child == v->producer.head.child &&
                          op->field.parent == v->producer.head.parent);
            }
            CHECK(changes == 1 && returns == 1 && constructors == 1);
            const NLCheckedNodeView saved = *v;
            for (size_t m = 0; m < 23; ++m) {
                switch (m) {
                case 0:
                    v->producer.entry_proved = false;
                    break;
                case 1:
                    v->producer.return_proved = false;
                    break;
                case 2:
                    v->producer.definition.live_return = false;
                    break;
                case 3:
                    v->producer.definition.head_link_required = false;
                    break;
                case 4:
                    v->producer.definition.requirements = 0;
                    break;
                case 5:
                    v->producer.definition.step_count = 1;
                    break;
                case 6:
                    v->producer.root = v->producer.head.parent;
                    break;
                case 7:
                    ++v->producer.incarnation;
                    break;
                case 8:
                    v->producer.domain = v->producer.head_domain;
                    break;
                case 9:
                    v->producer.range.region =
                        a->places[v->producer.head.parent - 1].placement.region;
                    break;
                case 10:
                    v->producer.result = 0;
                    break;
                case 11:
                    v->producer.head.child = v->producer.root;
                    break;
                case 12:
                    v->producer.head.parent = v->producer.root;
                    break;
                case 13:
                    ++v->producer.head_before_fact;
                    break;
                case 14:
                    v->producer.head_after_fact = v->producer.head_before_fact;
                    break;
                case 15:
                    v->producer.head_after = v->producer.head_before;
                    break;
                case 16:
                    v->producer.inputs[2] = v->producer.inputs[3];
                    break;
                case 17:
                    v->producer.parameters[2] = v->producer.parameters[3];
                    break;
                case 18:
                    v->producer.donor[2] = v->producer.parameters[2];
                    break;
                case 19:
                    v->producer.return_world = a;
                    break;
                case 20:
                    v->producer.entry_world = b;
                    break;
                case 21:
                    v->result_count = 0;
                    break;
                case 22:
                    v->body_backed = false;
                    break;
                }
                CHECK(!nl_checked_producer_valid(f, id));
                *v = saved;
                CHECK(nl_checked_producer_valid(f, id));
            }
            NLSemanticContext *entry = f->producer_entry;
            const NLBackingRegionId head_region =
                entry->places[v->producer.head.parent - 1].placement.region;
            const NLValueId head_value = v->producer.inputs[0],
                            tail_domain = v->producer.inputs[3];
            const NLValueId payload =
                entry->values[v->producer.head_before - 1].sum_payload;
            const NLScopeId scope =
                entry->values[head_value - 1].reference.scope;
            for (size_t m = 0; m < 7; ++m) {
                const NLSemanticBackingView region =
                    entry->regions[head_region - 1].view;
                const NLSemanticDomainView domain =
                    entry->domains[v->producer.head_domain - 1];
                const NLSemanticValueView ref = entry->values[head_value - 1],
                                          ptr = entry->values[payload - 1],
                                          d = entry->values[tail_domain - 1];
                const NLSemanticScopeView sc = entry->scopes[scope - 1];
                switch (m) {
                case 0:
                    entry->regions[head_region - 1].view.ordinary_write = false;
                    break;
                case 1:
                    entry->domains[v->producer.head_domain - 1].live = false;
                    break;
                case 2:
                    entry->values[head_value - 1].reference.writable = false;
                    break;
                case 3:
                    entry->scopes[scope - 1].active = false;
                    break;
                case 4:
                    entry->values[payload - 1].reference.place =
                        v->producer.head.parent;
                    break;
                case 5:
                    entry->values[tail_domain - 1].domain =
                        v->producer.head_domain;
                    break;
                case 6:
                    entry->values[head_value - 1].dependencies =
                        NL_DEPENDENCIES_UNKNOWN;
                    break;
                }
                CHECK(!nl_checked_producer_valid(f, id));
                entry->regions[head_region - 1].view = region;
                entry->domains[v->producer.head_domain - 1] = domain;
                entry->values[head_value - 1] = ref;
                entry->values[payload - 1] = ptr;
                entry->values[tail_domain - 1] = d;
                entry->scopes[scope - 1] = sc;
                CHECK(nl_checked_producer_valid(f, id));
            }
            NLSemanticContext *post = f->producer_return;
            const NLValueId package_id = v->producer.result;
            const NLSemanticValueView package = post->values[package_id - 1];
            const NLSemanticValueView pointer =
                post->values[package.fields[0] - 1];
            for (size_t m = 0; m < 10; ++m) {
                const NLSemanticPlaceView root =
                    post->places[v->producer.root - 1];
                const NLSemanticBackingView backing =
                    post->regions[v->producer.range.region - 1].view;
                const NLSemanticDomainView domain =
                    post->domains[v->producer.domain - 1];
                const NLSemanticValueView
                    allocation = post->values[package.fields[1] - 1],
                    life = post->values[package.fields[2] - 1];
                switch (m) {
                case 0:
                    post->places[v->producer.root - 1].live = false;
                    break;
                case 1:
                    post->regions[v->producer.range.region - 1].view.live =
                        false;
                    break;
                case 2:
                    post->domains[v->producer.domain - 1].live = false;
                    break;
                case 3:
                    post->values[package.fields[0] - 1].reference.scope =
                        a->values[v->producer.inputs[0] - 1].reference.scope;
                    break;
                case 4:
                    post->values[package.fields[0] - 1].dependencies =
                        NL_DEPENDENCIES_UNKNOWN;
                    break;
                case 5:
                    post->values[package.fields[1] - 1].allocation_region =
                        a->places[v->producer.head.parent - 1].placement.region;
                    break;
                case 6:
                    post->values[package.fields[2] - 1].domain =
                        v->producer.head_domain;
                    break;
                case 7:
                    post->values[package_id - 1].fields[2] = package.fields[1];
                    break;
                case 8:
                    post->regions[v->producer.range.region - 1]
                        .view.ordinary_write = false;
                    break;
                case 9:
                    post->values[package.fields[1] - 1].carrier =
                        NL_CARRIER_LOOSE;
                    break;
                }
                CHECK(!nl_checked_producer_valid(f, id));
                post->places[v->producer.root - 1] = root;
                post->regions[v->producer.range.region - 1].view = backing;
                post->domains[v->producer.domain - 1] = domain;
                post->values[package_id - 1] = package;
                post->values[package.fields[0] - 1] = pointer;
                post->values[package.fields[1] - 1] = allocation;
                post->values[package.fields[2] - 1] = life;
                CHECK(nl_checked_producer_valid(f, id));
            }
            NLSemanticContext *clone = NULL;
            CHECK(nl_sem_clone(f->producer_return, &clone) == NL_CHECK_OK);
            NLSemanticContext *original = f->producer_return;
            f->producer_return = clone;
            CHECK(!nl_checked_producer_valid(
                f, id)); /* identical numeric facts, different world */
            f->producer_return = original;
            nl_semantic_destroy(clone);
            CHECK(nl_checked_producer_valid(f, id));
        }
        if (v->owner_call.entry_proved) {
            ++*receivers;
            CHECK(v->owner_call.post_proved &&
                  !v->owner_call.definition.live_return);
            const NLSemanticContext *entry = nl_checked_owner_entry(f, id);
            CHECK(entry->places[v->owner_call.root - 1].live);
            CHECK(
                entry->values[v->owner_call.inputs[1] - 1].allocation_region ==
                v->owner_call.range.region);
            CHECK(entry->values[v->owner_call.inputs[2] - 1].domain ==
                  v->owner_call.domain);
        }
        if (v->kind == NL_CHECKED_AGGREGATE_BINDING &&
            f->context->types[v->type - 1].live_tail_target != 0)
            ++*destructures; /* node type is unit; receivers below checked
                                separately */
        if (v->kind == NL_CHECKED_AGGREGATE_BINDING) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, v->initializer);
            if (init->result_count == 1 &&
                f->context->types[init->results[0].type - 1].live_tail_target !=
                    0) {
                ++*destructures;
                CHECK(v->argument_count == 3);
                size_t affine = 0;
                for (NLCheckedNodeId n = v->first_argument; n != 0;
                     n = nl_checked_node_view(f, n)->next_argument) {
                    const NLCheckedNodeView *r = nl_checked_node_view(f, n);
                    CHECK(r->symbol != 0);
                    affine += !f->context->types[r->type - 1].view.is_copy;
                    CHECK(
                        f->context->bindings[r->symbol - 1].view.availability ==
                        NL_CONSUMED);
                }
                CHECK(affine == 2);
            }
        }
        if (v->kind == NL_CHECKED_MATCH &&
            nl_checked_node_view(f, v->initializer)->kind ==
                NL_CHECKED_TRY_ALLOCATE_ONE) {
            const NLCheckedFragment *none = nl_checked_match_arm(f, id, 0),
                                    *some = nl_checked_match_arm(f, id, 1);
            if (nl_checked_node_view(none, nl_checked_root(none))->variant !=
                1) {
                const NLCheckedFragment *swap = none;
                none = some;
                some = swap;
            }
            size_t ne = 0, nf = 0, np = 0, se = 0, sf = 0, sp = 0;
            lifecycle_counts(none, &ne, &nf, &np);
            lifecycle_counts(some, &se, &sf, &sp);
            CHECK(np == 0);
            if (v->captured_frame_closed) {
                CHECK(ne == 1 && nf == 1 && se == 2 && sf == 2 && sp == 1);
                CHECK(none->context->region_count == 1 &&
                      some->context->region_count == 2);
                const NLSemanticContext *worlds[] = {none->context,
                                                     some->context};
                for (size_t w = 0; w < 2; ++w) {
                    for (size_t r = 0; r < worlds[w]->region_count; ++r)
                        CHECK(!worlds[w]->regions[r].view.live);
                    for (size_t d = 0; d < worlds[w]->domain_count; ++d)
                        CHECK(!worlds[w]->domains[d].live);
                }
            } else
                CHECK(ne == 0 && nf == 0 && none->context->region_count == 0 &&
                      none->context->domain_count == 0);
        }
        if (v->kind == NL_CHECKED_MATCH) {
            for (size_t i = 0; i < v->item_count; ++i)
                CHECK(
                    inspect((NLCheckedFragment *)nl_checked_match_arm(f, id, i),
                            producers, receivers, destructures));
        }
        NLCheckedFragment *body =
            (NLCheckedFragment *)nl_checked_call_body(f, id);
        if (body != NULL)
            CHECK(inspect(body, producers, receivers, destructures));
    }
    return true;
}
static bool evidence(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    size_t producers = 0, receivers = 0, destructures = 0;
    CHECK(inspect(n.entry, &producers, &receivers, &destructures));
    CHECK(producers == 1 && receivers == 1 && destructures == 1);
    char *code = NULL;
    size_t length = 37;
    CHECK(nl_checked_c_node(n.entry, &code, &length) == NL_NODE_C_UNSUPPORTED &&
          code == NULL && length == 37);
    node_checked_destroy(&n);
    puts("owned live-return entry/post/correlation after AST teardown; 34 "
         "corruptions rejected; whole destructure and distinct terminal "
         "receiver; backend unsupported");
    return true;
}
static bool oom(const char *path)
{
    NLSource *s = NULL;
    NLParser *p = NULL;
    NLSyntaxTree *t = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &s) == NL_SOURCE_OK);
    CHECK(nl_parser_create(s, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p, &t, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {t};
    NLSemanticSnapshot before = {0}, after = {0};
    CHECK(nl_semantic_snapshot(c, &before));
    size_t faults = 0;
    for (fail_at = 0; fail_at < 12000; ++fail_at) {
        at = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, NULL);
        injecting = false;
        if (status == NL_CHECK_OK)
            break;
        CHECK(status == NL_CHECK_OUT_OF_MEMORY);
        CHECK(nl_semantic_snapshot(c, &after) &&
              memcmp(&before, &after, sizeof(before)) == 0);
        ++faults;
    }
    CHECK(fail_at < 12000 && faults > 100);
    CHECK(c->value_count == 0 && c->place_count == 0 && c->region_count == 0 &&
          c->domain_count == 0);
    nl_syntax_tree_destroy(t);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    s = NULL;
    p = NULL;
    t = NULL;
    CHECK(nl_source_create("main()", 6, "actual-entry", &s) == NL_SOURCE_OK);
    CHECK(nl_parser_create(s, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(p, &t, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_snapshot(c, &before));
    size_t calls = 0;
    for (fail_at = 0; fail_at < 12000; ++fail_at) {
        NLCheckedFragment *f = NULL;
        at = 0;
        injecting = true;
        NLCheckStatus status = nl_semantic_check_expression(c, t, &f, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            CHECK(f != NULL);
            nl_checked_destroy(f);
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && f == NULL);
        CHECK(nl_semantic_snapshot(c, &after) &&
              memcmp(&before, &after, sizeof(before)) == 0);
        ++calls;
    }
    CHECK(fail_at < 12000 && calls > 100);
    nl_syntax_tree_destroy(t);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    nl_semantic_destroy(c);
    printf("registration/branch/return/destructure OOM rollback: %zu + %zu "
           "fault points, clean completion\n",
           faults, calls);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    return (strcmp(argv[1], "oom") == 0 ? oom(argv[2]) : evidence(argv[2])) ? 0
                                                                            : 1;
}
