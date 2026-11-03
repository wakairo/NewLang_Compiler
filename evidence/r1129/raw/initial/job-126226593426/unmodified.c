/* North Star V0/V1/AVS Checked-C reference output. */
#include <stdint.h>

#include <limits.h>
_Static_assert(CHAR_BIT == 8, "NewLang C17 requires 8-bit bytes");
_Static_assert(UINT8_MAX == 255 && sizeof(uint8_t) == 1 && ((uint8_t)-1) == 255, "NewLang C17 requires exact 8-bit uint8_t");
#include <stddef.h>
_Static_assert(SIZE_MAX == UINTMAX_C(18446744073709551615), "NewLang usize target mismatch");

typedef struct { uint8_t f0; uint8_t f1; } nl_type_10;

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

#ifndef NL_LOOP_PHASE
#define NL_LOOP_PHASE(...) ((void)0)
#endif
#ifndef NL_LOOP_CONTINUE
#define NL_LOOP_CONTINUE(...) ((void)0)
#endif

static void nl_fn_11(void);
static uint8_t nl_fn_10(uint8_t *nl_local_4, uint8_t nl_local_5);

static void nl_fn_11(void)
{
    nl_type_10 nl_local_1 = { .f1 = 37, .f0 = 146, };
    (void)nl_local_1;
    NL_LOOP_BIND(&nl_local_1.f0, &nl_local_1.f1);
    nl_type_10 nl_local_2 = { .f0 = 89, .f1 = 211, };
    (void)nl_local_2;
    NL_LOOP_BIND(&nl_local_2.f0, &nl_local_2.f1);
    uint8_t nl_local_7;
    size_t nl_loop_carry_17 = (size_t)UINTMAX_C(4);
    for (;;) {
        const size_t nl_loop_phase_17 = nl_loop_carry_17;
        NL_LOOP_PHASE(nl_loop_phase_17, &nl_local_1.f0, &nl_local_1.f1);
        if (nl_loop_phase_17 == (size_t)UINTMAX_C(4)) {
            nl_loop_carry_17 = (size_t)UINTMAX_C(19);
            NL_LOOP_CONTINUE(nl_loop_phase_17, nl_loop_carry_17, &nl_local_1.f0, &nl_local_1.f1);
            continue;
        }
        NL_LOOP_BEFORE(&nl_local_1.f0, &nl_local_1.f1, 1);
        nl_local_7 = nl_fn_10(&nl_local_1.f1, 58);
        NL_LOOP_AFTER(&nl_local_1.f0, &nl_local_1.f1, 1, nl_local_7);
        break;
    }
    (void)nl_local_7;
    const uint8_t nl_local_8 = nl_local_1.f1;
    (void)nl_local_8;
    NL_LOOP_READ(&nl_local_1.f1, nl_local_8);
    const uint8_t nl_local_9 = nl_local_1.f0;
    (void)nl_local_9;
    NL_LOOP_READ(&nl_local_1.f0, nl_local_9);
    const uint8_t nl_local_10 = nl_local_2.f1;
    (void)nl_local_10;
    NL_LOOP_READ(&nl_local_2.f1, nl_local_10);
    const uint8_t nl_local_11 = nl_local_2.f0;
    (void)nl_local_11;
    NL_LOOP_READ(&nl_local_2.f0, nl_local_11);
    uint8_t nl_loan_result_2;
    {
        uint8_t *const nl_local_12 = &nl_local_1.f0;
        (void)nl_local_12;
        const uint8_t nl_call_34 = nl_fn_10(nl_local_12, nl_local_7);
        (void)nl_call_34;
        nl_loan_result_2 = nl_call_34;
    }
    (void)nl_loan_result_2;
    const uint8_t nl_local_15 = nl_loan_result_2;
    (void)nl_local_15;
    uint8_t nl_loan_result_3;
    {
        uint8_t *const nl_local_16 = &nl_local_2.f1;
        (void)nl_local_16;
        const uint8_t nl_call_41 = nl_fn_10(nl_local_16, nl_local_8);
        (void)nl_call_41;
        nl_loan_result_3 = nl_call_41;
    }
    (void)nl_loan_result_3;
    const uint8_t nl_local_19 = nl_loan_result_3;
    (void)nl_local_19;
    const uint8_t nl_local_20 = nl_local_1.f1;
    (void)nl_local_20;
    NL_LOOP_READ(&nl_local_1.f1, nl_local_20);
    const uint8_t nl_local_21 = nl_local_1.f0;
    (void)nl_local_21;
    NL_LOOP_READ(&nl_local_1.f0, nl_local_21);
    const uint8_t nl_local_22 = nl_local_2.f1;
    (void)nl_local_22;
    NL_LOOP_READ(&nl_local_2.f1, nl_local_22);
    const uint8_t nl_local_23 = nl_local_2.f0;
    (void)nl_local_23;
    NL_LOOP_READ(&nl_local_2.f0, nl_local_23);
    (void)0;
}

static uint8_t nl_fn_10(uint8_t *nl_local_4, uint8_t nl_local_5)
{
    (void)nl_local_5;
    (void)nl_local_4;
    const uint8_t nl_replace_old_2 = *nl_local_4;
    *nl_local_4 = nl_local_5;
    return nl_replace_old_2;
}

int main(void)
{
    nl_fn_11();
    return 0;
}
