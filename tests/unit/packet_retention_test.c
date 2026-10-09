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
static bool retention_evidence(NLCheckedFragment *f, NLCheckedNodeId id)
{
    NLCheckedNodeView *m = &f->nodes[id - 1];
    CHECK(nl_checked_packet_retaining_join_valid(f, id));
    CHECK(nl_checked_packet_retention_release_valid(f, id));
    CHECK(!nl_checked_packet_fork_valid(f, id));
    const NLCheckedNodeView *p =
        nl_checked_node_view(f, m->packet_fork.producer);
    const NLSemanticContext *post = f->packet_retained_post;
    CHECK(post != f->packet_entry && post != f->context &&
          post->value_count == f->packet_entry->value_count &&
          post->binding_count == f->packet_entry->binding_count);
    CHECK(post->bindings[m->packet_fork.binding - 1].view.availability ==
              NL_AVAILABLE &&
          post->values[m->packet_fork.packet - 1].carrier == NL_CARRIER_PLACE);
    CHECK(post->places[p->producer.root - 1].live &&
          post->places[p->producer.head.parent - 1].live &&
          post->regions[p->producer.range.region - 1].view.live &&
          post->domains[p->producer.domain - 1].live);
    NLCheckedFragment *arms[2] = {
        (NLCheckedFragment *)nl_checked_match_arm(f, id, 0),
        (NLCheckedFragment *)nl_checked_match_arm(f, id, 1)};
    CHECK(arms[0] != arms[1] && arms[0]->context != arms[1]->context);
    size_t corruptions = 0;
    for (size_t a = 0; a < 2; ++a) {
        NLCheckedFragment *arm = arms[a];
        NLCheckedNodeView *r = &arm->nodes[arm->root - 1];
        CHECK(arm->owner_entry == NULL &&
              arm->context->bindings[m->packet_fork.binding - 1]
                      .view.availability == NL_AVAILABLE);
        CHECK(nl_packet_same_entry(f->packet_entry, arm->packet_entry));
        /* Same numeric root ID in distinct worlds never substitutes lineage. */
        CHECK(arm->context->places[p->producer.root - 1].incarnation ==
              p->producer.incarnation);
        const NLCheckedNodeView saved = *r;
        for (size_t n = 0; n < 7; ++n) {
            if (n == 0)
                *r = arms[1 - a]->nodes[arms[1 - a]->root - 1];
            if (n == 1)
                r->packet_origin.world = arms[1 - a]->context;
            if (n == 2)
                r->packet_origin.entry_world = arms[1 - a]->packet_entry;
            if (n == 3)
                ++r->packet_origin.packet;
            if (n == 4)
                ++r->packet_origin.match;
            if (n == 5)
                r->packet_origin.ancestor = arm;
            if (n == 6)
                r->variant = arms[1 - a]->nodes[arms[1 - a]->root - 1].variant;
            CHECK(!nl_checked_packet_retaining_join_valid(f, id));
            *r = saved;
            ++corruptions;
        }
        NLSemanticContext *clone = NULL;
        CHECK(nl_sem_clone(arm->context, &clone) == NL_CHECK_OK);
        const NLSemanticContext *world = arm->context;
        arm->context = clone;
        CHECK(!nl_checked_packet_retaining_join_valid(f, id));
        arm->context = world;
        nl_semantic_destroy(clone);
        ++corruptions;
        NLSemanticContext *c = (NLSemanticContext *)world;
        const NLSemanticPlaceView root = c->places[p->producer.root - 1];
        ++c->places[p->producer.root - 1].incarnation;
        CHECK(!nl_checked_packet_retaining_join_valid(f, id));
        c->places[p->producer.root - 1] = root;
        ++corruptions;
        ++c->places[p->producer.root - 1].current_fact;
        CHECK(!nl_checked_packet_retaining_join_valid(f, id));
        c->places[p->producer.root - 1] = root;
        ++corruptions;
        const NLValueId allocation = p->producer.inputs[2],
                        domain = p->producer.inputs[3];
        const NLSemanticValueView va = c->values[allocation - 1],
                                  vd = c->values[domain - 1];
        c->values[allocation - 1].allocation_region = p->producer.head.parent;
        CHECK(!nl_checked_packet_retaining_join_valid(f, id));
        c->values[allocation - 1] = va;
        ++corruptions;
        c->values[domain - 1].domain = p->producer.head_domain;
        CHECK(!nl_checked_packet_retaining_join_valid(f, id));
        c->values[domain - 1] = vd;
        ++corruptions;
        c->values[allocation - 1].dependencies = NL_DEPENDENCIES_UNKNOWN;
        CHECK(!nl_checked_packet_retaining_join_valid(f, id));
        c->values[allocation - 1] = va;
        ++corruptions;
        NLSemanticBindingView binding =
            c->bindings[m->packet_fork.binding - 1].view;
        c->bindings[m->packet_fork.binding - 1].view.availability = NL_CONSUMED;
        CHECK(!nl_checked_packet_retaining_join_valid(f, id));
        c->bindings[m->packet_fork.binding - 1].view = binding;
        ++corruptions;
    }
    NLCheckedNodeView saved_match = *m;
    m->normal_frame_unchanged = true;
    CHECK(!nl_checked_packet_retaining_join_valid(f, id));
    *m = saved_match;
    ++corruptions;
    m->packet_fork.post_world = arms[0]->context;
    CHECK(!nl_checked_packet_retaining_join_valid(f, id));
    *m = saved_match;
    ++corruptions;
    NLSemanticContext *mutable_post = f->packet_retained_post;
    NLSemanticPlaceView root = mutable_post->places[p->producer.root - 1];
    ++mutable_post->places[p->producer.root - 1].current_fact;
    CHECK(!nl_checked_packet_retaining_join_valid(f, id));
    mutable_post->places[p->producer.root - 1] = root;
    ++corruptions;
    size_t receives = 0;
    NLCheckedNodeView *receive = NULL;
    for (NLCheckedNodeId n = 1; n <= f->count; ++n)
        if (f->nodes[n - 1].packet_origin.ancestor != NULL) {
            receive = &f->nodes[n - 1];
            ++receives;
        }
    CHECK(receives == 1 && receive->packet_origin.entry_world == post);
    NLCheckedNodeView saved_receive = *receive;
    receive->packet_origin.world = arms[0]->context;
    CHECK(!nl_checked_packet_retention_release_valid(f, id));
    *receive = saved_receive;
    ++corruptions;
    receive->packet_origin.entry_world = f->packet_entry;
    CHECK(!nl_checked_packet_retention_release_valid(f, id));
    *receive = saved_receive;
    ++corruptions;
    NLCheckedNodeView *call = &f->nodes[f->owner_entry_call - 1];
    NLCheckedNodeView saved_call = *call;
    CHECK(call->owner_call.root == p->producer.root &&
          call->owner_call.range.region == p->producer.range.region &&
          call->owner_call.domain == p->producer.domain &&
          call->owner_call.inputs[1] == p->producer.inputs[2] &&
          call->owner_call.inputs[2] == p->producer.inputs[3]);
    const NLCheckedFragment *body =
        nl_checked_call_body(f, f->owner_entry_call);
    size_t ends = 0, frees = 0;
    for (NLCheckedNodeId n = 1; n <= body->count; ++n) {
        ends += body->nodes[n - 1].kind == NL_CHECKED_DESTROY;
        frees += body->nodes[n - 1].kind == NL_CHECKED_DEALLOCATE;
    }
    CHECK(ends == 1 && frees == 1);
    for (size_t n = 0; n < 8; ++n) {
        if (n == 0)
            call->owner_call.entry_proved = false;
        if (n == 1)
            call->owner_call.post_proved = false;
        if (n == 2)
            call->owner_call.root = p->producer.head.parent;
        if (n == 3)
            ++call->owner_call.incarnation;
        if (n == 4)
            call->owner_call.range.region = p->producer.head.parent;
        if (n == 5)
            call->owner_call.domain = p->producer.head_domain;
        if (n == 6)
            call->owner_call.inputs[1] = p->producer.inputs[3];
        if (n == 7)
            call->owner_call.inputs[2] = p->producer.inputs[2];
        CHECK(!nl_checked_packet_retention_release_valid(f, id));
        *call = saved_call;
        ++corruptions;
    }
    CHECK(!f->context->places[p->producer.head.parent - 1].live);
    CHECK(nl_checked_packet_retention_release_valid(f, id));
    printf("retained original packet=%zu O=%zu/%zu R=%zu D=%zu A=%zu; two "
           "worlds; late whole receiving/EndRoot/free; %zu rejected "
           "certificate corruptions\n",
           m->packet_fork.packet, p->producer.root, p->producer.incarnation,
           p->producer.range.region, p->producer.domain, p->producer.inputs[2],
           corruptions);
    return true;
}
static bool inspect(NLCheckedFragment *f, size_t *forks)
{
    for (NLCheckedNodeId id = 1; id <= f->count; ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v->packet_fork.retained) {
            ++*forks;
            CHECK(retention_evidence(f, id));
        }
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a)
                CHECK(
                    inspect((NLCheckedFragment *)nl_checked_match_arm(f, id, a),
                            forks));
        NLCheckedFragment *body =
            (NLCheckedFragment *)nl_checked_call_body(f, id);
        if (body != NULL)
            CHECK(inspect(body, forks));
    }
    return true;
}
static bool evidence(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    /* Proof remains usable with no original source/AST available. */
    nl_source_destroy(n.entry_source);
    n.entry_source = NULL;
    size_t forks = 0;
    CHECK(inspect(n.entry, &forks) && forks == 1);
    char *code = NULL;
    size_t length = 37;
    CHECK(nl_checked_c_node(n.entry, &code, &length) == NL_NODE_C_UNSUPPORTED);
    CHECK(code == NULL && length == 37);
    node_checked_destroy(&n);
    return true;
}
static bool reject(const char *path, bool faults)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestChecked preserved = {0};
    CHECK(test_run(c, "let preserved=u8(7);", TEST_SOURCE, NL_CHECK_OK, NULL,
                   &preserved));
    NLSemanticSnapshot before, after;
    NLSemanticBindingView old_binding;
    NLSemanticValueView old_value;
    CHECK(nl_semantic_snapshot(c, &before));
    CHECK(nl_semantic_binding_view(c, 1, &old_binding));
    CHECK(nl_semantic_value_view(c, old_binding.value, &old_value));
    const NLSyntaxTree *inputs[] = {tree};
    size_t count = 0;
    for (fail_at = 0; fail_at < 12000; ++fail_at) {
        NLFunctionUnitDiagnostic diagnostic = {0};
        at = 0;
        injecting = faults;
        const NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, &diagnostic);
        injecting = false;
        CHECK(status != NL_CHECK_OK);
        CHECK(nl_semantic_snapshot(c, &after) &&
              memcmp(&before, &after, sizeof(before)) == 0);
        NLSemanticBindingView new_binding;
        NLSemanticValueView new_value;
        CHECK(nl_semantic_binding_view(c, 1, &new_binding) &&
              memcmp(&old_binding, &new_binding, sizeof(old_binding)) == 0);
        CHECK(nl_semantic_value_view(c, old_binding.value, &new_value) &&
              memcmp(&old_value, &new_value, sizeof(old_value)) == 0);
        if (status != NL_CHECK_OUT_OF_MEMORY) {
            CHECK(diagnostic.diagnostic.diagnostic.code != NULL);
            if (faults)
                CHECK(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
                      strcmp(diagnostic.diagnostic.diagnostic.code,
                             "P219-POSTSTATE-PRECISION") == 0);
            break;
        }
        CHECK(faults);
        ++count;
    }
    CHECK(fail_at < 12000 && (!faults || count > 100));
    printf("failed-arm registration rollback: %zu staged OOMs; existing "
           "source-created u8 carrier unchanged\n",
           count);
    test_checked_destroy(&preserved);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
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
    TestChecked preserved = {0};
    CHECK(test_run(c, "let preserved=u8(7);", TEST_SOURCE, NL_CHECK_OK, NULL,
                   &preserved));
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
    CHECK(c->value_count == 1 && c->place_count == 1 && c->region_count == 0 &&
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
            size_t forks = 0;
            CHECK(inspect(f, &forks) && forks == 1);
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
    test_checked_destroy(&preserved);
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
    bool ok;
    if (strcmp(argv[1], "oom") == 0)
        ok = oom(argv[2]);
    else if (strcmp(argv[1], "reject-oom") == 0)
        ok = reject(argv[2], true);
    else if (strcmp(argv[1], "reject") == 0)
        ok = reject(argv[2], false);
    else
        ok = evidence(argv[2]);
    return ok ? 0 : 1;
}
