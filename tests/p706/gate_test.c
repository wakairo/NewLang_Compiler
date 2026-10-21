#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t calls, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && calls++ == fail_at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_realloc(p, n);
}
static bool valid(const TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}
static size_t find_call(const NLCheckedFragment *f)
{
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_REGISTERED_CALL)
            return i + 1;
    return 0;
}
static bool controls(const char *file)
{
    TestNode n = {0}, foreign = {0};
    CHECK(node_checked_load(file, &n) && node_checked_load(file, &foreign) &&
          valid(&n));
    NLCheckedFragment *parent = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    const size_t call = find_call(parent);
    CHECK(call);
    NLCheckedFragment *f =
        (NLCheckedFragment *)nl_checked_call_body(parent, call);
    NLCopyBodyView p, original;
    CHECK(nl_checked_copy_body_view(parent, call, &original));
    p = original;
    size_t rejected = 0;
#define VIEW(change)                                                           \
    do {                                                                       \
        p = original;                                                          \
        change;                                                                \
        CHECK(nl_checked_copy_body_validate(parent, call, &p) ==               \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        ++rejected;                                                            \
    } while (0)
    VIEW(p.root = SIZE_MAX);
    VIEW(p.function = 0);
    VIEW(p.event_count--);
    VIEW(p.event_count++);
    VIEW(p.body = foreign.entry);
    VIEW(p.live_exit = foreign.context);
    VIEW(p.result.value = SIZE_MAX);
    VIEW(p.result.type = 1);
#undef VIEW
    for (size_t j = 0; j < original.event_count; ++j) {
        NLCopyBodyEventView a, b;
        CHECK(nl_checked_copy_body_event(parent, call, j, &a));
#define EVENT(change)                                                          \
    do {                                                                       \
        b = a;                                                                 \
        change;                                                                \
        CHECK(nl_checked_copy_body_event_validate(parent, call, j, &b) ==      \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        ++rejected;                                                            \
    } while (0)
        EVENT(b.node++);
        EVENT(b.before = foreign.context);
        EVENT(b.after = foreign.context);
        EVENT(b.kind = (NLCopyBodyEventKind)((b.kind + 1) % 3));
        EVENT(b.field.parent = SIZE_MAX);
        EVENT(b.field.child = SIZE_MAX);
        EVENT(b.field.parent_incarnation++);
        EVENT(b.field.child_incarnation++);
        EVENT(b.field.index = 1 - b.field.index);
        EVENT(b.field.parent_fact++);
        EVENT(b.field.child_fact++);
        EVENT(b.field.parent_post_fact++);
        EVENT(b.field.child_post_fact++);
        EVENT(b.field.old_value++);
        EVENT(b.result++);
        EVENT(b.incoming++);
#undef EVENT
    }
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(nl_checked_copy_body_validate(parent, call, NULL) ==             \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        CHECK(!valid(&n));                                                     \
        restore;                                                               \
        CHECK(valid(&n));                                                      \
        ++rejected;                                                            \
    } while (0)
    for (size_t i = 0; i < f->count; ++i) {
        NLCheckedNodeView *v = &f->nodes[i];
        const NLCheckedNodeView saved = *v;
        if (v->field.present) {
            REFUSE(v->field.parent_fact++, *v = saved);
            REFUSE(v->field.child_fact++, *v = saved);
            REFUSE(v->field.index = 1 - v->field.index, *v = saved);
            REFUSE(v->field.parent_incarnation++, *v = saved);
            REFUSE(v->field.child = SIZE_MAX, *v = saved);
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            REFUSE(v->field.parent_post_fact++, *v = saved);
            REFUSE(v->field.child_post_fact++, *v = saved);
            REFUSE(v->results[0].value = SIZE_MAX, *v = saved);
        }
        if ((v->kind == NL_CHECKED_BINDING || v->kind == NL_CHECKED_BLOCK) &&
            v->next_item) {
            REFUSE(v->next_item = 0,
                   *v = saved); /* only if originally nonzero */
        }
    }
    NLCheckedNodeView *v = &parent->nodes[call - 1];
    const NLCheckedNodeView saved = *v;
    REFUSE(v->results[0].value = SIZE_MAX, *v = saved);
    REFUSE(v->argument_count = 1, *v = saved);
    const NLSource *source = f->source;
    REFUSE(f->source = n.entry_source, f->source = source);
    NLFunctionBody *owner = f->body_owner;
    REFUSE(f->body_owner = NULL, f->body_owner = owner);
    NLCheckedNodeView *nodes = f->nodes;
    REFUSE(f->nodes = NULL, f->nodes = nodes);
    NLCopyBodyProof *attachment = f->copy_body;
    REFUSE(f->copy_body = NULL, f->copy_body = attachment);
    const NLCopyBodyProof *proof = f->copy_body;
    CopyEvent *e = proof->first;
    CHECK(e);
    while (e && e->view.kind != NL_COPY_BODY_CHANGE)
        e = e->next;
    CHECK(e);
    const NLSemanticPlaceView root = e->after->places[e->view.field.parent - 1];
    REFUSE(e->after->places[e->view.field.parent - 1].current_value = SIZE_MAX,
           e->after->places[e->view.field.parent - 1] = root);
    const NLPlaceId sibling = root.fixed_fields[1 - e->view.field.index];
    const NLSemanticPlaceView child = e->after->places[sibling - 1];
    REFUSE(e->after->places[sibling - 1].current_fact++,
           e->after->places[sibling - 1] = child);
    const NLValueId value =
        e->after->places[e->view.field.child - 1].current_value;
    const NLSemanticValueView package = e->after->values[value - 1];
    REFUSE(e->after->values[value - 1].scalar_value ^= 1,
           e->after->values[value - 1] = package);
    const NLSemanticScopeView scope = e->before->scopes[0];
    REFUSE(e->before->scopes[0].active = false, e->before->scopes[0] = scope);
    const NLCopyBodyEventView event = e->view;
    REFUSE(e->view.field.old_value++, e->view = event);
    if (e->next) {
        CopyEvent *next = e->next;
        REFUSE(e->next = next->next, e->next = next);
        const NLCopyBodyEventView other = next->view;
        REFUSE(next->view = e->view, next->view = other);
    }
    if (proof->first->next && proof->first->next->next) {
        CopyEvent *first = proof->first;
        CopyEvent *second = first->next;
        CopyEvent *third = second->next;
        CopyEvent *last = third->next;
        first->next = third;
        third->next = second;
        second->next = last;
        CHECK(!valid(&n));
        first->next = second;
        second->next = third;
        third->next = last;
        CHECK(valid(&n));
        ++rejected;
    }
    NLCopyBodyProof *m = f->copy_body;
    CHECK(m->formal_write && m->entry && m->handoff);
    NLCopyBodyParameters parameters;
    CHECK(nl_checked_copy_body_parameters(parent, call, &parameters));
#define PARAMETERS(change)                                                     \
    do {                                                                       \
        NLCopyBodyParameters bad = parameters;                                 \
        change;                                                                \
        CHECK(nl_checked_copy_body_parameters_validate(parent, call, &bad) ==  \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        ++rejected;                                                            \
    } while (0)
    PARAMETERS(bad.entry = foreign.context);
    PARAMETERS(bad.handoff = foreign.context);
    PARAMETERS(bad.parameters[0]++);
    PARAMETERS(bad.parameters[1]++);
    PARAMETERS(bad.arguments[0]++);
    PARAMETERS(bad.arguments[1]++);
    PARAMETERS(bad.loan++);
    PARAMETERS(bad.field.parent++);
    PARAMETERS(bad.field.child++);
    PARAMETERS(bad.field.index = 1 - bad.field.index);
    PARAMETERS(bad.field.old_value++);
    PARAMETERS(bad.field.parent_fact++);
#undef PARAMETERS
    const NLCheckedFragment *foreign_parent =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    NLCopyBodyParameters foreign_parameters;
    CHECK(nl_checked_copy_body_parameters(foreign_parent, call,
                                          &foreign_parameters) &&
          foreign_parameters.field.parent == parameters.field.parent &&
          foreign_parameters.parameters[0] == parameters.parameters[0] &&
          foreign_parameters.entry != parameters.entry);
    CHECK(nl_checked_copy_body_parameters_validate(parent, call,
                                                   &foreign_parameters) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    ++rejected;
    NLCopyBodyProof *foreign_proof =
        ((NLCheckedFragment *)nl_checked_call_body(foreign_parent, call))
            ->copy_body;
    REFUSE(f->copy_body = foreign_proof, f->copy_body = m);
    const NLValueId actual_ref = m->arguments[0].results[0].value;
    const NLSemanticValueView actual = m->entry->values[actual_ref - 1];
    REFUSE(m->entry->values[actual_ref - 1].reference.place++,
           m->entry->values[actual_ref - 1] = actual);
    REFUSE(m->entry->values[actual_ref - 1].reference.writable = false,
           m->entry->values[actual_ref - 1] = actual);
    REFUSE(m->entry->values[actual_ref - 1].reference.scope++,
           m->entry->values[actual_ref - 1] = actual);
    REFUSE(m->entry->values[actual_ref - 1].reference.incarnation++,
           m->entry->values[actual_ref - 1] = actual);
    REFUSE(m->entry->values[actual_ref - 1].reference.provenance =
               NL_PROVENANCE_INVALID,
           m->entry->values[actual_ref - 1] = actual);
    NLSemanticContext *entry_world = m->entry;
    REFUSE(m->entry = foreign.context, m->entry = entry_world);
    NLCheckedNodeView *caller_nodes = parent->nodes;
    REFUSE(parent->nodes = NULL, parent->nodes = caller_nodes);
    const NLSource *caller_source = parent->source;
    REFUSE(parent->source = foreign.entry_source,
           parent->source = caller_source);
    const NLCheckedNodeView loan = parent->nodes[parameters.loan - 1];
    REFUSE(parent->nodes[parameters.loan - 1].loan.scope++,
           parent->nodes[parameters.loan - 1] = loan);
    REFUSE(parent->nodes[parameters.loan - 1].field.index =
               1 - loan.field.index,
           parent->nodes[parameters.loan - 1] = loan);
    const NLSemanticPlaceView handoff =
        m->handoff->places[parameters.field.parent - 1];
    REFUSE(m->handoff->places[parameters.field.parent - 1].current_fact++,
           m->handoff->places[parameters.field.parent - 1] = handoff);
    const NLPlaceId sib = handoff.fixed_fields[1 - parameters.field.index];
    const NLSemanticPlaceView old_sib = m->handoff->places[sib - 1];
    REFUSE(m->handoff->places[sib - 1].current_value++,
           m->handoff->places[sib - 1] = old_sib);
    CopyEvent *first_event = m->first;
    REFUSE(m->first = NULL, m->first = first_event);
    CopyEvent duplicate = *first_event;
    duplicate.next = NULL;
    CopyEvent *old_next = first_event->next;
    REFUSE(first_event->next = &duplicate, first_event->next = old_next);
    for (size_t i = 0; i < parent->count; ++i) {
        NLCheckedNodeView *node = &parent->nodes[i];
        NLCheckedNodeView saved_node = *node;
        if (node->kind == NL_CHECKED_FIELD_READ) {
            REFUSE(node->field.old_value++, *node = saved_node);
            REFUSE(node->field.parent_fact++, *node = saved_node);
            REFUSE(node->field.base++, *node = saved_node);
        }
    }
    /* Equal scalar, different Copy origin must not replace the issued return.
     */
    const NLSemanticValueView ret =
        n.context->values[original.result.value - 1];
    bool equal = false;
    for (size_t i = 0; i < n.context->value_count; ++i)
        if (i + 1 != original.result.value &&
            n.context->values[i].type == ret.type &&
            n.context->values[i].scalar_known == ret.scalar_known &&
            n.context->values[i].scalar_value == ret.scalar_value) {
            REFUSE(v->results[0].value = i + 1, *v = saved);
            equal = true;
            break;
        }
    CHECK(equal);
#undef REFUSE
    /* Admission can validate the same proof at multiple structural gates;
     * every allocation failure must retain explicit resource status. */
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        V0Program admission = {
            .unit = nl_semantic_unit_type(n.context),
            .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};
        calls = 0;
        injecting = true;
        bool admitted =
            v0_validate_call(n.entry, nl_checked_root(n.entry), &admission);
        injecting = false;
        if (admitted)
            break;
        CHECK(admission.copy_body_status == NL_CHECK_OUT_OF_MEMORY);
    }
    CHECK(fail_at && fail_at < 8000);
    printf("backend admission OOM positions=%zu explicit_resource=PASS\n",
           fail_at);
    CHECK(valid(&n));
    V0Program program = {.unit = 1,
                         .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};
    CHECK(v0_validate_call(n.entry, nl_checked_root(n.entry), &program));
    FILE *stream = tmpfile();
    CHECK(stream && setvbuf(stream, NULL, _IONBF, 0) == 0);
    calls = fail_at = 0;
    injecting = true;
    bool emitted = v0_emit_c(stream, &program);
    injecting = false;
    CHECK(emitted && calls == 0 && fclose(stream) == 0);
    stream = fopen("/dev/full", "w");
    CHECK(stream && setvbuf(stream, NULL, _IONBF, 0) == 0);
    CHECK(!v0_emit_c(stream, &program));
    (void)fclose(stream);
    TestState *state = malloc(sizeof(*state));
    CHECK(state && test_state(n.context, state));
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckStatus s = nl_checked_copy_body_validate(parent, call, NULL);
        injecting = false;
        CHECK(test_unchanged(n.context, state));
        if (s == NL_CHECK_OK)
            break;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY);
    }
    CHECK(fail_at && fail_at < 8000);
    printf("copy proof rejects=%zu validator_OOM=%zu unchanged=PASS "
           "restored=PASS\n",
           rejected, fail_at);
    free(state);
    node_checked_destroy(&foreign);
    node_checked_destroy(&n);
    return true;
}
static bool oom(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    const NLSyntaxTree *units[] = {unit};
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckStatus s = nl_semantic_register_function_unit(c, units, 1, NULL);
        injecting = false;
        if (s == NL_CHECK_OK)
            break;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("registration OOM positions=%zu rollback=PASS\n", fail_at);
    CHECK(test_state(c, before));
    NLSource *entry = NULL;
    NLSyntaxTree *call = NULL;
    parser = NULL;
    CHECK(nl_source_create("main()", 6, "entry", &entry) == NL_SOURCE_OK &&
          nl_parser_create(entry, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &call, NULL) ==
          NL_PARSE_OK);
    nl_parser_destroy(parser);
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckedFragment *out = NULL;
        NLCheckStatus s = nl_semantic_check_expression(c, call, &out, NULL);
        injecting = false;
        if (s == NL_CHECK_OK) {
            nl_checked_destroy(out);
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && !out && test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("main-call OOM positions=%zu rollback=PASS artifact=NULL\n",
           fail_at);
    free(before);
    nl_syntax_tree_destroy(call);
    nl_source_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(unit);
    nl_source_destroy(source);
    return true;
}
static bool driver_oom(const char *path)
{
    FILE *diagnostics = tmpfile();
    CHECK(diagnostics);
    int saved = dup(STDERR_FILENO);
    CHECK(saved >= 0 && fflush(stderr) == 0 &&
          dup2(fileno(diagnostics), STDERR_FILENO) >= 0);
    bool ok = true;
    size_t host_oom_count = 0;
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        FILE *output = tmpfile();
        if (!output || setvbuf(output, NULL, _IONBF, 0) != 0) {
            ok = false;
            break;
        }
        CHECK(fflush(stderr) == 0 && fflush(diagnostics) == 0 &&
              ftruncate(fileno(diagnostics), 0) == 0 &&
              fseek(diagnostics, 0, SEEK_SET) == 0);
        calls = 0;
        injecting = true;
        int result = compile_v0_to(path, false, NULL, output);
        injecting = false;
        long length = ftell(output);
        if (fclose(output) != 0) {
            ok = false;
            break;
        }
        if (result == 0) {
            ok = length > 0;
            break;
        }
        bool host_oom = false;
        if (result == 3) {
            CHECK(fflush(stderr) == 0 && fflush(diagnostics) == 0 &&
                  fseek(diagnostics, 0, SEEK_SET) == 0);
            char buffer[2048] = {0};
            (void)fread(buffer, 1, sizeof(buffer) - 1, diagnostics);
            host_oom = strstr(buffer, "error(host)") &&
                       strstr(buffer, "OUT-OF-MEMORY");
            CHECK(fseek(diagnostics, 0, SEEK_END) == 0);
            if (host_oom)
                ++host_oom_count;
        }
        if ((result != 1 && !host_oom) || length != 0) {
            (void)fprintf(diagnostics,
                          "driver stop point=%zu status=%d length=%ld\n",
                          fail_at, result, length);
            ok = false;
            break;
        }
    }
    CHECK(fflush(stderr) == 0 && dup2(saved, STDERR_FILENO) >= 0 &&
          close(saved) == 0);
    if (!ok) {
        CHECK(fflush(diagnostics) == 0 && fseek(diagnostics, 0, SEEK_SET) == 0);
        char line[512];
        while (fgets(line, sizeof(line), diagnostics))
            fputs(line, stderr);
    }
    CHECK(fclose(diagnostics) == 0 && ok && fail_at && fail_at < 8000);
    printf("normal driver OOM positions=%zu existing_host_oom_CLI3=%zu "
           "zero_partial_C=PASS\n",
           fail_at, host_oom_count);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && controls(argv[1]) && controls(argv[2]) &&
                   oom(argv[1]) && oom(argv[2]) && driver_oom(argv[1]) &&
                   driver_oom(argv[2])
               ? 0
               : 1;
}
