#include "support.h"

static size_t refusals;
static void refused(PInput *p)
{
    char *text = NULL;
    size_t length = 98765;
    assert(!nl_h_custody_validate(p->entry));
    assert(nl_checked_c_node(p->entry, &text, &length) ==
           NL_NODE_C_UNSUPPORTED);
    assert(!text && length == 98765);
    ++refusals;
}
static void accepted(PInput *p)
{
    char *text = NULL;
    size_t length = 0;
    assert(nl_h_custody_validate(p->entry) && nl_h_origin_validate(p->entry) &&
           nl_h_custody_result_valid(p->entry, nl_checked_root(p->entry)));
    assert(nl_checked_c_node(p->entry, &text, &length) == NL_NODE_C_OK);
    assert(text && length && strlen(text) == length);
    free(text);
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    const size_t expected = (size_t)strtoul(argv[2], NULL, 10);
    assert(expected <= 255);
    PInput a = {0}, b = {0};
    p_load(&a, argv[1]);
    assert(p_check(&a) == NL_CHECK_OK);
    p_validate(&a, expected);
    accepted(&a);
    NLCheckedFragment *f = a.read_owner;
    NLCheckedNodeView *n = &f->nodes[a.read - 1];
    const NLCheckedNodeView saved = *n;
#define BAD(statement)                                                         \
    do {                                                                       \
        statement;                                                             \
        refused(&a);                                                           \
        *n = saved;                                                            \
        accepted(&a);                                                          \
    } while (0)
    BAD(n->field.index = 0);
    BAD(n->field.parent = n->field.child);
    BAD(n->field.child = n->field.parent);
    BAD(++n->field.child_incarnation);
    BAD(++n->field.parent_fact);
    BAD(++n->field.child_fact);
    BAD(n->field.access = NL_ACCESS_WRITE);
    BAD(n->field.old_value = n->results[0].value);
    BAD(n->results[0].value = n->field.old_value);
    BAD(n->argument_count = 2);
    /* A changed sealed read cannot fall back to a literal-only H profile. */
    BAD(n->kind = NL_CHECKED_U8_LITERAL; n->has_scalar_result = true;
        n->scalar_result.known = true; n->scalar_result.type = n->type;
        n->scalar_result.value = expected);
#undef BAD
    NLCheckedNodeView *ref = &f->nodes[n->first_argument - 1];
    const NLCheckedNodeView ref_saved = *ref;
    ref->reference_result.writable = true;
    refused(&a);
    *ref = ref_saved;
    accepted(&a);
    const size_t count = f->count;
    ++f->count;
    refused(&a);
    f->count = count;
    NLCheckedNodeView *base = f->nodes;
    void *short_allocation = calloc(1, 1);
    assert(short_allocation);
    f->nodes = short_allocation;
    refused(&a); /* actual issued base/count precede every node access */
    f->nodes = base;
    free(short_allocation);
    accepted(&a);
    const NLSemanticContext *world = f->context;
    NLSemanticContext *clone = NULL;
    assert(nl_sem_clone(world, &clone) == NL_CHECK_OK);
    NLSemanticValueView left, right;
    assert(nl_semantic_value_view(world, n->results[0].value, &left) &&
           nl_semantic_value_view(clone, n->results[0].value, &right) &&
           left.type == right.type && left.scalar_known == right.scalar_known &&
           left.scalar_value == right.scalar_value);
    f->context = clone;
    refused(&a); /* same IDs/serialized byte observation, foreign world */
    f->context = world;
    nl_semantic_destroy(clone);
    accepted(&a);

    p_load(&b, argv[1]);
    assert(p_check(&b) == NL_CHECK_OK);
    p_validate(&b, expected);
    NLCheckedFragment *ma = (NLCheckedFragment *)nl_checked_call_body(
        a.entry, nl_checked_root(a.entry));
    NLCheckedFragment *mb = (NLCheckedFragment *)nl_checked_call_body(
        b.entry, nl_checked_root(b.entry));
    assert(ma && mb && ma->arm_count >= 2 && mb->arm_count == ma->arm_count);
    ma->arms[1].artifact = mb->arms[1].artifact;
    refused(&a);
    /* Canonical custody, not displayed arm pointers, owns destruction. */
    nl_checked_destroy(a.entry);
    a.entry = NULL;
    accepted(&b);
    p_dispose(&a);
    p_dispose(&b);
    printf(
        "OWNED current/loan/clone/short-foreign/changed-read/canonical-destroy "
        "refusals=%zu\n",
        refusals);
    return 0;
}
