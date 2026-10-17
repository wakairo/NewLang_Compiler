/* Private layout is used ONLY to replace test-owned checkpoints with separately
 * cloned corrupted worlds; acceptance is always the PUBLIC validator/emitter.
 * All worlds originate in actual source, never in host semantic seeds. */
#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/array4.h"
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
static bool exercise(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    NLArray4Construction issued;
    CHECK(nl_checked_array4_view(f, &issued) &&
          nl_checked_array4_valid(f, &issued));
    NLSemanticContext *world = f->array4_world;
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(n.context, before));
    size_t refused = 0;
    for (size_t mode = 0; mode < 12; ++mode) {
        NLSemanticContext *copy = NULL;
        CHECK(nl_sem_clone(world, &copy) == NL_CHECK_OK);
        NLArray4Construction altered = issued;
        NLPlaceId root = issued.root, child = issued.elements[0].place;
        switch (mode) {
        case 0:
            copy->places[child - 1].parent_aggregate = issued.elements[1].place;
            break;
        case 1:
            copy->places[root - 1].fixed_fields[1] = child;
            break;
        case 2:
            copy->places[root - 1].live = false;
            break;
        case 3:
            copy->places[child - 1].current_fact = issued.elements[1].fact;
            break;
        case 4:
            ++copy->places[root - 1].incarnation;
            break;
        case 5:
            copy->places[root - 1].current_value = issued.elements[0].value;
            break;
        case 6:
            copy->types[issued.element_type - 1].variant_types[1] =
                nl_semantic_core_type(copy, NL_TYPE_ALLOCATION);
            break;
        case 7:
            copy->types[issued.array_type - 1].view.kind = NL_TYPE_NOMINAL;
            break;
        case 8:
            copy->values[issued.value - 1].fields[3] = issued.elements[0].value;
            break;
        case 9:
            altered.fact = ++copy->places[root - 1].current_fact;
            break;
        case 10:
            copy->places[root - 1].type = copy->type_count + 1;
            break;
        case 11:
            copy->places[child - 1].current_value = copy->value_count + 1;
            break;
        }
        f->array4_world = copy;
        CHECK(!nl_checked_array4_valid(f, &altered));
        char *out = NULL;
        size_t length = 99;
        CHECK(nl_checked_array4_c17(n.entry, &out, &length) ==
                  NL_CHECK_SEMANTIC_UNSUPPORTED &&
              out == NULL && length == 99);
        f->array4_world = world;
        nl_semantic_destroy(copy);
        CHECK(nl_checked_array4_valid(f, &issued) &&
              test_unchanged(n.context, before));
        ++refused;
    }
    TestNode foreign = {0};
    CHECK(node_checked_load(path, &foreign));
    const NLCheckedFragment *other =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    NLFunctionBody *plan = f->body_owner;
    const NLSource *source = f->source;
    f->body_owner = other->body_owner;
    f->source = other->source;
    CHECK(!nl_checked_array4_valid(f, &issued));
    f->body_owner = plan;
    f->source = source;
    node_checked_destroy(&foreign);
    CHECK(nl_checked_array4_valid(f, &issued));
    /* Missing source-issued event record never becomes a guessed root. */
    struct NLArray4Construction *record = f->array4_issued;
    f->array4_issued = NULL;
    CHECK(!nl_checked_array4_valid(f, &issued));
    f->array4_issued = record;
    allocations = 0;
    injecting = true;
    const bool valid = nl_checked_array4_valid(f, &issued);
    injecting = false;
    CHECK(valid && allocations == 0);
    char *out = NULL;
    size_t length = 77;
    allocations = 0;
    injecting = true;
    NLCheckStatus status = nl_checked_array4_c17(n.entry, &out, &length);
    injecting = false;
    CHECK(status == NL_CHECK_OUT_OF_MEMORY && allocations == 1 && out == NULL &&
          length == 77);
    CHECK(test_unchanged(n.context, before));
    CHECK(nl_checked_array4_c17(n.entry, &out, &length) == NL_CHECK_OK && out &&
          length > 0);
    free(out);
    printf("P534_FAULT corrupted_worlds=%zu "
           "source_issued_record_missing=REJECT foreign_source_plan=REJECT "
           "public_validator_allocations=0 emit_OOM=1 output_UNCHANGED "
           "semantic_world=UNCHANGED restored=PASS\n",
           refused);
    free(before);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && exercise(argv[1]) ? 0 : 1;
}
