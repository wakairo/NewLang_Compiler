#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdlib.h>
void *__real_malloc(size_t);
static bool inject;
static size_t at, calls;
void *__wrap_malloc(size_t n)
{
    return inject && calls++ == at ? NULL : __real_malloc(n);
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
    size_t staging_failures = 0, validation_failures = 0;
    for (at = 0; at < 32; ++at) {
        calls = 0;
        inject = true;
        char *out = NULL;
        size_t sentinel = 37;
        NLNodeCStatus status = nl_checked_c_node(n.entry, &out, &sentinel);
        inject = false;
        if (status == NL_NODE_C_OK) {
            CHECK(calls <= at && out != NULL && strcmp(out, baseline) == 0);
            free(out);
            break;
        }
        /* Validator's existing bool API is fail-closed on scratch OOM.
         * Optimizers may combine/remove scratch mallocs; enumerate actual
         * injected paths rather than making their count a software contract. */
        CHECK(calls > at && out == NULL && sentinel == 37);
        CHECK(status == NL_NODE_C_OUT_OF_MEMORY ||
              status == NL_NODE_C_UNSUPPORTED);
        staging_failures += status == NL_NODE_C_OUT_OF_MEMORY;
        validation_failures += status == NL_NODE_C_UNSUPPORTED;
        CHECK(nl_checked_producer_valid(f, id));
        CHECK(expect(&n, NL_NODE_C_OK, baseline));
    }
    CHECK(at < 32 && staging_failures == 3 && validation_failures != 0);
    free(baseline);
    node_checked_destroy(&n);
    printf(
        "22 certificate/body/world/profile controls leave no output; %zu OOM "
        "positions (%zu staging + %zu validator scratch) retry identically\n",
        at, staging_failures, validation_failures);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && controls(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
