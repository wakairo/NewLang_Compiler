/* Test-private issuer faults using the real implementation, not a production
 * hook or alternate proof issuer. Archive evidence remains P-side, not R. */
#include "../../src/usize_addition.c"
#include "../support/node_checked.h"

static bool core(const char *path, NLUsizeOperation operation)
{
    TestNode n = {0}, foreign = {0};
    CHECK(node_checked_load(path, &n) && node_checked_load(path, &foreign));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    const NLCheckedFragment *other =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    size_t id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == (operation == NL_USIZE_ADD
                            ? NL_CHECKED_USIZE_ADDITION
                            : NL_CHECKED_USIZE_SUBTRACTION))
            id = i;
    }
    CHECK(id);
    NLUsizeAdditionProof *p = (NLUsizeAdditionProof *)find(f, id);
    const NLUsizeAdditionProof *q = find(other, id);
    CHECK(p && q && p != q && p->issued.node == q->issued.node &&
          p->issued.result == q->issued.result &&
          p->issued.origin != q->issued.origin);
    CHECK(arithmetic_valid(f, id, operation, NULL));
    const NLUsizeAdditionProof saved = *p;
    size_t refused = 0;
#define FAULT(change)                                                          \
    do {                                                                       \
        change;                                                                \
        CHECK(!arithmetic_valid(f, id, operation, NULL));                      \
        CHECK(!nl_checked_usize_addition_valid(f, id, NULL) &&                 \
              !nl_checked_usize_subtraction_valid(f, id, NULL));               \
        *p = saved;                                                            \
        CHECK(arithmetic_valid(f, id, operation, NULL));                       \
        ++refused;                                                             \
    } while (0)
    FAULT(p->operation =
              operation == NL_USIZE_ADD ? NL_USIZE_SUBTRACT : NL_USIZE_ADD);
    FAULT(p->operation = (NLUsizeOperation)99);
    FAULT(p->issued.answer ^= 1);
    FAULT(p->issued.origin = other);
    FAULT(p->context = other->context);
    FAULT(p->syntax = q->syntax);
    FAULT(p->issued.type = 1);
    FAULT(p->issued.values[0] = p->issued.values[1]);
    FAULT(p->issued.operator_span.start_byte += 1);
    FAULT(p->issued.operands[0] = p->issued.operands[1]);
    if (operation == NL_USIZE_SUBTRACT) {
        FAULT((p->issued.numbers[0] = 0, p->issued.numbers[1] = 1,
               p->issued.answer = SIZE_MAX));
        NLUsizeSubtractionView v, alt;
        CHECK(nl_checked_usize_subtraction_view(f, id, &v));
        CHECK(nl_checked_usize_subtraction_view(other, id, &alt));
        CHECK(!nl_checked_usize_subtraction_valid(f, id, &alt));
        CHECK(nl_checked_usize_subtraction_valid(f, id, &v));
        NLUsizeAdditionView legacy = {0}, before = legacy;
        CHECK(!nl_checked_usize_addition_view(f, id, &legacy) &&
              memcmp(&legacy, &before, sizeof before) == 0);
        CHECK(!nl_checked_usize_addition_valid(f, id, NULL));
    } else {
        FAULT((p->issued.numbers[0] = SIZE_MAX, p->issued.numbers[1] = 1,
               p->issued.answer = 0));
        NLUsizeAdditionView v, alt;
        CHECK(nl_checked_usize_addition_view(f, id, &v));
        CHECK(nl_checked_usize_addition_view(other, id, &alt));
        CHECK(!nl_checked_usize_addition_valid(f, id, &alt));
        CHECK(nl_checked_usize_addition_valid(f, id, &v) && v.sum == 11);
        NLUsizeSubtractionView minus = {0}, before = minus;
        CHECK(!nl_checked_usize_subtraction_view(f, id, &minus) &&
              memcmp(&minus, &before, sizeof before) == 0);
        CHECK(!nl_checked_usize_subtraction_valid(f, id, NULL));
    }
#undef FAULT
    printf("op=%s actual-private-receipt-refusals=%zu; "
           "foreign-same-number-registered=REFUSED; cross-api=REFUSED\n",
           operation == NL_USIZE_ADD ? "+" : "-", refused);
    node_checked_destroy(&foreign);
    node_checked_destroy(&n);
    return true;
}
static bool boundaries(void)
{
    CHECK(arithmetic_result(NL_USIZE_SUBTRACT, 7, 4, 3));
    CHECK(!arithmetic_result(NL_USIZE_SUBTRACT, 4, 7, SIZE_MAX - 2));
    CHECK(!arithmetic_result(NL_USIZE_SUBTRACT, 0, 1, SIZE_MAX));
    CHECK(arithmetic_result(NL_USIZE_ADD, SIZE_MAX, 0, SIZE_MAX));
    CHECK(!arithmetic_result(NL_USIZE_ADD, SIZE_MAX, 1, 0));
    CHECK(!arithmetic_result((NLUsizeOperation)99, 0, 0, 0));
    printf("receipt=%zu fragment=%zu plus-view=%zu minus-view=%zu; "
           "guarded-math=PASS\n",
           sizeof(NLUsizeAdditionProof), sizeof(NLCheckedFragment),
           sizeof(NLUsizeAdditionView), sizeof(NLUsizeSubtractionView));
    return true;
}
/* Three completed flat expressions, never mixed/chained operators within one
 * expression. Exercise lookup/destruction through the ONE shared event list. */
static bool sequence(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    const size_t expected[] = {15, 11, 19};
    size_t count = 0;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v->kind == NL_CHECKED_USIZE_ADDITION) {
            NLUsizeAdditionView view;
            CHECK(count < 3 && count != 1 &&
                  nl_checked_usize_addition_view(f, id, &view));
            CHECK(nl_checked_usize_addition_valid(f, id, &view) &&
                  view.sum == expected[count]);
            CHECK(!nl_checked_usize_subtraction_valid(f, id, NULL));
            ++count;
        } else if (v->kind == NL_CHECKED_USIZE_SUBTRACTION) {
            NLUsizeSubtractionView view;
            CHECK(count == 1 &&
                  nl_checked_usize_subtraction_view(f, id, &view));
            CHECK(nl_checked_usize_subtraction_valid(f, id, &view) &&
                  view.difference == expected[count]);
            CHECK(!nl_checked_usize_addition_valid(f, id, NULL));
            ++count;
        }
    }
    CHECK(count == 3);
    puts("one-shared-list +,-,+ authentic-source-events=3; results=15,11,19; "
         "cross-api=REFUSED");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 5 && boundaries() && core(argv[1], NL_USIZE_SUBTRACT) &&
                   core(argv[2], NL_USIZE_ADD) &&
                   core(argv[3], NL_USIZE_SUBTRACT) && sequence(argv[4])
               ? 0
               : 1;
}
