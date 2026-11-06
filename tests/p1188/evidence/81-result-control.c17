/* North Star V0/V1/AVS Checked-C reference output. */
#include <stdint.h>

#include <limits.h>
_Static_assert(CHAR_BIT == 8, "NewLang C17 requires 8-bit bytes");
_Static_assert(UINT8_MAX == 255 && sizeof(uint8_t) == 1 && ((uint8_t)-1) == 255, "NewLang C17 requires exact 8-bit uint8_t");
#include <stddef.h>
_Static_assert(SIZE_MAX == UINTMAX_C(18446744073709551615), "NewLang usize target mismatch");

typedef struct { uint8_t f0; uint8_t f1; } nl_type_10;

#ifdef NEWLANG_CURRENT_LOOP_OBSERVER
#include NEWLANG_CURRENT_LOOP_OBSERVER
#endif
#ifndef NL_CURRENT_PHASE
#define NL_CURRENT_PHASE(...) ((void)0)
#endif
#ifndef NL_CURRENT_LOCAL
#define NL_CURRENT_LOCAL(...) ((void)0)
#endif
#ifndef NL_CURRENT_RESULT
#define NL_CURRENT_RESULT(...) ((void)0)
#endif

#ifdef NEWLANG_LOOP_OBSERVER
#include NEWLANG_LOOP_OBSERVER
#endif
#ifndef NL_LOOP_BIND
#define NL_LOOP_BIND(...) ((void)0)
#endif
#ifndef NL_LOOP_READ
#define NL_LOOP_READ(...) ((void)0)
#endif
#ifndef NL_LOOP_BEFORE
#define NL_LOOP_BEFORE(...) ((void)0)
#endif
#ifndef NL_LOOP_AFTER
#define NL_LOOP_AFTER(...) ((void)0)
#endif

#ifdef NEWLANG_SCALAR_LOOP_OBSERVER
#include NEWLANG_SCALAR_LOOP_OBSERVER
#endif
#ifndef NL_SCALAR_PHASE
#define NL_SCALAR_PHASE(...) ((void)0)
#endif
#ifndef NL_SCALAR_RESULT
#define NL_SCALAR_RESULT(...) ((void)0)
#endif

static void nl_fn_11(void);
static uint8_t nl_fn_10(size_t nl_local_3);

static void nl_fn_11(void)
{
    nl_type_10 nl_local_1 = { .f0 = 117, .f1 = 46, };
    (void)nl_local_1;
    NL_LOOP_BIND(&nl_local_1.f0, &nl_local_1.f1);
    size_t nl_local_2;
    size_t nl_loop_carry_10 = (size_t)UINTMAX_C(2);
    for (;;) {
        const size_t nl_loop_phase_10 = nl_loop_carry_10;
        NL_CURRENT_PHASE(nl_loop_phase_10, &nl_local_1.f0, &nl_local_1.f1);
        if (nl_loop_phase_10 < (size_t)UINTMAX_C(6)) {
            const uint8_t nl_local_6 = nl_fn_10(nl_loop_phase_10);
            (void)nl_local_6;
            NL_CURRENT_LOCAL(6, nl_local_6);
            uint8_t nl_loan_result_1;
            {
                uint8_t *const nl_local_7 = &nl_local_1.f1;
                (void)nl_local_7;
                const uint8_t nl_replace_old_8 = *nl_local_7;
                *nl_local_7 = nl_local_6;
                nl_loan_result_1 = nl_replace_old_8;
            }
            (void)nl_loan_result_1;
            const uint8_t nl_local_8 = nl_loan_result_1;
            (void)nl_local_8;
            NL_CURRENT_LOCAL(8, nl_local_8);
            const uint8_t nl_local_9 = nl_local_8;
            (void)nl_local_9;
            NL_CURRENT_LOCAL(9, nl_local_9);
            (void)nl_local_9;
            const uint8_t nl_local_10 = nl_local_1.f1;
            (void)nl_local_10;
            NL_CURRENT_LOCAL(10, nl_local_10);
            const uint8_t nl_local_11 = nl_local_1.f0;
            (void)nl_local_11;
            NL_CURRENT_LOCAL(11, nl_local_11);
            (void)nl_local_10;
            (void)nl_local_11;
            nl_loop_carry_10 = nl_loop_phase_10 + (size_t)UINTMAX_C(2);
            continue;
        }
        nl_local_2 = nl_loop_phase_10;
        NL_CURRENT_RESULT(nl_local_2);
        break;
    }
    (void)nl_local_2;
    const uint8_t nl_local_3 = nl_local_1.f1;
    (void)nl_local_3;
    NL_LOOP_READ(&nl_local_1.f1, nl_local_3);
    const uint8_t nl_local_4 = nl_local_1.f0;
    (void)nl_local_4;
    NL_LOOP_READ(&nl_local_1.f0, nl_local_4);
    const size_t nl_local_5 = nl_local_2;
    (void)nl_local_5;
    (void)0;
}

static uint8_t nl_fn_10(size_t nl_local_3)
{
    (void)nl_local_3;
    const size_t nl_less_left_2 = nl_local_3;
    const size_t nl_less_right_2 = ((size_t)UINTMAX_C(4));
    const uint8_t nl_local_4 = ((nl_less_left_2 < nl_less_right_2) ? 62 : 163);
    (void)nl_local_4;
    const uint8_t nl_local_5 = nl_local_4;
    (void)nl_local_5;
    return nl_local_5;
}

int main(void)
{
    nl_fn_11();
    return 0;
}
