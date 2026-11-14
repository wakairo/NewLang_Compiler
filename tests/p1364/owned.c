#include "../p1280/support.h"
#include "copy_body_internal.h"
#include <errno.h>

static bool armed, fired;
static size_t input_at[2], input_end[2], input_count;
static size_t events, ordinal, seals[3], seal_count, init_at[2], init_count,
    retain_at[2], retain_count, copy_seals[3], copy_seal_count;
static bool registration_trace;
static size_t clone_begin[128], clone_end[128], clone_count;
static size_t plan_at[8], plan_count;
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
NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,
                                  NLSemanticContext **);
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    size_t at = 128;
    if (armed && !ordinal && registration_trace && clone_count < 128) {
        at = clone_count++;
        clone_begin[at] = events + 1;
    }
    NLCheckStatus status = __real_nl_sem_clone(c, out);
    if (at < 128)
        clone_end[at] = events;
    return status;
}
NLCheckStatus __real_nl_body_create_span(const NLSource *, NLSourceSpan,
                                         const NLFunctionParameter *, size_t,
                                         NLFunctionBody **);
NLCheckStatus __wrap_nl_body_create_span(const NLSource *s, NLSourceSpan span,
                                         const NLFunctionParameter *p,
                                         size_t count, NLFunctionBody **out)
{
    if (armed && !ordinal && registration_trace && plan_count < 8)
        plan_at[plan_count++] = events + 1;
    return __real_nl_body_create_span(s, span, p, count, out);
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
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *p,
                                              NLCheckedFragment *c,
                                              NLCheckedNodeId id,
                                              const NLSemanticContext *world)
{
    if (armed && !ordinal && seal_count < 3)
        seals[seal_count++] = events + 1;
    return __real_nl_h_custody_seal_result(p, c, id, world);
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
NLCheckStatus __real_nl_copy_body_seal(NLCheckedFragment *,
                                       const NLCheckedFragment *,
                                       NLCheckedNodeId, size_t,
                                       const NLSemanticContext *);
NLCheckStatus __wrap_nl_copy_body_seal(NLCheckedFragment *f,
                                       const NLCheckedFragment *c,
                                       NLCheckedNodeId id, size_t fn,
                                       const NLSemanticContext *w)
{
    if (armed && !ordinal && copy_seal_count < 3)
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
    size_t slot = 2;
    if (armed && !ordinal && input_count < 2) {
        slot = input_count++;
        input_at[slot] = events + 1;
    }
    NLCheckStatus status =
        __real_nl_copy_body_readonly(f, caller, id, symbol, c);
    if (slot < 2)
        input_end[slot] = events;
    return status;
}
typedef struct {
    NLCheckedFragment *main_body, *wrapper, *inner;
    NLCheckedNodeId wrapper_call, inner_call, loop;
    NLLinearBreakView current;
    NLCopyBodyView outer_copy, inner_copy;
    NLPlaceId original, caller_root;
    NLSymbolId caller_base;
} P1364Frame;
static P1364Frame frame(PInput *p, size_t index)
{
    P1364Frame f = {0};
    assert(nl_h_custody_validate(p->entry));
    f.main_body =
        (NLCheckedFragment *)nl_checked_call_body(p->entry, p->entry->root);
    assert(f.main_body && f.main_body->body_count == 1);
    f.wrapper_call = f.main_body->body_calls[0];
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
    for (size_t id = 1; id <= f.main_body->count; ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f.main_body, id);
        if (n->kind == NL_CHECKED_BINDING && n->symbol &&
            !strcmp(p->context->bindings[n->symbol - 1].name, "caller")) {
            assert(!f.caller_root);
            f.caller_base = n->symbol;
            f.caller_root = p->context->bindings[n->symbol - 1].view.place;
            const NLCheckedNodeView *init =
                nl_checked_node_view(f.main_body, n->initializer);
            assert(init && init->kind == NL_CHECKED_AGGREGATE);
        }
    }
    assert(f.caller_root && f.caller_root != f.original &&
           f.caller_root != f.current.root &&
           !p->context->places[f.caller_root - 1].live);
    const NLSemanticContext *frames[] = {incoming->entry,
                                         q->entry,
                                         f.current.entry_world,
                                         f.current.post_world,
                                         f.current.iteration->context,
                                         f.current.breaking->context,
                                         q->exit,
                                         incoming->exit};
    NLSemanticPlaceView kept[3];
    assert(nl_copy_trio(incoming->entry, f.caller_root, kept));
    for (size_t i = 0; i < sizeof(frames) / sizeof(frames[0]); ++i) {
        NLSemanticPlaceView actual[3];
        assert(nl_copy_trio(frames[i], f.caller_root, actual) &&
               !memcmp(kept, actual, sizeof(kept)));
        for (size_t j = 0; j < 3; ++j)
            assert(nl_node_value_same(
                incoming->entry->values[kept[j].current_value - 1],
                frames[i]->values[actual[j].current_value - 1]));
    }
    size_t writes = 0, reads = 0, before_reads = 0;
    const NLCheckedNodeView *update = NULL;
    for (size_t id = 1; id <= f.main_body->count; ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f.main_body, id);
        if (n->kind == NL_CHECKED_REPLACE) {
            assert(!update && id > f.wrapper_call &&
                   n->field.parent == f.caller_root &&
                   n->field.base == f.caller_base && n->field.index == index &&
                   n->field.parent_incarnation == kept[0].incarnation &&
                   n->field.child_incarnation == kept[index + 1].incarnation &&
                   n->field.old_value == kept[index + 1].current_value &&
                   n->field.parent_fact == kept[0].current_fact &&
                   n->field.child_fact == kept[index + 1].current_fact);
            const NLCheckedNodeView *a =
                nl_checked_node_view(f.main_body, n->first_argument);
            assert(a && copy_origin(f.main_body, a->next_argument) ==
                            f.wrapper_call);
            NLSemanticValueView old =
                p->context->values[n->results[0].value - 1];
            assert(old.scalar_known && old.scalar_value == (index ? 29u : 47u));
            assert(!p->context->values[n->field.new_value - 1].scalar_known);
            update = n;
            ++writes;
        }
        if (n->kind == NL_CHECKED_FIELD_READ &&
            n->field.parent == f.caller_root) {
            if (id < f.wrapper_call)
                ++before_reads;
            else {
                assert(update &&
                       n->field.parent_fact == update->field.parent_post_fact);
                reads |= 1u << n->field.index;
            }
        }
        if (n->kind == NL_CHECKED_LOAN_HEADER)
            assert(n->loan.scope &&
                   !p->context->scopes[n->loan.scope - 1].active);
    }
    assert(writes == 1 && reads == 3 && before_reads == 1);
    printf("main root%zu, wrapper%zu, inner%zu are distinct; both caller "
           "frames unchanged "
           "across entry/cycle/exit; real main old%u -> call result -> "
           "current/sibling, "
           "all loans/locals ended PASS\n",
           f.caller_root, f.original, f.current.root, index ? 29u : 47u);
    printf("root->main->wrapper%zu->inner%zu: owner0 current0 Copy0/0; "
           "separate roots%zu/%zu index%zu; closed loan, inner "
           "old->Change->tail->caller Copy%zu\n",
           f.wrapper_call, f.inner_call, f.original, f.current.root, index,
           f.outer_copy.result.value);
    return f;
}
static void rejected(P1364Frame *f, const char *label)
{
    NLLinearBreakView out, before;
    memset(&out, 0xa5, sizeof(out));
    before = out;
    NLCheckStatus a = nl_linear_break_owner_validate(f->inner),
                  b = nl_linear_break_validate(f->inner, f->loop, &out),
                  c = nl_checked_copy_body_validate(f->wrapper, f->inner_call,
                                                    NULL),
                  d = nl_checked_copy_body_validate(f->main_body,
                                                    f->wrapper_call, NULL);
    printf("negative %s owner%d current%d innerCopy%d outerCopy%d\n", label, a,
           b, c, d);
    assert(a != NL_CHECK_OK && b != NL_CHECK_OK && c != NL_CHECK_OK &&
           d != NL_CHECK_OK && !memcmp(&before, &out, sizeof(out)));
}
static void attacks(PInput *p, const char *source, size_t index)
{
    P1364Frame f = frame(p, index);
    PInput other = {0};
    p_load(&other, source);
    assert(p_check(&other) == NL_CHECK_OK);
    P1364Frame foreign = frame(&other, index);
#define CHANGE(type, label, member, bad)                                       \
    do {                                                                       \
        type saved = (member);                                                 \
        (member) = (bad);                                                      \
        rejected(&f, label);                                                   \
        (member) = saved;                                                      \
        (void)frame(p, index);                                                 \
    } while (0)
    CHANGE(const NLSource *, "foreign main source", f.main_body->source,
           foreign.main_body->source);
    CHANGE(NLValueFactId, "main entry current",
           f.wrapper->copy_body->entry->places[f.caller_root - 1].current_fact,
           0);
    CHANGE(NLIncarnationId, "stale main incarnation",
           f.wrapper->copy_body->entry->places[f.caller_root - 1].incarnation,
           999);
    CHANGE(bool, "ended main inside loop",
           ((NLSemanticContext *)f.current.entry_world)
               ->places[f.caller_root - 1]
               .live,
           false);
    CHANGE(NLValueFactId, "main post sibling",
           ((NLSemanticContext *)f.current.post_world)
               ->places[f.wrapper->copy_body->entry->places[f.caller_root - 1]
                            .fixed_fields[1 - index] -
                        1]
               .current_fact,
           0);
    for (size_t id = 1; id <= f.main_body->count; ++id) {
        NLCheckedNodeView *n = &f.main_body->nodes[id - 1];
        if (n->kind != NL_CHECKED_REPLACE)
            continue;
        CHANGE(NLPlaceId, "main Change becomes wrapper", n->field.parent,
               f.original);
        CHANGE(size_t, "main Change wrong field", n->field.index, 1 - index);
        CHANGE(NLValueId, "main wrong old", n->field.old_value,
               n->field.new_value);
        CHANGE(NLValueFactId, "main wrong postcurrent",
               n->field.parent_post_fact, 0);
        const NLCheckedNodeView *a =
            nl_checked_node_view(f.main_body, n->first_argument);
        NLCheckedNodeView *incoming = &f.main_body->nodes[a->next_argument - 1];
        CHANGE(NLSymbolId, "main incoming loses actual result origin",
               incoming->symbol, f.caller_base);
    }
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
    const char *argv[] = {"p1347", "scope", path};

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
        assert(nl_source_create("main()", 6, "P1343 actual main", &p.call) ==
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
    printf("CHECK census%zu init[%zu,%zu] copy-seals[%zu,%zu,%zu] "
           "custody[%zu,%zu,%zu] retain-count%zu status%d\n",
           total, init_at[0], init_at[1], copy_seals[0], copy_seals[1],
           copy_seals[2], seals[0], seals[1], seals[2], retain_count, status);
    assert(status == NL_CHECK_OK && init_count == 2 && retain_count == 1 &&
           copy_seal_count == 3 && seal_count == 3 && input_count == 2);
    (void)frame(&p, index);
    p_dispose(&p);
    size_t targets[] = {1,
                        2,
                        init_at[0],
                        init_at[1],
                        copy_seals[0],
                        copy_seals[1],
                        copy_seals[2],
                        seals[0],
                        seals[1],
                        seals[2],
                        input_at[0],
                        input_end[0],
                        input_at[1],
                        input_end[1],
                        total / 2,
                        total};
    for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
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
        (void)frame(&trial, index);
        p_dispose(&trial);
    }
    p = (PInput){0};
    p_load(&p, path);
    assert(p_check(&p) == NL_CHECK_OK);
    P1364Frame f = frame(&p, index);
    for (size_t which = 0; which < 2; ++which) {
        NLCheckedFragment *caller = which ? f.wrapper : f.main_body;
        NLCheckedNodeId call = which ? f.inner_call : f.wrapper_call;
        NLCopyBodyView out = which ? f.inner_copy : f.outer_copy;
        events = ordinal = 0;
        fired = false;
        armed = true;
        status = nl_checked_copy_body_validate(caller, call, &out);
        armed = false;
        size_t total_calls = events;
        printf("validator%zu census status%d allocations%zu fired%d\n", which,
               status, total_calls, fired);
        assert(status == NL_CHECK_OK && total_calls &&
               total_calls <= (which ? 8u : 128u));
        printf("validator%zu actual allocations%zu\n", which, total_calls);
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
            (void)frame(&p, index);
        }
    }
    p_dispose(&p);
    puts("nested call retained/sealed full rollback and retry PASS");
}
typedef struct {
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *tree;
} Seed;
static void registration_input(PInput *p, Seed *seed, const char *path)
{
    NLParseDiagnostic parse = {0};
    NLFunctionUnitDiagnostic diagnostic = {0};
    const char text[] = "fn kept_seed() -> u8 { u8(7) }";
    assert(nl_semantic_create(&p->context) == NL_CHECK_OK);
    assert(nl_source_create(text, sizeof(text) - 1,
                            "P1364 genuine retained seed",
                            &seed->source) == NL_SOURCE_OK);
    assert(nl_parser_create(seed->source, &seed->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(seed->parser, &seed->tree, &parse) ==
           NL_PARSE_OK);
    const NLSyntaxTree *old[] = {seed->tree};
    assert(nl_semantic_register_function_unit(p->context, old, 1,
                                              &diagnostic) == NL_CHECK_OK);
    assert(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    assert(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p->parser, &p->tree, &parse) ==
           NL_PARSE_OK);
    assert(nl_source_create("main()", 6, "P1364 genuine main", &p->call) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(p->call, &p->call_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(p->call_parser, &p->call_tree,
                                               &parse) == NL_PARSE_OK);
}
static NLCheckStatus register_input(PInput *p)
{
    NLFunctionUnitDiagnostic d = {0};
    const NLSyntaxTree *units[] = {p->tree};
    return nl_semantic_register_function_unit(p->context, units, 1, &d);
}
static void seed_dispose(Seed *s)
{
    nl_syntax_tree_destroy(s->tree);
    nl_parser_destroy(s->parser);
    nl_source_destroy(s->source);
}
static void registration_oom(const char *path, size_t index)
{
    PInput p = {0};
    Seed seed = {0};
    registration_input(&p, &seed, path);
    events = ordinal = clone_count = plan_count = 0;
    fired = false;
    registration_trace = true;
    armed = true;
    NLCheckStatus status = register_input(&p);
    armed = false;
    registration_trace = false;
    const size_t total = events;
    printf("registration census%zu clones%zu plans%zu status%d\n", total,
           clone_count, plan_count, status);
    assert(status == NL_CHECK_OK && total > 8 && clone_count &&
           plan_count == 3);
    assert(p_check(&p) == NL_CHECK_OK);
    (void)frame(&p, index);
    p_dispose(&p);
    seed_dispose(&seed);
    size_t points[32] = {1, 2, total / 2, total}, count = 4;
    for (size_t i = 0; i < plan_count; ++i)
        points[count++] = plan_at[i];
    size_t choices[] = {0, clone_count / 2, clone_count - 1};
    for (size_t i = 0; i < 3; ++i) {
        points[count++] = clone_begin[choices[i]];
        points[count++] = clone_end[choices[i]];
        printf("registration clone%zu allocation range[%zu,%zu]\n", choices[i],
               clone_begin[choices[i]], clone_end[choices[i]]);
    }
    assert(count <= 32);
    for (size_t i = 0; i < count; ++i) {
        bool duplicate = false;
        for (size_t j = 0; j < i; ++j)
            duplicate |= points[j] == points[i];
        if (duplicate)
            continue;
        p = (PInput){0};
        seed = (Seed){0};
        registration_input(&p, &seed, path);
        Snapshot before = snapshot(p.context);
        events = 0;
        ordinal = points[i];
        fired = false;
        registration_trace = true;
        armed = true;
        status = register_input(&p);
        armed = false;
        registration_trace = false;
        printf("registration OOM target%zu fired%d actual%zu status%d\n",
               ordinal, fired, events, status);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY);
        unchanged(p.context, &before);
        ordinal = 0;
        assert(register_input(&p) == NL_CHECK_OK);
        assert(p_check(&p) == NL_CHECK_OK);
        (void)frame(&p, index);
        p_dispose(&p);
        seed_dispose(&seed);
    }
    puts("registration retained context/body owner rollback + retry PASS");
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
    else if (argc == 4 && !strcmp(argv[1], "registration-oom"))
        registration_oom(argv[2], strtoul(argv[3], NULL, 10));
    else if (argc == 3 && !strcmp(argv[1], "precision"))
        precision_negative(argv[2]);
    else {
        assert(argc == 4 && !strcmp(argv[1], "negative"));
        semantic_negative(argv[2], "P8-EXIT-DEPENDENCY");
        semantic_negative(argv[3], "P3-TYPE-MISMATCH");
    }
    return 0;
}
