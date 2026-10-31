#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/copy_body.h"
#include <stdint.h>
#include <stdlib.h>
static bool find_if(NLCheckedFragment *f, NLCheckedFragment **owner,
                    NLCheckedNodeId *id)
{
    for (size_t n = 1; n <= f->count; ++n)
        if (f->nodes[n - 1].kind == NL_CHECKED_IF) {
            *owner = f;
            *id = n;
            return true;
        }
    for (size_t n = 0; n < f->body_count; ++n)
        if (find_if(f->bodies[n], owner, id))
            return true;
    for (size_t n = 0; n < f->arm_count; ++n)
        if (find_if(f->arms[n].artifact, owner, id))
            return true;
    return false;
}
static bool refused(const NLCheckedFragment *f)
{
    char *out = NULL;
    size_t length = 0;
    NLNodeCStatus s = nl_checked_c_node(f, &out, &length);
    bool result = s == NL_NODE_C_UNSUPPORTED && !out && !length;
    free(out);
    return result;
}
static bool probe(const char *path)
{
    TestNode actual = {0}, foreign = {0};
    CHECK(node_checked_load(path, &actual) &&
          node_checked_load(path, &foreign));
    NLCheckedFragment *f = NULL, *alien = NULL;
    NLCheckedNodeId id = 0, aid = 0;
    CHECK(find_if(actual.entry, &f, &id) &&
          find_if(foreign.entry, &alien, &aid) && id == aid);
    char *baseline = NULL;
    size_t bytes = 0;
    CHECK(nl_checked_c_node(actual.entry, &baseline, &bytes) == NL_NODE_C_OK &&
          bytes);
    NLCheckedFragment *arm = (NLCheckedFragment *)nl_checked_if_arm(f, id, 0);
    CHECK(arm);
#define MUTATE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        bool rejected = refused(actual.entry);                                 \
        restore;                                                               \
        CHECK(rejected);                                                       \
        char *again = NULL;                                                    \
        size_t size = 0;                                                       \
        CHECK(nl_checked_c_node(actual.entry, &again, &size) ==                \
                  NL_NODE_C_OK &&                                              \
              size == bytes && !memcmp(again, baseline, size));                \
        free(again);                                                           \
        ++mutations;                                                           \
    } while (0)
    size_t mutations = 0, a = SIZE_MAX, b = SIZE_MAX;
    for (size_t n = 0; n < f->arm_count; ++n)
        if (f->arms[n].match == id) {
            if (a == SIZE_MAX)
                a = n;
            else
                b = n;
        }
    CHECK(a != SIZE_MAX && b != SIZE_MAX);
    NLCheckedArm first = f->arms[a], second = f->arms[b];
    MUTATE((f->arms[a] = second, f->arms[b] = first),
           (f->arms[a] = first, f->arms[b] = second));
    MUTATE(f->arms[a].match = 0, f->arms[a] = first);
    MUTATE(f->arms[a].context_origin =
               nl_checked_context(nl_checked_if_arm(alien, aid, 0)),
           f->arms[a] = first);
    const NLCheckedFragment *parent = arm->if_parent;
    MUTATE(arm->if_parent = alien, arm->if_parent = parent);
    size_t index = arm->if_arm_index;
    MUTATE(arm->if_arm_index = 1, arm->if_arm_index = index);
    NLSemanticContext *world = (NLSemanticContext *)nl_checked_context(arm);
    NLSemanticPlaceView place = world->places[0];
    MUTATE(world->places[0].current_fact++, world->places[0] = place);
    MUTATE(world->places[0].incarnation++, world->places[0] = place);
    if (world->domain_count) {
        NLSemanticDomainView d = world->domains[0];
        MUTATE(world->domains[0].live = !d.live, world->domains[0] = d);
    }
    const NLCheckedNodeView *block =
        nl_checked_node_view(arm, arm->nodes[arm->root - 1].initializer);
    CHECK(block && block->tail);
    NLCheckedNodeView literal = arm->nodes[block->tail - 1];
    MUTATE(arm->nodes[block->tail - 1].scalar_result.value++,
           arm->nodes[block->tail - 1] = literal);
    size_t copies = 0, owners = 0;
    for (size_t n = 1; n <= f->count; ++n) {
        NLCopyBodyView view;
        NLCopyBodyParameters parameters;
        if (nl_checked_copy_body_view(f, n, &view) &&
            nl_checked_copy_body_parameters(f, n, &parameters)) {
            NLSemanticValueView old, next;
            NLSemanticPlaceView child;
            CHECK(nl_checked_copy_body_validate(f, n, &view) == NL_CHECK_OK &&
                  nl_checked_copy_body_parameters_validate(f, n, &parameters) ==
                      NL_CHECK_OK &&
                  nl_semantic_value_view(view.live_exit, view.result.value,
                                         &old) &&
                  nl_semantic_place_view(parameters.handoff,
                                         parameters.field.child, &child) &&
                  nl_semantic_value_view(parameters.handoff,
                                         child.current_value, &next));
            printf("P945 public Copy root=%zu child=%zu incarnation=%zu "
                   "index=%zu old=%zu incoming_known=%u body=0 parameters=0\n",
                   parameters.field.parent, parameters.field.child,
                   parameters.field.parent_incarnation, parameters.field.index,
                   old.scalar_value, (unsigned)next.scalar_known);
            ++copies;
        }
        NLCheckedNodeView original = f->nodes[n - 1];
        if (original.owner_call.entry_proved) {
            MUTATE(f->nodes[n - 1].owner_call.root++,
                   f->nodes[n - 1] = original);
            MUTATE(f->nodes[n - 1].owner_call.domain++,
                   f->nodes[n - 1] = original);
            MUTATE(f->nodes[n - 1].owner_call.range.region++,
                   f->nodes[n - 1] = original);
            ++owners;
        }
    }
    CHECK(copies == 1);
    printf("P945 actual IF caller Copy=%zu terminal=%zu\n", copies, owners);
    printf("P945 source-owned IF tamper=%zu reject/restore; original complete "
           "C=%zu bytes\n",
           mutations, bytes);
    free(baseline);
    node_checked_destroy(&foreign);
    node_checked_destroy(&actual);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
