#include "../support/semantic_check.h"

#include <stdlib.h>

static bool pipeline(void)
{
    /* Two independent contexts execute the same source->P2->P3 sequence.
     * Helpers destroy P2 trees before every checked artifact is inspected. */
    TestSemantic a = {0}, b = {0};
    CHECK(test_semantic_create(&a) && test_semantic_create(&b));
    const char *texts[] = {
        "ptr<LinearT>", "let d = LifetimeDomain()", "let moved = d",
        "finalize_domain(moved)",
        "loan exclusive read life as ending { body_is_opaque() }"};
    const TestEntry entries[] = {TEST_TYPE, TEST_BINDING, TEST_BINDING,
                                 TEST_EXPRESSION, TEST_LOAN};
    for (size_t i = 0; i < sizeof(texts) / sizeof(texts[0]); ++i) {
        TestState before;
        CHECK(test_state(b.context, &before));
        TestChecked first = {0}, second = {0};
        CHECK(test_run(a.context, texts[i], entries[i], NL_CHECK_OK, NULL,
                       &first));
        CHECK(test_unchanged(b.context, &before));
        CHECK(test_run(b.context, texts[i], entries[i], NL_CHECK_OK, NULL,
                       &second));
        const NLCheckedNodeView x = *test_root(&first), y = *test_root(&second);
        CHECK(x.kind == y.kind && x.type == y.type &&
              x.result_count == y.result_count &&
              x.argument_count == y.argument_count);
        CHECK(x.span.start_byte == y.span.start_byte &&
              x.span.end_byte == y.span.end_byte &&
              nl_checked_node_count(first.artifact) ==
                  nl_checked_node_count(second.artifact));
        test_checked_destroy(&first);
        test_checked_destroy(&second);
    }
    TestChecked failed = {0};
    CHECK(test_run(a.context, "take()", TEST_EXPRESSION,
                   NL_CHECK_SEMANTIC_ERROR, "P3-ARITY", &failed));
    CHECK(failed.diagnostic.span.start_byte == 0 &&
          failed.diagnostic.span.end_byte == 4);
    FILE *stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(
        nl_check_diagnostic_render(stream, failed.source, &failed.diagnostic));
    CHECK(fflush(stream) == 0 && fseek(stream, 0, SEEK_SET) == 0);
    char rendered[512];
    const size_t count = fread(rendered, 1, sizeof(rendered) - 1, stream);
    rendered[count] = 0;
    CHECK(strstr(rendered, "P3-ARITY") != NULL &&
          strstr(rendered, "semantic-fixture:1:1") != NULL);
    CHECK(fclose(stream) == 0);
    test_checked_destroy(&failed);
    NLSymbolId affine, copy;
    CHECK(nl_semantic_seed_value(a.context, "affine", a.linear,
                                 NL_DEPENDENCY_FREE, &affine) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(a.context, "copy", a.copy, NL_DEPENDENCY_FREE,
                                 &copy) == NL_CHECK_OK);
    const NLTypeId parameters[] = {a.linear, a.linear};
    CHECK(nl_semantic_register_function(a.context, "pair", parameters, 2,
                                        a.copy, false, false) == NL_CHECK_OK);
    TestState before_failure;
    CHECK(test_state(a.context, &before_failure));
    CHECK(test_run(a.context, "pair(affine,copy)", TEST_EXPRESSION,
                   NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH", &failed));
    CHECK(failed.diagnostic.span.start_byte == 12 &&
          failed.diagnostic.span.end_byte == 16);
    CHECK(test_unchanged(a.context, &before_failure));
    test_checked_destroy(&failed);
    /* Destruction never dereferences borrowed context/source. Views/spans can
     * still be copied from the owned artifact; ID/text lookups would be
     * invalid. */
    TestChecked survivor = {0};
    CHECK(
        test_run(a.context, "CopyT", TEST_TYPE, NL_CHECK_OK, NULL, &survivor));
    nl_semantic_destroy(a.context);
    nl_source_destroy(survivor.source);
    CHECK(test_root(&survivor)->kind == NL_CHECKED_TYPE);
    nl_checked_destroy(survivor.artifact);
    nl_semantic_destroy(b.context);
    return true;
}

int main(void)
{
    if (!pipeline()) {
        return EXIT_FAILURE;
    }
    puts("source / P2 syntax / P3 checked frontend: PASS");
    return EXIT_SUCCESS;
}
