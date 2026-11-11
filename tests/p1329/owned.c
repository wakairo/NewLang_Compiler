#include "../p1280/support.h"
#include "copy_body_internal.h"
#include <errno.h>

static bool armed, fired;
static size_t events, ordinal, retained, seals[2], seal_count;
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
NLCheckStatus __real_nl_copy_body_current(NLCheckedFragment *, NLCheckedNodeId);
NLCheckStatus __wrap_nl_copy_body_current(NLCheckedFragment *f,
                                          NLCheckedNodeId id)
{
    if (armed && !ordinal)
        retained = events;
    return __real_nl_copy_body_current(f, id);
}
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *p,
                                              NLCheckedFragment *c,
                                              NLCheckedNodeId id)
{
    if (armed && !ordinal && seal_count < 2)
        seals[seal_count++] = events + 1;
    return __real_nl_h_custody_seal_result(p, c, id);
}

typedef struct {
    NLCheckedFragment *main_body, *callee;
    NLCheckedNodeId call, loop;
    NLLinearBreakView current;
    NLCopyBodyView copy;
} Relation;
static Relation relation(PInput *p, size_t index)
{
    Relation r = {0};
    r.main_body = (NLCheckedFragment *)nl_checked_call_body(
        p->entry, nl_checked_root(p->entry));
    assert(r.main_body && r.main_body != p->entry);
    for (NLCheckedNodeId id = 1; id <= r.main_body->count; ++id)
        if (r.main_body->nodes[id - 1].kind == NL_CHECKED_REGISTERED_CALL) {
            assert(!r.call);
            r.call = id;
        }
    assert(r.call && nl_h_custody_result_valid(p->entry, p->entry->root) &&
           nl_h_custody_result_valid(r.main_body, r.call));
    r.callee = (NLCheckedFragment *)nl_checked_call_body(r.main_body, r.call);
    assert(r.callee && r.callee != p->entry && r.callee != r.main_body);
    for (NLCheckedNodeId id = 1; id <= r.callee->count; ++id)
        if (r.callee->nodes[id - 1].kind == NL_CHECKED_LOOP) {
            assert(!r.loop);
            r.loop = id;
        }
    assert(r.loop && nl_linear_break_owner_validate(r.callee) == NL_CHECK_OK);
    assert(nl_linear_break_validate(r.callee, r.loop, &r.current) ==
           NL_CHECK_OK);
    assert(nl_checked_copy_body_view(r.main_body, r.call, &r.copy));
    const NLCheckStatus status =
        nl_checked_copy_body_validate(r.main_body, r.call, &r.copy);
    printf("COPY status=%d events=%zu index=%zu\n", status, r.copy.event_count,
           index);
    fflush(stdout);
    assert(status == NL_CHECK_OK && r.callee->copy_body->sealed &&
           r.callee->copy_body->current_loop == r.loop &&
           r.current.changing_current && r.current.terminal_old &&
           !r.current.paired_current && !r.current.scalar_loop &&
           !r.current.incoming_call && !r.current.terminal_call &&
           r.current.index == index && r.copy.event_count == 3);
    const NLSemanticContext *exit = r.copy.live_exit;
    const NLValueId value = r.copy.result.value;
    assert(r.copy.body == r.callee &&
           r.main_body->nodes[r.call - 1].results[0].value == value &&
           nl_copy_plain(exit, value, r.copy.result.type) &&
           nl_copy_plain(p->context, value, r.copy.result.type) &&
           !exit->values[value - 1].scalar_known &&
           !p->context->values[value - 1].scalar_known &&
           exit->places[r.current.root - 1].live &&
           !p->context->places[r.current.root - 1].live &&
           r.current.entry_world->places[r.current.root - 1].incarnation ==
               r.current.incarnation &&
           r.current.post_world->places[r.current.root - 1].incarnation ==
               r.current.incarnation);
    /* Follow real binding/Copy operands back to the actual LOOP result. */
    NLCheckedNodeId lineage = r.callee->nodes[r.callee->root - 1].tail;
    size_t steps = 0;
    while (r.callee->nodes[lineage - 1].kind == NL_CHECKED_IDENTIFIER) {
        const NLCheckedNodeView use = r.callee->nodes[lineage - 1];
        assert(use.value_use == NL_VALUE_COPIED && ++steps < 8);
        NLCheckedNodeId binding = 0;
        for (NLCheckedNodeId id = 1; id <= r.callee->count; ++id)
            if (r.callee->nodes[id - 1].kind == NL_CHECKED_BINDING &&
                r.callee->nodes[id - 1].symbol == use.symbol) {
                assert(!binding);
                binding = id;
            }
        assert(binding);
        lineage = r.callee->nodes[binding - 1].initializer;
    }
    assert(lineage == r.loop && steps == 2);
    for (size_t arm = 0; arm < 2; ++arm) {
        const NLCheckedFragment *g =
            arm ? r.current.breaking : r.current.iteration;
        size_t closed = 0;
        for (size_t i = 0; i < g->count; ++i)
            if (g->nodes[i].kind == NL_CHECKED_LOAN_HEADER) {
                const NLScopeId scope = g->nodes[i].loan.scope;
                assert(scope && scope <= g->context->scope_count &&
                       !g->context->scopes[scope - 1].active);
                ++closed;
            }
        assert(closed == 1);
    }
    printf("genuine root/main/call/callee current0 owner0 Copy0 "
           "old->tail->call free-unknown-u8 local-ended closed-loans2 "
           "root%zu child%zu index%zu\n",
           (size_t)r.current.root, (size_t)r.current.child, index);
    return r;
}
static void rejected(Relation *r, const char *name)
{
    NLLinearBreakView output, before;
    memset(&output, 0x5a, sizeof(output));
    memcpy(&before, &output, sizeof(before));
    const NLCheckStatus owner = nl_linear_break_owner_validate(r->callee),
                        current = nl_linear_break_validate(r->callee, r->loop,
                                                           &output),
                        copy = nl_checked_copy_body_validate(r->main_body,
                                                             r->call, NULL);
    printf("negative %s owner=%d current=%d Copy=%d\n", name, owner, current,
           copy);
    fflush(stdout);
    assert(owner != NL_CHECK_OK && current != NL_CHECK_OK &&
           copy != NL_CHECK_OK && !memcmp(&output, &before, sizeof(output)));
}
static void attacks(PInput *p, const char *path, size_t index)
{
    Relation r = relation(p, index);
    PInput other = {0};
    p_load(&other, path);
    assert(p_check(&other) == NL_CHECK_OK);
    Relation foreign = relation(&other, index);
    assert(foreign.call == r.call && foreign.loop == r.loop);
#define MUTATE(type, label, member, bad)                                       \
    do {                                                                       \
        type saved = (member);                                                 \
        (member) = (bad);                                                      \
        rejected(&r, label);                                                   \
        (member) = saved;                                                      \
        relation(p, index);                                                    \
    } while (0)
    /* The macro is expanded with actual typed objects, never arbitrary ptrs. */
    MUTATE(NLCheckedNodeId, "actual-call", r.main_body->body_calls[0],
           r.call + 1);
    MUTATE(NLCheckedFragment *, "foreign-body-same-ID", r.main_body->bodies[0],
           foreign.callee);
    MUTATE(const NLSource *, "callee-source", r.callee->source,
           foreign.callee->source);
    MUTATE(NLFunctionBody *, "callee-plan", r.callee->body_owner,
           foreign.callee->body_owner);
    NLFunctionBody *plan = r.callee->body_owner;
    MUTATE(NLSource *, "plan-source", plan->source,
           foreign.callee->body_owner->source);
    const size_t function = r.main_body->nodes[r.call - 1].function;
    MUTATE(NLTypeId, "current-signature-result",
           p->context->functions[function - 1].result,
           nl_semantic_unit_type(p->context));
    NLCheckedFragment *breaking = (NLCheckedFragment *)r.current.breaking;
    NLCheckedNodeView *old = &breaking->nodes[r.current.break_value - 1];
    MUTATE(NLValueId, "old-Copy-origin", old->results[0].value, 0);
    NLControlExitView *edge =
        (NLControlExitView *)nl_control_exit_view(breaking->exits, 0);
    MUTATE(const NLControlTarget *, "Break-target", edge->target, NULL);
    NLSemanticContext *post = (NLSemanticContext *)r.current.post_world;
    MUTATE(NLValueFactId, "postcurrent",
           post->places[r.current.child - 1].current_fact,
           post->places[r.current.child - 1].current_fact + 1);
    NLCheckedFragment *iteration = (NLCheckedFragment *)r.current.iteration;
    NLScopeId scope = 0;
    for (size_t i = 0; i < iteration->count; ++i)
        if (iteration->nodes[i].kind == NL_CHECKED_LOAN_HEADER)
            scope = iteration->nodes[i].loan.scope;
    assert(scope);
    NLSemanticContext *world = (NLSemanticContext *)iteration->context;
    MUTATE(bool, "closed-loan-reopened", world->scopes[scope - 1].active, true);
    NLCheckedNodeView *tail =
        &r.callee->nodes[r.callee->nodes[r.callee->root - 1].tail - 1];
    MUTATE(NLValueId, "normal-tail", tail->results[0].value, 0);
    MUTATE(NLValueId, "call-result",
           r.main_body->nodes[r.call - 1].results[0].value, 0);
#undef MUTATE
    NLCopyBodyView fake = r.copy;
    fake.result.value = 0;
    assert(nl_checked_copy_body_validate(r.main_body, r.call, &fake) !=
           NL_CHECK_OK);
    assert(nl_checked_copy_body_view(r.main_body, r.call, &fake));
    relation(p, index);
    p_dispose(&other);
    puts("negative12 + candidate-view1 restore/retry PASS");
}

