#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include <stdlib.h>
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_calls, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && allocation_calls++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t z)
{
    return injecting && allocation_calls++ == fail_at ? NULL
                                                      : __real_calloc(n, z);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && allocation_calls++ == fail_at ? NULL
                                                      : __real_realloc(p, n);
}
static const NLCheckedFragment *first_call(const NLCheckedFragment *root,
                                           NLCheckedNodeId *id)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(root, nl_checked_root(root));
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_parameters(f, i, &p)) {
            *id = i;
            return f;
        }
    }
    return NULL;
}
static bool challenge(const char *path, const char *foreign_path)
{
    TestNode a = {0}, b = {0};
    CHECK(node_checked_load(path, &a) && node_checked_load(foreign_path, &b));
    NLCheckedNodeId id = 0, bid = 0;
    const NLCheckedFragment *f = first_call(a.entry, &id),
                            *other = first_call(b.entry, &bid);
    NLCopyBodyView v, foreign;
    NLCopyBodyParameters p, fp;
    CHECK(f && other && nl_checked_copy_body_view(f, id, &v) &&
          nl_checked_copy_body_view(other, bid, &foreign) &&
          nl_checked_copy_body_parameters(f, id, &p) &&
          nl_checked_copy_body_parameters(other, bid, &fp) &&
          nl_checked_copy_body_validate(f, id, &v) == NL_CHECK_OK &&
          nl_checked_copy_body_validate(other, bid, &foreign) == NL_CHECK_OK &&
          id == bid && v.result.value == foreign.result.value &&
          p.field.parent == fp.field.parent &&
          p.field.nominal == fp.field.nominal && v.body != foreign.body &&
          p.entry != fp.entry);
    size_t rejected = 0;
#define BODY(change)                                                           \
    do {                                                                       \
        NLCopyBodyView bad = v;                                                \
        change;                                                                \
        CHECK(nl_checked_copy_body_validate(f, id, &bad) != NL_CHECK_OK);      \
        ++rejected;                                                            \
    } while (0)
    BODY(bad.body = foreign.body);
    BODY(bad.live_exit = foreign.live_exit);
    BODY(bad.function++);
    BODY(bad.root++);
    BODY(bad.event_count++);
    BODY(bad.result.value++);
    BODY(bad.result.type = p.field.nominal);
#undef BODY
#define PARAM(change)                                                          \
    do {                                                                       \
        NLCopyBodyParameters bad = p;                                          \
        change;                                                                \
        CHECK(nl_checked_copy_body_parameters_validate(f, id, &bad) !=         \
              NL_CHECK_OK);                                                    \
        ++rejected;                                                            \
    } while (0)
    PARAM(bad.entry = fp.entry);
    PARAM(bad.handoff = fp.handoff);
    PARAM(bad.field.parent++);
    PARAM(bad.field.child++);
    PARAM(bad.field.index = 1 - p.field.index);
    PARAM(bad.field.nominal++);
    PARAM(bad.field.parent_incarnation++);
    PARAM(bad.field.child_incarnation++);
    PARAM(bad.field.parent_fact++);
    PARAM(bad.field.child_fact++);
    PARAM(bad.field.old_value++);
    PARAM(bad.loan++);
    PARAM(bad.arguments[0]++);
    PARAM(bad.parameters[0]++);
#undef PARAM
    for (size_t i = 0; i < v.event_count; ++i) {
        NLCopyBodyEventView event;
        CHECK(nl_checked_copy_body_event(f, id, i, &event));
        CHECK(nl_checked_copy_body_event_validate(f, id, i, &event) ==
              NL_CHECK_OK);
        NLCopyBodyEventView bad = event;
        bad.before = fp.entry;
        CHECK(nl_checked_copy_body_event_validate(f, id, i, &bad) !=
              NL_CHECK_OK);
        ++rejected;
        bad = event;
        bad.incoming++;
        CHECK(nl_checked_copy_body_event_validate(f, id, i, &bad) !=
              NL_CHECK_OK);
        ++rejected;
        bad = event;
        bad.result++;
        CHECK(nl_checked_copy_body_event_validate(f, id, i, &bad) !=
              NL_CHECK_OK);
        ++rejected;
    }
    /* Independently tamper one actual historical world at a time, restoring
     * and revalidating after each. No attacker-supplied authority is installed.
     */
    NLCopyBodyProof *receipt = v.body->copy_body;
    NLSemanticContext *handoff = receipt->handoff;
    NLSemanticContext *current = (NLSemanticContext *)nl_checked_context(f);
    NLTypeId h = 0;
    for (size_t i = 1; i <= current->type_count; ++i)
        if (nl_recursive_local_type(current, i))
            h = i;
    CHECK(h && h != p.field.nominal);
#define MUTATE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(nl_checked_copy_body_validate(f, id, NULL) != NL_CHECK_OK);      \
        restore;                                                               \
        CHECK(nl_checked_copy_body_validate(f, id, &v) == NL_CHECK_OK);        \
        ++rejected;                                                            \
    } while (0)
    NLSemanticTypeView original = current->types[h - 1].view;
    MUTATE(current->types[h - 1].view.layout_known = false,
           current->types[h - 1].view = original);
    MUTATE(current->types[h - 1].view.size++,
           current->types[h - 1].view = original);
    MUTATE(current->types[h - 1].view.alignment *= 2,
           current->types[h - 1].view = original);
    NLTypeId ptr = 0;
    CHECK(nl_semantic_recursive_link_types(current, h, &ptr));
    NLSemanticTypeView ptr_original = current->types[ptr - 1].view;
    MUTATE(current->types[ptr - 1].view.target = p.field.nominal,
           current->types[ptr - 1].view = ptr_original);
    NLSemanticPlaceView child = handoff->places[p.field.child - 1];
    MUTATE(handoff->places[p.field.child - 1].current_fact = p.field.child_fact,
           handoff->places[p.field.child - 1] = child);
    MUTATE(handoff->places[p.field.child - 1].parent_aggregate++,
           handoff->places[p.field.child - 1] = child);
    const NLCheckedNodeView *loan = nl_checked_node_view(f, p.loan);
    NLSemanticScopeView scope = handoff->scopes[loan->loan.scope - 1];
    MUTATE(handoff->scopes[loan->loan.scope - 1].active = false,
           handoff->scopes[loan->loan.scope - 1] = scope);
    NLCheckedFragment *body = (NLCheckedFragment *)v.body;
    NLCheckedNodeView node = body->nodes[body->root - 1];
    MUTATE(body->nodes[body->root - 1].span.start_byte++,
           body->nodes[body->root - 1] = node);
