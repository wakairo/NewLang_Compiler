#include "../p1280/support.h"
#include <errno.h>

static bool armed, fired;
static size_t events, ordinal, clones, clone_begin[128], clone_end[128];
static size_t plans, plan_at[8], seals, seal_at[8];
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool fault(void)
{
    if (!armed)
        return false;
    ++events;
    if (!ordinal || ordinal != events)
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
NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,
                                  NLSemanticContext **);
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **o)
{
    size_t slot = 128;
    if (armed && !ordinal && clones < 128) {
        slot = clones++;
        clone_begin[slot] = events + 1;
    }
    NLCheckStatus status = __real_nl_sem_clone(c, o);
    if (slot < 128)
        clone_end[slot] = events;
    return status;
}
NLCheckStatus __real_nl_body_create_span(const NLSource *, NLSourceSpan,
                                         const NLFunctionParameter *, size_t,
                                         NLFunctionBody **);
NLCheckStatus __wrap_nl_body_create_span(const NLSource *s, NLSourceSpan span,
                                         const NLFunctionParameter *p, size_t n,
                                         NLFunctionBody **out)
{
    if (armed && !ordinal && plans < 8)
        plan_at[plans++] = events + 1;
    return __real_nl_body_create_span(s, span, p, n, out);
}
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *p,
                                              NLCheckedFragment *c,
                                              NLCheckedNodeId id)
{
    if (armed && !ordinal && seals < 8)
        seal_at[seals++] = events + 1;
    return __real_nl_h_custody_seal_result(p, c, id);
}

