#include "../p1280/support.h"
#include "copy_body_internal.h"
#include <errno.h>

static bool armed, fired;
static size_t input_at[4], input_end[4], input_count;
static size_t events, ordinal, seals[5], seal_count, init_at[4], init_count,
    retain_at[2], retain_count, copy_seals[5], copy_seal_count;
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
    if (armed && !ordinal && seal_count < 5)
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
    if (armed && !ordinal && placement && init_count < 4)
        init_at[init_count++] = events + 1;
    return __real_nl_copy_body_event(f, id, c, placement);
}
NLCheckStatus __real_nl_copy_body_seal(NLCheckedFragment *,
                                       const NLCheckedFragment *,
                                       NLCheckedNodeId, size_t,
                                       const NLSemanticContext *);
NLCheckStatus __wrap_nl_copy_body_seal(NLCheckedFragment *f,
                                       const NLCheckedFragment *c,
                                       NLCheckedNodeId id, size_t fn,
                                       const NLSemanticContext *w)
{
    if (armed && !ordinal && copy_seal_count < 5)
        copy_seals[copy_seal_count++] = events + 1;
    return __real_nl_copy_body_seal(f, c, id, fn, w);
}
NLCheckStatus __real_nl_copy_body_readonly(NLCheckedFragment *,
                                           const NLCheckedFragment *,
                                           NLCheckedNodeId, NLSymbolId,
                                           const NLSemanticContext *);
