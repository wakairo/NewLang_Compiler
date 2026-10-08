/* Test-only read-only oracle. Platform interception is a separate translation
 * unit; this observer never allocates, writes a Node or calls free. All memory
 * observations precede EndRoot. Only integer address observations survive free.
 */
#include <stdlib.h>
#define VERIFY(x)                                                              \
    do {                                                                       \
        if (!(x))                                                              \
            abort();                                                           \
    } while (0)
static uintptr_t heap_address;
static const nl_node *observed_head;
static size_t phase, changes, copies, domain_token, trials, successes, arms,
    binds;
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(void);
static void heap_trial(void *p, size_t size, size_t alignment)
{
    VERIFY(trials++ == 0 && phase == 0 && size == 24 && alignment == 8);
    VERIFY(test_malloc_calls() == 1);
    heap_address = (uintptr_t)p;
    successes = p != NULL;
    VERIFY(heap_address == test_allocated_address());
    phase = p != NULL ? 1 : 99;
}
static void heap_arm(size_t variant)
{
    VERIFY(arms++ == 0 && variant == (successes ? 2 : 1));
}
static void heap_slot(const void *p, size_t n)
{
    VERIFY(phase == 1 && (uintptr_t)p == heap_address && n == 24);
    phase = 2;
}
static void heap_domain(size_t d)
{
    VERIFY(phase == 2 && d != 0);
    domain_token = d;
    phase = 3;
}
static void heap_initialize(const nl_node *p, size_t d)
{
    VERIFY(phase == 3 && (uintptr_t)p == heap_address && d == domain_token);
    VERIFY(p->f0.tag == 0 && p->f0.ptr == NULL && p->f1 == EXPECT_TAIL);
    phase = 4;
}
static void heap_bind(size_t frame, size_t symbol, size_t kind,
                      const void *value)
{
    (void)frame;
    (void)symbol;
    if (kind == 1) {
        const nl_node *p = value;
        VERIFY(phase == 4 && observed_head == NULL &&
               (uintptr_t)p != heap_address);
        VERIFY(p->f1 == EXPECT_HEAD && p->f0.tag == 0 && p->f0.ptr == NULL);
        observed_head = p;
        ++binds;
    } else if (kind == 2) {
        const nl_node *const *p = value;
        VERIFY(phase == 4 && (uintptr_t)*p == heap_address);
        ++binds;
    } else if (kind == 3) {
        const nl_node_option *p = value;
        ++copies;
        if (copies == 1)
            VERIFY(phase == 5 && p->tag == 0 && p->ptr == NULL);
        else if (copies == 2)
            VERIFY(phase == 5 && p->tag == 1 &&
                   (uintptr_t)p->ptr == heap_address &&
                   p != &observed_head->f0);
        else {
            VERIFY(copies == 3 && phase == 7 && p->tag == 1 &&
                   (uintptr_t)p->ptr == heap_address);
        }
    }
}
static void heap_replace(const nl_node *head, const nl_node_option *old)
{
    VERIFY(head == observed_head);
    if (changes++ == 0) {
        VERIFY(phase == 4 && old->tag == 0 && old->ptr == NULL &&
               head->f0.tag == 1 && (uintptr_t)head->f0.ptr == heap_address);
        phase = 5;
    } else {
        VERIFY(changes == 2 && phase == 6 && old->tag == 1 &&
               (uintptr_t)old->ptr == heap_address && head->f0.tag == 0 &&
               head->f0.ptr == NULL);
        phase = 7;
    }
}
static void heap_node_arm(size_t variant, const nl_node *p)
{
    VERIFY(phase == 5 && copies == 2 && variant == 2 &&
           (uintptr_t)p == heap_address);
}
static void heap_reloan(const nl_node *p, size_t d)
{
    VERIFY(phase == 5 && (uintptr_t)p == heap_address && d == domain_token);
    VERIFY(p->f1 == EXPECT_TAIL && p->f0.tag == 0);
    phase = 6;
}
static void heap_end(const nl_node *p, size_t d)
{
    VERIFY(phase == 7 && copies == 3 && changes == 2 &&
           (uintptr_t)p == heap_address && d == domain_token);
    VERIFY(observed_head->f0.tag == 0 && observed_head->f0.ptr == NULL);
    phase = 8;
}
static void heap_raw(const void *p, size_t n)
{
    VERIFY(phase == 8 && (uintptr_t)p == heap_address && n == 24);
    phase = 9;
}
static void heap_finalize(size_t d)
{
    VERIFY(phase == 9 && d == domain_token);
    phase = 10;
}
static void heap_release(const void *allocation, const void *raw, size_t n)
{
    VERIFY(phase == 10 && (uintptr_t)allocation == heap_address &&
           (uintptr_t)raw == heap_address && n == 24);
    phase = 11;
}
/* Platform shim invokes this read-only check on the requested free, then
 * delegates that exact request. No observer cleanup or synthetic release. */
void test_observe_free(uintptr_t address)
{
    VERIFY(phase == 11 && address == heap_address);
    phase = 12;
}
static void heap_finish(void)
{
    VERIFY(trials == 1 && arms == 1 && test_malloc_calls() == 1);
    if (successes)
        VERIFY(phase == 12 && binds == 2 && test_free_calls() == 1);
    else
        VERIFY(phase == 99 && binds == 0 && changes == 0 && copies == 0 &&
               domain_token == 0 && test_free_calls() == 0);
}
#undef NL_NODE_BIND
#undef NL_NODE_REPLACE
#undef NL_NODE_ARM
#define NL_NODE_BIND(f, s, k, v) heap_bind(f, s, k, v)
#define NL_NODE_REPLACE(h, o) heap_replace(h, o)
#define NL_NODE_ARM(v, p) heap_node_arm(v, p)
#define NL_HEAP_TRIAL(p, n, a) heap_trial(p, n, a)
#define NL_HEAP_ARM(v) heap_arm(v)
#define NL_HEAP_SLOT(p, n) heap_slot(p, n)
#define NL_HEAP_DOMAIN(d) heap_domain(d)
#define NL_HEAP_INITIALIZE(p, d) heap_initialize(p, d)
#define NL_HEAP_RELOAN(p, d) heap_reloan(p, d)
#define NL_HEAP_END(p, d) heap_end(p, d)
#define NL_HEAP_RAW(p, n) heap_raw(p, n)
#define NL_HEAP_FINALIZE(d) heap_finalize(d)
#define NL_HEAP_RELEASE(a, r, n) heap_release(a, r, n)
#define NL_HEAP_FINISH() heap_finish()
