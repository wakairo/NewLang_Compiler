#include "../p1280/support.h"
#include "copy_body_internal.h"
#include <errno.h>

static bool armed, fired;
static size_t events, ordinal, seals[3], seal_count, init_at[2], init_count,
    retain_at[2], retain_count;
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
    if (armed && !ordinal) {
        if (retain_count < 2)
            retain_at[retain_count++] = events;
    }
    return __real_nl_copy_body_current(f, id);
}
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *p,
                                              NLCheckedFragment *c,
                                              NLCheckedNodeId id)
{
    if (armed && !ordinal && seal_count < 3)
        seals[seal_count++] = events + 1;
    return __real_nl_h_custody_seal_result(p, c, id);
}

NLCheckStatus __real_nl_copy_body_event(NLCheckedFragment *, NLCheckedNodeId,
                                        const NLSemanticContext *, bool);
NLCheckStatus __wrap_nl_copy_body_event(NLCheckedFragment *f,
                                        NLCheckedNodeId id,
                                        const NLSemanticContext *c,
                                        bool placement)
{
    if (armed && !ordinal && placement && init_count < 2)
        init_at[init_count++] = events + 1;
    return __real_nl_copy_body_event(f, id, c, placement);
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

typedef struct {
    NLCheckedFragment *caller, *body;
    NLCheckedNodeId call, loop;
    NLCopyBodyView copy;
    NLLinearBreakView current;
} InputCall;

static InputCall valid(PInput *p, size_t input)
{
    InputCall r = {0};
    assert(nl_h_custody_validate(p->entry));
    r.caller =
        (NLCheckedFragment *)nl_checked_call_body(p->entry, p->entry->root);
    assert(r.caller && r.caller->body_count == 1);
    r.call = r.caller->body_calls[0];
    r.body = (NLCheckedFragment *)nl_checked_call_body(r.caller, r.call);
    assert(r.body && r.body == r.caller->bodies[0]);
    for (size_t i = 1; i <= r.body->count; ++i)
        if (r.body->nodes[i - 1].kind == NL_CHECKED_LOOP) {
            assert(!r.loop);
            r.loop = i;
        }
    assert(r.loop && nl_linear_break_owner_validate(r.body) == NL_CHECK_OK);
    assert(nl_linear_break_validate(r.body, r.loop, &r.current) == NL_CHECK_OK);
    assert(nl_checked_copy_body_view(r.caller, r.call, &r.copy));
    NLCheckStatus s = nl_checked_copy_body_validate(r.caller, r.call, &r.copy);
    printf("owner/current0 Copy%d\n", s);
    assert(s == NL_CHECK_OK);
    NLCopyBodyProof *proof = r.body->copy_body;
    assert(proof && !proof->formal_write && proof->readonly_parameter &&
           proof->readonly_parameter == r.current.readonly_parameter &&
           proof->current_loop == r.loop && proof->sealed);
    NLCopyBodyParameters ref_write;
    assert(!nl_checked_copy_body_parameters(r.caller, r.call, &ref_write));
    const NLCheckedNodeView *call = nl_checked_node_view(r.caller, r.call);
    const NLCheckedNodeView *arg =
        nl_checked_node_view(r.caller, call->first_argument);
    assert(call->argument_count == 1 && arg && !arg->next_argument &&
           arg->has_scalar_result && arg->scalar_result.known &&
           arg->scalar_result.value == input);
    const NLSymbolId formal = proof->readonly_parameter;
    const NLSemanticBindingView binding =
        proof->entry->bindings[formal - 1].view;
    assert(binding.type == arg->type &&
           proof->entry->values[binding.value - 1].scalar_value == input);
    assert(r.current.changing_current && r.current.terminal_old &&
           !r.current.paired_current && !r.current.incoming_if &&
           !r.current.incoming_call && !r.current.terminal_call &&
           r.current.index == 0 && r.current.limit == 2 &&
           r.current.next_phase == 1 && r.copy.event_count == 3);
    assert(!p->context->places[r.current.root - 1].live &&
           !p->context->places[binding.place - 1].live &&
           !p->context->values[r.copy.result.value - 1].scalar_known &&
           nl_copy_plain(p->context, r.copy.result.value, arg->type));
    for (size_t arm = 0; arm < 2; ++arm) {
        const NLCheckedFragment *f =
            arm ? r.current.breaking : r.current.iteration;
        size_t loans = 0, inputs = 0;
        for (size_t i = 0; i < f->count; ++i) {
            const NLCheckedNodeView *v = &f->nodes[i];
            if (v->kind == NL_CHECKED_LOAN_HEADER) {
                assert(v->loan.scope &&
                       !f->context->scopes[v->loan.scope - 1].active);
                ++loans;
            }
            if (v->kind == NL_CHECKED_IDENTIFIER && v->symbol == formal) {
                assert(!arm && v->value_use == NL_VALUE_COPIED);
                ++inputs;
            }
        }
        assert(loans == 1 && inputs == (arm ? 0u : 1u));
    }
    printf("call%zu formal%zu argument%zu loop%zu root%zu child%zu old%zu "
           "unknown Copy; input%zu entry/header/closed-loans PASS\n",
           r.call, formal, call->first_argument, r.loop, r.current.root,
           r.current.child, r.copy.result.value, input);
    return r;
}
static void rejected(const InputCall *r)
{
    NLLinearBreakView before = r->current, out = before;
    assert(nl_linear_break_owner_validate(r->body) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    assert(nl_linear_break_validate(r->body, r->loop, &out) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    assert(!memcmp(&before, &out, sizeof(out)));
    assert(nl_checked_copy_body_validate(r->caller, r->call, &r->copy) ==
           NL_CHECK_ANALYSIS_PRECISION_LIMIT);
}
static void attacks(PInput *p, const char *path, size_t input)
{
    InputCall r = valid(p, input);
    NLCopyBodyProof *proof = r.body->copy_body;
    NLCheckedNodeView *call = &r.caller->nodes[r.call - 1];
    NLCheckedNodeView *argument =
        &r.caller->nodes[proof->readonly_argument - 1];
    NLCheckedNodeView saved = *argument;
    argument->results[0].value = r.copy.result.value;
    rejected(&r);
    *argument = saved;
    valid(p, input);
    NLCheckedNodeId slot = call->first_argument;
    /* Preserve all value bits/IDs while claiming a different source origin. */
    argument->kind = NL_CHECKED_IDENTIFIER;
    argument->symbol = proof->readonly_parameter;
    argument->value_use = NL_VALUE_COPIED;
    rejected(&r);
    *argument = saved;
    valid(p, input);
    call->first_argument = 0;
    rejected(&r);
    call->first_argument = slot;
    valid(p, input);
    NLSymbolId formal = proof->readonly_parameter;
    proof->readonly_parameter = r.current.base;
    rejected(&r);
    proof->readonly_parameter = formal;
    valid(p, input);
    NLValueId value = proof->entry->bindings[formal - 1].view.value;
    NLDependencyKnowledge dep = proof->entry->values[value - 1].dependencies;
    proof->entry->values[value - 1].dependencies = NL_DEPENDENCIES_UNKNOWN;
    rejected(&r);
    proof->entry->values[value - 1].dependencies = dep;
    valid(p, input);
    NLCheckedNodeView *tail = &r.body->nodes[r.body->root - 1];
    NLCheckedNodeId tail_id = tail->tail;
    tail->tail = r.loop;
    rejected(&r);
    tail->tail = tail_id;
    valid(p, input);
    NLFunctionEntry *fn = &p->context->functions[call->function - 1];
    NLTypeId *parameters = fn->parameters;
    void *short_foreign = malloc(1);
    assert(short_foreign);
    fn->parameters = short_foreign;
    rejected(&r);
    fn->parameters = parameters;
    free(short_foreign);
    valid(p, input);
    NLTypeId *entry_parameters =
        proof->entry->functions[call->function - 1].parameters;
    short_foreign = malloc(1);
    assert(short_foreign);
    proof->entry->functions[call->function - 1].parameters = short_foreign;
    rejected(&r);
    proof->entry->functions[call->function - 1].parameters = entry_parameters;
    free(short_foreign);
    valid(p, input);
    NLFunctionEntry *functions = p->context->functions;
    p->context->functions = NULL;
    rejected(&r);
    p->context->functions = functions;
    valid(p, input);
    size_t count = p->context->function_count;
    p->context->function_count = 0;
    rejected(&r);
    p->context->function_count = count;
    valid(p, input);
    PInput foreign = {0};
    p_load(&foreign, path);
    assert(p_check(&foreign) == NL_CHECK_OK);
    NLCheckedFragment *other = (NLCheckedFragment *)nl_checked_call_body(
        foreign.entry, foreign.entry->root);
    NLCheckedFragment *body = r.caller->bodies[0];
    r.caller->bodies[0] = other->bodies[0];
    rejected(&r);
    r.caller->bodies[0] = body;
    valid(p, input);
    p_dispose(&foreign);
    puts("actual/formal/slot/foreign/stale/dependency/signature拒否 and "
         "restore PASS");
}
static void oom(const char *path, size_t input)
{
    PInput p = {0};
    p_load(&p, path);
    events = ordinal = seal_count = init_count = retain_count = 0;
    armed = true;
    NLCheckStatus status = p_check(&p);
    armed = false;
    const size_t total = events;
    printf("CHECK census%zu init%zu retention%zu seals[%zu,%zu] status%d\n",
           total, init_at[0], retain_at[0], seals[0], seals[1], status);
    assert(status == NL_CHECK_OK && total > 8 && retain_count == 1 &&
           seal_count == 2);
    valid(&p, input);
    p_dispose(&p);
    const size_t points[] = {1,        2,        init_at[0], retain_at[0],
                             seals[0], seals[1], total / 2,  total};
    for (size_t i = 0; i < 8; ++i) {
        PInput x = {0};
        p_load(&x, path);
        Snapshot before = snapshot(x.context);
        events = 0;
        ordinal = points[i];
        fired = false;
        armed = true;
        status = p_check(&x);
        armed = false;
        ordinal = 0;
        printf("OOM ordinal%zu actual%zu fired%d status%d\n", points[i], events,
               fired, status);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY && !x.entry);
        unchanged(x.context, &before);
        assert(p_check(&x) == NL_CHECK_OK);
        valid(&x, input);
        p_dispose(&x);
    }
    p_load(&p, path);
    assert(p_check(&p) == NL_CHECK_OK);
    InputCall r = valid(&p, input);
    events = ordinal = 0;
    armed = true;
    status = nl_checked_copy_body_validate(r.caller, r.call, &r.copy);
    armed = false;
    const size_t allocations = events;
    assert(status == NL_CHECK_OK && allocations <= 8);
    printf("Copy allocations%zu\n", allocations);
    for (size_t i = 1; i <= allocations; ++i) {
        Snapshot before = snapshot(p.context);
        events = 0;
        ordinal = i;
        fired = false;
        armed = true;
        status = nl_checked_copy_body_validate(r.caller, r.call, &r.copy);
        armed = false;
        ordinal = 0;
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY);
        unchanged(p.context, &before);
        valid(&p, input);
    }
    p_dispose(&p);
}
int main(int argc, char **argv)
{
    assert(argc == 4);
    setbuf(stdout, NULL);
    const size_t input = strtoul(argv[3], NULL, 10);
    if (!strcmp(argv[1], "oom"))
        oom(argv[2], input);
    else {
        PInput p = {0};
        p_load(&p, argv[2]);
        NLCheckStatus status = p_check(&p);
        printf("actual CHECK%d entry%p\n", status, (void *)p.entry);
        assert(status == NL_CHECK_OK);
        attacks(&p, argv[2], input);
        p_dispose(&p);
    }
    return 0;
}
