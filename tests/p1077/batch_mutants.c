#include "../p1062/support.h"

static bool test(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    P1062Call calls[2];
    CHECK(registry(path, &base) && checked(base, &node) &&
          p1062_calls(&node, calls) && p1062_valid(calls));
    char *expected = NULL;
    size_t bytes = 0;
    CHECK(nl_checked_c_node(node.entry, &expected, &bytes) == NL_NODE_C_OK);
    NLCheckedFragment *f = calls[0].caller;
    NLCheckedNodeView *loan = NULL, *body = NULL, *write = NULL;
    for (size_t id = 1; id <= f->count; ++id) {
        NLCheckedNodeView *n = &f->nodes[id - 1];
        if (n->kind == NL_CHECKED_LOAN_HEADER && n->loan.implicit_local) {
            NLCheckedNodeView *b =
                (NLCheckedNodeView *)nl_checked_node_view(f, n->initializer);
            if (b && b->kind == NL_CHECKED_BLOCK && b->item_count) {
                CHECK(!loan);
                loan = n;
                body = b;
            }
        }
    }
    CHECK(loan && body);
    for (size_t id = 1; id <= f->count; ++id) {
        NLCheckedNodeView *n = &f->nodes[id - 1];
        if (n->kind == NL_CHECKED_REPLACE &&
            n->span.start_byte > body->span.start_byte &&
            n->span.end_byte < body->span.end_byte) {
            write = n;
            break;
        }
    }
    CHECK(write);
    NLCheckedNodeView saved_loan = *loan, saved_body = *body,
                      saved_write = *write;
    for (size_t fault = 0; fault < 7; ++fault) {
        if (fault == 0)
            write->field.index ^= 1;
        else if (fault == 1)
            write->field.child_fact = write->field.child_post_fact;
        else if (fault == 2)
            write->field.old_value = write->field.new_value;
        else if (fault == 3)
            loan->loan.ref_symbol = calls[0].parameters.parameters[0];
        else if (fault == 4)
            loan->loan.body_nonescape_proved = false;
        else if (fault == 5)
            body->tail = write->first_argument;
        else
            body->item_count += 1;
        NLCopyBodyView historical;
        CHECK(nl_checked_copy_body_view(f, calls[0].call, &historical) &&
              nl_checked_copy_body_validate(f, calls[0].call, &historical) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
              p1062_unpublished(&node));
        *loan = saved_loan;
        *body = saved_body;
        *write = saved_write;
        CHECK(p1062_valid(calls) && p1062_exact(&node, expected, bytes));
        printf("P1077 BATCH REJECT fault=%zu historical_GET=YES "
               "current=REFUSED staged=UNCHANGED exact_retry=YES\n",
               fault);
    }
    free(expected);
    node_checked_destroy(&node);
    nl_semantic_destroy(base);
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 2 && test(argv[1]) ? 0 : 1;
}
