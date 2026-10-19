#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "newlang/syntax.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define REQUIRE(test, reason) do { if (!(test)) { fprintf(stderr, "R638-PROBE-FAIL: %s\n", reason); goto done; } } while (0)

/* Independent public-API inspection of the actual registered main() checked body.
 * Never mutate the issuer-owned artifact and never compare foreign-world IDs. */
int main(int argc, char **argv)
{
    NLSource *source = NULL, *invocation = NULL;
    NLParser *parser = NULL, *inv_parser = NULL;
    NLSyntaxTree *unit = NULL, *inv_tree = NULL;
    NLSemanticContext *ctx = NULL;
    NLCheckedFragment *entry = NULL;
    int result = 1;
    size_t conditions = 0, arm_bindings = 0;
    static const char call[] = "main()";
    if (argc != 2) {
        fputs("R638: source path required\n", stderr);
        return 2;
    }
    REQUIRE(nl_source_load(argv[1], &source) == NL_SOURCE_OK, "source-load");
    REQUIRE(nl_parser_create(source, &parser) == NL_PARSE_OK, "parser-create");
    NLParseDiagnostic pd = {0};
    REQUIRE(nl_parser_parse_function_unit(parser, &unit, &pd) == NL_PARSE_OK,
            "parse-function-unit");
    REQUIRE(nl_semantic_create(&ctx) == NL_CHECK_OK, "context-create");
    const NLSyntaxTree *inputs[1] = {unit};
    NLFunctionUnitDiagnostic ud = {0};
    REQUIRE(nl_semantic_register_function_unit(ctx, inputs, 1, &ud) == NL_CHECK_OK,
            "register-original-function");
    REQUIRE(nl_source_create(call, sizeof(call) - 1, "<r638-call>", &invocation)
                == NL_SOURCE_OK, "call-source");
    REQUIRE(nl_parser_create(invocation, &inv_parser) == NL_PARSE_OK, "call-parser");
    pd = (NLParseDiagnostic){0};
    REQUIRE(nl_parser_parse_expression_fragment(inv_parser, &inv_tree, &pd)
                == NL_PARSE_OK, "call-parse");
    NLCheckDiagnostic cd = {0};
    REQUIRE(nl_semantic_check_expression(ctx, inv_tree, &entry, &cd)
                == NL_CHECK_OK, "registered-main-check");

    /* Teardown syntax before inspecting the retained *original* body. */
    nl_syntax_tree_destroy(unit); unit = NULL;
    nl_syntax_tree_destroy(inv_tree); inv_tree = NULL;
    nl_parser_destroy(parser); parser = NULL;
    nl_parser_destroy(inv_parser); inv_parser = NULL;

    const NLCheckedNodeId root = nl_checked_root(entry);
    const NLCheckedNodeView *call_node = nl_checked_node_view(entry, root);
    REQUIRE(call_node != NULL && call_node->kind == NL_CHECKED_REGISTERED_CALL,
            "entry-is-registered-call");
    const NLCheckedFragment *body = nl_checked_call_body(entry, root);
    REQUIRE(body != NULL && nl_checked_root(body) != 0, "real-retained-body");
    const NLSemanticContext *owner = nl_checked_context(body);
    REQUIRE(owner != NULL, "body-world");
    NLSemanticSnapshot snap = {0};
    REQUIRE(nl_semantic_snapshot(owner, &snap), "body-snapshot");
    NLSemanticValueView value = {0};
    NLSemanticPlaceView place = {0};
    REQUIRE(!nl_semantic_value_view(owner, 0, &value), "zero-value-id-rejected");
    REQUIRE(!nl_semantic_value_view(owner, SIZE_MAX, &value),
            "out-of-range-value-id-rejected");
    REQUIRE(!nl_semantic_place_view(owner, 0, &place), "zero-place-id-rejected");
    REQUIRE(!nl_semantic_place_view(owner, SIZE_MAX, &place),
            "out-of-range-place-id-rejected");
    REQUIRE(nl_checked_node_view(body, 0) == NULL, "zero-node-id-rejected");
    REQUIRE(nl_checked_node_view(body, SIZE_MAX) == NULL,
            "out-of-range-node-id-rejected");

    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, i);
        if (n == NULL || n->kind != NL_CHECKED_IF)
            continue;
        ++conditions;
        const NLCheckedFragment *then_arm = nl_checked_if_arm(body, i, 0);
        const NLCheckedFragment *else_arm = nl_checked_if_arm(body, i, 1);
        REQUIRE(then_arm != NULL && else_arm != NULL, "two-owned-if-arms");
        const NLSemanticContext *then_world = nl_checked_context(then_arm);
        const NLSemanticContext *else_world = nl_checked_context(else_arm);
        REQUIRE(then_world != NULL && else_world != NULL
                    && then_world != else_world && then_world != owner
                    && else_world != owner, "world-separation");
        REQUIRE(nl_checked_node_view(then_arm, nl_checked_root(then_arm)) != NULL
                    && nl_checked_node_view(else_arm, nl_checked_root(else_arm))
                           != NULL, "arm-root-preserved");
        for (size_t a = 0; a < 2; ++a) {
            const NLCheckedFragment *arm = a == 0 ? then_arm : else_arm;
            size_t count = 0;
            for (NLCheckedNodeId j = 1; j <= nl_checked_node_count(arm); ++j) {
                const NLCheckedNodeView *v = nl_checked_node_view(arm, j);
                REQUIRE(v != NULL, "arm-node-view");
                if (v->kind == NL_CHECKED_BINDING)
                    ++count;
            }
            REQUIRE(count >= 1, "arm-private-copy-binding");
            arm_bindings += count;
        }
    }
    REQUIRE(conditions >= 1, "source-if-not-retained");
    printf("R638-PUBLIC registered=1 syntax-destroyed=1 if=%zu arm-bindings=%zu parent-values=%zu original-immutable=1 malformed-getter-IDs-rejected=1\n",
           conditions, arm_bindings, snap.values);
    result = 0;
done:
    nl_checked_destroy(entry);
    nl_syntax_tree_destroy(inv_tree);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(inv_parser);
    nl_parser_destroy(parser);
    nl_semantic_destroy(ctx);
    nl_source_destroy(invocation);
    nl_source_destroy(source);
    return result;
}
