/* Independent R787: inspect genuinely source-issued arm callers; never mint a
 * proof. */
#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t seen, producers, receivers, initials;
    const unsigned *indexes, *old, *next, *sibling;
    size_t expected;
} Audit;

static bool scalar(const NLSemanticContext *c, NLValueId v, unsigned n)
{
    NLSemanticValueView value = {0};
    return nl_semantic_value_view(c, v, &value) && value.scalar_known &&
           value.scalar_value == n;
}

static bool audit_call(NLCheckedFragment *arm, NLCheckedFragment *parent,
                       NLCheckedNodeId match, size_t index,
                       NLCheckedNodeId call, NLCheckedFragment *body, Audit *a)
{
    CHECK(parent && a->seen < a->expected && body->copy_body);
    const NLCopyBodyProof *seal = body->copy_body;
    NLCopyBodyView v = {0};
    NLCopyBodyParameters p = {0};
    CHECK(nl_checked_copy_body_view(arm, call, &v) &&
          nl_checked_copy_body_parameters(arm, call, &p));
    CHECK(nl_checked_copy_body_validate(arm, call, &v) == NL_CHECK_OK &&
          nl_checked_copy_body_parameters_validate(arm, call, &p) ==
              NL_CHECK_OK);
    const NLCheckedNodeView *root = nl_checked_node_view(arm, arm->root);
    CHECK(root && root->kind == NL_CHECKED_MATCH_ARM && root->variant == 2 &&
          index == 1 && nl_checked_match_arm(parent, match, index) == arm);
    CHECK(seal->caller == arm && seal->owner == body && seal->caller_nodes &&
          seal->caller_nodes_origin == arm->nodes &&
          seal->caller_root == arm->root && seal->caller_count == arm->count &&
          seal->caller_source == arm->source &&
          seal->caller_body == arm->body_owner &&
          !memcmp(seal->caller_nodes, arm->nodes,
                  arm->count * sizeof(*arm->nodes)));
    CHECK(v.body == body &&
          nl_checked_context(body) == nl_checked_context(arm));
    size_t i = a->seen++;
    CHECK(p.field.index == a->indexes[i] && p.field.dependency_compatible &&
          scalar(v.live_exit, v.result.value, a->old[i]));
    NLSemanticPlaceView before = {0}, after = {0}, aggregate = {0};
    NLSemanticPlaceView sib_before = {0}, sib_after = {0};
    CHECK(
        nl_semantic_place_view(p.entry, p.field.child, &before) &&
        nl_semantic_place_view(p.handoff, p.field.child, &after) &&
        before.incarnation == after.incarnation &&
        before.current_fact == p.field.child_fact &&
        before.current_fact != after.current_fact &&
        scalar(p.entry, before.current_value, a->old[i]) &&
        scalar(p.handoff, after.current_value, a->next[i]) &&
        nl_semantic_place_view(p.handoff, p.field.parent, &aggregate) &&
        aggregate.incarnation == p.field.parent_incarnation &&
        aggregate.current_fact != p.field.parent_fact &&
        nl_semantic_place_view(
            p.entry, aggregate.fixed_fields[1 - p.field.index], &sib_before) &&
        nl_semantic_place_view(
            p.handoff, aggregate.fixed_fields[1 - p.field.index], &sib_after) &&
        sib_before.current_fact == sib_after.current_fact &&
        sib_before.current_value == sib_after.current_value &&
        scalar(p.handoff, sib_after.current_value, a->sibling[i]));
    NLCopyBodyEventView event = {0};
    CHECK(v.event_count == 1 &&
          nl_checked_copy_body_event(arm, call, 0, &event) &&
          nl_checked_copy_body_event_validate(arm, call, 0, &event) ==
              NL_CHECK_OK &&
          event.kind == NL_COPY_BODY_CHANGE);
    printf("R787_PUBLIC call=%zu arm=%zu index=%zu old=%u new=%u "
           "sibling=%u source_exact=1 graph_exact=1 body=0 parameters=0\n",
           (size_t)call, (size_t)match, p.field.index, a->old[i], a->next[i],
           a->sibling[i]);
    return true;
}

static bool walk(NLCheckedFragment *f, NLCheckedFragment *parent,
                 NLCheckedNodeId match, size_t arm_index, Audit *a,
                 size_t depth)
{
    CHECK(f && depth < 24);
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *node = nl_checked_node_view(f, i);
        CHECK(node);
        if (node->kind == NL_CHECKED_INITIALIZE) {
            CHECK(node->lifetime_place && node->lifetime_incarnation &&
                  node->lifetime_range.region && node->lifetime_domain);
            ++a->initials;
        }
        if (node->producer.entry_proved) {
            NLProducerLocalReturnView ret = {0};
            CHECK(nl_checked_producer_valid(f, i) &&
                  nl_checked_producer_local_return_view(f, i, &ret) &&
                  nl_checked_producer_local_return_valid(f, i, &ret) &&
                  ret.entry != ret.returned);
            ++a->producers;
        }
        if (node->owner_call.entry_proved) {
            CHECK(node->owner_call.post_proved);
            ++a->receivers;
        }
    }
    for (size_t i = 0; i < f->body_count; ++i) {
        NLCheckedFragment *child = f->bodies[i];
        if (child->copy_body && child->copy_body->formal_write && parent)
            CHECK(audit_call(f, parent, match, arm_index, f->body_calls[i],
                             child, a));
        CHECK(walk(child, NULL, 0, 0, a, depth + 1));
    }
    for (size_t i = 0; i < f->arm_count; ++i) {
        size_t ordinal = 0;
        for (size_t j = 0; j < i; ++j)
            ordinal += f->arms[j].match == f->arms[i].match;
        CHECK(walk(f->arms[i].artifact, f, f->arms[i].match, ordinal, a,
                   depth + 1));
    }
    return true;
}

static bool run(const char *path, size_t expected, const unsigned *indexes,
                const unsigned *old, const unsigned *next,
                const unsigned *sibling)
{
    TestNode node = {0};
    CHECK(node_checked_load(path, &node));
    Audit a = {.indexes = indexes,
               .old = old,
               .next = next,
               .sibling = sibling,
               .expected = expected};
    CHECK(walk(node.entry, NULL, 0, 0, &a, 0));
    CHECK(a.seen == expected && a.producers == 1 && a.receivers == 1 &&
          a.initials == 2);
    char *c = NULL;
    size_t length = 787;
    CHECK(nl_checked_c_node(node.entry, &c, &length) == NL_NODE_C_UNSUPPORTED &&
          c == NULL && length == 787);
    printf("R787_PUBLIC source=%s calls=%zu H_originals=2 "
           "producer_return=1 terminal_receiver=1 C17=UNSUPPORTED\n",
           path, a.seen);
    node_checked_destroy(&node);
    return true;
}
int main(int argc, char **argv)
{
    const unsigned ia[] = {0, 1}, oa[] = {21, 196}, na[] = {64, 33},
                   sa[] = {174, 88};
    const unsigned ib[] = {1}, ob[] = {177}, nb[] = {91}, sb[] = {43};
    return argc == 3 && run(argv[1], 2, ia, oa, na, sa) &&
                   run(argv[2], 1, ib, ob, nb, sb)
               ? 0
               : 1;
}