#undef MUTATE
    TestState *before = malloc(sizeof(*before)),
              *history = malloc(sizeof(*history));
    CHECK(before && history && test_state(current, before) &&
          test_state(handoff, history));
    CHECK(
        nl_semantic_set_layout(current, h, 16, 8) == NL_CHECK_SEMANTIC_ERROR &&
        nl_semantic_set_layout(current, h, 24, 16) == NL_CHECK_SEMANTIC_ERROR &&
        test_unchanged(current, before));
    for (size_t mode = 0; mode < 3; ++mode) {
        bool done = false;
        for (fail_at = 0; fail_at < 10000; ++fail_at) {
            allocation_calls = 0;
            injecting = true;
            NLCheckStatus status =
                mode == 0 ? nl_checked_copy_body_validate(f, id, &v)
                : mode == 1
                    ? nl_checked_copy_body_parameters_validate(f, id, &p)
                    : nl_checked_copy_body_event_validate(f, id, 0, NULL);
            injecting = false;
            CHECK(test_unchanged(current, before) &&
                  test_unchanged(handoff, history));
            if (status == NL_CHECK_OK) {
                done = true;
                break;
            }
            CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
                  allocation_calls > fail_at);
            CHECK(nl_checked_copy_body_validate(f, id, &v) == NL_CHECK_OK);
        }
        CHECK(done && fail_at > 0 && fail_at < 10000);
        printf("PUBLIC_OOM validator=%zu positions=%zu immutable_worlds=PASS "
               "retry=PASS\n",
               mode, fail_at);
    }
    free(before);
    free(history);
    printf("TAMPER refusals=%zu equal_numeric_foreign_ids=verified "
           "restoration=PASS\n",
           rejected);
    node_checked_destroy(&a);
    node_checked_destroy(&b);
    return true;
}
static bool uncalled(const char *path)
{
    TestNode a = {0};
    CHECK(node_checked_load(path, &a));
    NLCheckedNodeId id = 0;
    CHECK(!first_call(a.entry, &id));
    puts("UNCALLED: definition is not a caller Copy-call certificate");
    node_checked_destroy(&a);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "uncalled"))
        return uncalled(argv[2]) ? 0 : 1;
    return argc == 3 && challenge(argv[1], argv[2]) ? 0 : 1;
}
