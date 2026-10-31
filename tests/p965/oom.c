#include "shared.h"

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
NLCheckStatus __real_nl_h_custody_attach(NLCheckedFragment *,
                                         NLCheckedFragment *);
NLCheckStatus __real_nl_h_custody_reserve(NLCheckedFragment *,
                                          NLCheckedFragment *, NLCustodyRole,
                                          NLCheckedNodeId, size_t);
static size_t position, fail_at, scope, phase, slot, committed;
static bool injecting, recording, hit;
typedef struct {
    const void *pointer;
    size_t bytes, scope, phase, slot, committed;
    char kind;
} Event;
static Event events[65536];
static size_t event_count;
static bool fail(void)
{
    if (!injecting)
        return false;
    bool yes = position++ == fail_at;
    hit |= yes;
    return yes;
}
static void record(void *p, size_t bytes, char kind)
{
    if (recording && event_count < sizeof(events) / sizeof(*events))
        events[event_count++] =
            (Event){p, bytes, scope, phase++, slot, committed, kind};
}
void *__wrap_malloc(size_t bytes)
{
    void *p = fail() ? NULL : __real_malloc(bytes);
    record(p, bytes, 'm');
    return p;
}
void *__wrap_calloc(size_t n, size_t bytes)
{
    void *p = fail() ? NULL : __real_calloc(n, bytes);
    record(p, n * bytes, 'c');
    return p;
}
void *__wrap_realloc(void *old, size_t bytes)
{
    void *p = fail() ? NULL : __real_realloc(old, bytes);
    record(p, bytes, 'r');
    return p;
}
NLCheckStatus __wrap_nl_h_custody_attach(NLCheckedFragment *parent,
                                         NLCheckedFragment *child)
{
    scope = 1;
    phase = 0;
    slot = parent ? parent->arm_count : 0;
    committed = slot;
    NLCheckStatus s = __real_nl_h_custody_attach(parent, child);
    scope = 0;
    return s;
}
NLCheckStatus __wrap_nl_h_custody_reserve(NLCheckedFragment *parent,
                                          NLCheckedFragment *child,
                                          NLCustodyRole role,
                                          NLCheckedNodeId site, size_t index)
{
    scope = 2 + (size_t)role;
    phase = 0;
    slot = index;
    committed = role == NL_CUSTODY_ARM ? parent->arm_count : parent->body_count;
    NLCheckStatus s =
        __real_nl_h_custody_reserve(parent, child, role, site, index);
    scope = 0;
    return s;
}
static size_t selected[2048], selected_count;
static bool choose(size_t i)
{
    for (size_t j = 0; j < selected_count; ++j)
        if (selected[j] == i)
            return true;
    CHECK(selected_count < sizeof(selected) / sizeof(*selected));
    selected[selected_count++] = i;
    return true;
}
static bool pointer(const void *p)
{
    if (!p)
        return true;
    size_t found = SIZE_MAX;
    for (size_t i = 0; i < event_count; ++i)
        if (events[i].pointer == p)
            found = i;
    CHECK(found != SIZE_MAX);
    return choose(found);
}
static bool collect(NLCheckedFragment *f)
{
    CHECK(pointer(f));
    if (f->destroy_context)
        CHECK(pointer(f->context));
    CHECK(pointer(f->custody_entry) && pointer(f->packet_entry));
    for (size_t i = 0; i < f->arm_count; ++i)
        CHECK(collect(f->arms[i].artifact));
    for (size_t i = 0; i < f->body_count; ++i)
        CHECK(collect(f->bodies[i]));
    for (size_t i = 0; i < 2; ++i)
        if (f->custody_continuations[i])
            CHECK(collect(f->custody_continuations[i]));
    return true;
}
static bool probe(const char *path, bool registration)
{
    NLSemanticContext *base = NULL, *c = NULL;
    TestNode n = {0};
    NLSyntaxTree *tree = NULL;
    NLSource *source = NULL;
    NLParser *parser = NULL;
    if (registration) {
        CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
        CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
        CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) ==
              NL_PARSE_OK);
        CHECK(nl_semantic_create(&base) == NL_CHECK_OK);
    } else {
        CHECK(registry(path, &base) && entry_tree(&n, &tree));
    }
    CHECK(nl_sem_clone(base, &c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {tree};
    fail_at = SIZE_MAX;
    injecting = recording = true;
    NLCheckedFragment *f = NULL;
    NLCheckStatus status =
        registration ? nl_semantic_register_function_unit(c, inputs, 1, NULL)
                     : nl_semantic_check_expression(c, tree, &f, NULL);
    injecting = recording = false;
    CHECK(status == NL_CHECK_OK && !hit && event_count == position);
    for (size_t i = 0; i < event_count; ++i)
        if (events[i].scope)
            CHECK(choose(i));
    if (!registration) {
        CHECK(f && nl_h_custody_validate(f) && collect(f));
    }
    nl_checked_destroy(f);
    f = NULL;
    nl_semantic_destroy(c);
    c = NULL;
    CHECK(selected_count > 0);
    for (size_t i = 0; i < selected_count; ++i) {
        size_t at = selected[i];
        Event event = events[at];
        CHECK(nl_sem_clone(base, &c) == NL_CHECK_OK);
        position = 0;
        hit = false;
        fail_at = at;
        injecting = true;
        status = registration
                     ? nl_semantic_register_function_unit(c, inputs, 1, NULL)
                     : nl_semantic_check_expression(c, tree, &f, NULL);
        injecting = false;
        CHECK(hit && position > at && status == NL_CHECK_OUT_OF_MEMORY && !f &&
              nl_packet_same_entry(base, c));
        status = registration
                     ? nl_semantic_register_function_unit(c, inputs, 1, NULL)
                     : nl_semantic_check_expression(c, tree, &f, NULL);
        CHECK(status == NL_CHECK_OK);
        if (!registration)
            CHECK(f && nl_h_custody_validate(f) && emit(f, true, NULL, 0));
        nl_checked_destroy(f);
        f = NULL;
        nl_semantic_destroy(c);
        c = NULL;
        printf("P965 OOM source=%s mode=%s position=%zu scope=%zu phase=%zu "
               "slot=%zu prior-committed=%zu kind=%c bytes=%zu "
               "reached/unpublished/rollback/retry PASS\n",
               path, registration ? "definition" : "CHECK", at, event.scope,
               event.phase, event.slot, event.committed, event.kind,
               event.bytes);
    }
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    nl_source_destroy(n.entry_source);
    nl_semantic_destroy(base);
    printf("P965 reached OOM positions=%zu; final publication/close has no "
           "allocator; finite selected coverage\n",
           selected_count);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 4)
        return 1;
    node_backend = strcmp(argv[3], "node") == 0;
    return probe(argv[1], strcmp(argv[2], "definition") == 0) ? 0 : 1;
}
