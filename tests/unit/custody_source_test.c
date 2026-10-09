#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
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
static const NLCheckedFragment *codegen_entry;
static bool backend_reject(void)
{
    char *code = NULL;
    size_t length = 37;
    CHECK(nl_checked_c_node(codegen_entry, &code, &length) != NL_NODE_C_OK);
    CHECK(code == NULL && length == 37);
    return true;
}
static bool invalid_codegen(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    CHECK(!nl_checked_custody_valid(f, id));
    return backend_reject();
}
static bool poison(NLCheckedFragment *f)
{
    const NLCheckedNodeId id = f->custody_join;
    size_t faults = 0;
    NLCheckedNodeView *m = &f->nodes[id - 1];
    const NLCheckedNodeView saved = *m;
    m->normal_frame_unchanged = true;
    CHECK(invalid_codegen(f, id));
    *m = saved;
    ++faults;
    m->packet_fork.retained = true;
    CHECK(invalid_codegen(f, id));
    *m = saved;
    ++faults;
    m->normal_arms = 1;
    CHECK(invalid_codegen(f, id));
    *m = saved;
    ++faults;
    const NLSymbolId original = f->custody_binding;
    f->custody_binding = 0;
    CHECK(invalid_codegen(f, id));
    f->custody_binding = original;
    ++faults;
    for (size_t a = 0; a < 2; ++a) {
        NLCheckedFragment *arm = (NLCheckedFragment *)nl_checked_match_arm(
                              f, id, a),
                          *suffix = f->custody_continuations[a];
        NLCheckedFragment *other = f->custody_continuations[1 - a];
        f->custody_continuations[a] = other;
        CHECK(invalid_codegen(f, id));
        f->custody_continuations[a] = suffix;
        ++faults;
        const NLSemanticContext *world = suffix->context;
        NLSemanticContext *clone = NULL;
        CHECK(nl_sem_clone(world, &clone) == NL_CHECK_OK);
        suffix->context = clone;
        CHECK(invalid_codegen(f, id));
        suffix->context = world;
        nl_semantic_destroy(clone);
        ++faults;
        const NLSemanticContext *policy = suffix->custody_policy_world;
        suffix->custody_policy_world =
            nl_checked_match_arm(f, id, 1 - a)->context;
        CHECK(invalid_codegen(f, id));
        suffix->custody_policy_world = policy;
        ++faults;
        NLSemanticContext *entry = suffix->custody_entry;
        suffix->custody_entry = other->custody_entry;
        CHECK(invalid_codegen(f, id));
        suffix->custody_entry = entry;
        ++faults;
        for (size_t n = 0; n < suffix->count; ++n) {
            NLCheckedNodeView *v = &suffix->nodes[n];
            const NLCheckedNodeView old = *v;
            if (v->custody_extraction.present) {
                ++v->custody_extraction.old_sum;
                CHECK(invalid_codegen(f, id));
                *v = old;
                ++faults;
                ++v->custody_extraction.old_occurrence;
                CHECK(invalid_codegen(f, id));
                *v = old;
                ++faults;
                ++v->custody_extraction.packet;
                CHECK(invalid_codegen(f, id));
                *v = old;
                ++faults;
                ++v->custody_extraction.new_sum;
                CHECK(invalid_codegen(f, id));
                *v = old;
                ++faults;
            }
            if (v->custody_none_site == 2) {
                v->custody_none_site = 0;
                CHECK(invalid_codegen(f, id));
                *v = old;
                ++faults;
                v->custody_selected_variant = 2;
                CHECK(invalid_codegen(f, id));
                *v = old;
                ++faults;
            }
            if (v->custody_selected_variant && !v->custody_none_site) {
                v->custody_selected_variant =
                    v->custody_selected_variant == 1 ? 2 : 1;
                CHECK(invalid_codegen(f, id));
                *v = old;
                ++faults;
            }
        }
        const unsigned variant = arm->nodes[arm->root - 1].variant;
        arm->nodes[arm->root - 1].variant = 0;
        CHECK(invalid_codegen(f, id));
        arm->nodes[arm->root - 1].variant = variant;
        ++faults;
        for (size_t n = 0; n < suffix->count; ++n) {
            NLCheckedNodeView *v = &suffix->nodes[n];
            if (v->kind == NL_CHECKED_AGGREGATE_BINDING &&
                v->packet_origin.ancestor) {
                NLCheckedNodeView *receiver =
                    &suffix->nodes[v->first_argument - 1];
                const NLCheckedNodeView old = *receiver;
                receiver->field_index = 3;
                CHECK(invalid_codegen(f, id));
                *receiver = old;
                ++faults;
                receiver->symbol = 0;
                CHECK(invalid_codegen(f, id));
                *receiver = old;
                ++faults;
            }
        }
        if (arm->custody_call_id) {
            NLCheckedNodeView *call = &arm->nodes[arm->custody_call_id - 1];
            const NLCheckedNodeView old = *call;
            for (size_t k = 0; k < 14; ++k) {
                if (k == 0)
                    call->custody_call.entry_proved = false;
                if (k == 1)
                    call->custody_call.post_proved = false;
                if (k == 2)
                    call->custody_call.world =
                        nl_checked_match_arm(f, id, 1 - a)->context;
                if (k == 3)
                    call->custody_call.entry_world = arm->custody_post;
                if (k == 4)
                    call->custody_call.origin = arm;
                if (k == 5)
                    ++call->custody_call.origin_match;
                if (k == 6)
                    ++call->custody_call.packet;
                if (k == 7)
                    ++call->custody_call.occurrence;
                if (k == 8)
                    ++call->custody_call.sink_incarnation;
                if (k == 9)
                    ++call->custody_call.old_none;
                if (k == 10)
                    call->custody_call.donor = 0;
                if (k == 11)
                    call->custody_call.parameters[1] = 0;
                if (k == 12)
                    call->custody_call.new_some = 0;
                if (k == 13)
                    call->custody_call.sink = 0;
                CHECK(invalid_codegen(f, id));
                *call = old;
                ++faults;
            }
            {
                NLCheckedFragment *body =
                    (NLCheckedFragment *)nl_checked_call_body(
                        arm, arm->custody_call_id);
                NLCheckedNodeView *body_root = &body->nodes[body->root - 1];
                const NLCheckedNodeView saved_body = *body_root;
                body_root->item_count = 1;
                CHECK(backend_reject());
                *body_root = saved_body;
                body_root->tail = 0;
                CHECK(backend_reject());
                *body_root = saved_body;
                for (size_t k = 0; k < body->count; ++k) {
                    NLCheckedNodeView *op = &body->nodes[k];
                    const NLCheckedNodeView saved_op = *op;
                    if (op->kind == NL_CHECKED_REPLACE) {
                        ++op->results[0].value;
                        CHECK(backend_reject());
                        *op = saved_op;
                    }
                    if (op->kind == NL_CHECKED_SUM_CONSTRUCTOR) {
                        op->variant = 1;
                        CHECK(backend_reject());
                        *op = saved_op;
                        op->initializer = 0;
                        CHECK(backend_reject());
                        *op = saved_op;
                    }
                    if (op->kind == NL_CHECKED_IDENTIFIER &&
                        op->type == call->custody_call.entry_world
                                        ->bindings[call->custody_call.donor - 1]
                                        .view.type) {
                        ++op->results[0].value;
                        CHECK(backend_reject());
                        *op = saved_op;
                    }
                }
            }
            NLSemanticContext *post = arm->custody_post;
            const NLCheckedNodeView *producer =
                nl_checked_node_view(f, m->packet_fork.producer);
            const NLPlaceId tail = producer->producer.root;
            const NLSemanticPlaceView tail_before = post->places[tail - 1];
            ++post->places[tail - 1].incarnation;
            CHECK(invalid_codegen(f, id));
            post->places[tail - 1] = tail_before;
            ++faults;
            post->places[tail - 1].governing_domain =
                producer->producer.head_domain;
            CHECK(invalid_codegen(f, id));
            post->places[tail - 1] = tail_before;
            ++faults;
            const NLValueId displaced = call->custody_call.old_none;
            const NLSemanticValueView displaced_before =
                post->values[displaced - 1];
            post->values[displaced - 1].carrier = NL_CARRIER_LOOSE;
            CHECK(invalid_codegen(f, id));
            post->values[displaced - 1] = displaced_before;
            ++faults;
            const NLOccurrenceId o = call->custody_call.occurrence;
            post->occurrences[o - 1].live = false;
            CHECK(invalid_codegen(f, id));
            post->occurrences[o - 1].live = true;
            ++faults;
            const NLValueId owned_packet = call->custody_call.packet;
            const NLSemanticValueView owned = post->values[owned_packet - 1];
            post->values[owned_packet - 1].fields[1] = owned.fields[2];
            CHECK(invalid_codegen(f, id));
            post->values[owned_packet - 1] = owned;
            ++faults;
            post->values[owned_packet - 1].dependencies =
                NL_DEPENDENCIES_UNKNOWN;
            CHECK(invalid_codegen(f, id));
            post->values[owned_packet - 1] = owned;
            ++faults;
            const NLPlaceId sink = call->custody_call.sink;
            const NLSemanticValueView none =
                arm->custody_entry->values[call->custody_call.old_none - 1];
            arm->custody_entry->values[call->custody_call.old_none - 1]
                .variant = 0;
            CHECK(invalid_codegen(f, id));
            arm->custody_entry->values[call->custody_call.old_none - 1] = none;
            ++faults;
            arm->custody_entry->values[call->custody_call.old_none - 1]
                .variant = 2;
            CHECK(invalid_codegen(f, id));
            arm->custody_entry->values[call->custody_call.old_none - 1] = none;
            ++faults;
            const NLIncarnationId inc =
                arm->custody_entry->places[sink - 1].incarnation;
            ++arm->custody_entry->places[sink - 1].incarnation;
            CHECK(invalid_codegen(f, id));
            arm->custody_entry->places[sink - 1].incarnation = inc;
            ++faults;
            NLSemanticContext *current = (NLSemanticContext *)arm->context;
            const NLValueId packet = call->custody_call.packet;
            const NLSemanticValueView owner = current->values[packet - 1];
            current->values[packet - 1].fields[1] = owner.fields[2];
            CHECK(invalid_codegen(f, id));
            current->values[packet - 1] = owner;
            ++faults;
            current->values[packet - 1].dependencies = NL_DEPENDENCIES_UNKNOWN;
            CHECK(invalid_codegen(f, id));
            current->values[packet - 1] = owner;
            ++faults;
            const NLSemanticValueView e =
                arm->custody_entry->values[packet - 1];
            arm->custody_entry->values[packet - 1].dependencies =
                NL_DEPENDENCIES_UNKNOWN;
            CHECK(invalid_codegen(f, id));
            arm->custody_entry->values[packet - 1] = e;
            ++faults;
            NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
                arm, arm->custody_call_id);
            for (size_t n = 0; n < body->count; ++n)
                if (body->nodes[n].custody_none_site == 1) {
                    body->nodes[n].custody_none_site = 0;
                    CHECK(invalid_codegen(f, id));
                    body->nodes[n].custody_none_site = 1;
                    ++faults;
                }
        }
        if (suffix->owner_entry_call) {
            NLCheckedNodeView *call =
                &suffix->nodes[suffix->owner_entry_call - 1];
            const NLCheckedNodeView old = *call;
            for (size_t k = 0; k < 7; ++k) {
                if (k == 0)
                    call->owner_call.entry_proved = false;
                if (k == 1)
                    call->owner_call.post_proved = false;
                if (k == 2)
                    ++call->owner_call.incarnation;
                if (k == 3)
                    call->owner_call.inputs[1] = call->owner_call.inputs[2];
                if (k == 4)
                    call->owner_call.domain = 1;
                if (k == 5)
                    call->owner_call.parameters[1] = 0;
                if (k == 6)
                    call->owner_call.donor[2] = 0;
                CHECK(invalid_codegen(f, id));
                *call = old;
                ++faults;
            }
        }
        CHECK(nl_checked_custody_valid(f, id));
    }
    printf("%zu owned custody/conditional/extraction/terminal poison controls "
           "rejected\n",
           faults);
    return true;
}
static bool inspect(NLCheckedFragment *f, size_t *count)
{
    if (f->custody_join != 0) {
        CHECK(nl_checked_custody_valid(f, f->custody_join));
        CHECK(poison(f));
        ++*count;
        puts("actual recipient / both conditional worlds / extraction / "
             "exact-None / original terminal OWNED proof valid");
    }
    for (NLCheckedNodeId id = 1; id <= f->count; ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a) {
                NLCheckedFragment *arm =
                    (NLCheckedFragment *)nl_checked_match_arm(f, id, a);
                if (arm != NULL)
                    CHECK(inspect(arm, count));
            }
        NLCheckedFragment *body =
            (NLCheckedFragment *)nl_checked_call_body(f, id);
        if (body != NULL)
            CHECK(inspect(body, count));
    }
    return true;
}
static bool evidence(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    nl_source_destroy(n.entry_source);
    n.entry_source = NULL;
    codegen_entry = n.entry;
    size_t count = 0;
    CHECK(inspect(n.entry, &count) && count == 1);
    char *code = NULL;
    size_t length = 37;
    CHECK(nl_checked_c_node(n.entry, &code, &length) == NL_NODE_C_OK);
    CHECK(code != NULL && length > 37);
    free(code);
    codegen_entry = NULL;
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
                CHECK(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT ||
                      status == NL_CHECK_SEMANTIC_ERROR ||
                      status == NL_CHECK_INTERNAL_ERROR);
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
static bool codegen_oom(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    nl_source_destroy(n.entry_source);
    n.entry_source = NULL;
    char *baseline = NULL;
    size_t length = 0;
    at = 0;
    fail_at = SIZE_MAX;
    injecting = true;
    NLNodeCStatus status = nl_checked_c_node(n.entry, &baseline, &length);
    injecting = false;
    CHECK(status == NL_NODE_C_OK);
    const size_t allocations = at;
    for (size_t fault = 0; fault < allocations; ++fault) {
        char *code = NULL;
        size_t bytes = 37;
        at = 0;
        fail_at = fault;
        injecting = true;
        status = nl_checked_c_node(n.entry, &code, &bytes);
        injecting = false;
        CHECK(status != NL_NODE_C_OK && code == NULL && bytes == 37);
        CHECK(nl_checked_c_node(n.entry, &code, &bytes) == NL_NODE_C_OK);
        CHECK(bytes == length && memcmp(code, baseline, length + 1) == 0);
        free(code);
    }
    printf("codegen OOM: %zu allocation paths; unchanged output slots and "
           "deterministic retry\n",
           allocations);
    free(baseline);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    bool ok;
    if (strcmp(argv[1], "codegen-oom") == 0)
        ok = codegen_oom(argv[2]);
    else if (strcmp(argv[1], "oom") == 0)
        ok = oom(argv[2]);
    else if (strcmp(argv[1], "reject-oom") == 0)
        ok = reject(argv[2], true);
    else if (strcmp(argv[1], "reject") == 0)
        ok = reject(argv[2], false);
    else
        ok = evidence(argv[2]);
    return ok ? 0 : 1;
}
