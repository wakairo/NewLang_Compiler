/* Existing registration/actual-call snapshot and relation controls, reused
 * with calloc injection in addition to the original malloc/realloc hooks. */
#define main p733_existing_history_main
#include "../unit/live_tail_return_test.c"
#undef main
void *__real_calloc(size_t, size_t);
void *__wrap_calloc(size_t n, size_t s)
{
    if (injecting && fail_at >= 10000)
        abort();
    return injecting && at++ == fail_at ? NULL : __real_calloc(n, s);
}
int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "oom") && !evidence(argv[2]))
        return 1;
    return p733_existing_history_main(argc, argv);
}
