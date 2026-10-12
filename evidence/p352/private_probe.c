#ifndef NEWLANG_TEST_CHRONOLOGY_SHADOW
#error "P352 test-only primitive/fault tags, never linked in normal compiler"
#endif
#include "../../src/semantic_internal.h"
#include "probe.h"
#include <stdlib.h>
/* GCC may eliminate a nonvolatile tag around builtin calloc. The malloc
 * wrappers must actually observe the fault site even under -O3. */
volatile unsigned nl_test_p352_phase;
static size_t captures;
static unsigned next_clone;
void nl_test_p352_reset_faults(void)
{
    captures = 0;
    next_clone = 0;
    nl_test_p352_phase = 0;
}
static void *tagged_calloc(size_t n, size_t size)
{
    if (n == 1 && size == sizeof(NLCapturedChange)) {
        ++captures;
        nl_test_p352_phase = captures == 9 ? 1 : 0;
        next_clone = captures == 9 ? 2 : 0;
    }
    void *p = calloc(n, size);
    nl_test_p352_phase = 0;
    return p;
}
static NLCheckStatus tagged_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    nl_test_p352_phase = next_clone;
    NLCheckStatus status = nl_sem_clone(c, out);
    nl_test_p352_phase = 0;
    next_clone = next_clone == 2 ? 3 : 0;
    return status;
}
/* Original source TU; tagging only in this copied test archive. */
#define calloc tagged_calloc
#define nl_sem_clone tagged_clone
#include "../../src/captured_closure.c"
#undef nl_sem_clone
#undef calloc
NLCheckStatus nl_test_p352_primitive(const NLCheckedFragment *f,
                                     const NLCapturedChange *c)
{
    return change_validate(f, c);
}
