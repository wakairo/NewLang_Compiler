/* R787 passive legacy A regression: real host malloc/free and OOM worlds. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
size_t test_malloc_calls(void);
size_t test_free_calls(void);
uintptr_t test_allocated_address(size_t);
static void r787_finish(void)
{
    const char *fault = getenv("NEWLANG_TEST_FAIL_CALL");
    char f = fault ? fault[0] : '0';
    size_t requests = f == '1' ? 1u : 2u;
    size_t frees = f == '1' ? 0u : f == '2' ? 1u : 2u;
    if (test_malloc_calls() != requests || test_free_calls() != frees ||
        (f == '0' && (!test_allocated_address(0) ||
                      !test_allocated_address(1) ||
                      test_allocated_address(0) == test_allocated_address(1))) ||
        (f == '2' && !test_allocated_address(0))) {
        fprintf(stderr, "R787 legacy original heap accounting mismatch\n");
        abort();
    }
    printf("R787_RA_NATIVE requests=%zu frees=%zu fail=%c\n",
           requests, frees, f);
}
#define NL_HEAP_FINISH() r787_finish()
