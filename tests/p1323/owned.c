#include "../p1280/support.h"
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

typedef struct {
    NLCheckedFragment *body, *graph;
    const NLCheckedFragment *entry;
    NLCheckedNodeId loop;
    NLLinearBreakView view;
    NLCheckedNodeView *writes[2], *loans[2];
} Pair;
static Pair relation(PInput *input, bool paired)
{
    Pair x = {.entry = input->entry};
    x.body = (NLCheckedFragment *)nl_checked_call_body(
        input->entry, nl_checked_root(input->entry));
    assert(x.body);
    for (NLCheckedNodeId id = 1; id <= x.body->count; ++id)
        if (x.body->nodes[id - 1].kind == NL_CHECKED_LOOP) {
            assert(!x.loop);
            x.loop = id;
        }
    assert(x.loop && nl_linear_break_owner_validate(x.body) == NL_CHECK_OK &&
           nl_linear_break_validate(x.body, x.loop, &x.view) == NL_CHECK_OK);
    assert(x.view.changing_current && x.view.paired_current == paired);
    assert(!nl_checked_loop_break_provenance_valid(x.body, x.loop));
    x.graph = (NLCheckedFragment *)nl_checked_loop_body(x.body, x.loop);
    assert(x.graph);
    if (!paired)
        return x;
    size_t stores = 0, loans = 0;
    NLCheckedFragment *iteration = (NLCheckedFragment *)x.view.iteration;
    for (size_t i = 0; i < iteration->count; ++i) {
        NLCheckedNodeView *n = &iteration->nodes[i];
        if (n->kind == NL_CHECKED_REPLACE) {
            assert(stores < 2);
            x.writes[stores++] = n;
        }
        if (n->kind == NL_CHECKED_LOAN_HEADER) {
            assert(loans < 2 && n->loan.scope &&
                   n->loan.scope <= iteration->context->scope_count &&
                   !iteration->context->scopes[n->loan.scope - 1].active);
            x.loans[loans++] = n;
        }
    }
    assert(stores == 2 && loans == 2);
    const NLSemanticContext *header = x.graph->context,
                            *entry = x.view.entry_world,
                            *post = x.view.post_world;
    const NLPlaceId root = x.view.root;
    assert(root && root <= header->place_count && root <= entry->place_count &&
           root <= post->place_count && x.view.terminal_old);
    NLPlaceId child[2];
    for (size_t i = 0; i < 2; ++i) {
        child[i] = header->places[root - 1].fixed_fields[i];
        assert(child[i] && child[i] <= header->place_count &&
               entry->places[root - 1].fixed_fields[i] == child[i] &&
               post->places[root - 1].fixed_fields[i] == child[i]);
        const NLSemanticPlaceView h = header->places[child[i] - 1],
                                  e = entry->places[child[i] - 1],
                                  p = post->places[child[i] - 1];
        assert(h.live && e.live && p.live && h.incarnation == e.incarnation &&
               p.incarnation == e.incarnation && h.parent_aggregate == root &&
               h.parent_field_index == i &&
               !header->values[h.current_value - 1].scalar_known);
        assert(post->values[p.current_value - 1].scalar_known ==
               (i == x.view.index));
    }
    assert(child[0] != child[1]);
    const NLCheckedField a = x.writes[0]->field, b = x.writes[1]->field;
    assert(a.index == x.view.index && b.index == 1 - a.index &&
           a.parent == root && b.parent == root && a.child == child[a.index] &&
           b.child == child[b.index] && a.child != b.child &&
           a.parent_fact == header->places[root - 1].current_fact &&
           b.parent_fact == a.parent_post_fact &&
           iteration->context->places[root - 1].current_fact ==
               b.parent_post_fact &&
           iteration->context->places[a.child - 1].current_fact ==
               a.child_post_fact &&
           iteration->context->places[b.child - 1].current_fact ==
               b.child_post_fact);
    const NLCheckedField end = x.view.terminal_change;
    assert(end.child == x.view.child &&
           end.old_value == header->places[end.child - 1].current_value &&
           post->places[root - 1].current_fact == end.parent_post_fact &&
           post->places[end.child - 1].current_fact == end.child_post_fact &&
           post->places[end.child - 1].current_value == end.new_value &&
           post->places[b.child - 1].current_fact ==
               header->places[b.child - 1].current_fact);
    const NLSemanticValueView old = post->values[x.view.result.value - 1];
    assert(old.type == nl_semantic_core_type(post, NL_TYPE_U8) &&
           !old.scalar_known && old.dependencies == NL_DEPENDENCY_FREE &&
           !old.reference_count && !old.reference.place);
    V0Program program = {.unit = nl_semantic_unit_type(input->context),
                         .u8 =
                             nl_semantic_core_type(input->context, NL_TYPE_U8)};
    assert(v0_validate_call(input->entry, nl_checked_root(input->entry),
                            &program) &&
           program.current_loop && program.uses_usize);
    program.entry_function =
        nl_checked_node_view(input->entry, nl_checked_root(input->entry))
            ->function;
    FILE *serialized = tmpfile();
    assert(serialized && v0_emit_c(serialized, &program) &&
           !fflush(serialized));
    assert(ftell(serialized) > 0 && !fclose(serialized));
    char *output = NULL;
    size_t length = 98765;
    assert(nl_checked_c_node(input->entry, &output, &length) ==
               NL_NODE_C_UNSUPPORTED &&
           !output && length == 98765);
    printf("PAIR owned/current0 root%zu children%zu/%zu index-order%zu/%zu "
           "parent-facts%zu->%zu->%zu closed-loans2 old-u8Unknown "
           "post-Known-only-index%zu CLI-C17-emitted H-API-unsupported\n",
           root, child[0], child[1], a.index, b.index, a.parent_fact,
           a.parent_post_fact, b.parent_post_fact, x.view.index);
    return x;
}
static void reject(Pair *p, const char *label)
{
    NLLinearBreakView view = {0};
    NLCheckStatus owner = nl_linear_break_owner_validate(p->body);
    NLCheckStatus current = nl_linear_break_validate(p->body, p->loop, &view);
    printf("negative %s owner=%d current=%d\n", label, owner, current);
    assert(owner != NL_CHECK_OK && current != NL_CHECK_OK);
    const NLSemanticContext *context = nl_checked_context(p->entry);
    V0Program program = {.unit = nl_semantic_unit_type(context),
                         .u8 = nl_semantic_core_type(context, NL_TYPE_U8)};
    assert(!v0_validate_call(p->entry, nl_checked_root(p->entry), &program));
}
static void attacks(PInput *input, const char *path, size_t profile)
{
    Pair p = relation(input, true);
    if (!profile) {
        NLCheckedFragment *breaking = (NLCheckedFragment *)p.view.breaking;
        NLCheckedNodeView *old = &breaking->nodes[p.view.break_value - 1];
        NLCheckedNodeView saved = *old;
        old->results[0].value = 0;
        reject(&p, "old-Copy-origin");
        *old = saved;
        relation(input, true);
        NLSemanticContext *post = (NLSemanticContext *)p.view.post_world;
        NLPlaceId c0 = post->places[p.view.root - 1].fixed_fields[0],
                  c1 = post->places[p.view.root - 1].fixed_fields[1];
        ++post->places[c0 - 1].incarnation;
        reject(&p, "first-child-incarnation");
        --post->places[c0 - 1].incarnation;
        relation(input, true);
        ++post->places[c1 - 1].current_fact;
        reject(&p, "second-child-postcurrent");
        --post->places[c1 - 1].current_fact;
        relation(input, true);
        saved = *p.writes[1];
        p.writes[1]->field.parent_fact = p.writes[0]->field.parent_fact;
        reject(&p, "missing-first-to-second-parent-chain");
        *p.writes[1] = saved;
        relation(input, true);
    } else {
        NLCheckedNodeView saved = *p.writes[1];
        p.writes[1]->field.index = p.writes[0]->field.index;
        reject(&p, "duplicate-field-index");
        *p.writes[1] = saved;
        relation(input, true);
        NLSemanticContext *world =
            (NLSemanticContext *)p.view.iteration->context;
        NLSemanticScopeView *loan = &world->scopes[p.loans[1]->loan.scope - 1];
        assert(!loan->active);
        loan->active = true;
        reject(&p, "second-loan-reopened");
        loan->active = false;
        relation(input, true);
        PInput foreign = {0};
        p_load(&foreign, path);
        assert(p_check(&foreign) == NL_CHECK_OK);
        Pair other = relation(&foreign, true);
        NLCheckedFragment *iteration = (NLCheckedFragment *)p.view.iteration;
        const NLSource *source = iteration->source;
        iteration->source = other.view.iteration->source;
        assert(iteration->source != source);
        reject(&p, "foreign-source-instance");
        iteration->source = source;
        relation(input, true);
        size_t slot = 0;
        while (slot < p.graph->arm_count &&
               p.graph->arms[slot].artifact != iteration)
            ++slot;
        assert(slot < p.graph->arm_count);
        NLCheckedArm saved_arm = p.graph->arms[slot];
        p.graph->arms[slot].artifact =
            (NLCheckedFragment *)other.view.iteration;
        reject(&p, "foreign-genuine-CHECK-arm-world");
        p.graph->arms[slot] = saved_arm;
        relation(input, true);
        relation(&foreign, true);
        p_dispose(&foreign);
    }
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
static void oom(const char *path)
{
    PInput census = {0};
    p_load(&census, path);
    events = ordinal = 0;
    armed = true;
    NLCheckStatus status = p_check(&census);
    armed = false;
    size_t total = events;
    assert(status == NL_CHECK_OK && total > 4);
    relation(&census, true);
    p_dispose(&census);
    printf("actual main CHECK allocations=%zu\n", total);
    const size_t selected[] = {1, 2, total / 2, total};
    for (size_t i = 0; i < 4; ++i) {
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
        printf("OOM actual=%zu target=%zu fired=%d status=%d out=%p\n", events,
               selected[i], fired, status, (void *)p.entry);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY && !p.entry);
        unchanged(p.context, &before);
        assert(p_check(&p) == NL_CHECK_OK);
        relation(&p, true);
        p_dispose(&p);
    }
}
static void prepare_registration(PInput *p, const char *path)
{
    NLParseDiagnostic d = {0};
    assert(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    assert(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p->parser, &p->tree, &d) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&p->context) == NL_CHECK_OK);
}
static NLCheckStatus register_unit(PInput *p)
{
    NLFunctionUnitDiagnostic d = {0};
    const NLSyntaxTree *units[] = {p->tree};
    return nl_semantic_register_function_unit(p->context, units, 1, &d);
}
static void registration_oom(const char *path)
{
    PInput census = {0};
    prepare_registration(&census, path);
    events = ordinal = 0;
    armed = true;
    NLCheckStatus status = register_unit(&census);
    armed = false;
    const size_t total = events;
    assert(status == NL_CHECK_OK && total > 4);
    p_dispose(&census);
    printf("actual SOURCE registration allocations=%zu\n", total);
    const size_t selected[] = {1, 2, total / 2, total};
    for (size_t i = 0; i < 4; ++i) {
        PInput p = {0};
        prepare_registration(&p, path);
        Snapshot before = snapshot(p.context);
        events = 0;
        ordinal = selected[i];
        fired = false;
        armed = true;
        status = register_unit(&p);
        armed = false;
        ordinal = 0;
        printf("REGISTER OOM actual=%zu target=%zu fired=%d status=%d\n",
               events, selected[i], fired, status);
        assert(fired && status == NL_CHECK_OUT_OF_MEMORY);
        unchanged(p.context, &before);
        assert(register_unit(&p) == NL_CHECK_OK);
        p_dispose(&p);
    }
}
int main(int argc, char **argv)
{
    assert(argc >= 3);
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!strcmp(argv[1], "--registration-only")) {
        for (int i = 2; i < argc; ++i)
            registration_oom(argv[i]);
        puts("SOURCE registration OOM/rollback/retry PASS");
        return 0;
    }
    if (!strcmp(argv[1], "--legacy-only")) {
        for (int i = 2; i < argc; ++i) {
            PInput p = {0};
            p_load(&p, argv[i]);
            assert(p_check(&p) == NL_CHECK_OK);
            relation(&p, false);
            p_dispose(&p);
        }
        puts("legacy one-child current PASS");
        return 0;
    }
    for (int i = 1; i < argc; ++i) {
        PInput p = {0};
        p_load(&p, argv[i]);
        assert(p_check(&p) == NL_CHECK_OK);
        if (i <= 2) {
            relation(&p, true);
            attacks(&p, argv[i], (size_t)(i - 1));
        } else {
            relation(&p, false);
            printf("legacy one-child relation0 %s\n", argv[i]);
        }
        p_dispose(&p);
        if (i <= 2) {
            oom(argv[i]);
            registration_oom(argv[i]);
        }
    }
    puts("paired owned/current negative8 CHECK-OOM8 REGISTER-OOM8 "
         "rollback/retry no native PASS");
    return 0;
}
