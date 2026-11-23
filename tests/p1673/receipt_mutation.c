/* Test the private receipt without exporting a writable view from production.
 */
#include "../../src/usize_addition.c"

bool p1673_receipt_mutation_rejected(NLCheckedFragment *body,
                                     NLCheckedNodeId id)
{
    if (!nl_usize_multiplication_valid(body, id))
        return false;
    NLUsizeAdditionProof *receipt = body->usize_additions;
    while (receipt && receipt->issued.node != id)
        receipt = receipt->next;
    if (!receipt)
        return false;
    const size_t saved = receipt->issued.answer;
    receipt->issued.answer ^= 1;
    const bool rejected = !nl_usize_multiplication_valid(body, id);
    receipt->issued.answer = saved;
    return rejected && nl_usize_multiplication_valid(body, id);
}
