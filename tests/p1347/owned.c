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
    NLCheckedFragment *caller, *body;
    NLCheckedNodeId call, loop;
    NLLinearBreakView current;
    NLCopyBodyView copy;
} Call;
static Call get_call(PInput *p, size_t slot, size_t index)
{
    Call r = {0};
    assert(nl_h_custody_validate(p->entry));
    r.caller =
        (NLCheckedFragment *)nl_checked_call_body(p->entry, p->entry->root);
    assert(r.caller && r.caller->body_count == 2 && slot < 2);
    r.call = r.caller->body_calls[slot];
    r.body = (NLCheckedFragment *)nl_checked_call_body(r.caller, r.call);
    assert(r.body && r.body == r.caller->bodies[slot]);
    assert(nl_h_custody_result_valid(p->entry, p->entry->root));
    assert(nl_h_custody_result_valid(r.caller, r.call));
    for (size_t i = 1; i <= r.body->count; ++i)
        if (nl_checked_node_view(r.body, i)->kind == NL_CHECKED_LOOP) {
            assert(!r.loop);
            r.loop = i;
        }
    assert(r.loop);
    assert(nl_linear_break_owner_validate(r.body) == NL_CHECK_OK);
    assert(nl_linear_break_validate(r.body, r.loop, &r.current) == NL_CHECK_OK);
    assert(nl_checked_copy_body_view(r.caller, r.call, &r.copy));
    assert(nl_checked_copy_body_validate(r.caller, r.call, &r.copy) ==
           NL_CHECK_OK);
    assert(r.copy.body == r.body && r.body->copy_body->current_loop == r.loop &&
           r.current.changing_current && r.current.terminal_old &&
           !r.current.paired_current && r.current.index == index &&
           r.copy.event_count == 3 &&
           r.copy.result.value ==
               nl_checked_node_view(r.caller, r.call)->results[0].value);
    assert(nl_copy_plain(p->context, r.copy.result.value, r.copy.result.type));
    assert(!p->context->values[r.copy.result.value - 1].scalar_known);
    assert(!p->context->places[r.current.root - 1].live);
    for (size_t arm = 0; arm < 2; ++arm) {
        const NLCheckedFragment *f =
            arm ? r.current.breaking : r.current.iteration;
        size_t loans = 0;
        for (size_t i = 0; i < f->count; ++i)
            if (f->nodes[i].kind == NL_CHECKED_LOAN_HEADER) {
                const NLScopeId s = f->nodes[i].loan.scope;
                assert(s && s <= f->context->scope_count &&
                       !f->context->scopes[s - 1].active);
                ++loans;
            }
        assert(loans == 1);
    }
    printf("slot%zu call%zu loop%zu root%zu child%zu index%zu owner0 current0 "
           "Copy0 closed-loans2 result%zu unknown-free-u8\n",
           slot, r.call, r.loop, r.current.root, r.current.child, index,
           r.copy.result.value);
    return r;
}
static void valid(PInput *p, size_t index)
{
    Call a = get_call(p, 0, index), b = get_call(p, 1, index);
    assert(a.body != b.body && a.call < b.call &&
           a.body->body_owner == b.body->body_owner &&
           a.current.root != b.current.root &&
           a.current.incarnation != b.current.incarnation &&
           a.copy.result.value != b.copy.result.value);
    assert(a.body->copy_body != b.body->copy_body);
    NLCopyBodyView before = a.copy;
    assert(nl_checked_copy_body_validate(b.caller, b.call, &a.copy) !=
           NL_CHECK_OK);
    assert(nl_checked_copy_body_validate(a.caller, a.call, &b.copy) !=
           NL_CHECK_OK);
    assert(!memcmp(&a.copy, &before, sizeof(before)));
    const NLCopyBodyProof *pa = a.body->copy_body, *pb = b.body->copy_body;
    assert(pa->readonly_parameter && pb->readonly_parameter &&
           pa->readonly_parameter != pb->readonly_parameter);
    size_t late_first = 0, calls = 0;
    NLPlaceId caller_root = 0;
    for (NLCheckedNodeId id = 1; id <= a.caller->count; ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(a.caller, id);
        if (n->kind == NL_CHECKED_REGISTERED_CALL)
            ++calls;
        if (n->kind == NL_CHECKED_FIELD_READ) {
            if (!caller_root)
                caller_root = n->field.parent;
            assert(caller_root == n->field.parent);
        }
        if (n->kind == NL_CHECKED_IDENTIFIER &&
            n->span.start_byte >
                nl_checked_node_view(a.caller, b.call)->span.end_byte &&
            copy_origin(a.caller, id) == a.call)
            ++late_first;
    }
    assert(calls == 2 && late_first);
    NLPlaceId formal_places[2];
    for (size_t i = 0; i < 2; ++i) {
        const Call r = i ? b : a;
        const NLCopyBodyProof *proof = r.body->copy_body;
        const NLCheckedNodeView *call = nl_checked_node_view(r.caller, r.call);
        const NLCheckedNodeView *arg =
            nl_checked_node_view(r.caller, call->first_argument);
        const NLSemanticBindingView formal =
            proof->entry->bindings[proof->readonly_parameter - 1].view;
        assert(!proof->formal_write && call->argument_count == 1 && arg &&
               !arg->next_argument && arg->result_count == 1 &&
               formal.value == arg->results[0].value &&
               formal.type == arg->type &&
               nl_copy_plain(proof->entry, formal.value, formal.type));
        formal_places[i] = formal.place;
        assert(formal.place && !p->context->places[formal.place - 1].live);
        NLCopyBodyParameters ref_write;
        assert(!nl_checked_copy_body_parameters(r.caller, r.call, &ref_write));
        if (!caller_root)
            continue;
        NLSemanticPlaceView frame[3];
        assert(nl_copy_trio(proof->entry, caller_root, frame) &&
               caller_root != r.current.root &&
               frame[0].type == r.current.nominal);
        const NLSemanticContext *worlds[] = {
            r.current.entry_world, r.current.post_world,
            r.current.iteration->context, r.current.breaking->context,
            proof->exit};
        for (size_t j = 0; j < sizeof(worlds) / sizeof(worlds[0]); ++j) {
            NLSemanticPlaceView seen[3];
            assert(nl_copy_trio(worlds[j], caller_root, seen) &&
                   !memcmp(frame, seen, sizeof(frame)));
        }
        printf("slot%zu readonly formal%zu place%zu caller-root%zu unchanged "
               "within callee\n",
               i, proof->readonly_parameter, formal.place, caller_root);
    }
    assert(formal_places[0] != formal_places[1]);
    assert(nl_copy_plain(pb->entry, a.copy.result.value, a.copy.result.type));
    if (caller_root) {
        NLSemanticPlaceView first[3], second[3];
        assert(nl_copy_trio(pa->exit, caller_root, first) &&
               nl_copy_trio(pb->entry, caller_root, second) &&
               first[0].incarnation == second[0].incarnation &&
               !memcmp(first[0].fixed_fields, second[0].fixed_fields,
                       sizeof(first[0].fixed_fields)));
        size_t changes = 0;
        for (NLCheckedNodeId id = a.call + 1; id < b.call; ++id) {
            const NLCheckedNodeView *n = nl_checked_node_view(a.caller, id);
            if (n->kind != NL_CHECKED_REPLACE || n->field.parent != caller_root)
                continue;
            assert(n->field.parent_incarnation == first[0].incarnation &&
                   n->field.parent_post_fact == second[0].current_fact &&
                   n->field.child_post_fact ==
                       second[n->field.index + 1].current_fact &&
                   n->field.new_value ==
                       second[n->field.index + 1].current_value);
            const NLCheckedNodeView *ref =
                nl_checked_node_view(a.caller, n->first_argument);
            assert(copy_origin(a.caller, ref->next_argument) == a.call);
            ++changes;
        }
        assert(changes <= 1);
        if (changes)
            assert(first[0].current_fact != second[0].current_fact);
        printf("same caller current transfer: %zu actual intervening Change; "
               "first Copy retained after second\n",
               changes);
    }
}
static void rejected(PInput *p, Call *r, const char *label)
{
    NLLinearBreakView v, old;
    memset(&v, 0x5a, sizeof(v));
    old = v;
    NLCheckStatus owner = nl_linear_break_owner_validate(r->body);
    NLCheckStatus current = nl_linear_break_validate(r->body, r->loop, &v);
    NLCheckStatus copy =
        nl_checked_copy_body_validate(r->caller, r->call, NULL);
    printf("negative %s owner%d current%d Copy%d\n", label, owner, current,
           copy);
    assert(owner != NL_CHECK_OK && current != NL_CHECK_OK &&
           copy != NL_CHECK_OK);
    assert(!memcmp(&v, &old, sizeof(v)));
    (void)p;
}
static void attacks(PInput *p, const char *path, size_t index)
{
    valid(p, index);
    PInput other = {0};
    p_load(&other, path);
    assert(p_check(&other) == NL_CHECK_OK);
    for (size_t slot = 0; slot < 2; ++slot) {
        Call a = get_call(p, slot, index), b = get_call(p, 1 - slot, index);
        Call foreign = get_call(&other, slot, index);
#define CHANGE(type, label, member, bad)                                       \
    do {                                                                       \
        type saved = (member);                                                 \
        (member) = (bad);                                                      \
        rejected(p, &a, label);                                                \
        (member) = saved;                                                      \
        valid(p, index);                                                       \
    } while (0)
        NLCopyBodyProof *cp = a.body->copy_body;
        CHANGE(NLSymbolId, "cross-readonly-formal", cp->readonly_parameter,
               b.body->copy_body->readonly_parameter);
        const NLCheckedNodeView *cn = nl_checked_node_view(a.caller, a.call);
        CHANGE(NLValueId, "cross-actual-Copy",
               a.caller->nodes[cn->first_argument - 1].results[0].value,
               b.copy.result.value);
        NLFunctionEntry *entry = &p->context->functions[cn->function - 1];
        void *short_foreign = malloc(1);
        assert(short_foreign);
        CHANGE(NLTypeId *, "foreign-short-signature", entry->parameters,
               short_foreign);
        free(short_foreign);
        CHANGE(NLCheckedNodeId, "cross-call-slot", a.caller->body_calls[slot],
               b.call);
        CHANGE(NLCheckedFragment *, "cross-BODY-slot", a.caller->bodies[slot],
               b.body);
        CHANGE(NLCheckedFragment *, "same-ID-foreign-BODY",
               a.caller->bodies[slot], foreign.body);
        CHANGE(const NLSource *, "source", a.body->source,
               foreign.body->source);
        CHANGE(NLFunctionBody *, "plan", a.body->body_owner,
               foreign.body->body_owner);
        CHANGE(struct NLLinearBreakProof *, "cross-current-record",
               a.body->linear_breaks, b.body->linear_breaks);
        NLSemanticContext *post = (NLSemanticContext *)a.current.post_world;
        CHANGE(NLValueFactId, "postcurrent",
               post->places[a.current.child - 1].current_fact,
               post->places[a.current.child - 1].current_fact + 1);
        NLControlExitView *edge = (NLControlExitView *)nl_control_exit_view(
            a.current.breaking->exits, 0);
        const NLControlExitView *other_edge =
            nl_control_exit_view(b.current.breaking->exits, 0);
        CHANGE(const NLControlTarget *, "cross-Break-target", edge->target,
               other_edge->target);
        NLCheckedFragment *arm = (NLCheckedFragment *)a.current.breaking;
        CHANGE(NLValueId, "cross-old-result",
               arm->nodes[a.current.break_value - 1].results[0].value,
               b.current.result.value);
        NLCheckedNodeView *tail =
            &a.body->nodes[a.body->nodes[a.body->root - 1].tail - 1];
        CHANGE(NLValueId, "cross-normal-tail", tail->results[0].value,
               b.copy.result.value);
        CHANGE(NLValueId, "cross-call-result",
               a.caller->nodes[a.call - 1].results[0].value,
               b.copy.result.value);
        const size_t fn = a.caller->nodes[a.call - 1].function;
        CHANGE(NLTypeId, "stale-signature",
               p->context->functions[fn - 1].result,
               nl_semantic_unit_type(p->context));
        for (NLCheckedNodeId id = a.call + 1; id < b.call; ++id) {
            NLCheckedNodeView *change = &a.caller->nodes[id - 1];
            if (change->kind != NL_CHECKED_REPLACE)
                continue;
            CHANGE(NLValueFactId, "caller-Change-post-fact",
                   change->field.parent_post_fact, 0);
            CHANGE(NLValueId, "caller-Change-value", change->field.new_value,
                   b.copy.result.value);
            CHANGE(NLPlaceId, "caller-Change-foreign-root",
                   change->field.parent, a.current.root);
            CHANGE(size_t, "caller-Change-wrong-field", change->field.index,
                   1 - index);
        }
        NLCheckedFragment *iteration = (NLCheckedFragment *)a.current.iteration;
        NLScopeId loan = 0;
        for (size_t i = 0; i < iteration->count; ++i)
            if (iteration->nodes[i].kind == NL_CHECKED_LOAN_HEADER)
                loan = iteration->nodes[i].loan.scope;
        assert(loan);
        CHANGE(
            bool, "loan-reopened",
            ((NLSemanticContext *)iteration->context)->scopes[loan - 1].active,
            true);
#undef CHANGE
    }
    p_dispose(&other);
    puts("two actual witnesses: cross-call/foreign/stale rejection + restore "
         "PASS");
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

static void oom_two(const char *path, size_t index)
{
    PInput census = {0};
    p_load(&census, path);
    events = ordinal = seal_count = init_count = retain_count = 0;
    armed = true;
    NLCheckStatus status = p_check(&census);
    armed = false;
    const size_t total = events;
    printf("CHECK census%zu init[%zu,%zu] retain[%zu,%zu] seals[%zu,%zu,%zu] "
           "status%d\n",
           total, init_at[0], init_at[1], retain_at[0], retain_at[1], seals[0],
           seals[1], seals[2], status);
    assert(status == NL_CHECK_OK && init_count == 2 && retain_count == 2 &&
           seal_count == 3);
    valid(&census, index);
    p_dispose(&census);
    const size_t points[] = {
        1,        2,        init_at[0], init_at[1], retain_at[0], retain_at[1],
        seals[0], seals[1], seals[2],   total / 2,  total - 1,    total};
    for (size_t i = 0; i < sizeof(points) / sizeof(points[0]); ++i) {
        bool duplicate = false;
        for (size_t j = 0; j < i; ++j)
            duplicate |= points[i] == points[j];
        if (duplicate)
            continue;
        PInput p = {0};
        p_load(&p, path);
        Snapshot s = snapshot(p.context);
        events = 0;
        ordinal = points[i];
        fired = false;
        armed = true;
        status = p_check(&p);
        armed = false;
        ordinal = 0;
        printf("CHECK OOM target%zu actual%zu fired%d status%d entry%p\n",
               points[i], events, fired, status, (void *)p.entry);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY && !p.entry);
        unchanged(p.context, &s);
        assert(p_check(&p) == NL_CHECK_OK);
        valid(&p, index);
        p_dispose(&p);
    }
    PInput p = {0};
    p_load(&p, path);
    assert(p_check(&p) == NL_CHECK_OK);
    for (size_t slot = 0; slot < 2; ++slot) {
        Call r = get_call(&p, slot, index);
        events = ordinal = 0;
        armed = true;
        status = nl_checked_copy_body_validate(r.caller, r.call, &r.copy);
        armed = false;
        const size_t count = events;
        assert(status == NL_CHECK_OK && count <= 8);
        printf("Copy slot%zu allocations%zu\n", slot, count);
        for (size_t i = 1; i <= count; ++i) {
            Snapshot s = snapshot(p.context);
            NLCopyBodyView before = r.copy;
            events = 0;
            ordinal = i;
            fired = false;
            armed = true;
            status = nl_checked_copy_body_validate(r.caller, r.call, &r.copy);
            armed = false;
            ordinal = 0;
            assert(fired && status == NL_CHECK_OUT_OF_MEMORY &&
                   !memcmp(&r.copy, &before, sizeof(before)));
            unchanged(p.context, &s);
            valid(&p, index);
        }
    }
    p_dispose(&p);
    puts("second-call generation/retention/seal/full rollback + each Copy "
         "OOM/retry PASS");
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
        oom_two(argv[2], strtoul(argv[3], NULL, 10));
    else if (argc == 3 && !strcmp(argv[1], "precision"))
        precision_negative(argv[2]);
    else {
        assert(argc == 4 && !strcmp(argv[1], "negative"));
        semantic_negative(argv[2], "P3-TYPE-MISMATCH");
        semantic_negative(argv[3], "P8-EXIT-DEPENDENCY");
    }
    return 0;
}
