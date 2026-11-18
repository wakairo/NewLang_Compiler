/* Compiler-host fault apparatus, never guest allocation policy. */
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
void *__real_malloc(size_t);
static size_t text_allocations;
void *__wrap_malloc(size_t size)
{
    const char *setting = getenv("P1517_HOST_TEXT_FAIL");
    if (setting && size == 131072 &&
        ++text_allocations == strtoul(setting, NULL, 10)) {
        static const char message[] =
            "P1517 selected compiler-host text-buffer OOM\n";
        const ssize_t written = write(2, message, sizeof message - 1);
        (void)written;
        return NULL;
    }
    return __real_malloc(size);
}
