/* R386 independent read-only snapshot verifier. Not part of the candidate compiler. */
#include <stdio.h>
#include <stdlib.h>

static int valid_a(unsigned old_a, unsigned old_b, unsigned a0, unsigned a1, unsigned b0, unsigned b1)
{
    return old_a == 0U && old_b == 255U && a0 == 240U && a1 == 199U &&
           b0 == 1U && b1 == 13U;
}

static int valid_b(unsigned old_a, unsigned old_b, unsigned old_a2,
                   unsigned a0, unsigned a1, unsigned b0, unsigned b1)
{
    return old_a == 255U && old_b == 42U && old_a2 == 1U &&
           a0 == 30U && a1 == 0U && b0 == 250U && b1 == 255U;
}

void r386_observe_a(unsigned old_a, unsigned old_b, unsigned a0,
                    unsigned a1, unsigned b0, unsigned b1)
{
    printf("R386_NATIVE_A old=%u,%u alpha=%u,%u beta=%u,%u\n",
           old_a, old_b, a0, a1, b0, b1);
    fflush(stdout);
    if (!valid_a(old_a, old_b, a0, a1, b0, b1)) {
        fputs("R386 independent observation mismatch in A\n", stderr);
        exit(86);
    }
}

void r386_observe_b(unsigned old_a, unsigned old_b, unsigned old_a2,
                    unsigned a0, unsigned a1, unsigned b0, unsigned b1)
{
    printf("R386_NATIVE_B old=%u,%u,%u alpha=%u,%u beta=%u,%u\n",
           old_a, old_b, old_a2, a0, a1, b0, b1);
    fflush(stdout);
    if (!valid_b(old_a, old_b, old_a2, a0, a1, b0, b1)) {
        fputs("R386 independent observation mismatch in B\n", stderr);
        exit(86);
    }
}

#ifdef R386_OBSERVER_SELFTEST
int main(void)
{
    /* Controls intentionally corrupt the observed snapshot, not guest calls. */
    if (!valid_a(0, 255, 240, 199, 1, 13)) return 11;
    if (valid_a(255, 0, 240, 199, 1, 13)) return 12; /* swapped old */
    if (valid_a(0, 255, 0, 199, 1, 13)) return 13; /* missed write */
    if (valid_a(0, 255, 240, 0, 1, 13)) return 14; /* sibling changed */
    if (valid_a(0, 255, 240, 199, 240, 13)) return 15; /* other root changed */
    if (!valid_b(255, 42, 1, 30, 0, 250, 255)) return 21;
    if (valid_b(255, 42, 255, 30, 0, 250, 255)) return 22; /* stale third old */
    if (valid_b(255, 42, 1, 1, 0, 250, 255)) return 23; /* missed third write */
    if (valid_b(255, 42, 1, 30, 0, 1, 255)) return 24; /* wrong second actual */
    if (valid_b(255, 42, 1, 30, 255, 250, 255)) return 25; /* sibling changed */
    if (valid_b(255, 42, 1, 30, 0, 30, 255)) return 26; /* cross-root */
    puts("R386_OBSERVER_CONTROLS_PASS 11 cases");
    return 0;
}
#endif
