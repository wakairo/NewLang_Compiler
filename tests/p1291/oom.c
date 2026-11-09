#include "../p1280/support.h"
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
static bool fail(void *pc)
{
    if (!armed)
        return false;
    ++hits;
    size_t i = 0;
    while (i < site_count && sites[i].pc != (uintptr_t)pc)
        ++i;
    if (i == site_count) {
        assert(site_count < 512);
        sites[site_count].pc = (uintptr_t)pc;
        sites[site_count++].first = hits;
    }
    if (hits != failure)
        return false;
    fired = true;
    return true;
}
void *__wrap_malloc(size_t n)
{
    return fail(__builtin_return_address(0)) ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return fail(__builtin_return_address(0)) ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return fail(__builtin_return_address(0)) ? NULL : __real_realloc(p, n);
}
static void begin(size_t ordinal)
{
    hits = 0;
    failure = ordinal;
    fired = false;
    armed = true;
}
static void healthy(PInput *p)
{
    assert(nl_h_origin_validate(p->entry) && nl_h_custody_validate(p->entry));
    assert(nl_h_custody_result_valid(p->entry, nl_checked_root(p->entry)));
}
int main(int argc, char **argv)
{
    assert(argc == 3 || (argc == 4 && !strcmp(argv[3], "census")));
    const bool emit = strcmp(argv[2], "emit") == 0;
    PInput warm = {0};
    p_load(&warm, argv[1]);
    if (emit)
        assert(p_check(&warm) == NL_CHECK_OK);
    char *baseline = NULL;
    size_t length = 98765;
    begin(0);
    const int status =
        emit ? (int)nl_checked_c_node(warm.entry, &baseline, &length)
             : (int)p_check(&warm);
    armed = false;
    const size_t total = hits;
    assert(status == 0 && !fired && total);
    healthy(&warm);
    if (!emit)
        assert(nl_checked_c_node(warm.entry, &baseline, &length) ==
               NL_NODE_C_OK);
    const size_t baseline_length = length;
    assert(baseline && baseline_length);
    bool *selected = calloc(total + 1, sizeof(*selected));
    assert(selected);
    printf("CENSUS phase=%s total=%zu sites=%zu\n", argv[2], total, site_count);
    for (size_t i = 0; i < site_count; ++i) {
        selected[sites[i].first] = true;
        printf("SITE first=%zu pc=0x%jx\n", sites[i].first,
               (uintmax_t)sites[i].pc);
    }
    for (size_t i = 1; i <= total; ++i)
        if (emit || i <= 64 || total - i < 64 || i % (total / 128 + 1) == 0)
            selected[i] = true;
    p_dispose(&warm);
    size_t tested = 0;
    for (size_t ordinal = 1; ordinal <= total; ++ordinal) {
        if (!selected[ordinal])
            continue;
        if (argc == 4) {
            printf("SELECT %zu\n", ordinal);
            continue;
        }
        PInput p = {0};
        p_load(&p, argv[1]);
        if (emit)
            assert(p_check(&p) == NL_CHECK_OK);
        const NLSemanticContext before = *p.context;
        char *text = NULL;
        length = 98765;
        begin(ordinal);
        const int failed = emit
                               ? (int)nl_checked_c_node(p.entry, &text, &length)
                               : (int)p_check(&p);
        armed = false;
        assert(fired && hits >= ordinal);
        printf("OOM phase=%s ordinal=%zu status=%d reached=%zu\n", argv[2],
               ordinal, failed, hits);
        fflush(stdout);
        if (emit) {
            assert(failed == NL_NODE_C_OUT_OF_MEMORY && !text &&
                   length == 98765);
            healthy(&p);
        } else {
            assert(failed == NL_CHECK_OUT_OF_MEMORY && !p.entry &&
                   !memcmp(&before, p.context, sizeof(before)));
            assert(nl_sem_validate(p.context) == NL_CHECK_OK &&
                   nl_raw_validate(p.context) == NL_CHECK_OK);
            assert(p_check(&p) == NL_CHECK_OK);
            healthy(&p);
        }
        assert(nl_checked_c_node(p.entry, &text, &length) == NL_NODE_C_OK &&
               text && length == baseline_length &&
               !memcmp(text, baseline, length));
        free(text);
        p_dispose(&p);
        ++tested;
    }
    free(selected);
    free(baseline);
    printf("OOM-PASS phase=%s total=%zu selected=%zu rollback-retry=%zu\n",
           argv[2], total, tested, tested);
    return 0;
}
