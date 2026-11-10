#include "../p1280/support.h"
#include <errno.h>

static bool armed, fired;
static size_t ordinal, events;
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
static NLCheckedFragment *body(PInput *p)
{
    return (NLCheckedFragment *)nl_checked_call_body(p->entry,
                                                     nl_checked_root(p->entry));
}
static NLCheckedNodeId loop_id(PInput *p)
{
    NLCheckedFragment *f = body(p);
    NLCheckedNodeId found = 0;
    assert(f);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id)
        if (nl_checked_node_view(f, id)->kind == NL_CHECKED_LOOP) {
            assert(!found);
            found = id;
        }
    assert(found);
    return found;
}
static bool admission(PInput *p, V0Program *program)
{
    *program = (V0Program){.unit = nl_semantic_unit_type(p->context),
                           .u8 = nl_semantic_core_type(p->context, NL_TYPE_U8)};
    return v0_validate_call(p->entry, nl_checked_root(p->entry), program);
}
static NLLinearBreakView valid(PInput *p)
{
    NLLinearBreakView v = {0};
    assert(nl_linear_break_validate(body(p), loop_id(p), &v) == NL_CHECK_OK);
    assert(v.changing_current && v.terminal_old && v.root && v.child &&
           v.incarnation && v.child_incarnation && v.entry_world &&
           v.post_world && v.terminal_loan && v.limit == 2 &&
           v.initial_phase == 0 && v.next_phase == 1);
    /* Cyclic evidence is not relabelled as the public direct-BREAK C178. */
    assert(!nl_checked_loop_break_provenance_valid(body(p), loop_id(p)));
    const NLCheckedNodeView *loan =
        nl_checked_node_view(v.breaking, v.terminal_loan);
    NLSemanticScopeView scope;
    assert(loan && loan->loan.scope &&
           nl_semantic_scope_view(nl_checked_context(v.breaking),
                                  loan->loan.scope, &scope) &&
           !scope.active);
    V0Program program;
    assert(admission(p, &program) && program.current_loop &&
           program.uses_usize && program.linear_break);
    return v;
}
static void rejected(PInput *p, NLCheckedFragment *checked,
                     NLCheckedNodeId loop, const char *label)
{
    NLLinearBreakView v = {0};
    NLCheckStatus status = nl_linear_break_validate(checked, loop, &v);
    V0Program program;
    bool admitted = admission(p, &program);
    printf("negative %s private-status=%d CLI-admitted=%d\n", label, status,
           admitted);
    assert(status != NL_CHECK_OK && !admitted);
}
static void negatives(PInput *p, const char *path)
{
    NLLinearBreakView v = valid(p);
    /* Borrow while authentic. A later negative can make the public body
     * getter itself reject; do not assume it still returns the old view. */
    NLCheckedFragment *checked = body(p);
    const NLCheckedNodeId loop_node = loop_id(p);
    NLSemanticContext *post = (NLSemanticContext *)v.post_world;
    NLIncarnationId incarnation = post->places[v.child - 1].incarnation;
    ++post->places[v.child - 1].incarnation;
    rejected(p, checked, loop_node, "stale-postcurrent-child");
    post->places[v.child - 1].incarnation = incarnation;
    v = valid(p);

    NLCheckedFragment *terminal = (NLCheckedFragment *)v.breaking;
    NLCheckedNodeView *replace = NULL;
    for (NLCheckedNodeId id = 1; id <= terminal->count; ++id)
        if (terminal->nodes[id - 1].kind == NL_CHECKED_REPLACE) {
            assert(!replace);
            replace = &terminal->nodes[id - 1];
        }
    assert(replace && replace->field.present);
    NLCheckedNodeView saved = *replace;
    replace->field.index = 1 - v.index;
    rejected(p, checked, loop_node, "wrong-terminal-field");
    *replace = saved;
    v = valid(p);

    const NLCheckedNodeView *loan =
        nl_checked_node_view(terminal, v.terminal_loan);
    NLSemanticContext *at = (NLSemanticContext *)terminal->context;
    assert(loan->loan.scope && loan->loan.scope <= at->scope_count);
    NLSemanticScopeView *scope = &at->scopes[loan->loan.scope - 1];
    assert(!scope->active);
    scope->active = true;
    rejected(p, checked, loop_node, "reopened-terminal-loan");
    scope->active = false;
    v = valid(p);

    PInput other = {0};
    p_load(&other, path);
    assert(p_check(&other) == NL_CHECK_OK);
    NLLinearBreakView foreign = valid(&other);
    NLCheckedFragment *loop =
        (NLCheckedFragment *)nl_checked_loop_body(body(p), loop_id(p));
    size_t slot = 0;
    while (slot < loop->arm_count && loop->arms[slot].artifact != v.iteration)
        ++slot;
    assert(slot < loop->arm_count && foreign.iteration != v.iteration);
    NLCheckedArm original = loop->arms[slot];
    loop->arms[slot].artifact = (NLCheckedFragment *)foreign.iteration;
    rejected(p, checked, loop_node, "foreign-CHECK-world-arm");
    loop->arms[slot] = original;
    valid(p);
    valid(&other);
    p_dispose(&other);
    puts("NEGATIVE4 restoration4 genuine-world4 PASS");
}
static void check_oom(const char *path)
{
    PInput baseline = {0};
    p_load(&baseline, path);
    events = ordinal = 0;
    armed = true;
    NLCheckStatus status = p_check(&baseline);
    armed = false;
    const size_t total = events;
    assert(status == NL_CHECK_OK && total > 4);
    valid(&baseline);
    p_dispose(&baseline);
    printf("CHECK actual allocations=%zu\n", total);
    const size_t selected[] = {1, 2, total / 2, total};
    for (size_t i = 0; i < 4; ++i) {
        PInput p = {0};
        p_load(&p, path);
        NLSemanticContext before = *p.context;
        events = 0;
        fired = false;
        ordinal = selected[i];
        armed = true;
        status = p_check(&p);
        armed = false;
        ordinal = 0;
        printf("CHECK OOM ordinal=%zu actual=%zu fired=%d status=%d out=%p\n",
               selected[i], events, fired, status, (void *)p.entry);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY && !p.entry &&
               !memcmp(&before, p.context, sizeof(before)));
        assert(p_check(&p) == NL_CHECK_OK);
        valid(&p);
        p_dispose(&p);
    }
    puts("CHECK OOM4 rollback4 retry4 PASS");
}
int main(int argc, char **argv)
{
    assert(argc == 3);
    setvbuf(stdout, NULL, _IONBF, 0);
    for (int i = 1; i < 3; ++i) {
        PInput p = {0};
        p_load(&p, argv[i]);
        assert(p_check(&p) == NL_CHECK_OK);
        NLLinearBreakView v = valid(&p);
        printf("issued original=%zu child=%zu index=%zu incarnation=%zu "
               "child-incarnation=%zu old-result-type=%zu\n",
               v.root, v.child, v.index, v.incarnation, v.child_incarnation,
               v.result.type);
        negatives(&p, argv[i]);
        V0Program program;
        events = ordinal = 0;
        armed = true;
        bool admitted = admission(&p, &program);
        armed = false;
        assert(admitted);
        printf("admission allocations=%zu\n", events);
        p_dispose(&p);
        check_oom(argv[i]);
    }
    return 0;
}
