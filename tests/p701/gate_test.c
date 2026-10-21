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
            REFUSE(v->results[0].value = f->copy_body->view.result.value,
                   *v = saved);
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
int main(int argc, char **argv)
{
    return argc == 3 && controls(argv[1]) && controls(argv[2]) &&
                   oom(argv[1]) && oom(argv[2])
               ? 0
               : 1;
}
