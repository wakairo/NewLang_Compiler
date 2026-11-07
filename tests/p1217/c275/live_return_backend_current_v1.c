#include "../../../src/copy_body_internal.h"
#include "../../../src/semantic_internal.h"
#include "../../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdlib.h>
void *__real_malloc(size_t);
static bool inject;
static size_t at, calls;
static bool failure_injected;
static void *failure_site;
void *__wrap_malloc(size_t n)
{
    if (inject && calls++ == at) {
        failure_injected = true;
        failure_site = __builtin_return_address(0);
        return NULL;
    }
    return __real_malloc(n);
}
static NLCheckedFragment *producer(NLCheckedFragment *f, NLCheckedNodeId *id)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->producer.entry_proved) {
            *id = i;
            return f;
        }
        NLCheckedFragment *b = (NLCheckedFragment *)nl_checked_call_body(f, i);
        NLCheckedFragment *found = b == NULL ? NULL : producer(b, id);
        if (found != NULL)
            return found;
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a) {
                found = producer(
                    (NLCheckedFragment *)nl_checked_match_arm(f, i, a), id);
                if (found != NULL)
                    return found;
            }
    }
    return NULL;
}
static bool expect(TestNode *n, NLNodeCStatus wanted, const char *baseline)
{
    char *out = NULL;
    size_t length = 37;
    NLNodeCStatus actual = nl_checked_c_node(n->entry, &out, &length);
    if (actual != wanted)
        fprintf(stderr, "emit actual=%d wanted=%d at=%zu calls=%zu\n", actual,
                wanted, at, calls);
    CHECK(actual == wanted);
    if (wanted == NL_NODE_C_OK) {
        CHECK(out != NULL && length == strlen(out) &&
              strcmp(out, baseline) == 0);
        free(out);
    } else
        CHECK(out == NULL && length == 37);
    return true;
}
static bool controls(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    /* Original source/syntax is gone; output depends on owned checked evidence.
     */
    char *baseline = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(n.entry, &baseline, &length) == NL_NODE_C_OK);
    NLCheckedNodeId id = 0;
    NLCheckedFragment *f = producer(n.entry, &id);
    CHECK(f != NULL && nl_checked_producer_valid(f, id));
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(f, id);
    NLCheckedNodeView *call = &f->nodes[id - 1];
    NLCheckedNodeView saved = *call;
    for (size_t m = 0; m < 9; ++m) {
        switch (m) {
        case 0:
            call->producer.return_proved = false;
            break;
        case 1:
            call->producer.definition.requirements = 0;
            break;
        case 2:
            call->producer.root = call->producer.head.parent;
            break;
        case 3:
            call->producer.domain = call->producer.head_domain;
            break;
        case 4:
            call->producer.result = 0;
            break;
        case 5:
            call->producer.donor[2] = call->producer.parameters[2];
            break;
        case 6:
            call->producer.return_world = call->producer.entry_world;
            break;
        case 7:
            call->type = 0;
            break;
        case 8:
            call->producer.range.length = 23;
            break;
        }
        CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
        *call = saved;
        CHECK(expect(&n, NL_NODE_C_OK, baseline));
    }
    size_t mutated = 0;
    for (size_t i = 0; i < body->count; ++i) {
        NLCheckedNodeView *v = &body->nodes[i];
        NLCheckedNodeView original = *v;
        if (v->kind == NL_CHECKED_REPLACE) {
            for (size_t m = 0; m < 6; ++m) {
                switch (m) {
                case 0:
                    v->field.child = v->field.parent;
                    break;
                case 1:
                    v->field.access = NL_ACCESS_READ;
                    break;
                case 2:
                    v->field.new_value = v->field.old_value;
                    break;
                case 3:
                    v->field.parent_post_fact = v->field.parent_fact;
                    break;
                case 4:
                    v->field.post_payload_occurrence = 1;
                    break;
                case 5:
                    v->field.dependency_compatible = false;
                    break;
                }
                CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
                *v = original;
                ++mutated;
            }
        } else if (v->kind == NL_CHECKED_RETURN) {
            v->returned.value = 0;
            CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
            *v = original;
            ++mutated;
        } else if (v->kind == NL_CHECKED_AGGREGATE_FIELD) {
            v->field_index = (v->field_index + 1) % 3;
            CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
            *v = original;
            ++mutated;
        } else if (v->kind == NL_CHECKED_SUM_CONSTRUCTOR) {
            v->variant = 2;
            CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
            *v = original;
            ++mutated;
        }
        CHECK(expect(&n, NL_NODE_C_OK, baseline));
    }
    CHECK(mutated == 11);
    NLSemanticContext *clone = NULL;
    CHECK(nl_sem_clone(body->context, &clone) == NL_CHECK_OK);
    NLSemanticContext *original = (NLSemanticContext *)body->context;
    body->context = clone;
    CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
    body->context = original;
    nl_semantic_destroy(clone);
    /* Closed backend depth fence, without growing semantic state or partial C.
     */
    NLCheckedNodeView *root = &body->nodes[body->root - 1];
    NLCheckedNodeView root_saved = *root;
    root->item_count = SIZE_MAX;
    CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
    *root = root_saved;
    /* CURRENT v1: authentic host OOM, not historical bool-loss counts.
     * Capture all source-owned caller/entry/return/body worlds before faults.
     */
    NLSemanticContext *worlds[] = {
        n.context, (NLSemanticContext *)nl_checked_context(f),
        (NLSemanticContext *)nl_checked_producer_entry(f, id),
        (NLSemanticContext *)nl_checked_producer_return(f, id),
        (NLSemanticContext *)nl_checked_context(body)};
    enum {
        WORLD_COUNT = sizeof(worlds) / sizeof(worlds[0])
    };
    NLSemanticContext headers[WORLD_COUNT];
    TestState *states = calloc(WORLD_COUNT, sizeof(*states));
    NLCopyCallAnchor anchors[WORLD_COUNT] = {0};
    CHECK(states);
    for (size_t i = 0; i < WORLD_COUNT; ++i) {
        CHECK(worlds[i] && test_state(worlds[i], &states[i]));
        memcpy(&headers[i], worlds[i], sizeof(headers[i]));
        CHECK(nl_copy_state_anchor(worlds[i], &anchors[i]) == NL_CHECK_OK);
    }
    NLCheckedFragment *root_before = n.entry;
    size_t reached_oom = 0;
    for (at = 0; at < 64; ++at) {
        calls = 0;
        failure_injected = false;
        failure_site = NULL;
        inject = true;
        char *out = NULL;
        size_t sentinel = 37;
        NLNodeCStatus status = nl_checked_c_node(n.entry, &out, &sentinel);
        inject = false;
        CHECK(n.entry == root_before);
        for (size_t i = 0; i < WORLD_COUNT; ++i)
            CHECK(!memcmp(worlds[i], &headers[i], sizeof(headers[i])) &&
                  test_unchanged(worlds[i], &states[i]) &&
                  nl_copy_state_matches(worlds[i], &anchors[i]));
        if (status == NL_NODE_C_OK) {
            CHECK(!failure_injected && !failure_site && calls <= at && out &&
                  sentinel == strlen(out) && strcmp(out, baseline) == 0);
            free(out);
            break;
        }
        CHECK(failure_injected && failure_site && calls > at && !out &&
              sentinel == 37 && status == NL_NODE_C_OUT_OF_MEMORY);
        printf("C275 CURRENT OOM at=%zu calls=%zu status=%d site=%p "
               "out=NULL length=37 root/world/loan unchanged\n",
               at, calls, status, failure_site);
        ++reached_oom;
        CHECK(nl_checked_producer_valid(f, id));
        CHECK(expect(&n, NL_NODE_C_OK, baseline));
    }
    CHECK(at < 64 && reached_oom != 0);
    for (size_t i = 0; i < WORLD_COUNT; ++i)
        nl_copy_state_destroy(&anchors[i]);
    free(states);
    free(baseline);
    node_checked_destroy(&n);
    printf("C275 CURRENT: original22 non-OOM controls UNSUPPORTED/no-output; "
           "%zu genuine OOM positions rollback/retry/canonical destroy PASS\n",
           reached_oom);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && controls(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
