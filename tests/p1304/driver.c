#include "../p1280/support.h"

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool armed, fired;
static size_t ordinal, hits;
static bool fail(void)
{
    if (!armed)
        return false;
    ++hits;
    if (!ordinal || hits != ordinal)
        return false;
    fired = true;
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

static size_t refusals;
static V0Program program(const PInput *p)
{
    return (V0Program){.unit = nl_semantic_unit_type(p->context),
                       .u8 = nl_semantic_core_type(p->context, NL_TYPE_U8)};
}
static bool validate(const PInput *p, V0Program *v)
{
    return v0_validate_call(p->entry, nl_checked_root(p->entry), v);
}
static char *render(const PInput *p, size_t *length)
{
    V0Program v = program(p);
    assert(validate(p, &v));
    v.entry_function =
        nl_checked_node_view(p->entry, nl_checked_root(p->entry))->function;
    FILE *stream = tmpfile();
    assert(stream && v0_emit_c(stream, &v) && !fflush(stream));
    long size = ftell(stream);
    assert(size > 0 && size < 65536 && !fseek(stream, 0, SEEK_SET));
    char *text = malloc((size_t)size + 1);
    assert(text && fread(text, 1, (size_t)size, stream) == (size_t)size &&
           !fclose(stream));
    text[size] = 0;
    *length = (size_t)size;
    return text;
}
static void healthy(const PInput *p, const char *baseline, size_t length)
{
    size_t n = 0;
    char *text = render(p, &n);
    assert(n == length && !memcmp(text, baseline, length));
    free(text);
}
static void rejected(const PInput *p)
{
    V0Program v = program(p);
    assert(!validate(p, &v));
    ++refusals;
}
static void load(PInput *p, const char *path)
{
    p_load(p, path);
    assert(p_check(p) == NL_CHECK_OK);
}
static void negative(const char *path)
{
    PInput a = {0};
    load(&a, path);
    size_t length = 0;
    char *baseline = render(&a, &length);
    NLCheckedFragment *main_body = (NLCheckedFragment *)nl_checked_call_body(
        a.entry, nl_checked_root(a.entry));
    assert(main_body && main_body->body_count == 2);
    for (size_t slot = 0; slot < 2; ++slot) {
        NLCheckedFragment *body = main_body->bodies[slot];
        NLCheckedNodeView *read = NULL;
        for (size_t j = 0; j < body->count; ++j)
            if (body->nodes[j].kind == NL_CHECKED_FIELD_READ) {
                read = &body->nodes[j];
                break;
            }
        assert(read);
        const NLCheckedNodeView saved_read = *read;
        read->field.index = 1 - read->field.index;
        rejected(&a);
        *read = saved_read;
        healthy(&a, baseline, length);
        NLCheckedNodeView *root = &body->nodes[body->root - 1];
        const NLCheckedNodeView saved_root = *root;
        ++root->results[0].value;
        rejected(&a);
        *root = saved_root;
        healthy(&a, baseline, length);
        const size_t count = body->count;
        body->count = count - 1;
        rejected(&a);
        body->count = count;
        healthy(&a, baseline, length);
        body->count = count + 100;
        rejected(&a);
        body->count = count;
        healthy(&a, baseline, length);
        NLCheckedNodeView *nodes = body->nodes;
        NLCheckedNodeView *foreign = malloc(sizeof(*foreign));
        assert(foreign);
        *foreign = nodes[0];
        body->nodes = foreign;
        rejected(&a);
        body->nodes = nodes;
        free(foreign);
        healthy(&a, baseline, length);
        const NLSemanticContext *context = body->context;
        NLSemanticContext *clone = NULL;
        assert(nl_sem_clone(context, &clone) == NL_CHECK_OK);
        body->context = clone;
        rejected(&a);
        body->context = context;
        nl_semantic_destroy(clone);
        healthy(&a, baseline, length);
        assert(body->body_count == 1);
        const size_t function =
            nl_checked_node_view(body, body->body_calls[0])->function;
        NLFunctionEntry *fn = &a.context->functions[function - 1];
        NLCheckedKind kind = fn->kind;
        fn->kind = NL_CHECKED_UNIT;
        rejected(&a);
        fn->kind = kind;
        healthy(&a, baseline, length);
    }
    NLFunctionEntry *functions = a.context->functions;
    a.context->functions = NULL;
    rejected(&a);
    a.context->functions = functions;
    healthy(&a, baseline, length);
    p_dispose(&a);
    free(baseline);
    for (size_t variant = 0; variant < 3; ++variant) {
        PInput x = {0}, b = {0};
        load(&x, path);
        load(&b, path);
        char *saved = render(&b, &length);
        NLCheckedFragment *xm = (NLCheckedFragment *)nl_checked_call_body(
            x.entry, nl_checked_root(x.entry));
        const NLCheckedFragment *bm =
            nl_checked_call_body(b.entry, nl_checked_root(b.entry));
        assert(xm && bm && xm->body_count == 2 && bm->body_count == 2);
        if (variant == 0)
            xm->bodies[0] = bm->bodies[0];
        if (variant == 1)
            xm->body_count = 0;
        if (variant == 2)
            xm->body_count += 100;
        rejected(&x);
        p_dispose(&x); /* Deliberately unrestored: canonical children only. */
        healthy(&b, saved, length);
        p_dispose(&b);
        free(saved);
    }
    printf("P1304 driver negative=%zu incl short-node-base/count/current/world "
           "and unrestored-A/B-survival3 PASS\n",
           refusals);
}
static void oom(const char *path)
{
    PInput p = {0};
    load(&p, path);
    size_t length = 0;
    char *baseline = render(&p, &length);
    V0Program v = program(&p);
    armed = true;
    bool ok = validate(&p, &v);
    armed = false;
    assert(ok && !fired && hits && hits <= 1024);
    const size_t census = hits;
    printf("CENSUS phase=consumer total=%zu selected=%zu\n", census, census);
    fflush(stdout);
    for (size_t i = 1; i <= census; ++i) {
        NLSemanticSnapshot before = {0}, after = {0};
        assert(nl_semantic_snapshot(p.context, &before));
        v = program(&p);
        ordinal = i;
        hits = 0;
        fired = false;
        armed = true;
        ok = validate(&p, &v);
        armed = false;
        assert(fired && !ok &&
               (v.copy_body_status == NL_CHECK_OUT_OF_MEMORY ||
                v.copy_body_status == NL_CHECK_RESOURCE_LIMIT));
        assert(nl_semantic_snapshot(p.context, &after) &&
               !memcmp(&before, &after, sizeof(before)));
        assert(nl_sem_validate(p.context) == NL_CHECK_OK);
        healthy(&p, baseline, length);
    }
    p_dispose(&p);
    free(baseline);
    printf("P1304 consumer OOM all %zu fired, retained-context "
           "unchanged/healthy byte retry PASS\n",
           census);
}
int main(int argc, char **argv)
{
    assert(argc == 3);
    if (!strcmp(argv[1], "negative"))
        negative(argv[2]);
    else {
        assert(!strcmp(argv[1], "oom"));
        oom(argv[2]);
    }
    return 0;
}
