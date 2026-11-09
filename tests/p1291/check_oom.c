#include "../p1280/support.h"
#include <stdint.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool armed, fired;
static size_t hits, failure, site_count;
static struct {
    uintptr_t pc;
    size_t first;
} sites[512];
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
static void healthy(const PInput *p)
{
    assert(p->entry && nl_h_origin_validate(p->entry) &&
           nl_h_custody_validate(p->entry) &&
           nl_h_custody_result_valid(p->entry, nl_checked_root(p->entry)));
}
int main(int argc, char **argv)
{
    assert(argc == 3);
    setvbuf(stdout, NULL, _IONBF, 0);
    PInput warm = {0};
    p_load(&warm, argv[1]);
    begin(0);
    const NLCheckStatus status = p_check(&warm);
    armed = false;
    const size_t total = hits;
    assert(status == NL_CHECK_OK && !fired && total);
    healthy(&warm);
    bool *selected = calloc(total + 1, sizeof(*selected));
    assert(selected);
    printf("CENSUS phase=check total=%zu sites=%zu\n", total, site_count);
    for (size_t i = 0; i < site_count; ++i) {
        selected[sites[i].first] = true;
        printf("SITE first=%zu pc=0x%jx\n", sites[i].first,
               (uintmax_t)sites[i].pc);
    }
    for (size_t i = 1; i <= total; ++i)
        if (i <= 8 || total - i < 8 || i % (total / 32 + 1) == 0)
            selected[i] = true;
    p_dispose(&warm);
    size_t tested = 0;
    for (size_t ordinal = 1; ordinal <= total; ++ordinal) {
        if (!selected[ordinal])
            continue;
        ++tested;
        assert(tested <= 128);
        if (!strcmp(argv[2], "census")) {
            printf("SELECT %zu\n", ordinal);
            continue;
        }
        assert(!strcmp(argv[2], "check"));
        PInput p = {0};
        p_load(&p, argv[1]);
        const NLSemanticContext before = *p.context;
        begin(ordinal);
        const NLCheckStatus failed = p_check(&p);
        armed = false;
        printf("OOM ordinal=%zu status=%d reached=%zu\n", ordinal, failed,
               hits);
        assert(fired && hits >= ordinal && failed == NL_CHECK_OUT_OF_MEMORY &&
               !p.entry && !memcmp(&before, p.context, sizeof(before)) &&
               nl_sem_validate(p.context) == NL_CHECK_OK &&
               nl_raw_validate(p.context) == NL_CHECK_OK);
        assert(p_check(&p) == NL_CHECK_OK);
        healthy(&p);
        p_dispose(&p);
    }
    free(selected);
    printf("CHECK-%s total=%zu selected=%zu retry-owned=1\n",
           !strcmp(argv[2], "census") ? "CENSUS" : "OOM-PASS", total, tested);
    return 0;
}
