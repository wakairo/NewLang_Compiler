#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/node_links.h"
#include <stdint.h>
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t calls, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && calls++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && calls++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    if (injecting && calls++ == fail_at)
        return NULL;
    return __real_calloc(n, s);
}
static uint64_t hash(uint64_t h, const void *p, size_t n)
{
    const unsigned char *b = p;
    while (n--) {
        h ^= *b++;
        h *= UINT64_C(1099511628211);
    }
    return h;
}
static uint64_t fingerprint(const NLCheckedFragment *f)
{
    uint64_t h = UINT64_C(1469598103934665603);
    h = hash(h, f->context->places,
             f->context->place_count * sizeof(*f->context->places));
    h = hash(h, f->context->values,
             f->context->value_count * sizeof(*f->context->values));
    h = hash(h, f->context->bindings,
             f->context->binding_count * sizeof(*f->context->bindings));
    h = hash(h, f->nodes, f->count * sizeof(*f->nodes));
    for (size_t i = 0; i < 2; ++i) {
        const NLCapturedChange *v = f->field_changes[i];
        const NLSemanticContext *w[2] = {v->before, v->after};
        for (size_t j = 0; j < 2; ++j) {
            const NLSemanticContext *c = w[j];
            h = hash(h, c->places, c->place_count * sizeof(*c->places));
            h = hash(h, c->values, c->value_count * sizeof(*c->values));
            h = hash(h, c->bindings, c->binding_count * sizeof(*c->bindings));
            h = hash(h, c->occurrences,
                     c->occurrence_count * sizeof(*c->occurrences));
            h = hash(h, c->scopes, c->scope_count * sizeof(*c->scopes));
        }
    }
    return h;
}
static bool refused(TestNode *n, const NLCheckedFragment *f)
{
    char *out = NULL;
    size_t len = 77;
    CHECK(nl_checked_node_links_validate(f, NULL) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(nl_checked_c_node(n->entry, &out, &len) == NL_NODE_C_UNSUPPORTED &&
          !out && len == 77);
    return true;
}
static bool exercise(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    NLNodeLinksView v;
    CHECK(nl_checked_node_links_view(f, &v) &&
          nl_checked_node_links_validate(f, &v) == NL_CHECK_OK);
    size_t rejected = 0;
    for (size_t mode = 0; mode < 7; ++mode) {
        NLCheckedNodeView *op = &f->nodes[v.changes[0].operation - 1];
        NLCheckedNodeView saved = *op;
        switch (mode) {
        case 0:
            op->field.parent = v.originals[2];
            break;
        case 1:
            op->field.child =
                v.changes[0].before->places[v.originals[1] - 1].fixed_fields[1];
            break;
        case 2:
            op->field.parent_fact = 1;
            break;
        case 3:
            op->field.child_fact = 1;
            break;
        case 4:
            op->field.old_value = op->field.new_value;
            break;
        case 5:
            op->field.payload_occurrence = 0;
            break;
        case 6:
            op->field.parent_incarnation = v.originals[0];
            break;
        }
        CHECK(refused(&n, f));
        *op = saved;
        CHECK(nl_checked_node_links_validate(f, NULL) == NL_CHECK_OK);
        ++rejected;
    }
    NLCapturedChange *one = f->field_changes[0], *two = f->field_changes[1];
    f->field_changes[1] = one;
    CHECK(refused(&n, f));
    f->field_changes[1] = two;
    ++rejected;
    f->field_changes[0] = two;
    f->field_changes[1] = one;
    CHECK(refused(&n, f));
    f->field_changes[0] = one;
    f->field_changes[1] = two;
    ++rejected;
    f->field_change_count = 1;
    CHECK(refused(&n, f));
    f->field_change_count = 2;
    ++rejected;
    const NLCheckedNodeView *op =
        nl_checked_node_view(f, v.changes[0].operation);
    NLSemanticPlaceView *child = &one->after->places[op->field.child - 1];
    NLValueFactId saved = child->current_fact;
    child->current_fact = op->field.child_fact;
    CHECK(refused(&n, f));
    child->current_fact = saved;
    ++rejected;
    const NLPlaceId sibling =
        one->after->places[op->field.parent - 1].fixed_fields[1];
    NLSemanticValueView *s =
        &one->after->values[one->after->places[sibling - 1].current_value - 1];
    size_t scalar = s->scalar_value;
    s->scalar_value ^= 1;
    CHECK(refused(&n, f));
    s->scalar_value = scalar;
    ++rejected;
    NLSemanticOccurrenceView *occ =
        &one->after->occurrences[op->field.payload_occurrence - 1];
    bool live = occ->live;
    occ->live = true;
    CHECK(refused(&n, f));
    occ->live = live;
    ++rejected;
    /* A coupled, well-typed mutation in BOTH second snapshots can pass an
     * isolated Change replay. The bridge must still reject resurrected
     * different content. */
    const NLPlaceId left = v.originals[1];
    NLValueId link =
        two->before->places[two->before->places[left - 1].fixed_fields[0] - 1]
            .current_value;
    NLValueId payload = two->before->values[link - 1].sum_payload;
    NLReferenceFacts a = two->before->values[payload - 1].reference,
                     b = two->after->values[payload - 1].reference;
    two->before->values[payload - 1].reference.place = v.originals[0];
    two->before->values[payload - 1].reference.incarnation =
        one->before->places[v.originals[0] - 1].incarnation;
    two->after->values[payload - 1].reference =
        two->before->values[payload - 1].reference;
    CHECK(nl_sem_validate(two->before) == NL_CHECK_OK &&
          nl_sem_validate(two->after) == NL_CHECK_OK);
    NLSemanticContext *isolated = NULL;
    const NLCheckedNodeView *second =
        nl_checked_node_view(f, v.changes[1].operation);
    CHECK(nl_sem_clone(two->before, &isolated) == NL_CHECK_OK);
    CHECK(nl_fixed_change(isolated, second->field.child,
                          second->field.new_value, false) == NL_CHECK_OK);
    for (size_t k = 0; k < isolated->binding_count; ++k)
        if (isolated->bindings[k].view.place == second->field.parent &&
            isolated->bindings[k].view.availability == NL_AVAILABLE)
            isolated->bindings[k].view.value =
                isolated->places[second->field.parent - 1].current_value;
    CHECK(nl_packet_same_entry(isolated, two->after));
    nl_semantic_destroy(isolated);
    CHECK(refused(&n, f));
    two->before->values[payload - 1].reference = a;
    two->after->values[payload - 1].reference = b;
    ++rejected;
    const NLCheckedNodeView *argument =
        nl_checked_node_view(f, op->first_argument);
    NLSemanticValueView *write_ref =
        &one->before->values[argument->results[0].value - 1];
    const NLReferenceFacts original_ref = write_ref->reference;
    for (size_t mode = 0; mode < 3; ++mode) {
        if (!mode)
            write_ref->reference.scope = 0;
        else if (mode == 1)
            write_ref->reference.writable = false;
        else
            write_ref->reference.provenance = NL_PROVENANCE_UNKNOWN;
        CHECK(refused(&n, f));
        write_ref->reference = original_ref;
        ++rejected;
    }
    /* First draft accepted a coupled corrupted ref<T> registry. Preserve a
     * regression for actual source type identity, not just access-mode bits. */
    const NLTypeId ref_type = write_ref->type;
    const NLTypeId target = one->before->types[ref_type - 1].view.target;
    one->before->types[ref_type - 1].view.target =
        nl_semantic_core_type(n.context, NL_TYPE_U8);
    one->after->types[ref_type - 1].view.target =
        one->before->types[ref_type - 1].view.target;
    CHECK(nl_sem_validate(one->before) == NL_CHECK_OK &&
          nl_sem_validate(one->after) == NL_CHECK_OK);
    CHECK(refused(&n, f));
    one->before->types[ref_type - 1].view.target = target;
    one->after->types[ref_type - 1].view.target = target;
    ++rejected;
    CHECK(nl_checked_node_links_validate(f, NULL) == NL_CHECK_OK);
    uint64_t before = fingerprint(f);
    fail_at = SIZE_MAX;
    calls = 0;
    injecting = true;
    NLCheckStatus valid = nl_checked_node_links_validate(f, NULL);
    injecting = false;
    CHECK(valid == NL_CHECK_OK);
    size_t allocations = calls;
    for (size_t i = 0; i < allocations; ++i) {
        fail_at = i;
        calls = 0;
        injecting = true;
        valid = nl_checked_node_links_validate(f, NULL);
        injecting = false;
        CHECK(valid == NL_CHECK_OUT_OF_MEMORY && fingerprint(f) == before &&
              nl_checked_node_links_validate(f, NULL) == NL_CHECK_OK);
    }
    char *out = NULL;
    size_t len = 77;
    fail_at = SIZE_MAX;
    calls = 0;
    injecting = true;
    NLNodeCStatus status = nl_checked_c_node(n.entry, &out, &len);
    injecting = false;
    CHECK(status == NL_NODE_C_OK && out && len);
    const size_t emit_allocations = calls;
    free(out);
    for (size_t i = 0; i < emit_allocations; ++i) {
        out = NULL;
        len = 77;
        fail_at = i;
        calls = 0;
        injecting = true;
        status = nl_checked_c_node(n.entry, &out, &len);
        injecting = false;
        CHECK(status == NL_NODE_C_OUT_OF_MEMORY && !out && len == 77 &&
              fingerprint(f) == before);
    }
    printf("FAULT refusals=%zu coupled_isolated_replay=PASS bridge=REJECT "
           "validator_OOM=%zu "
           "rollback=PASS emit_OOM=%zu worlds=UNCHANGED\n",
           rejected, allocations, emit_allocations);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && exercise(argv[1]) ? 0 : 1;
}
