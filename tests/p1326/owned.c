#include "../p1280/support.h"
#include <errno.h>

static bool armed, fired;
static size_t events, ordinal;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool fault(void)
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
    return fault() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return fault() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return fault() ? NULL : __real_realloc(p, n);
}

typedef struct {
    NLCheckedFragment *body;
    NLCheckedNodeId loop;
    NLLinearBreakView view;
} Borrow;

static bool admission(PInput *p)
{
    V0Program program = {.unit = nl_semantic_unit_type(p->context),
                         .u8 = nl_semantic_core_type(p->context, NL_TYPE_U8)};
    return v0_validate_call(p->entry, nl_checked_root(p->entry), &program);
}
static Borrow healthy(PInput *p, bool supported)
{
    Borrow b = {0};
    b.body = (NLCheckedFragment *)nl_checked_call_body(
        p->entry, nl_checked_root(p->entry));
    assert(b.body);
    for (NLCheckedNodeId id = 1; id <= b.body->count; ++id)
        if (b.body->nodes[id - 1].kind == NL_CHECKED_LOOP) {
            assert(!b.loop);
            b.loop = id;
        }
    assert(b.loop && nl_linear_break_owner_validate(b.body) == NL_CHECK_OK);
    assert(nl_linear_break_validate(b.body, b.loop, &b.view) == NL_CHECK_OK);
    const NLLinearBreakView *v = &b.view;
    assert(v->terminal_old && v->changing_current && !v->scalar_loop &&
           v->incoming_call && v->terminal_call && v->iteration != v->breaking);
    assert(v->paired_current == !supported && admission(p) == supported);
    const NLCheckedNodeView *incoming =
        nl_checked_node_view(v->iteration, v->incoming_call);
    const NLCheckedNodeView *terminal =
        nl_checked_node_view(v->breaking, v->terminal_call);
    const NLCheckedNodeView *argument =
        nl_checked_node_view(v->breaking, terminal->first_argument);
    assert(incoming && terminal && incoming->function == terminal->function &&
           argument && argument->kind == NL_CHECKED_IDENTIFIER &&
           argument->symbol == v->phase_symbol &&
           argument->value_use == NL_VALUE_COPIED &&
           v->terminal_helper_body ==
               nl_checked_call_body(v->breaking, v->terminal_call));
    const NLSemanticPlaceView root = v->post_world->places[v->root - 1];
    assert(root.incarnation == v->incarnation &&
           root.fixed_fields[v->index] == v->child &&
           v->post_world->places[v->child - 1].incarnation ==
               v->child_incarnation);
    if (supported) {
        const NLSemanticValueView current =
            v->post_world
                ->values[v->post_world->places[v->child - 1].current_value - 1];
        assert(!current.scalar_known);
        const NLPlaceId peer = root.fixed_fields[1 - v->index];
        const NLSemanticValueView other =
            v->post_world
                ->values[v->post_world->places[peer - 1].current_value - 1];
        assert(other.scalar_known && other.scalar_value == 173);
    }
    return b;
}
static void rejected(PInput *p, Borrow b, const char *name)
{
    NLLinearBreakView ignored = {0};
    const NLCheckStatus owner = nl_linear_break_owner_validate(b.body);
    const NLCheckStatus current =
        nl_linear_break_validate(b.body, b.loop, &ignored);
    const bool admitted = admission(p);
    printf("negative %s owner=%d current=%d CLI=%d\n", name, owner, current,
           admitted);
    assert(current != NL_CHECK_OK && !admitted);
}
static void attacks(PInput *p, const char *path)
{
    Borrow b = healthy(p, true);
    NLCheckedFragment *f = (NLCheckedFragment *)b.view.breaking;
    NLCheckedNodeView *call = &f->nodes[b.view.terminal_call - 1];
    NLCheckedNodeView *arg = &f->nodes[call->first_argument - 1];
    NLCheckedNodeView saved = *arg;
    ++arg->symbol;
    rejected(p, b, "wrong-actual-phase");
    *arg = saved;
    healthy(p, true);
    const size_t fn = call->function;
    call->function = fn + 1;
    rejected(p, b, "different-live-function");
    call->function = fn;
    healthy(p, true);
    NLCheckedNodeView *replace = NULL;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_REPLACE) {
            assert(!replace);
            replace = &f->nodes[i];
        }
    assert(replace && replace->field.present);
    saved = *replace;
    replace->field.old_value = replace->field.new_value;
    rejected(p, b, "old-result-origin");
    *replace = saved;
    healthy(p, true);
    replace->field.new_value = replace->field.old_value;
    rejected(p, b, "call-result-to-new-origin");
    *replace = saved;
    healthy(p, true);
    NLSemanticContext *post = (NLSemanticContext *)b.view.post_world;
    ++post->places[b.view.child - 1].incarnation;
    rejected(p, b, "stale-postcurrent");
    --post->places[b.view.child - 1].incarnation;
    healthy(p, true);
    const NLCheckedNodeView *loan =
        nl_checked_node_view(f, b.view.terminal_loan);
    assert(loan && loan->loan.scope);
    NLSemanticScopeView *scope = &f->context->scopes[loan->loan.scope - 1];
    assert(!scope->active);
    scope->active = true;
    rejected(p, b, "terminal-loan-reopened");
    scope->active = false;
    healthy(p, true);
    PInput foreign = {0};
    p_load(&foreign, path);
    assert(p_check(&foreign) == NL_CHECK_OK);
    Borrow other = healthy(&foreign, true);
    const NLSource *source = f->source;
    f->source = other.view.breaking->source;
    assert(f->source != source);
    rejected(p, b, "foreign-source-instance");
    f->source = source;
    healthy(p, true);
    size_t slot = 0;
    while (slot < f->body_count && f->body_calls[slot] != b.view.terminal_call)
        ++slot;
    assert(slot < f->body_count);
    NLCheckedFragment *saved_body = f->bodies[slot];
    f->bodies[slot] = (NLCheckedFragment *)other.view.terminal_helper_body;
    rejected(p, b, "same-id-foreign-owner-fragment");
    f->bodies[slot] = saved_body;
    healthy(p, true);
    healthy(&foreign, true);
    p_dispose(&foreign);
}