NLCheckStatus __wrap_nl_copy_body_readonly(NLCheckedFragment *f,
                                           const NLCheckedFragment *caller,
                                           NLCheckedNodeId id,
                                           NLSymbolId symbol,
                                           const NLSemanticContext *c)
{
    size_t slot = 4;
    if (armed && !ordinal && input_count < 4) {
        slot = input_count++;
        input_at[slot] = events + 1;
    }
    NLCheckStatus status =
        __real_nl_copy_body_readonly(f, caller, id, symbol, c);
    if (slot < 4)
        input_end[slot] = events;
    return status;
}
typedef struct {
    NLCheckedFragment *main_body, *wrapper, *inner;
    NLCheckedNodeId wrapper_call, inner_call, loop;
    NLLinearBreakView current;
    NLCopyBodyView outer_copy, inner_copy;
    NLPlaceId original;
} P1359Frame;
static P1359Frame frame(PInput *p, size_t index, size_t slot)
{
    P1359Frame f = {0};
    assert(nl_h_custody_validate(p->entry));
    f.main_body =
        (NLCheckedFragment *)nl_checked_call_body(p->entry, p->entry->root);
    assert(f.main_body && f.main_body->body_count == 2 && slot < 2);
    f.wrapper_call = f.main_body->body_calls[slot];
    f.wrapper =
        (NLCheckedFragment *)nl_checked_call_body(f.main_body, f.wrapper_call);
    assert(f.wrapper && f.wrapper->body_count == 1 &&
           !f.wrapper->linear_breaks);
    f.inner_call = f.wrapper->body_calls[0];
    f.inner =
        (NLCheckedFragment *)nl_checked_call_body(f.wrapper, f.inner_call);
    assert(f.inner && f.inner != f.wrapper && f.inner != f.main_body);
    for (size_t id = 1; id <= f.inner->count; ++id)
        if (f.inner->nodes[id - 1].kind == NL_CHECKED_LOOP) {
            assert(!f.loop);
            f.loop = id;
        }
    assert(f.loop && f.wrapper->copy_body && f.inner->copy_body);
    assert(f.wrapper->copy_body->current_call == f.inner_call &&
           !f.wrapper->copy_body->current_loop &&
           f.wrapper->copy_body->readonly_parameter);
    assert(nl_h_custody_result_valid(p->entry, p->entry->root) &&
           nl_h_custody_result_valid(f.main_body, f.wrapper_call) &&
           nl_h_custody_result_valid(f.wrapper, f.inner_call));
    assert(nl_linear_break_owner_validate(f.inner) == NL_CHECK_OK);
    assert(nl_linear_break_validate(f.inner, f.loop, &f.current) ==
           NL_CHECK_OK);
    assert(nl_checked_copy_body_view(f.wrapper, f.inner_call, &f.inner_copy) &&
           nl_checked_copy_body_validate(f.wrapper, f.inner_call,
                                         &f.inner_copy) == NL_CHECK_OK);
    assert(
        nl_checked_copy_body_view(f.main_body, f.wrapper_call, &f.outer_copy) &&
        nl_checked_copy_body_validate(f.main_body, f.wrapper_call,
                                      &f.outer_copy) == NL_CHECK_OK);
    assert(f.current.changing_current && f.current.terminal_old &&
           !f.current.paired_current && f.current.index == index &&
           f.current.readonly_parameter);
    const NLCopyBodyProof *incoming = f.wrapper->copy_body;
    const NLCheckedNodeView *outer_call =
        nl_checked_node_view(f.main_body, f.wrapper_call);
    const NLCheckedNodeView *outer_arg =
        nl_checked_node_view(f.main_body, outer_call->first_argument);
    assert(outer_call->argument_count == 1 && outer_arg &&
           !outer_arg->next_argument &&
           incoming->readonly_argument == outer_call->first_argument &&
           incoming->readonly_parameter !=
               f.inner->copy_body->readonly_parameter &&
           nl_copy_body_readonly_valid(f.wrapper) &&
           incoming->entry->bindings[incoming->readonly_parameter - 1]
                   .view.value == outer_arg->results[0].value);
    f.original = f.wrapper->copy_body->first->view.field.parent;
    assert(f.original && f.original != f.current.root &&
           !p->context->places[f.original - 1].live &&
           !p->context->places[f.current.root - 1].live);
    const NLCopyBodyProof *q = f.inner->copy_body;
    NLSemanticPlaceView before[3], after[3];
    assert(nl_copy_trio(q->entry, f.original, before) &&
           nl_copy_trio(q->exit, f.original, after) &&
           !memcmp(before, after, sizeof(before)));
    const NLSemanticContext *worlds[] = {
        f.current.entry_world, f.current.post_world,
        f.current.iteration->context, f.current.breaking->context};
    for (size_t i = 0; i < 4; ++i) {
        NLSemanticPlaceView a[3];
        assert(nl_copy_trio(worlds[i], f.original, a) &&
               !memcmp(a, before, sizeof(a)));
    }
    const NLCheckedNodeView *call =
        nl_checked_node_view(f.wrapper, f.inner_call);
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f.wrapper, call->first_argument);
    assert(q->readonly_parameter <= q->entry->binding_count &&
           q->entry->bindings[q->readonly_parameter - 1].view.value ==
               arg->results[0].value);
    size_t changes = 0, closed = 0;
    for (size_t id = 1; id <= f.wrapper->count; ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f.wrapper, id);
        if (n->kind == NL_CHECKED_REPLACE) {
            const NLCheckedNodeView *a =
                nl_checked_node_view(f.wrapper, n->first_argument);
            assert(id > f.inner_call && n->field.parent == f.original &&
                   n->field.index == index &&
                   copy_origin(f.wrapper, a->next_argument) == f.inner_call);
            ++changes;
        }
        if (n->kind == NL_CHECKED_LOAN_HEADER) {
            assert(n->loan.scope &&
                   !p->context->scopes[n->loan.scope - 1].active);
            ++closed;
        }
    }
    assert(changes == 1 && closed == 1);
    const NLCheckedNodeView *root =
        nl_checked_node_view(f.wrapper, f.wrapper->root);
    assert(root->tail && copy_origin(f.wrapper, root->tail) == f.inner_call &&
           root->results[0].value == f.outer_copy.result.value &&
           f.main_body->nodes[f.wrapper_call - 1].results[0].value ==
               f.outer_copy.result.value);
    assert(nl_copy_plain(p->context, f.outer_copy.result.value,
                         f.outer_copy.result.type) &&
           !p->context->values[f.outer_copy.result.value - 1].scalar_known);
    printf("root->main->wrapper%zu->inner%zu: owner0 current0 Copy0/0; "
           "separate roots%zu/%zu index%zu; closed loan, inner "
           "old->Change->tail->caller Copy%zu\n",
           f.wrapper_call, f.inner_call, f.original, f.current.root, index,
           f.outer_copy.result.value);
    return f;
}
static P1359Frame challenged[2];
static void rejected(PInput *p, size_t index, const char *label)
{
    for (size_t slot = 0; slot < 2; ++slot) {
        P1359Frame *f = &challenged[slot];
        NLLinearBreakView out, before;
        memset(&out, 0xa5, sizeof(out));
        before = out;
        NLCheckStatus a = nl_linear_break_owner_validate(f->inner),
                      b = nl_linear_break_validate(f->inner, f->loop, &out),
                      c = nl_checked_copy_body_validate(f->wrapper,
                                                        f->inner_call, NULL),
                      d = nl_checked_copy_body_validate(f->main_body,
                                                        f->wrapper_call, NULL);
        printf(
            "negative %s slot%zu owner%d current%d innerCopy%d outerCopy%d\n",
            label, slot, a, b, c, d);
        assert(a != NL_CHECK_OK && b != NL_CHECK_OK && c != NL_CHECK_OK &&
               d != NL_CHECK_OK && !memcmp(&out, &before, sizeof(out)));
    }
    (void)p;
    (void)index;
}
static void pair(PInput *p, size_t index)
{
    P1359Frame a = frame(p, index, 0), b = frame(p, index, 1);
    const NLCopyBodyProof *x = a.wrapper->copy_body, *y = b.wrapper->copy_body;
    assert(a.wrapper != b.wrapper && a.inner != b.inner &&
           a.original != b.original && a.current.root != b.current.root);
    assert(x->view.function == y->view.function &&
           x->readonly_parameter != y->readonly_parameter &&
           a.inner->copy_body->readonly_parameter !=
               b.inner->copy_body->readonly_parameter &&
           a.outer_copy.result.value != b.outer_copy.result.value);
    assert(nl_copy_plain(y->entry, a.outer_copy.result.value,
                         a.outer_copy.result.type));
    const NLCopyBodyProof *closed[] = {x, a.inner->copy_body};
    for (size_t k = 0; k < 2; ++k) {
        const NLCopyBodyProof *q = closed[k];
        const CopyEvent *e = q->first;
        const NLPlaceId ids[] = {
            e->view.field.parent, e->post[0].fixed_fields[0],
            e->post[0].fixed_fields[1],
            q->entry->bindings[q->readonly_parameter - 1].view.place};
        for (size_t i = 0; i < 4; ++i) {
            const NLSemanticPlaceView v = y->entry->places[ids[i] - 1];
            assert(!v.live && !v.current_value && !v.current_fact);
            assert(v.incarnation ==
                   (i < 3 ? e->post[i].incarnation
                          : q->entry->places[ids[i] - 1].incarnation));
        }
    }
    const NLCheckedNodeView *second =
        nl_checked_node_view(b.main_body, b.wrapper_call);
    size_t after_first = 0, after_second = 0;
    for (size_t id = 1; id <= a.main_body->count; ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(a.main_body, id);
        if (n->kind != NL_CHECKED_BINDING ||
            n->span.start_byte <= second->span.end_byte)
            continue;
        NLCheckedNodeId origin = copy_origin(a.main_body, n->initializer);
        after_first += origin == a.wrapper_call;
        after_second += origin == b.wrapper_call;
    }
    assert(after_first && after_second);
    puts("distinct wrapper/inner/formal/result pairs; first free Copy retained "
         "at second entry and post-second bindings PASS");
}
static void attacks(PInput *p, const char *source, size_t index)
{
    P1359Frame f = frame(p, index, 0);
    P1359Frame second = frame(p, index, 1);
    pair(p, index);
    challenged[0] = f;
    challenged[1] = second;
    PInput other = {0};
    p_load(&other, source);
    assert(p_check(&other) == NL_CHECK_OK);
    P1359Frame foreign = frame(&other, index, 0);
#define CHANGE(type, label, member, bad)                                       \
    do {                                                                       \
        type saved = (member);                                                 \
        (member) = (bad);                                                      \
        rejected(p, index, label);                                             \
        (member) = saved;                                                      \
        pair(p, index);                                                        \
    } while (0)
    CHANGE(NLCheckedNodeId, "wrapper call order", f.main_body->body_calls[0],
           f.main_body->body_calls[1]);
    CHANGE(NLCheckedFragment *, "same-origin second wrapper in first slot",
           f.main_body->bodies[0], second.wrapper);
    CHANGE(NLValueId, "first result replaced by second result",
           f.main_body->nodes[f.wrapper_call - 1].results[0].value,
           second.outer_copy.result.value);
    CHANGE(NLSymbolId, "second formal replaced by first",
           second.wrapper->copy_body->readonly_parameter,
           f.wrapper->copy_body->readonly_parameter);
    CHANGE(
        size_t, "first Copy gets reference dependency at second entry",
        second.wrapper->copy_body->entry->values[f.outer_copy.result.value - 1]
            .reference_count,
        1);
    CHANGE(bool, "first original live again at second entry",
           second.wrapper->copy_body->entry->places[f.original - 1].live, true);
    CHANGE(NLCheckedNodeId, "main wrapper slot", f.main_body->body_calls[0],
           f.main_body->root);
    CHANGE(NLCheckedFragment *, "foreign wrapper BODY", f.main_body->bodies[0],
           foreign.wrapper);
    CHANGE(NLCheckedNodeId, "wrapper inner slot", f.wrapper->body_calls[0],
           f.wrapper->root);
    CHANGE(NLCheckedFragment *, "foreign inner BODY", f.wrapper->bodies[0],
           foreign.inner);
    CHANGE(const NLSource *, "wrapper source", f.wrapper->source,
           foreign.wrapper->source);
    CHANGE(NLFunctionBody *, "wrapper plan", f.wrapper->body_owner,
           foreign.wrapper->body_owner);
    CHANGE(NLCheckedNodeId, "forwarded call identity",
           f.wrapper->copy_body->current_call, 0);
    CHANGE(NLSymbolId, "cross wrapper formal",
           f.wrapper->copy_body->readonly_parameter,
           f.inner->copy_body->readonly_parameter);
    CHANGE(NLCheckedNodeId, "wrong wrapper actual",
           f.wrapper->copy_body->readonly_argument, 0);
    const NLCheckedNodeView *outer_call =
        nl_checked_node_view(f.main_body, f.wrapper_call);
    CHANGE(NLValueId, "wrong outer actual package",
           f.main_body->nodes[outer_call->first_argument - 1].results[0].value,
           1);
    CHANGE(NLValueId, "wrapper entry formal package",
           f.wrapper->copy_body->entry
               ->bindings[f.wrapper->copy_body->readonly_parameter - 1]
               .view.value,
           0);
    CHANGE(const char *, "same name foreign origin",
           f.wrapper->copy_body->readonly_plan_name,
           foreign.wrapper->copy_body->readonly_plan_name);
    CHANGE(NLValueId, "outer call result",
           f.main_body->nodes[f.wrapper_call - 1].results[0].value, 1);
    CHANGE(NLValueId, "inner old call result",
           f.wrapper->nodes[f.inner_call - 1].results[0].value, 1);
    CHANGE(NLValueFactId, "postcurrent",
           ((NLSemanticContext *)f.current.post_world)
               ->places[f.current.child - 1]
               .current_fact,
           0);
    for (size_t id = 1; id <= f.wrapper->count; ++id)
        if (f.wrapper->nodes[id - 1].kind == NL_CHECKED_REPLACE) {
            CHANGE(NLPlaceId, "wrapper Change root",
                   f.wrapper->nodes[id - 1].field.parent, f.current.root);
            CHANGE(size_t, "wrapper Change field",
                   f.wrapper->nodes[id - 1].field.index, 1 - index);
        }
    void *small = malloc(1);
    assert(small);
    size_t functions[] = {f.main_body->nodes[f.wrapper_call - 1].function,
                          f.wrapper->nodes[f.inner_call - 1].function};
    for (size_t i = 0; i < 2; ++i) {
        CHANGE(NLTypeId *, "foreign short signature",
               p->context->functions[functions[i] - 1].parameters, small);
        CHANGE(NLTypeId, "changed result signature",
               p->context->functions[functions[i] - 1].result,
               nl_semantic_unit_type(p->context));
    }
    free(small);
    CHANGE(NLFunctionEntry *, "NULL current function table",
           p->context->functions, NULL);
    CHANGE(size_t, "truncated current function count",
           p->context->function_count, 1);
#undef CHANGE
    p_dispose(&other);
    puts("wrapper/inner origin-negative refusal + restore PASS");
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
    const char *argv[] = {"p1359", "scope", path};

    PInput p = {0};
    NLParseDiagnostic parse = {0};
    NLFunctionUnitDiagnostic diagnostic = {0};
    assert(nl_source_load(argv[2], &p.source) == NL_SOURCE_OK);
    assert(nl_parser_create(p.source, &p.parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p.parser, &p.tree, &parse) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&p.context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {p.tree};
    Snapshot before = snapshot(p.context);
    NLCheckStatus status =
        nl_semantic_register_function_unit(p.context, inputs, 1, &diagnostic);
    printf("scope definition status%d code%s\n", status,
           diagnostic.diagnostic.diagnostic.code
               ? diagnostic.diagnostic.diagnostic.code
               : "none");
    if (status == NL_CHECK_OK) {
        for (size_t i = 0; i < 8; ++i)
            free(before.arrays[i]);
        assert(nl_source_create("main()", 6, "P1359 actual main", &p.call) ==
               NL_SOURCE_OK);
        assert(nl_parser_create(p.call, &p.call_parser) == NL_PARSE_OK);
        assert(nl_parser_parse_expression_fragment(p.call_parser, &p.call_tree,
                                                   &parse) == NL_PARSE_OK);
        before = snapshot(p.context);
        status = p_check(&p);
        printf("scope actual CHECK%d entry%p\n", status, (void *)p.entry);
    }
    assert(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT && !p.entry);
    unchanged(p.context, &before);
    p_dispose(&p);
}

static void oom(const char *path, size_t index)
{
    PInput p = {0};
    p_load(&p, path);
    events = ordinal = seal_count = init_count = retain_count =
        copy_seal_count = input_count = 0;
    armed = true;
    NLCheckStatus status = p_check(&p);
    armed = false;
    const size_t total = events;
    printf("CHECK census%zu init-count%zu copy-seals%zu custody%zu "
           "current-attach%zu input-count%zu status%d\n",
           total, init_count, copy_seal_count, seal_count, retain_count,
           input_count, status);
    assert(status == NL_CHECK_OK && init_count == 4 && retain_count == 2 &&
           copy_seal_count == 5 && seal_count == 5 && input_count == 4);
    pair(&p, index);
    p_dispose(&p);
    size_t targets[40] = {1, 2, total / 2, total};
    size_t target_count = 4;
    for (size_t k = 0; k < 4; ++k) {
        targets[target_count++] = init_at[k];
        targets[target_count++] = input_at[k];
        targets[target_count++] = input_end[k];
        printf("readonly capture%zu [%zu,%zu], original init%zu\n", k,
               input_at[k], input_end[k], init_at[k]);
    }
    for (size_t k = 0; k < 5; ++k) {
        targets[target_count++] = copy_seals[k];
        targets[target_count++] = seals[k];
    }
    for (size_t k = 0; k < 2; ++k) {
        targets[target_count++] = retain_at[k];
        printf("current attach%zu marker%zu\n", k, retain_at[k]);
    }
    assert(target_count <= 40);
    for (size_t i = 0; i < target_count; ++i) {
        bool duplicate = false;
        for (size_t j = 0; j < i; ++j)
            duplicate |= targets[i] == targets[j];
        if (duplicate)
            continue;
        PInput trial = {0};
        p_load(&trial, path);
        Snapshot before = snapshot(trial.context);
        events = 0;
        ordinal = targets[i];
        fired = false;
        armed = true;
        status = p_check(&trial);
        armed = false;
        printf("CHECK OOM target%zu fired%d actual%zu status%d entry%p\n",
               ordinal, fired, events, status, (void *)trial.entry);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY && !trial.entry);
        unchanged(trial.context, &before);
        ordinal = 0;
        assert(p_check(&trial) == NL_CHECK_OK);
        pair(&trial, index);
        p_dispose(&trial);
    }
    p = (PInput){0};
    p_load(&p, path);
    assert(p_check(&p) == NL_CHECK_OK);
    P1359Frame frames[2] = {frame(&p, index, 0), frame(&p, index, 1)};
    for (size_t slot = 0; slot < 2; ++slot) {
        P1359Frame f = frames[slot];
        for (size_t which = 0; which < 2; ++which) {
            NLCheckedFragment *caller = which ? f.wrapper : f.main_body;
            NLCheckedNodeId call = which ? f.inner_call : f.wrapper_call;
            NLCopyBodyView out = which ? f.inner_copy : f.outer_copy;
            events = ordinal = 0;
            armed = true;
            status = nl_checked_copy_body_validate(caller, call, &out);
            armed = false;
            size_t total_calls = events;
            printf(
                "slot%zu validator%zu census status%d allocations%zu fired%d\n",
                slot, which, status, total_calls, fired);
            assert(status == NL_CHECK_OK && total_calls &&
                   total_calls <= (which ? 64u : 256u));
            printf("slot%zu validator%zu actual allocations%zu\n", slot, which,
                   total_calls);
            for (size_t point = 1; point <= total_calls; ++point) {
                Snapshot before = snapshot(p.context);
                NLCopyBodyView old = out;
                events = 0;
                ordinal = point;
                fired = false;
                armed = true;
                status = nl_checked_copy_body_validate(caller, call, &out);
                armed = false;
                ordinal = 0;
                assert(fired && status == NL_CHECK_OUT_OF_MEMORY &&
                       !memcmp(&out, &old, sizeof(old)));
                unchanged(p.context, &before);
                pair(&p, index);
            }
        }
    }
    p_dispose(&p);
    puts("nested call retained/sealed full rollback and retry PASS");
}
int main(int argc, char **argv)
{
    setbuf(stdout, NULL);
    if (argc == 4 && !strcmp(argv[1], "positive")) {
        PInput p = {0};
        p_load(&p, argv[2]);
        NLCheckStatus s = p_check(&p);
        printf("actual CHECK%d entry%p\n", s, (void *)p.entry);
        assert(s == NL_CHECK_OK);
        attacks(&p, argv[2], strtoul(argv[3], NULL, 10));
        p_dispose(&p);
    } else if (argc == 4 && !strcmp(argv[1], "oom"))
        oom(argv[2], strtoul(argv[3], NULL, 10));
    else if (argc == 3 && !strcmp(argv[1], "precision"))
        precision_negative(argv[2]);
    else {
        assert(argc == 4 && !strcmp(argv[1], "negative"));
        semantic_negative(argv[2], "P8-EXIT-DEPENDENCY");
        semantic_negative(argv[3], "P3-TYPE-MISMATCH");
    }
    return 0;
}
