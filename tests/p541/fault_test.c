/* Controlled negative copies of actual source-issued checkpoints only.
 * Acceptance/refusal always uses the PUBLIC collection validator/C17 API. */
#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/array4.h"
#include <stdint.h>
#include <stdlib.h>
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t allocations;
void *__wrap_malloc(size_t n)
{
    if (injecting) {
        ++allocations;
        return NULL;
    }
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting) {
        ++allocations;
        return NULL;
    }
    return __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    if (injecting) {
        ++allocations;
        return NULL;
    }
    return __real_calloc(n, s);
}
static bool valid_all(const NLCheckedFragment *f)
{
    if (nl_checked_array4_count(f) != 2)
        return false;
    for (size_t i = 0; i < 2; ++i) {
        NLArray4Construction p;
        if (!nl_checked_array4_view_at(f, i, &p) ||
            !nl_checked_array4_valid_at(f, i, &p))
            return false;
    }
    return true;
}
static uint64_t bytes(uint64_t h, const void *p, size_t n)
{
    const unsigned char *s = p;
    for (size_t i = 0; i < n; ++i)
        h = (h ^ s[i]) * UINT64_C(1099511628211);
    return h;
}
static uint64_t world_hash(const NLSemanticContext *c)
{
    uint64_t h = bytes(UINT64_C(14695981039346656037), c, sizeof(*c));
    h = bytes(h, c->places, c->place_count * sizeof(*c->places));
    h = bytes(h, c->values, c->value_count * sizeof(*c->values));
    h = bytes(h, c->bindings, c->binding_count * sizeof(*c->bindings));
    return bytes(h, c->types, c->type_count * sizeof(*c->types));
}
static bool denied(const TestNode *n, const NLCheckedFragment *f)
{
    const uint64_t before = world_hash(n->context);
    CHECK(!valid_all(f));
    char *out = NULL;
    size_t length = 99;
    CHECK(nl_checked_array4_c17(n->entry, &out, &length) ==
              NL_CHECK_SEMANTIC_UNSUPPORTED &&
          !out && length == 99 && before == world_hash(n->context));
    return true;
}
static bool exercise(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    CHECK(valid_all(f) && f->array4_more && !f->array4_more->next);
    NLArray4Construction p[2];
    CHECK(nl_checked_array4_view_at(f, 0, &p[0]) &&
          nl_checked_array4_view_at(f, 1, &p[1]));
    NLSemanticContext *worlds[] = {f->array4_world, f->array4_more->world};
    const uint64_t original[] = {world_hash(worlds[0]), world_hash(worlds[1])};
    size_t refused = 0;
    for (size_t index = 0; index < 2; ++index)
        for (size_t mode = 0; mode < 20; ++mode) {
            NLSemanticContext *copy = NULL;
            CHECK(nl_sem_clone(worlds[index], &copy) == NL_CHECK_OK);
            const NLArray4Construction a = p[index], peer = p[1 - index];
            const NLPlaceId child = a.elements[0].place;
            switch (mode) {
            case 0:
                copy->places[child - 1].parent_aggregate = peer.root;
                break;
            case 1:
                copy->places[a.root - 1].fixed_fields[1] = child;
                break;
            case 2:
                copy->places[a.root - 1].live = false;
                break;
            case 3:
                copy->places[child - 1].live = false;
                break;
            case 4:
                copy->places[child - 1].current_fact = a.elements[1].fact;
                break;
            case 5:
                ++copy->places[child - 1].incarnation;
                break;
            case 6:
                ++copy->places[a.root - 1].incarnation;
                break;
            case 7:
                copy->places[a.root - 1].current_value = a.elements[0].value;
                break;
            case 8:
                copy->types[a.element_type - 1].variant_types[1] =
                    nl_semantic_core_type(copy, NL_TYPE_ALLOCATION);
                break;
            case 9:
                copy->types[a.array_type - 1].view.kind = NL_TYPE_NOMINAL;
                break;
            case 10:
                copy->values[a.value - 1].fields[3] = a.elements[0].value;
                break;
            case 11:
                copy->places[a.root - 1].type = copy->type_count + 1;
                break;
            case 12:
                copy->places[child - 1].current_value = copy->value_count + 1;
                break;
            case 13:
                copy->bindings[a.symbol - 1].view.place = peer.root;
                break;
            case 14:
                copy->places[a.root - 1].governing_domain = 1;
                break;
            case 15:
                copy->places[child - 1].payload_occurrence = 1;
                break;
            case 16:
                ++copy->places[child - 1].parent_incarnation;
                break;
            case 17:
                copy->values[a.value - 1].reference_count = 1;
                break;
            case 18:
                copy->places[a.root - 1].placement.length = 4;
                break;
            case 19:
                copy->values[a.value - 1].aggregate_owner = peer.value;
                break;
            }
            NLSemanticContext **slot =
                index ? &f->array4_more->world : &f->array4_world;
            *slot = copy;
            const uint64_t prior = world_hash(copy);
            CHECK(denied(&n, f) && prior == world_hash(copy));
            *slot = worlds[index];
            nl_semantic_destroy(copy);
            CHECK(valid_all(f));
            ++refused;
        }
    /* This fully coupled cross-root reparenting remains internally
     * well-formed; it has NO corresponding source-issued operation. */
    for (size_t mode = 0; mode < 2; ++mode) {
        NLSemanticContext *copy = NULL;
        CHECK(nl_sem_clone(worlds[1], &copy) == NL_CHECK_OK);
        if (!mode) {
            for (size_t e = 0; e < 4; ++e) {
                const NLArray4Element a = p[0].elements[e],
                                      b = p[1].elements[e];
                copy->places[p[0].root - 1].fixed_fields[e] = b.place;
                copy->places[p[1].root - 1].fixed_fields[e] = a.place;
                copy->values[p[0].value - 1].fields[e] = b.value;
                copy->values[p[1].value - 1].fields[e] = a.value;
                copy->places[a.place - 1].parent_aggregate = p[1].root;
                copy->places[a.place - 1].parent_incarnation = p[1].incarnation;
                copy->places[b.place - 1].parent_aggregate = p[0].root;
                copy->places[b.place - 1].parent_incarnation = p[0].incarnation;
                copy->values[a.value - 1].aggregate_owner = p[1].value;
                copy->values[b.value - 1].aggregate_owner = p[0].value;
            }
        } else {
            copy->bindings[p[1].symbol - 1].view.place = p[0].root;
            copy->bindings[p[1].symbol - 1].view.value = p[0].value;
        }
        CHECK(nl_sem_validate(copy) == NL_CHECK_OK);
        f->array4_more->world = copy;
        CHECK(denied(&n, f));
        f->array4_more->world = worlds[1];
        nl_semantic_destroy(copy);
        ++refused;
    }
    size_t graph_refused = 0;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(denied(&n, f));                                                  \
        restore;                                                               \
        CHECK(valid_all(f));                                                   \
        ++graph_refused;                                                       \
    } while (0)
    NLArray4Capture *more = f->array4_more;
    REFUSE(f->array4_more = NULL, f->array4_more = more);
    REFUSE(more->next = more, more->next = NULL);
    REFUSE(more->world = worlds[0], more->world = worlds[1]);
    REFUSE(f->array4_world = worlds[1], f->array4_world = worlds[0]);
    struct NLArray4Construction *issued = more->issued;
    NLCheckedNodeId binding = more->binding;
    REFUSE((more->issued = f->array4_issued, more->binding = f->array4_binding),
           (more->issued = issued, more->binding = binding));
    REFUSE(more->issued = NULL, more->issued = issued);
    NLCheckedNodeView *b = &f->nodes[p[1].binding - 1], saved = *b;
    REFUSE(b->initializer = p[0].constructor, *b = saved);
    REFUSE(b->span.start_byte += 1, *b = saved);
    NLCheckedNodeView *root = &f->nodes[f->root - 1], root_saved = *root;
    REFUSE(root->item_count = 1, *root = root_saved);
    REFUSE(root->first_item = p[1].binding, *root = root_saved);
    NLSemanticPlaceView *final_root = &n.context->places[p[1].root - 1];
    const NLSemanticPlaceView final_saved = *final_root;
    REFUSE(++final_root->incarnation, *final_root = final_saved);
#undef REFUSE
    CHECK(original[0] == world_hash(worlds[0]) &&
          original[1] == world_hash(worlds[1]));
    allocations = 0;
    injecting = true;
    const bool valid = valid_all(f);
    injecting = false;
    CHECK(valid && !allocations);
    const uint64_t before = world_hash(n.context);
    char *out = NULL;
    size_t length = 77;
    allocations = 0;
    injecting = true;
    NLCheckStatus status = nl_checked_array4_c17(n.entry, &out, &length);
    injecting = false;
    CHECK(status == NL_CHECK_OUT_OF_MEMORY && allocations == 1 && !out &&
          length == 77 && before == world_hash(n.context) &&
          original[0] == world_hash(worlds[0]) &&
          original[1] == world_hash(worlds[1]));
    CHECK(nl_checked_array4_c17(n.entry, &out, &length) == NL_CHECK_OK && out);
    free(out);
    printf("P541_FAULT checkpoint_mutants=%zu graph_mutants=%zu "
           "coupled_raw_kernel_valid_but_not_issued=2 "
           "validator_allocations=0 emit_OOM=1 rollback=PASS restored=PASS\n",
           refused, graph_refused);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && exercise(argv[1]) ? 0 : 1;
}
