#include "../p1280/support.h"
#include "copy_body_internal.h"
#include <errno.h>

static bool armed, fired;
static size_t events, ordinal;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool fail(void)
{
    if (!armed)
        return false;
    ++events;
    if (!ordinal || events != ordinal)
        return false;
    fired = true;
    errno = ENOMEM;
    return true;
}
void *__wrap_malloc(size_t n)
{
    return fail() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return fail() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return fail() ? NULL : __real_realloc(p, n);
}
static bool admitted(PInput *p, V0Program *program)
{
    *program = (V0Program){.unit = nl_semantic_unit_type(p->context),
                           .u8 = nl_semantic_core_type(p->context, NL_TYPE_U8)};
    return v0_validate_call(p->entry, p->entry->root, program);
}
static void positive(const char *path)
{
    PInput p = {0};
    p_load(&p, path);
    assert(p_check(&p) == NL_CHECK_OK);
    V0Program program;
    assert(admitted(&p, &program) && program.count == 2 &&
           program.current_callee && program.uses_usize &&
           program.current_loop && program.linear_break && program.aggregate &&
           !program.stuttering_loop && !program.scalar_loop);
    NLCheckedFragment *caller =
        (NLCheckedFragment *)nl_checked_call_body(p.entry, p.entry->root);
    assert(caller && caller->body_count == 1);
    NLCheckedFragment *callee = caller->bodies[0];
    NLCopyBodyProof *copy = callee->copy_body;
    assert(copy && copy->current_loop && copy->owner == callee &&
           copy->caller == caller);
    const NLCheckedNodeId loop = copy->current_loop;
    copy->current_loop = 0;
    assert(!admitted(&p, &program));
    copy->current_loop = loop;
    assert(admitted(&p, &program));
    NLCheckedNodeView *tail =
        &callee->nodes[callee->nodes[callee->root - 1].tail - 1];
    const NLCheckedResult result = tail->results[0];
    tail->results[0].value = 0;
    assert(!admitted(&p, &program));
    tail->results[0] = result;
    assert(admitted(&p, &program));
    const size_t function = caller->nodes[caller->body_calls[0] - 1].function;
    const NLTypeId type = p.context->functions[function - 1].result;
    p.context->functions[function - 1].result = 1;
    assert(!admitted(&p, &program));
    p.context->functions[function - 1].result = type;
    assert(admitted(&p, &program));
    admitted(&p, &program);
    program =
        (V0Program){.unit = program.unit, .u8 = program.u8, .u8_export = true};
    assert(!v0_validate_call(p.entry, p.entry->root, &program));
    assert(admitted(&p, &program));
    program =
        (V0Program){.unit = program.unit, .u8 = program.u8, .scalar_depth = 1};
    assert(!v0_validate_call(p.entry, p.entry->root, &program));
    assert(admitted(&p, &program));
    NLSemanticContext before;
    memcpy(&before, p.context, sizeof(before));
    NLCopyBodyView view;
    assert(nl_checked_copy_body_view(caller, caller->body_calls[0], &view));
    events = ordinal = 0;
    armed = true;
    const bool ok = admitted(&p, &program);
    armed = false;
    const size_t total = events;
    assert(ok && total);
    printf("consumer census=%zu\n", total);
    size_t points[] = {1, 2, total / 2, total};
    for (size_t i = 0; i < 4; ++i) {
        if (!points[i])
            continue;
        bool duplicate = false;
        for (size_t j = 0; j < i; ++j)
            duplicate |= points[i] == points[j];
        if (duplicate)
            continue;
        events = 0;
        ordinal = points[i];
        fired = false;
        armed = true;
        const bool accepted = admitted(&p, &program);
        armed = false;
        ordinal = 0;
        printf("consumer OOM target=%zu actual=%zu fired=%d accepted=%d "
               "status=%d\n",
               points[i], events, fired, accepted, program.copy_body_status);
        assert(fired && !accepted &&
               program.copy_body_status == NL_CHECK_OUT_OF_MEMORY &&
               !memcmp(&before, p.context, sizeof(before)));
        NLCopyBodyView after;
        assert(
            nl_checked_copy_body_view(caller, caller->body_calls[0], &after) &&
            after.body == view.body &&
            after.result.value == view.result.value &&
            after.live_exit == view.live_exit);
        assert(admitted(&p, &program));
    }
    p_dispose(&p);
    puts(
        "genuine callee admission/metadata lineage negatives + OOM/retry PASS");
}
int main(int argc, char **argv)
{
    assert(argc == 3);
    setbuf(stdout, NULL);
    positive(argv[1]);
    positive(argv[2]);
    return 0;
}
