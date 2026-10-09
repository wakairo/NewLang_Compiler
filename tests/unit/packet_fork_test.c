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
static bool fork_evidence(NLCheckedFragment *f, NLCheckedNodeId id)
{
    NLCheckedNodeView *m = &f->nodes[id - 1];
    CHECK(nl_checked_packet_fork_valid(f, id));
    CHECK(!m->normal_frame_unchanged && m->normal_arms == 2);
    const NLCheckedNodeView *p =
        nl_checked_node_view(f, m->packet_fork.producer);
    const NLSemanticContext *entry = f->packet_entry, *post = f->packet_post;
    CHECK(entry != post && entry != f->context);
    CHECK(post->value_count == entry->value_count &&
          post->binding_count == entry->binding_count);
    CHECK(entry->places[p->producer.root - 1].live &&
          !post->places[p->producer.root - 1].live);
    CHECK(!post->regions[p->producer.range.region - 1].view.live &&
          !post->domains[p->producer.domain - 1].live);
    CHECK(post->places[p->producer.head.parent - 1].live &&
          post->regions
              [entry->places[p->producer.head.parent - 1].placement.region - 1]
                  .view.live &&
          post->domains[p->producer.head_domain - 1].live);
    CHECK(entry->bindings[m->packet_fork.binding - 1].view.availability ==
              NL_AVAILABLE &&
          post->bindings[m->packet_fork.binding - 1].view.availability ==
              NL_CONSUMED);
    CHECK(post->values[m->packet_fork.packet - 1].carrier == NL_CARRIER_ENDED);
    NLCheckedFragment *arms[2] = {f->arms[f->arm_count - 2].artifact,
                                  f->arms[f->arm_count - 1].artifact};
    CHECK(arms[0] != arms[1] && arms[0]->context != arms[1]->context);
    NLCheckedNodeId receiving[2] = {0};
    for (size_t a = 0; a < 2; ++a) {
        NLCheckedFragment *arm = arms[a];
        CHECK(arm == nl_checked_match_arm(f, id, a));
        CHECK(arm->packet_parent == f && arm->packet_match == id &&
              arm->packet_entry != entry && arm->packet_entry != arm->context);
        CHECK(nl_packet_same_entry(entry, arm->packet_entry));
        /* Read-only pre-consumption lookup uses a retained actual-source
         * entry world, not the ended current arm or a type-created owner.
         * This is original-packet evidence only, never a custody certificate.
         */
        NLCheckedFragment before_arm = *arm;
        before_arm.context = arm->packet_entry;
        NLSemanticSnapshot before_lookup, after_lookup;
        const NLSemanticValueView original_packet =
            before_arm.context->values[m->packet_fork.packet - 1];
        CHECK(nl_semantic_snapshot(before_arm.context, &before_lookup));
        CHECK(
            nl_packet_available_inherited(&before_arm, m->packet_fork.packet));
        CHECK(!nl_packet_available_inherited(arm, m->packet_fork.packet));
        CHECK(!nl_packet_available_inherited(&before_arm,
                                             m->packet_fork.packet + 1));
        CHECK(nl_semantic_snapshot(before_arm.context, &after_lookup) &&
              memcmp(&before_lookup, &after_lookup, sizeof(before_lookup)) ==
                  0);
        CHECK(memcmp(&original_packet,
                     &before_arm.context->values[m->packet_fork.packet - 1],
                     sizeof(original_packet)) == 0);
        size_t receives = 0, calls = 0;
        for (NLCheckedNodeId n = 1; n <= arm->count; ++n) {
            const NLCheckedNodeView *v = nl_checked_node_view(arm, n);
            if (v->packet_origin.ancestor != NULL) {
                ++receives;
                receiving[a] = n;
                CHECK(v->packet_origin.ancestor == f &&
                      v->packet_origin.world == arm->context &&
                      v->packet_origin.entry_world == arm->packet_entry &&
                      v->packet_origin.packet == m->packet_fork.packet);
                size_t affine = 0;
                for (NLCheckedNodeId r = v->first_argument; r != 0;
                     r = nl_checked_node_view(arm, r)->next_argument) {
                    const NLCheckedNodeView *receiver =
                        nl_checked_node_view(arm, r);
                    const NLSemanticBindingView b =
                        arm->context->bindings[receiver->symbol - 1].view;
                    affine += !arm->context->types[b.type - 1].view.is_copy;
                    CHECK(b.availability == NL_CONSUMED);
                }
                CHECK(affine == 2);
            }
            if (v->owner_call.entry_proved) {
                ++calls;
                const NLSemanticContext *call_entry =
                    nl_checked_owner_entry(arm, n);
                CHECK(call_entry != arm->context && call_entry != entry &&
                      call_entry != arms[1 - a]->context);
                CHECK(v->owner_call.post_proved && v->body_backed);
                CHECK(v->owner_call.root == p->producer.root &&
                      v->owner_call.incarnation == p->producer.incarnation &&
                      v->owner_call.range.region == p->producer.range.region &&
                      v->owner_call.domain == p->producer.domain);
                CHECK(v->owner_call.inputs[1] == p->producer.inputs[2] &&
                      v->owner_call.inputs[2] == p->producer.inputs[3]);
                CHECK(nl_owner_relations(call_entry, v->owner_call.inputs,
                                         &v->owner_call.definition, v->span,
                                         NULL) == NL_CHECK_OK);
                const NLCheckedFragment *body = nl_checked_call_body(arm, n);
                size_t ends = 0, frees = 0;
                for (NLCheckedNodeId k = 1; k <= body->count; ++k) {
                    const NLCheckedNodeView *op = nl_checked_node_view(body, k);
                    ends += op->kind == NL_CHECKED_DESTROY;
                    frees += op->kind == NL_CHECKED_DEALLOCATE;
                }
                CHECK(ends == 1 && frees == 1);
            }
        }
        CHECK(receives == 1 && calls == 1);
    }
    /* A copied numeric node/certificate cannot stand for the sibling world. */
    for (size_t a = 0; a < 2; ++a) {
        NLCheckedFragment *arm = arms[a];
        NLCheckedNodeView *v = &arm->nodes[receiving[a] - 1];
        const NLCheckedNodeView saved = *v;
        *v = arms[1 - a]->nodes[receiving[1 - a] - 1];
        CHECK(!nl_checked_packet_fork_valid(f, id));
        *v = saved;
        v->packet_origin.world = arms[1 - a]->context;
        CHECK(!nl_checked_packet_fork_valid(f, id));
        *v = saved;
        v->packet_origin.entry_world = arms[1 - a]->packet_entry;
        CHECK(!nl_checked_packet_fork_valid(f, id));
        *v = saved;
        ++v->packet_origin.packet;
        CHECK(!nl_checked_packet_fork_valid(f, id));
        *v = saved;
        ++v->packet_origin.match;
        CHECK(!nl_checked_packet_fork_valid(f, id));
        *v = saved;
        v->packet_origin.ancestor = arm;
        CHECK(!nl_checked_packet_fork_valid(f, id));
        *v = saved;
        const NLCheckedFragment *parent = arm->packet_parent;
        arm->packet_parent = arms[1 - a];
        CHECK(!nl_checked_packet_fork_valid(f, id));
        arm->packet_parent = parent;
        NLSemanticContext *cloned = NULL;
        CHECK(nl_sem_clone(arm->context, &cloned) == NL_CHECK_OK);
        const NLSemanticContext *own = arm->context;
        arm->context = cloned;
        CHECK(!nl_checked_packet_fork_valid(f, id));
        arm->context = own;
        nl_semantic_destroy(cloned);
        NLCheckedNodeView *call = &arm->nodes[arm->owner_entry_call - 1];
        const NLCheckedNodeView saved_call = *call;
        for (size_t n = 0; n < 10; ++n) {
            if (n == 0)
                call->owner_call.post_proved = false;
            if (n == 1)
                call->owner_call.root = p->producer.head.parent;
            if (n == 2)
                call->owner_call.domain = p->producer.head_domain;
            if (n == 3)
                ++call->owner_call.inputs[1];
            if (n == 4)
                call->owner_call.definition.requirements = 0;
            if (n == 5)
                call->owner_call.definition.live_return = true;
            if (n == 6)
                call->owner_call.definition.step_count = 0;
            if (n == 7)
                ++call->owner_call.range.length;
            if (n == 8)
                ++call->owner_call.donor[1];
            if (n == 9)
                call->argument_count = 2;
            CHECK(!nl_checked_packet_fork_valid(f, id));
            *call = saved_call;
        }
        const NLValueId packet = m->packet_fork.packet;
        NLSemanticValueView saved_value = arm->packet_entry->values[packet - 1];
        arm->packet_entry->values[packet - 1].dependencies =
            NL_DEPENDENCIES_UNKNOWN;
        CHECK(!nl_checked_packet_fork_valid(f, id));
        arm->packet_entry->values[packet - 1] = saved_value;
        CHECK(nl_checked_packet_fork_valid(f, id));
    }
    bool collision = false;
    const size_t min_values =
        arms[0]->context->value_count < arms[1]->context->value_count
            ? arms[0]->context->value_count
            : arms[1]->context->value_count;
    for (size_t n = entry->value_count; n < min_values; ++n) {
        const NLSemanticValueView a = arms[0]->context->values[n],
                                  b = arms[1]->context->values[n];
        if (a.type != b.type || a.reference.place != b.reference.place ||
            a.reference.provenance != b.reference.provenance)
            collision = true;
    }
    CHECK(collision); /* equal branch-local numeric IDs, different meanings */
    NLSemanticContext *changed = NULL;
    CHECK(nl_sem_clone(entry, &changed) == NL_CHECK_OK);
    const NLValueId packet = m->packet_fork.packet;
    const NLSemanticValueView package = changed->values[packet - 1];
    const NLValueId pointer = package.fields[0], allocation = package.fields[1],
                    domain = package.fields[2];
    const NLSemanticValueView old_pointer = changed->values[pointer - 1],
                              old_allocation = changed->values[allocation - 1],
                              old_domain = changed->values[domain - 1];
    const NLSemanticPlaceView root = changed->places[p->producer.root - 1];
    for (size_t k = 0; k < 6; ++k) {
        if (k == 0)
            changed->values[pointer - 1].reference.provenance =
                NL_PROVENANCE_UNKNOWN;
        if (k == 1)
            changed->values[pointer - 1].reference_count = 1;
        if (k == 2)
            changed->values[allocation - 1].allocation_region =
                root.placement.region - 1;
        if (k == 3)
            changed->values[domain - 1].domain = p->producer.head_domain;
        if (k == 4)
            ++changed->places[p->producer.root - 1].incarnation;
        if (k == 5)
            changed->values[packet - 1].dependencies = NL_DEPENDENCIES_UNKNOWN;
        NLCheckedFragment preflight = *arms[0];
        preflight.context = changed;
        CHECK(!nl_packet_available_inherited(&preflight, packet));
        NLSemanticContext *refused = NULL;
        CHECK(nl_packet_closed(f, id, changed, &refused) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
              refused == NULL);
        changed->values[pointer - 1] = old_pointer;
        changed->values[allocation - 1] = old_allocation;
        changed->values[domain - 1] = old_domain;
        changed->values[packet - 1] = package;
        changed->places[p->producer.root - 1] = root;
    }
    nl_semantic_destroy(changed);
    CHECK(!nl_packet_available_inherited(NULL, packet));
    NLSemanticContext *empty = NULL;
    CHECK(nl_packet_closed(NULL, 0, NULL, &empty) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          empty == NULL);
    const NLCheckedNodeView saved_match = *m;
    m->normal_frame_unchanged = true;
    CHECK(!nl_checked_packet_fork_valid(f, id));
    *m = saved_match;
    m->packet_fork.entry_world = arms[0]->packet_entry;
    CHECK(!nl_checked_packet_fork_valid(f, id));
    *m = saved_match;
    m->packet_fork.post_world = entry;
    CHECK(!nl_checked_packet_fork_valid(f, id));
    *m = saved_match;
    m->packet_fork.binding++;
    CHECK(!nl_checked_packet_fork_valid(f, id));
    *m = saved_match;
    CHECK(nl_checked_packet_fork_valid(f, id));
    printf("parent-qualified packet=%zu O=%zu/%zu R=%zu D=%zu A=%zu; two owned "
           "arms/terminal calls; common closed-tail/live-head prefix; 42 "
           "certificate corruptions + 6 refused post constructors; numeric "
           "collision excluded\n",
           m->packet_fork.packet, p->producer.root, p->producer.incarnation,
           p->producer.range.region, p->producer.domain, p->producer.inputs[2]);
    return true;
}
static bool inspect(NLCheckedFragment *f, size_t *forks)
{
    for (NLCheckedNodeId id = 1; id <= f->count; ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v->packet_fork.closed) {
            ++*forks;
            CHECK(fork_evidence(f, id));
        }
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t i = 0; i < v->item_count; ++i)
                CHECK(
                    inspect((NLCheckedFragment *)nl_checked_match_arm(f, id, i),
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