typedef struct {
    NLSemanticContext context;
    void *data[8];
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
            s.data[i] = malloc(n[i]);
            assert(s.data[i]);
            memcpy(s.data[i], p[i], n[i]);
        }
    assert(c->function_count <= NL_SEMANTIC_MAX_ENTRIES);
    for (size_t i = 0; i < c->function_count; ++i)
        s.owners[i] = c->functions[i].body ? c->functions[i].body->owners : 0;
    return s;
}
static void unchanged(NLSemanticContext *c, Snapshot *s)
{
    assert(!memcmp(c, &s->context, sizeof(*c)));
    void *p[8];
    size_t n[8];
    arrays(c, p, n);
    for (size_t i = 0; i < 8; ++i) {
        assert(!n[i] || !memcmp(p[i], s->data[i], n[i]));
        free(s->data[i]);
    }
    for (size_t i = 0; i < c->function_count; ++i)
        assert((c->functions[i].body ? c->functions[i].body->owners : 0) ==
               s->owners[i]);
}
static void oom(const char *path)
{
    PInput p = {0};
    p_load(&p, path);
    events = ordinal = 0;
    armed = true;
    NLCheckStatus status = p_check(&p);
    armed = false;
    const size_t count = events;
    assert(status == NL_CHECK_OK && count > 4);
    healthy(&p, true);
    p_dispose(&p);
    printf("actual CHECK census=%zu\n", count);
    const size_t selected[] = {1, 2, count / 2, count};
    for (size_t i = 0; i < 4; ++i) {
        PInput q = {0};
        p_load(&q, path);
        Snapshot saved = snapshot(q.context);
        events = 0;
        ordinal = selected[i];
        fired = false;
        armed = true;
        status = p_check(&q);
        armed = false;
        ordinal = 0;
        printf("CHECK OOM %zu events=%zu fired=%d status=%d out=%p\n",
               selected[i], events, fired, status, (void *)q.entry);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY && !q.entry);
        unchanged(q.context, &saved);
        assert(p_check(&q) == NL_CHECK_OK);
        healthy(&q, true);
        p_dispose(&q);
    }
}
int main(int argc, char **argv)
{
    assert(argc == 3);
    PInput p = {0};
    p_load(&p, argv[1]);
    assert(p_check(&p) == NL_CHECK_OK);
    Borrow b = healthy(&p, true);
    printf("healthy root%zu child%zu phase%zu call%zu "
           "owner-fragment-distinct=%d\n",
           (size_t)b.view.root, (size_t)b.view.child,
           (size_t)b.view.phase_symbol, (size_t)b.view.terminal_call,
           b.view.iteration != b.view.breaking);
    attacks(&p, argv[1]);
    p_dispose(&p);
    oom(argv[1]);
    PInput paired = {0};
    p_load(&paired, argv[2]);
    assert(p_check(&paired) == NL_CHECK_OK);
    healthy(&paired, false);
    p_dispose(&paired);
    puts("P1326 owned/current/call/phase/old/new negatives8 OOM4 "
         "rollback/retry paired-refusal PASS");
    return 0;
}
