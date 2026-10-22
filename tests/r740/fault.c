/* R736 finite fault injection in real source registration and borrowed
 * validation. */
#include "../support/node_checked.h"
#include "newlang/checked.h"
#include "newlang/checked_c_node.h"
#include <stdio.h>
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t calls, position;
void *__wrap_malloc(size_t n)
{
    return injecting && calls++ == position ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && calls++ == position ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && calls++ == position ? NULL : __real_realloc(p, n);
}
static bool same(const NLSemanticSnapshot *a, const NLSemanticSnapshot *b)
{
    return a->types == b->types && a->bindings == b->bindings &&
           a->values == b->values && a->places == b->places &&
           a->domains == b->domains && a->scopes == b->scopes &&
           a->functions == b->functions &&
           a->last_incarnation == b->last_incarnation &&
           a->last_value_fact == b->last_value_fact &&
           a->backing_regions == b->backing_regions &&
           a->raw_intervals == b->raw_intervals &&
           a->occurrences == b->occurrences;
}
static bool find(const NLCheckedFragment *f, const NLCheckedFragment **at,
                 NLCheckedNodeId *id)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->producer.entry_proved) {
            if (*at)
                return false;
            *at = f;
            *id = i;
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body && !find(body, at, id))
            return false;
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t arm = 0; arm < n->item_count; ++arm)
                if (!find(nl_checked_match_arm(f, i, arm), at, id))
                    return false;
    }
    return true;
}
static bool registration(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    const NLSyntaxTree *units[] = {unit};
    size_t rollback = 0, other = 0;
    for (position = 0; position < 96; ++position) {
        NLSemanticContext *context = NULL;
        NLSemanticSnapshot before, after;
        CHECK(nl_semantic_create(&context) == NL_CHECK_OK);
        CHECK(nl_semantic_snapshot(context, &before));
        calls = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_register_function_unit(context, units, 1, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_semantic_destroy(context);
            break;
        }
        CHECK(calls > position && status != NL_CHECK_INTERNAL_ERROR);
        CHECK(nl_semantic_snapshot(context, &after) && same(&before, &after));
        if (status == NL_CHECK_OUT_OF_MEMORY)
            ++rollback;
        else
            ++other;
        CHECK(nl_semantic_register_function_unit(context, units, 1, NULL) ==
              NL_CHECK_OK);
        nl_semantic_destroy(context);
    }
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    CHECK(rollback + other > 0);
    printf("R736_REGISTRATION_OOM tested=%zu explicit_oom=%zu "
           "other_fail_closed=%zu rollback_recovery=PASS\n",
           rollback + other, rollback, other);
    return true;
}
static bool public_fault(const char *path)
{
    TestNode owner = {0};
    CHECK(node_checked_load(path, &owner));
    const NLCheckedFragment *f = NULL;
    NLCheckedNodeId id = 0;
    CHECK(find(owner.entry, &f, &id) && f && id);
    NLProducerLocalReturnView v;
    CHECK(nl_checked_producer_local_return_view(f, id, &v) &&
          nl_checked_producer_local_return_valid(f, id, &v));
    NLSemanticBindingView original;
    CHECK(nl_semantic_binding_view(v.returned, v.local, &original));
    size_t faulted = 0;
    for (position = 0; position < 128; ++position) {
        calls = 0;
        injecting = true;
        bool passed = nl_checked_producer_local_return_valid(f, id, &v);
        injecting = false;
        if (passed)
            break;
        NLSemanticBindingView restored;
        CHECK(calls > position &&
              nl_semantic_binding_view(v.returned, v.local, &restored) &&
              restored.availability == original.availability &&
              restored.value == original.value &&
              restored.place == original.place &&
              nl_checked_producer_local_return_valid(f, id, &v));
        ++faulted;
    }
    CHECK(faulted > 0);
    char *c = NULL;
    size_t length = 919;
    NLNodeCStatus stage = nl_checked_c_node(owner.entry, &c, &length);
    if (stage == NL_NODE_C_OK) {
        CHECK(c && length && length != 919);
        free(c);
        /* R740: bounded fail-at-N within actual admitted emitter stage. */
        size_t failures = 0, explicit_oom = 0;
        for (position = 0; position < 256; ++position) {
            char *fault_c = NULL;
            size_t fault_length = 919;
            calls = 0;
            injecting = true;
            NLNodeCStatus st =
                nl_checked_c_node(owner.entry, &fault_c, &fault_length);
            injecting = false;
            if (st == NL_NODE_C_OK) {
                CHECK(fault_c && fault_length && fault_length != 919);
                free(fault_c);
                break;
            }
            CHECK(calls > position && fault_c == NULL && fault_length == 919);
            ++failures;
            if (st == NL_NODE_C_OUT_OF_MEMORY)
                ++explicit_oom;
            CHECK(nl_checked_producer_local_return_valid(f, id, &v));
            char *recovered = NULL;
            size_t recovered_len = 919;
            CHECK(nl_checked_c_node(owner.entry, &recovered, &recovered_len) ==
                      NL_NODE_C_OK &&
                  recovered && recovered_len > 0 && recovered_len != 919);
            free(recovered);
        }
        CHECK(failures > 0);
        printf("R740_EMIT_OOM faulted=%zu explicit_oom=%zu "
               "atomic_output_recovery=PASS\n",
               failures, explicit_oom);
    } else {
        CHECK(c == NULL && length == 919);
        printf(
            "R736_EMIT baseline_status=%d OOM_NOT_TESTED_NO_SUCCESS_BASELINE\n",
            (int)stage);
    }
    printf("R736_PUBLIC_VALIDATOR_OOM positions=%zu borrowed_unmodified=PASS\n",
           faulted);
    node_checked_destroy(&owner);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2 || !registration(argv[1]) || !public_fault(argv[1]))
        return 1;
    return 0;
}