typedef struct {
    NLCheckedFragment *main, *arm, *helper;
    NLCheckedNodeId loop;
    NLLinearBreakView v;
} Borrow;
static bool admission(PInput *p)
{
    V0Program program = {.unit = nl_semantic_unit_type(p->context),
                         .u8 = nl_semantic_core_type(p->context, NL_TYPE_U8)};
    return v0_validate_call(p->entry, nl_checked_root(p->entry), &program);
}
static Borrow healthy(PInput *p, size_t index)
{
    Borrow b = {0};
    b.main =
        (NLCheckedFragment *)nl_checked_call_body(p->entry, p->entry->root);
    assert(b.main && nl_h_custody_validate(p->entry));
    for (size_t i = 1; i <= b.main->count; ++i)
        if (b.main->nodes[i - 1].kind == NL_CHECKED_LOOP) {
            assert(!b.loop);
            b.loop = i;
        }
    assert(b.loop && nl_linear_break_owner_validate(b.main) == NL_CHECK_OK);
    assert(nl_linear_break_validate(b.main, b.loop, &b.v) == NL_CHECK_OK);
    NLLinearBreakView *v = &b.v;
    assert(v->changing_current && v->terminal_old && !v->paired_current &&
           !v->scalar_loop && v->incoming_call && !v->terminal_call &&
           !v->incoming_if && v->index == index && v->helper_formal);
    b.arm = (NLCheckedFragment *)v->iteration;
    b.helper =
        (NLCheckedFragment *)nl_checked_call_body(b.arm, v->incoming_call);
    assert(b.helper && b.helper != b.main && b.helper != b.arm &&
           !b.helper->arm_count && !b.helper->body_count &&
           !b.helper->copy_body &&
           nl_h_custody_result_valid(b.arm, v->incoming_call));
    const NLCheckedNodeView *call =
        nl_checked_node_view(b.arm, v->incoming_call);
    const NLCheckedNodeView *arg =
        nl_checked_node_view(b.arm, call->first_argument);
    const NLSemanticContext *c = b.arm->context;
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    assert(call->kind == NL_CHECKED_REGISTERED_CALL &&
           call->argument_count == 1 && arg && arg->type == u8 &&
           arg->parameter_type == u8 && arg->kind == NL_CHECKED_IDENTIFIER &&
           arg->value_use == NL_VALUE_COPIED && call->result_count == 1 &&
           call->type == u8 &&
           !c->values[call->results[0].value - 1].scalar_known);
    assert(call->function && call->function <= c->function_count);
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    assert(fn->count == 1 && fn->parameters[0] == u8 && fn->result == u8 &&
           fn->body == b.helper->body_owner &&
           fn->body->source == b.helper->source);
    const NLSemanticBindingView formal = c->bindings[v->helper_formal - 1].view;
    assert(c->bindings[v->helper_formal - 1].hidden && formal.type == u8 &&
           formal.value == arg->results[0].value && formal.place &&
           !c->places[formal.place - 1].live &&
           !c->places[formal.place - 1].current_fact);
    const NLCheckedNodeView *root =
        nl_checked_node_view(b.helper, b.helper->root);
    assert(root && !root->terminates && root->result_count == 1 &&
           root->results[0].value == call->results[0].value &&
           !nl_control_exits_count(b.helper->exits));
    size_t reads = 0;
    for (size_t i = 0; i < b.arm->count; ++i) {
        const NLCheckedNodeView *n = &b.arm->nodes[i];
        if (n->kind == NL_CHECKED_FIELD_READ) {
            assert(n->field.parent == v->root && n->field.child == v->child &&
                   n->field.index == index && n->value_use == NL_VALUE_COPIED);
            ++reads;
        }
    }
    assert(reads == 1);
    const NLSemanticContext *post = v->post_world;
    const NLSemanticPlaceView original = post->places[v->root - 1];
    const NLPlaceId sibling = original.fixed_fields[1 - index];
    assert(original.live && original.incarnation == v->incarnation &&
           original.fixed_fields[index] == v->child &&
           post->places[v->child - 1].incarnation == v->child_incarnation &&
           post->values[post->places[v->child - 1].current_value - 1]
               .scalar_known &&
           post->values[post->places[v->child - 1].current_value - 1]
                   .scalar_value == (index ? 203u : 151u) &&
           post->values[post->places[sibling - 1].current_value - 1]
               .scalar_known &&
           post->values[post->places[sibling - 1].current_value - 1]
                   .scalar_value == (index ? 181u : 217u));
    printf("owned original%zu child%zu index%zu call%zu formal%zu aliases%zu "
           "sealed/closed PASS\n",
           (size_t)v->root, (size_t)v->child, index, (size_t)v->incoming_call,
           (size_t)v->helper_formal, root->item_count);
    return b;
}
static void rejected(PInput *p, Borrow b, const char *name)
{
    NLLinearBreakView out, before;
    memset(&out, 0x63, sizeof(out));
    memcpy(&before, &out, sizeof(out));
    NLCheckStatus status = nl_linear_break_validate(b.main, b.loop, &out);
    assert(status != NL_CHECK_OK && !memcmp(&out, &before, sizeof(out)));
    assert(nl_linear_break_owner_validate(b.main) != NL_CHECK_OK);
    printf("negative %s status=%d unchanged-output PASS\n", name, status);
    (void)p;
}
static void attacks(PInput *p, const char *path, size_t index)
{
    Borrow b = healthy(p, index);
    NLCheckedNodeView *call = &b.arm->nodes[b.v.incoming_call - 1];
    NLCheckedNodeView *arg = &b.arm->nodes[call->first_argument - 1];
    NLCheckedNodeView saved = *arg;
    ++arg->symbol;
    rejected(p, b, "actual binding");
    *arg = saved;
    healthy(p, index);
    saved = *call;
    call->results[0].value = arg->results[0].value;
    rejected(p, b, "result package");
    *call = saved;
    healthy(p, index);
    call->function = b.arm->context->function_count + 1;
    rejected(p, b, "function index");
    *call = saved;
    healthy(p, index);
    NLCheckedNodeView *tail =
        &b.helper->nodes[b.helper->nodes[b.helper->root - 1].tail - 1];
    saved = *tail;
    ++tail->symbol;
    rejected(p, b, "formal/alias lineage");
    *tail = saved;
    healthy(p, index);
    const NLSource *source = b.helper->source;
    b.helper->source = p->call;
    rejected(p, b, "source");
    b.helper->source = source;
    healthy(p, index);
    NLFunctionBody *plan = b.helper->body_owner;
    b.helper->body_owner = NULL;
    rejected(p, b, "BODY");
    b.helper->body_owner = plan;
    healthy(p, index);
    NLCheckedNodeView *read = NULL;
    for (size_t i = 0; i < b.arm->count; ++i)
        if (b.arm->nodes[i].kind == NL_CHECKED_FIELD_READ)
            read = &b.arm->nodes[i];
    assert(read);
    saved = *read;
    read->field.index = 1 - index;
    rejected(p, b, "wrong field");
    *read = saved;
    healthy(p, index);
    ++read->field.child_fact;
    rejected(p, b, "stale current fact");
    *read = saved;
    healthy(p, index);
    ++read->field.child_incarnation;
    rejected(p, b, "stale incarnation");
    *read = saved;
    healthy(p, index);
    NLSemanticContext *c = (NLSemanticContext *)b.arm->context;
    NLFunctionEntry *fn = &c->functions[call->function - 1];
    NLTypeId *params = fn->parameters;
    size_t count = fn->count;
    void *short_allocation = malloc(1);
    assert(short_allocation);
    fn->parameters = short_allocation;
    rejected(p, b, "short foreign signature before read");
    fn->parameters = params;
    free(short_allocation);
    healthy(p, index);
    fn->count = count + 1;
    rejected(p, b, "signature extent");
    fn->count = count;
    healthy(p, index);
    NLFunctionEntry *functions = c->functions;
    size_t nfunctions = c->function_count;
    c->functions = NULL;
    rejected(p, b, "NULL functions");
    c->functions = functions;
    healthy(p, index);
    c->function_count = 0;
    rejected(p, b, "empty function extent");
    c->function_count = nfunctions;
    healthy(p, index);
    NLCheckedFragment *breaking = (NLCheckedFragment *)b.v.breaking;
    const NLCheckedNodeView *loan =
        nl_checked_node_view(breaking, b.v.terminal_loan);
    assert(loan && loan->loan.scope);
    NLSemanticScopeView *scope =
        &breaking->context->scopes[loan->loan.scope - 1];
    assert(!scope->active);
    scope->active = true;
    rejected(p, b, "reopened loan");
    scope->active = false;
    healthy(p, index);
    PInput foreign = {0};
    p_load(&foreign, path);
    assert(p_check(&foreign) == NL_CHECK_OK);
    Borrow other = healthy(&foreign, index);
    size_t slot = 0;
    while (slot < b.arm->body_count &&
           b.arm->body_calls[slot] != b.v.incoming_call)
        ++slot;
    assert(slot < b.arm->body_count);
    NLCheckedFragment *child = b.arm->bodies[slot];
    b.arm->bodies[slot] = other.helper;
    rejected(p, b, "same bits foreign CHECK body");
    b.arm->bodies[slot] = child;
    healthy(p, index);
    healthy(&foreign, index);
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
static void unchanged(const NLSemanticContext *c, Snapshot *s)
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
static void start(size_t at)
{
    events = clones = plans = seals = 0;
    ordinal = at;
    fired = false;
    armed = true;
}
static size_t selected(size_t total, size_t *out)
{
    size_t count = 0;
    size_t all[32] = {1, 2, total / 2, total};
    size_t n = 4;
    for (size_t i = 0; i < plans; ++i)
        all[n++] = plan_at[i];
    for (size_t i = 0; i < seals; ++i)
        all[n++] = seal_at[i];
    if (clones) {
        const size_t k[] = {0, clones / 2, clones - 1};
        for (size_t i = 0; i < 3; ++i) {
            all[n++] = clone_begin[k[i]];
            all[n++] = clone_end[k[i]];
        }
    }
    for (size_t i = 0; i < n; ++i) {
        if (!all[i] || all[i] > total)
            continue;
        size_t j = 0;
        while (j < count && out[j] != all[i])
            ++j;
        if (j == count)
            out[count++] = all[i];
    }
    return count;
}
static void actual_oom(const char *path, size_t index)
{
    PInput p = {0};
    p_load(&p, path);
    start(0);
    NLCheckStatus status = p_check(&p);
    armed = false;
    assert(status == NL_CHECK_OK);
    size_t total = events, points[32];
    size_t count = selected(total, points);
    printf("actual census=%zu clones=%zu plans=%zu finished-seals=%zu "
           "selected=%zu\n",
           total, clones, plans, seals, count);
    assert(seals >= 1);
    healthy(&p, index);
    p_dispose(&p);
    for (size_t i = 0; i < count; ++i) {
        PInput q = {0};
        p_load(&q, path);
        Snapshot before = snapshot(q.context);
        start(points[i]);
        status = p_check(&q);
        armed = false;
        printf("actual OOM %zu events=%zu fired=%d status=%d out=%p\n",
               points[i], events, fired, status, (void *)q.entry);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY && !q.entry);
        unchanged(q.context, &before);
        ordinal = 0;
        assert(p_check(&q) == NL_CHECK_OK);
        healthy(&q, index);
        p_dispose(&q);
    }
}
static void seed(PInput *p, const char *path)
{
    static const char text[] = "fn retained_seed() -> u8 { u8(7) }";
    NLParseDiagnostic d = {0};
    NLFunctionUnitDiagnostic u = {0};
    assert(nl_source_create(text, sizeof(text) - 1, "existing registered seed",
                            &p->call) == NL_SOURCE_OK);
    assert(nl_parser_create(p->call, &p->call_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p->call_parser, &p->call_tree, &d) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&p->context) == NL_CHECK_OK);
    const NLSyntaxTree *seed[] = {p->call_tree};
    assert(nl_semantic_register_function_unit(p->context, seed, 1, &u) ==
           NL_CHECK_OK);
    assert(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    assert(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p->parser, &p->tree, &d) ==
           NL_PARSE_OK);
}
static NLCheckStatus registration(PInput *p)
{
    const NLSyntaxTree *t[] = {p->tree};
    NLFunctionUnitDiagnostic d = {0};
    return nl_semantic_register_function_unit(p->context, t, 1, &d);
}
static void registration_oom(const char *path)
{
    PInput p = {0};
    seed(&p, path);
    start(0);
    NLCheckStatus status = registration(&p);
    armed = false;
    assert(status == NL_CHECK_OK);
    size_t total = events, points[32], count = selected(total, points);
    printf(
        "registration census=%zu clones=%zu plans=%zu seals=%zu selected=%zu\n",
        total, clones, plans, seals, count);
    p_dispose(&p);
    for (size_t i = 0; i < count; ++i) {
        PInput q = {0};
        seed(&q, path);
        Snapshot before = snapshot(q.context);
        start(points[i]);
        status = registration(&q);
        armed = false;
        printf("registration OOM %zu events=%zu fired=%d status=%d\n",
               points[i], events, fired, status);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY);
        unchanged(q.context, &before);
        ordinal = 0;
        assert(registration(&q) == NL_CHECK_OK);
        p_dispose(&q);
    }
}
static void negative(const char *path, const char *code)
{
    PInput p = {0};
    NLParseDiagnostic parse = {0};
    NLFunctionUnitDiagnostic d = {0};
    assert(nl_source_load(path, &p.source) == NL_SOURCE_OK);
    assert(nl_parser_create(p.source, &p.parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p.parser, &p.tree, &parse) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&p.context) == NL_CHECK_OK);
    Snapshot before = snapshot(p.context);
    const NLSyntaxTree *unit[] = {p.tree};
    assert(nl_semantic_register_function_unit(p.context, unit, 1, &d) !=
           NL_CHECK_OK);
    assert(d.diagnostic.diagnostic.code &&
           !strcmp(d.diagnostic.diagnostic.code, code));
    unchanged(p.context, &before);
    p_dispose(&p);
}
int main(int argc, char **argv)
{
    assert(argc == 4);
    size_t index = strtoull(argv[3], NULL, 10);
    assert(index < 2);
    if (!strcmp(argv[1], "registration-oom")) {
        registration_oom(argv[2]);
        return 0;
    }
    if (!strcmp(argv[1], "oom")) {
        actual_oom(argv[2], index);
        return 0;
    }
    if (!strcmp(argv[1], "negative-loan")) {
        negative(argv[2], "P8-EXIT-DEPENDENCY");
        return 0;
    }
    if (!strcmp(argv[1], "negative-type")) {
        negative(argv[2], "P3-TYPE-MISMATCH");
        return 0;
    }
    PInput p = {0};
    p_load(&p, argv[2]);
    assert(p_check(&p) == NL_CHECK_OK);
    healthy(&p, index);
    attacks(&p, argv[2], index);
    p_dispose(&p);
    puts("P1368 genuine current-u8/call/Copy/closure negatives restore PASS");
    return 0;
}
