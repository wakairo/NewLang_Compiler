/* R809-owned independent public validator tamper test, source-issued views. */
#include "../p785/arm.h"
#include "newlang/copy_body.h"
#include <stdio.h>

static bool test(const char *filename)
{
    TestNode primary = {0}, foreign = {0};
    CHECK(node_checked_load(filename, &primary));
    CHECK(node_checked_load(filename, &foreign));
    ArmCall originals[8], aliens[8];
    size_t a_count = 0, b_count = 0;
    CHECK(collect(primary.entry, NULL, 0, 0, originals, &a_count, 0));
    CHECK(collect(foreign.entry, NULL, 0, 0, aliens, &b_count, 0));
    CHECK(a_count == 2 && b_count == 2);
    for (size_t i = 0; i < a_count; ++i) {
        ArmCall a = originals[i], b = aliens[i];
        CHECK(a.caller != b.caller && a.call == b.call);
        NLCopyBodyView body = {0}, other_body = {0};
        NLCopyBodyParameters par = {0}, other_par = {0};
        CHECK(valid(a) && valid(b));
        CHECK(nl_checked_copy_body_view(a.caller, a.call, &body));
        CHECK(nl_checked_copy_body_parameters(a.caller, a.call, &par));
        CHECK(nl_checked_copy_body_view(b.caller, b.call, &other_body));
        CHECK(nl_checked_copy_body_parameters(b.caller, b.call, &other_par));
        /* Equal numeric call and place IDs are not enough across issuer/world.
         */
        CHECK(par.field.parent == other_par.field.parent);
        CHECK(nl_checked_copy_body_validate(a.caller, a.call, &other_body) !=
              NL_CHECK_OK);
        CHECK(nl_checked_copy_body_parameters_validate(
                  a.caller, a.call, &other_par) != NL_CHECK_OK);
        NLCopyBodyParameters corrupt = par;
        corrupt.entry = other_par.entry;
        CHECK(nl_checked_copy_body_parameters_validate(
                  a.caller, a.call, &corrupt) != NL_CHECK_OK);
        corrupt = par;
        corrupt.handoff = other_par.handoff;
        CHECK(nl_checked_copy_body_parameters_validate(
                  a.caller, a.call, &corrupt) != NL_CHECK_OK);
        corrupt = par;
        corrupt.field.index = 1 - par.field.index;
        CHECK(nl_checked_copy_body_parameters_validate(
                  a.caller, a.call, &corrupt) != NL_CHECK_OK);
        corrupt = par;
        ++corrupt.field.child_fact;
        CHECK(nl_checked_copy_body_parameters_validate(
                  a.caller, a.call, &corrupt) != NL_CHECK_OK);
        corrupt = par;
        ++corrupt.loan;
        CHECK(nl_checked_copy_body_parameters_validate(
                  a.caller, a.call, &corrupt) != NL_CHECK_OK);
        NLSemanticPlaceView parent = {0}, child = {0};
        CHECK(nl_semantic_place_view(par.handoff, par.field.parent, &parent));
        CHECK(nl_semantic_place_view(par.handoff, par.field.child, &child));
        NLCopyBodyView stale = body;
        stale.result.value = child.current_value;
        CHECK(nl_checked_copy_body_validate(a.caller, a.call, &stale) !=
              NL_CHECK_OK);
        CHECK(valid(a)); /* Never change the genuine issuer view. */
        printf("R809_ALIEN_COPY call=%zu field_index=%zu old-world-only "
               "status=REJECTED\n",
               (size_t)a.call, par.field.index);
    }
    node_checked_destroy(&foreign);
    node_checked_destroy(&primary);
    return true;
}

int main(int argc, char **argv)
{
    return argc == 2 && test(argv[1]) ? 0 : 1;
}
