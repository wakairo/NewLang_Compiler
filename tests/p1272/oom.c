#include "support.h"
#include <stdint.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool armed, fired;
static size_t hits, failure;
static struct {
    uintptr_t pc;
    size_t first;
} sites[512];
static size_t site_count;
static bool fail_here(void *pc)
{
    if (!armed)
        return false;
    ++hits;
    size_t i = 0;
    while (i < site_count && sites[i].pc != (uintptr_t)pc)
        ++i;
    if (i == site_count) {
        assert(site_count < sizeof(sites) / sizeof(sites[0]));
        sites[site_count].pc = (uintptr_t)pc;
        sites[site_count++].first = hits;
    }
    if (hits == failure) {
        fired = true;
        return true;
    }
    return false;
}
void *__wrap_malloc(size_t n)
{
    return fail_here(__builtin_return_address(0)) ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return fail_here(__builtin_return_address(0)) ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return fail_here(__builtin_return_address(0)) ? NULL : __real_realloc(p, n);
}
static void begin(size_t ordinal)
{
    hits = 0;
    failure = ordinal;
    fired = false;
    armed = true;
}
static size_t end(void)
{
    armed = false;
    return hits;
}
static void ready(PInput *p, const char *path)
{
    p_load(p, path);
    assert(p_check(p) == NL_CHECK_OK && p->entry);
    assert(nl_h_custody_result_valid(p->entry, nl_checked_root(p->entry)));
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    const bool emit = strcmp(argv[2], "emit") == 0;
    PInput warm = {0};
    if (emit)
        ready(&warm, argv[1]);
    else
        p_load(&warm, argv[1]);
    begin(0);
    char *text = NULL;
    size_t length = 98765;
    int status = emit ? (int)nl_checked_c_node(warm.entry, &text, &length)
                      : (int)p_check(&warm);
    const size_t total = end();
    assert(status == 0 && !fired && total);
    p_validate(&warm, 181);
    free(text);
    printf("CENSUS phase=%s total=%zu sites=%zu\n", argv[2], total, site_count);
    bool *selected = calloc(total + 1, sizeof(*selected));
    assert(selected);
    for (size_t i = 0; i < site_count; ++i) {
        selected[sites[i].first] = true;
        printf("SITE %s first=%zu pc=0x%jx\n", argv[2], sites[i].first,
               (uintmax_t)sites[i].pc);
    }
    if (getenv("P1272_CENSUS_ONLY")) {
        free(selected);
        p_dispose(&warm);
        return 0;
    }
    /* All emitter allocations; CHECK: every first reached allocator site,
     * first/last64 and a bounded even spread. This is explicitly selected,
     * not a claim that every CHECK ordinal was exhausted. */
    for (size_t i = 1; i <= total; ++i)
        if (emit || i <= 64 || total - i < 64 || i % (total / 128 + 1) == 0)
            selected[i] = true;
    p_dispose(&warm);
    size_t tested = 0, recovered = 0;
    for (size_t ordinal = 1; ordinal <= total; ++ordinal) {
        if (!selected[ordinal])
            continue;
        PInput p = {0};
        if (emit)
            ready(&p, argv[1]);
        else
            p_load(&p, argv[1]);
        const NLSemanticContext before = *p.context;
        text = NULL;
        length = 98765;
        begin(ordinal);
        status = emit ? (int)nl_checked_c_node(p.entry, &text, &length)
                      : (int)p_check(&p);
        const size_t reached = end();
        assert(fired && reached >= ordinal);
        printf("OOM %s ordinal=%zu reached=%zu status=%d\n", argv[2], ordinal,
               reached, status);
        fflush(stdout);
        if (emit) {
            assert(status == NL_NODE_C_OUT_OF_MEMORY && !text &&
                   length == 98765);
            assert(
                nl_h_origin_validate(p.entry) &&
                nl_h_custody_result_valid(p.entry, nl_checked_root(p.entry)));
            assert(nl_checked_c_node(p.entry, &text, &length) == NL_NODE_C_OK &&
                   text && length);
            free(text);
        } else {
            assert(status == NL_CHECK_OUT_OF_MEMORY && !p.entry);
            assert(!memcmp(&before, p.context, sizeof(before)));
            assert(nl_sem_validate(p.context) == NL_CHECK_OK &&
                   nl_raw_validate(p.context) == NL_CHECK_OK);
            assert(
                p_check(&p) == NL_CHECK_OK && p.entry &&
                nl_h_custody_result_valid(p.entry, nl_checked_root(p.entry)));
        }
        ++recovered;
        ++tested;
        p_dispose(&p);
    }
    free(selected);
    printf("OOM-FINAL phase=%s total=%zu selected=%zu rollback-retry=%zu\n",
           argv[2], total, tested, recovered);
    return 0;
}
