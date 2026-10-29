/* R1023 C120 independently reviewed native observer candidate. SOURCE ONLY. NEVER RUN now.
 * Only future compiler-emitted unmodified C includes this on an observed build.
 * No T bridge, P fixture/runner or generated source edits.
 */
#ifndef R1022_OBSERVER_H
#define R1022_OBSERVER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned r1022_budget = 3u, r1022_success, r1022_frees, r1022_events;
static uintptr_t r1022_alloc_ptrs[2];
static void r1022_log(const char *label, const void *p, unsigned long v)
{
    if (r1022_events < 256u)
        (void)fprintf(stderr, "R1022_OBS %s %p %lu\n", label, p, v);
    else if (r1022_events == 256u)
        (void)fputs("R1022_OBS OVERFLOW NOT_OBSERVABLE\n", stderr);
    ++r1022_events;
}
static unsigned r1022_world(void)
{
    if (r1022_budget == 3u) {
        const char *s = getenv("R1022_GUEST_SUCCESS_BUDGET");
        if (!s || s[0] < '0' || s[0] > '2' || s[1]) {
            (void)fputs("R1022_OBS world invalid\n", stderr);
            exit(121);
        }
        r1022_budget = (unsigned)(s[0] - '0');
    }
    return r1022_budget;
}
static void *r1022_malloc(size_t bytes)
{
    (void)r1022_world();
    if (bytes > 4096u || r1022_success >= 2u ||
        r1022_success >= r1022_budget) {
        r1022_log("ALLOC_FAIL", NULL, (unsigned long)bytes);
        return NULL;
    }
    void *p = malloc(bytes);
    r1022_log("ALLOC_RESULT", p, (unsigned long)bytes);
    if (p)
        r1022_alloc_ptrs[r1022_success++] = (uintptr_t)p;
    return p;
}
static void r1022_free(void *p)
{
    uintptr_t raw = (uintptr_t)p;
    unsigned matches = 0u;
    for (unsigned i = 0; i < r1022_success; ++i)
        if (r1022_alloc_ptrs[i] == raw)
            ++matches;
    r1022_log("FREE", p, matches);
    if (p)
        ++r1022_frees;
    free(p);
}
/* Defined after real libc calls to redirect only subsequent generated C. */
#define malloc r1022_malloc
#define free r1022_free
#define NL_HEAP_TRIAL(p, bytes, align) r1022_log("TRIAL", (p), (bytes))
#define NL_HEAP_ARM(arm) r1022_log("ARM", NULL, (arm))
#define NL_HEAP_DOMAIN(token) r1022_log("DOMAIN", NULL, (token))
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(ptr, token) r1022_log("INITIALIZE", (ptr), (token))
#define NL_HEAP_RELOAN(ptr, token) r1022_log("RELOAN", (ptr), (token))
#define NL_HEAP_ROOT(...) ((void)0)
#define NL_HEAP_FIELD(...) ((void)0)
#define NL_HEAP_COPY(...) ((void)0)
#define NL_HEAP_CHANGE(...) r1022_log("CHANGE", NULL, 0ul)
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(...) r1022_log("END", NULL, 0ul)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(...) r1022_log("FINALIZE", NULL, 0ul)
#define NL_HEAP_RELEASE(handle, bytes, length) r1022_log("RELEASE", (handle), (length))
#define NL_HEAP_FINISH() \
    r1022_log("FINISH_SUCCESS_FREE", NULL, 10ul * r1022_success + r1022_frees)
#define NL_HEAP_HANDOFF(ptr, handle, token, ...) \
    do { r1022_log("HANDOFF_PTR", (ptr), 0ul); \
         r1022_log("HANDOFF_HANDLE", (handle), (token)); } while (0)
#define NL_HEAP_RECEIVER_ENTER(ptr, handle, token, ...) \
    do { r1022_log("RECEIVER_PTR", (ptr), 0ul); \
         r1022_log("RECEIVER_HANDLE", (handle), (token)); } while (0)
#define NL_HEAP_RECEIVER_EXIT() r1022_log("RECEIVER_EXIT", NULL, 0ul)
#define NL_HEAP_RETURNED(...) r1022_log("RETURNED", NULL, 0ul)
#define NL_LIVE_SEND(...) r1022_log("LIVE_SEND", NULL, 0ul)
#define NL_LIVE_ENTER(...) r1022_log("LIVE_ENTER", NULL, 0ul)
#define NL_LIVE_RETURN(...) r1022_log("LIVE_RETURN", NULL, 0ul)
#define NL_LIVE_RECEIVE(...) r1022_log("LIVE_RECEIVE", NULL, 0ul)
/* The compiler emits this hook after *target=next. Reading that byte at the
 * precise live callback is physical, unlike semantic IDs. The macro cannot
 * recover its containing original T base or unchanged sibling field. */
static void r1023_copy_live(const uint8_t *p, unsigned long old, unsigned long fresh)
{
    if (!p) { r1022_log("COPY_NULL_NOT_OBSERVABLE", p, old); return; }
    r1022_log("COPY_PHYSICAL_OLD_NEW", p, 256ul * old + fresh);
    r1022_log("COPY_PHYSICAL_CURRENT_BYTE", p, (unsigned long)*p);
}
#define NL_MIXED_COPY_CALL(ptr, old, fresh) \
    r1023_copy_live((const uint8_t *)(ptr), (unsigned long)(old), (unsigned long)(fresh))
#define NL_MIXED_COPY_BIND(...) r1022_log("COPY_BIND_UNRESOLVED", NULL, 0ul)
#define NL_MIXED_COPY_STORE(...) r1022_log("COPY_STORE_UNRESOLVED", NULL, 0ul)
#define NL_MIXED_OWNER_LOCAL(...) r1022_log("OWNER_LOCAL_UNRESOLVED", NULL, 0ul)
/* These are transient pointer values and host events, NOT proof of
 * original region/allocation/domain/incarnation/current or sibling state.
 * Never dereference a borrowed view after owner destruction.
 */
#endif