typedef struct {
    NLSemanticContext context;
    void *arrays[8];
    size_t owners[NL_SEMANTIC_MAX_ENTRIES];
} Snapshot;
static void arrays(const NLSemanticContext *c, void **p, size_t *n)
{
    p[0] = c->types;
    n[0] = c->type_count * sizeof(*c->types);
    p[1] = c->bindings;
    n[1] = c->binding_count * sizeof(*c->bindings);
    p[2] = c->values;
    n[2] = c->value_count * sizeof(*c->values);
    p[3] = c->places;
    n[3] = c->place_count * sizeof(*c->places);
    p[4] = c->scopes;
    n[4] = c->scope_count * sizeof(*c->scopes);
    p[5] = c->functions;
    n[5] = c->function_count * sizeof(*c->functions);
    p[6] = c->domains;
    n[6] = c->domain_count * sizeof(*c->domains);
    p[7] = c->regions;
    n[7] = c->region_count * sizeof(*c->regions);
}
static Snapshot snapshot(const NLSemanticContext *c)
{
    Snapshot s = {0};
    memcpy(&s.context, c, sizeof(*c));
    void *p[8];
    size_t n[8];
    arrays(c, p, n);
    for (size_t i = 0; i < 8; ++i)
        if (n[i]) {
            s.arrays[i] = malloc(n[i]);
            assert(s.arrays[i]);
            memcpy(s.arrays[i], p[i], n[i]);
        }
    assert(c->function_count <= NL_SEMANTIC_MAX_ENTRIES);
    for (size_t i = 0; i < c->function_count; ++i)
        s.owners[i] = c->functions[i].body ? c->functions[i].body->owners : 0;
    return s;
}
static void unchanged(NLSemanticContext *c, Snapshot *s)
{
    assert(!memcmp(&s->context, c, sizeof(*c)));
    void *p[8];
    size_t n[8];
    arrays(c, p, n);
    for (size_t i = 0; i < 8; ++i) {
        assert(!n[i] || !memcmp(s->arrays[i], p[i], n[i]));
        free(s->arrays[i]);
    }
    for (size_t i = 0; i < c->function_count; ++i)
        assert((c->functions[i].body ? c->functions[i].body->owners : 0) ==
               s->owners[i]);
}
static void oom(const char *path, bool validators_only)
{
    NLCheckStatus status;
    if (!validators_only) {
        PInput census = {0};
        p_load(&census, path);
        events = ordinal = retained = seal_count = 0;
        armed = true;
        status = p_check(&census);
        armed = false;
        const size_t total = events;
        printf(
            "CHECK census=%zu retention-end=%zu inner-seal=%zu outer-seal=%zu "
            "status=%d\n",
            total, retained, seals[0], seals[1], status);
        fflush(stdout);
        assert(status == NL_CHECK_OK && total > 8 && retained &&
               seal_count == 2);
        relation(&census, 0);
        p_dispose(&census);
        const size_t selected[] = {1,        2,         retained,  seals[0],
                                   seals[1], total / 2, total - 1, total};
        for (size_t i = 0; i < 8; ++i) {
            bool duplicate = false;
            for (size_t j = 0; j < i; ++j)
                duplicate |= selected[i] == selected[j];
            if (duplicate)
                continue;
            PInput p = {0};
            p_load(&p, path);
            Snapshot before = snapshot(p.context);
            events = 0;
            ordinal = selected[i];
            fired = false;
            armed = true;
            status = p_check(&p);
            armed = false;
            ordinal = 0;
            printf(
                "CHECK OOM target=%zu actual=%zu fired=%d status=%d entry=%p\n",
                selected[i], events, fired, status, (void *)p.entry);
            fflush(stdout);
            assert(fired && status == NL_CHECK_OUT_OF_MEMORY && !p.entry);
            unchanged(p.context, &before);
            assert(p_check(&p) == NL_CHECK_OK);
            relation(&p, 0);
            p_dispose(&p);
        }
    }
    PInput p = {0};
    p_load(&p, path);
    assert(p_check(&p) == NL_CHECK_OK);
    Relation r = relation(&p, 0);
    for (size_t kind = 0; kind < 2; ++kind) {
        NLLinearBreakView out;
        events = ordinal = 0;
        armed = true;
        status =
            kind ? nl_checked_copy_body_validate(r.main_body, r.call, &r.copy)
                 : nl_linear_break_validate(r.callee, r.loop, &out);
        armed = false;
        const size_t count = events;
        printf("%s validator census=%zu status=%d\n", kind ? "Copy" : "current",
               count, status);
        assert(status == NL_CHECK_OK);
        if (!count) {
            puts("allocation-free validator: selected OOM0, not injected");
            continue;
        }
        const size_t points[] = {1, 2, count / 2, count};
        for (size_t i = 0; i < 4; ++i) {
            if (!points[i])
                continue;
            bool duplicate = false;
            for (size_t j = 0; j < i; ++j)
                duplicate |= points[j] == points[i];
            if (duplicate)
                continue;
            Snapshot before = snapshot(p.context);
            NLLinearBreakView old;
            memset(&out, 0x5a, sizeof(out));
            memcpy(&old, &out, sizeof(old));
            NLCopyBodyView copy_before = r.copy;
            events = 0;
            ordinal = points[i];
            fired = false;
            armed = true;
            status = kind ? nl_checked_copy_body_validate(r.main_body, r.call,
                                                          &r.copy)
                          : nl_linear_break_validate(r.callee, r.loop, &out);
            armed = false;
            ordinal = 0;
            printf("%s OOM target=%zu fired=%d status=%d\n",
                   kind ? "Copy" : "current", points[i], fired, status);
            fflush(stdout);
            assert(fired && status == NL_CHECK_OUT_OF_MEMORY);
            assert(kind ? !memcmp(&copy_before, &r.copy, sizeof(r.copy))
                        : !memcmp(&old, &out, sizeof(out)));
            unchanged(p.context, &before);
            relation(&p, 0);
        }
    }
    p_dispose(&p);
    puts("CHECK/current/Copy OOM rollback/retry PASS");
}
static void semantic_negative(const char *path, const char *code)
{
    PInput p = {0};
    NLParseDiagnostic parse = {0};
    NLFunctionUnitDiagnostic d = {0};
    assert(nl_source_load(path, &p.source) == NL_SOURCE_OK);
    assert(nl_parser_create(p.source, &p.parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p.parser, &p.tree, &parse) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&p.context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {p.tree};
    Snapshot before = snapshot(p.context);
    NLCheckStatus s =
        nl_semantic_register_function_unit(p.context, inputs, 1, &d);
    printf("meaning negative status=%d code=%s\n", s,
           d.diagnostic.diagnostic.code);
    assert(s == NL_CHECK_SEMANTIC_ERROR && d.diagnostic.diagnostic.code &&
           !strcmp(code, d.diagnostic.diagnostic.code) && !p.entry);
    unchanged(p.context, &before);
    p_dispose(&p);
}
static void precision_negative(const char *path)
{
    PInput p = {0};
    p_load(&p, path);
    Snapshot before = snapshot(p.context);
    const NLCheckStatus status = p_check(&p);
    printf("lawful out-of-profile precision status=%d entry=%p\n", status,
           (void *)p.entry);
    assert(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT && !p.entry);
    unchanged(p.context, &before);
    p_dispose(&p);
}
int main(int argc, char **argv)
{
    setbuf(stdout, NULL);
    if (argc == 4 && !strcmp(argv[1], "positive")) {
        PInput p = {0};
        p_load(&p, argv[2]);
        NLCheckStatus s = p_check(&p);
        printf("actual CHECK status=%d\n", s);
        assert(s == NL_CHECK_OK);
        attacks(&p, argv[2], (size_t)strtoul(argv[3], NULL, 10));
        p_dispose(&p);
    } else if (argc == 3 &&
               (!strcmp(argv[1], "oom") || !strcmp(argv[1], "validators"))) {
        oom(argv[2], !strcmp(argv[1], "validators"));
    } else if (argc == 4 && !strcmp(argv[1], "precision")) {
        precision_negative(argv[2]);
        precision_negative(argv[3]);
    } else {
        assert(argc == 4 && !strcmp(argv[1], "negative"));
        semantic_negative(argv[2], "P3-TYPE-MISMATCH");
        semantic_negative(argv[3], "P8-EXIT-DEPENDENCY");
    }
    return 0;
}
