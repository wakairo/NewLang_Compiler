#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t allocation, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && allocation++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && allocation++ == fail_at ? NULL : __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && allocation++ == fail_at ? NULL : __real_calloc(n, s);
}

/* Expectations from public source-issued nodes only; never calls codegen. */
static bool view(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    const NLSemanticContext *c = nl_checked_context(f);
    struct {
        size_t symbol, place, incarnation, tag, target;
    } roots[2] = {{0}};
    size_t count = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind != NL_CHECKED_BINDING)
            continue;
        const NLCheckedNodeView *init = nl_checked_node_view(f, v->initializer);
        const NLCheckedNodeView *r = nl_checked_node_view(f, v->first_argument);
        CHECK(init && r);
        if (nl_semantic_recursive_local_type(c, r->type)) {
            CHECK(count < 2 && init->kind == NL_CHECKED_AGGREGATE);
            NLSemanticBindingView b;
            NLSemanticPlaceView p;
            CHECK(nl_semantic_binding_view(c, r->symbol, &b) &&
                  nl_semantic_place_view(c, b.place, &p));
            CHECK(p.independent_root && p.implicit_local &&
                  p.governing_domain == 0);
            size_t scalar = 0, tag = 0, target = 0;
            for (size_t field = init->first_argument; field;) {
                const NLCheckedNodeView *a = nl_checked_node_view(f, field);
                const NLCheckedNodeView *x =
                    nl_checked_node_view(f, a->initializer);
                if (a->field_index == 1) {
                    CHECK(x->has_scalar_result && x->scalar_result.known);
                    scalar = x->scalar_result.value;
                } else {
                    CHECK(a->field_index == 0 &&
                          x->kind == NL_CHECKED_SUM_CONSTRUCTOR);
                    tag = x->variant - 1;
                    if (tag) {
                        const NLCheckedNodeView *arg =
                            nl_checked_node_view(f, x->initializer);
                        CHECK(arg && arg->has_reference_result && count == 1);
                        CHECK(arg->reference_result.place == roots[0].place &&
                              arg->reference_result.incarnation ==
                                  roots[0].incarnation);
                        target = roots[0].symbol;
                    }
                }
                field = a->next_argument;
            }
            roots[count].symbol = r->symbol;
            roots[count].place = b.place;
            roots[count].incarnation = p.incarnation;
            roots[count].tag = tag;
            roots[count].target = target;
            ++count;
            printf("B %zu 1 %zu %zu %zu\n", r->symbol, scalar, tag, target);
        } else if (init->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(count == 1 && !init->loan.from_ptr &&
                  init->loan.source == roots[0].symbol);
            CHECK(init->loan.place == roots[0].place &&
                  init->loan.incarnation == roots[0].incarnation &&
                  init->loan.body_nonescape_proved);
            const NLCheckedNodeView *body =
                nl_checked_node_view(f, init->initializer);
            const NLCheckedNodeView *tail = nl_checked_node_view(f, body->tail);
            CHECK(tail->kind == NL_CHECKED_PTR_FROM_REF &&
                  tail->has_reference_result &&
                  tail->reference_result.scope == 0 &&
                  tail->reference_result.provenance == NL_PROVENANCE_VALID);
            printf("B %zu 2 0 0 %zu\n", r->symbol, roots[0].symbol);
        } else {
            CHECK(init->kind == NL_CHECKED_FIELD_READ && count == 2 &&
                  init->field.base == roots[1].symbol);
            CHECK(init->field.present && init->field.dependency_compatible &&
                  init->field.index == 0);
            printf("B %zu 3 0 %zu %zu\n", r->symbol, roots[1].tag,
                   roots[1].target);
        }
    }
    CHECK(count == 2 && roots[0].place != roots[1].place);
    node_checked_destroy(&n);
    return true;
}

static bool emit(TestNode *n, NLNodeCStatus expected)
{
    TestState *state = malloc(sizeof(*state));
    CHECK(state);
    CHECK(test_state(n->context, state));
    char *output = NULL;
    size_t length = 123;
    CHECK(nl_checked_c_node(n->entry, &output, &length) == expected);
    CHECK(test_unchanged(n->context, state));
    free(state);
    if (expected == NL_NODE_C_OK) {
        CHECK(output && length == strlen(output));
        free(output);
    } else
        CHECK(output == NULL && length == 123);
    return true;
}

static bool controls(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    CHECK(emit(&n, NL_NODE_C_OK));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    NLCheckedNodeView *some = NULL;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_SUM_CONSTRUCTOR &&
            f->nodes[i].variant == 2)
            some = &f->nodes[i];
    CHECK(some);
    NLCheckedNodeView *arg = &f->nodes[some->initializer - 1];
    NLCheckedNodeView original = *arg;
    size_t refused = 0;
#define POISON(change)                                                         \
    do {                                                                       \
        change;                                                                \
        CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));                                \
        *arg = original;                                                       \
        ++refused;                                                             \
    } while (0)
    POISON(arg->has_reference_result = false);
    POISON(arg->reference_result.incarnation += 100);
    POISON(arg->reference_result.place += 100);
    POISON(arg->reference_result.scope = 1);
    POISON(arg->reference_result.occurrence_dependency = 1);
    POISON(arg->reference_result.provenance = NL_PROVENANCE_INVALID);
#undef POISON
    NLSemanticValueView *sum = &n.context->values[some->results[0].value - 1];
    NLSemanticValueView saved = *sum;
    sum->variant = 0;
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    *sum = saved;
    ++refused;
    sum->sum_payload = 0;
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    *sum = saved;
    ++refused;
    NLPlaceId peer = arg->reference_result.place;
    NLIncarnationId inc = n.context->places[peer - 1].incarnation;
    n.context->places[peer - 1].incarnation += 100;
    CHECK(emit(&n, NL_NODE_C_UNSUPPORTED));
    n.context->places[peer - 1].incarnation = inc;
    ++refused;
    CHECK(emit(&n, NL_NODE_C_OK));
    printf("initial Some: %zu wrong packet/carrier/constructor controls "
           "refused; output and owned state unchanged\n",
           refused);
    node_checked_destroy(&n);
    return true;
}

static bool oom(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    TestState *state = malloc(sizeof(*state));
    CHECK(state && test_state(n.context, state));
    bool success = false;
    for (fail_at = 0; fail_at < 4096; ++fail_at) {
        char *out = NULL;
        size_t length = 123;
        allocation = 0;
        injecting = true;
        NLNodeCStatus s = nl_checked_c_node(n.entry, &out, &length);
        injecting = false;
        CHECK(test_unchanged(n.context, state));
        if (s == NL_NODE_C_OK) {
            CHECK(out && length == strlen(out));
            free(out);
            success = true;
            break;
        }
        CHECK(s == NL_NODE_C_OUT_OF_MEMORY && !out && length == 123);
    }
    CHECK(success);
    printf("initial Some OOM positions=%zu; output sentinel and owned state "
           "unchanged\n",
           fail_at);
    free(state);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    return (strcmp(argv[1], "view") == 0  ? view(argv[2])
            : strcmp(argv[1], "oom") == 0 ? oom(argv[2])
                                          : controls(argv[2]))
               ? 0
               : 1;
}
