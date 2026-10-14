/* R435 independent black-box first pass. Only public NewLang headers.
 * No synthetic semantic roots or P-owned source fixtures. */
#include "newlang/node_join.h"
#include "newlang/parser.h"
#include "newlang/source.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
} Witness;

static void drop(Witness *w) {
    nl_checked_destroy(w->artifact);
    nl_semantic_destroy(w->context);
    w->artifact = NULL;
    w->context = NULL;
}

static int read_source(const char *path, Witness *out) {
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSource *call_source = NULL;
    NLParser *call_parser = NULL;
    NLSyntaxTree *call_tree = NULL;
    NLSemanticContext *ctx = NULL;
    NLCheckedFragment *artifact = NULL;
    NLParseDiagnostic parse_diag = {0};
    NLFunctionUnitDiagnostic unit_diag = {0};
    NLCheckDiagnostic check_diag = {0};
    int status = 99;
    NLSourceStatus ss = nl_source_load(path, &source);
    if (ss != NL_SOURCE_OK) {
        printf("STAGE=load SOURCE_STATUS=%d\n", (int)ss);
        goto done;
    }
    NLParseStatus ps = nl_parser_create(source, &parser);
    if (ps != NL_PARSE_OK) {
        printf("STAGE=parser_create PARSE_STATUS=%d\n", (int)ps);
        goto done;
    }
    ps = nl_parser_parse_function_unit(parser, &tree, &parse_diag);
    if (ps != NL_PARSE_OK) {
        printf("STAGE=parse_unit PARSE_STATUS=%d CODE=%s\n", (int)ps,
               parse_diag.diagnostic.code ? parse_diag.diagnostic.code : "(none)");
        status = 1;
        goto done;
    }
    nl_parser_destroy(parser);
    parser = NULL;
    if (nl_semantic_create(&ctx) != NL_CHECK_OK) {
        printf("STAGE=context_create FAILURE\n");
        goto done;
    }
    const NLSyntaxTree *trees[1] = {tree};
    NLCheckStatus cs = nl_semantic_register_function_unit(ctx, trees, 1, &unit_diag);
    if (cs != NL_CHECK_OK) {
        printf("STAGE=register_unit CHECK_STATUS=%d CODE=%s INPUT_INDEX=%zu\n",
               (int)cs, unit_diag.diagnostic.diagnostic.code
                                ? unit_diag.diagnostic.diagnostic.code : "(none)",
               unit_diag.input_index);
        status = 2;
        goto done;
    }
    nl_syntax_tree_destroy(tree);
    tree = NULL;
    nl_source_destroy(source);
    source = NULL;
    const char *call = "main()";
    ss = nl_source_create(call, strlen(call), "r435_main_call", &call_source);
    if (ss != NL_SOURCE_OK) {
        printf("STAGE=call_source FAILURE=%d\n", (int)ss);
        goto done;
    }
    ps = nl_parser_create(call_source, &call_parser);
    if (ps != NL_PARSE_OK) {
        printf("STAGE=call_parser FAILURE=%d\n", (int)ps);
        goto done;
    }
    ps = nl_parser_parse_expression_fragment(call_parser, &call_tree, &parse_diag);
    if (ps != NL_PARSE_OK) {
        printf("STAGE=parse_call PARSE_STATUS=%d CODE=%s\n", (int)ps,
               parse_diag.diagnostic.code ? parse_diag.diagnostic.code : "(none)");
        status = 3;
        goto done;
    }
    cs = nl_semantic_check_expression(ctx, call_tree, &artifact, &check_diag);
    if (cs != NL_CHECK_OK) {
        printf("STAGE=check_main_call CHECK_STATUS=%d CODE=%s\n", (int)cs,
               check_diag.diagnostic.code ? check_diag.diagnostic.code : "(none)");
        status = 4;
        goto done;
    }
    printf("SOURCE_CHECKED path=%s nodes=%zu\n", path,
           nl_checked_node_count(artifact));
    status = 0;
done:
    nl_syntax_tree_destroy(call_tree);
    nl_parser_destroy(call_parser);
    nl_source_destroy(call_source);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    if (status == 0) {
        out->context = ctx;
        out->artifact = artifact;
    } else {
        nl_checked_destroy(artifact);
        nl_semantic_destroy(ctx);
    }
    return status;
}

typedef struct {
    const NLCheckedFragment *artifact;
    NLCheckedNodeId id;
} MatchSite;

static MatchSite find_match(const NLCheckedFragment *frag, unsigned depth) {
    MatchSite none = {NULL, 0};
    if (!frag || depth > 8) return none;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(frag); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(frag, id);
        if (n && n->kind == NL_CHECKED_MATCH && n->normal_arms == 2) {
            MatchSite site = {frag, id};
            return site;
        }
    }
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(frag); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(frag, id);
        if (!n) continue;
        if (n->kind == NL_CHECKED_REGISTERED_CALL) {
            MatchSite inside = find_match(nl_checked_call_body(frag, id), depth+1);
            if (inside.id) return inside;
        }
        if (n->kind == NL_CHECKED_MATCH) {
            for (size_t i = 0; i < 2; ++i) {
                MatchSite inside = find_match(nl_checked_match_arm(frag, id, i),
                                              depth+1);
                if (inside.id) return inside;
            }
        }
    }
    return none;
}

static int failures = 0;
static int cases = 0;
static void corrupt(const char *label, const NLCheckedFragment *owner,
                    NLCheckedNodeId id, const NLNodeJoinView *valid,
                    const NLNodeJoinView *bad) {
    if (memcmp(valid, bad, sizeof(*valid)) == 0) {
        printf("NEGATIVE=%s SKIP_IDENTICAL\n", label);
        return;
    }
    ++cases;
    NLCheckStatus result = nl_checked_node_join_validate(owner, id, bad);
    NLCheckStatus intact = nl_checked_node_join_validate(owner, id, valid);
    printf("NEGATIVE=%s STATUS=%d ORIGINAL_REVALIDATE=%d %s\n",
           label, (int)result, (int)intact,
           result != NL_CHECK_OK && intact == NL_CHECK_OK ? "REJECT" : "FINDING");
    if (result == NL_CHECK_OK || intact != NL_CHECK_OK) ++failures;
}
#define POISON(label, code) do {                                      \
    NLNodeJoinView bad = good;                                        \
    code;                                                             \
    corrupt(label, site.artifact, site.id, &good, &bad);               \
} while (0)

int main(void) {
    Witness positive = {0};
    int p = read_source("tests/red_team/r435/positive.nl", &positive);
    if (p != 0) {
        printf("R435_FINAL=SOURCE_PROFILE_BLOCKED FIRST_STAGE=%d\n", p);
        return 21;
    }
    MatchSite site = find_match(positive.artifact, 0);
    if (!site.id) {
        printf("R435_FINAL=SOURCE_PROFILE_BLOCKED REASON=no_two_normal_match\n");
        drop(&positive);
        return 21;
    }
    const NLCheckedNodeView *node = nl_checked_node_view(site.artifact, site.id);
    NLNodeJoinView good = {0};
    bool issued = nl_checked_node_join_view(site.artifact, site.id, &good);
    printf("JOIN_ISSUED=%d MATCH_ID=%zu NORMAL_ARMS=%zu FRAME_UNCHANGED=%d\n",
           (int)issued, (size_t)site.id, node ? node->normal_arms : 0,
           node ? (int)node->normal_frame_unchanged : -1);
    if (!issued) {
        printf("R435_FINAL=SOURCE_PROFILE_BLOCKED REASON=no_public_join_proof\n");
        drop(&positive);
        return 21;
    }
    NLCheckStatus owned = nl_checked_node_join_validate(site.artifact,site.id,NULL);
    NLCheckStatus copied = nl_checked_node_join_validate(site.artifact,site.id,&good);
    printf("PUBLIC_ISSUER_OWNER=%d COPY=%d ENTRY=%d POST=%d "
           "ARMS=%d,%d ORIGINALS=%zu,%zu CHANGED=%zu SIBLING=%zu\n",
           (int)owned, (int)copied, good.entry != NULL, good.post != NULL,
           good.arms[0].world != NULL, good.arms[1].world != NULL,
           good.originals[0], good.originals[1],good.changed,good.sibling);
    if (owned != NL_CHECK_OK || copied != NL_CHECK_OK) {
        printf("R435_FINAL=PUBLIC_PROOF_FINDING REASON=issued_view_not_valid\n");
        drop(&positive);
        return 22;
    }
    if (!good.entry || !good.post || !good.arms[0].world ||
        !good.arms[1].world || good.originals[0] == good.originals[1] ||
        good.changed == good.sibling) {
        printf("R435_FINAL=PUBLIC_PROOF_FINDING REASON=missing_world_or_original_identity\n");
        drop(&positive);
        return 22;
    }
    /* Only substitute real pointers borrowed from this already-issued view:
     * no invented addresses or dereference of foreign/arbitrary pointers. */
    POISON("entry_from_post", bad.entry = good.post);
    POISON("post_from_entry", bad.post = good.entry);
    POISON("foreign_arm_artifact", bad.arms[0].artifact = good.arms[1].artifact);
    POISON("foreign_arm_world", bad.arms[0].world = good.arms[1].world);
    POISON("foreign_arm_entry", bad.arms[0].entry = good.arms[1].entry);
    POISON("foreign_arm_before", bad.arms[0].before_change = good.arms[1].before_change);
    POISON("foreign_arm_after", bad.arms[0].after_change = good.arms[1].after_change);
    POISON("swap_arm_order", { bad.arms[0] = good.arms[1]; bad.arms[1] = good.arms[0]; });
    POISON("wrong_match", bad.match = good.match + 1);
    POISON("wrong_input", bad.input = good.input + 1);
    POISON("root_substitution", bad.originals[0] = good.originals[1]);
    POISON("changed_is_sibling", bad.changed = good.sibling);
    POISON("sibling_is_changed", bad.sibling = good.changed);
    POISON("wrong_variant", bad.arms[0].variant = good.arms[0].variant + 1);
    size_t change_arm = good.arms[0].operation ? 0 : 1;
    if (good.arms[change_arm].operation) {
        POISON("missing_change", bad.arms[change_arm].operation = 0);
        POISON("wrong_operation", bad.arms[change_arm].operation += 1);
        POISON("wrong_parent_fact", bad.arms[change_arm].change.parent_fact += 1);
        POISON("wrong_post_fact", bad.arms[change_arm].change.parent_post_fact += 1);
        POISON("wrong_child_fact", bad.arms[change_arm].change.child_post_fact += 1);
        POISON("wrong_occurrence", bad.arms[change_arm].change.post_payload_occurrence += 1);
        POISON("wrong_field_parent", bad.arms[change_arm].change.parent = good.sibling);
    } else {
        printf("CHANGE_OPERATION=ABSENT (no mutation proof)\n");
        ++failures;
    }
    printf("PUBLIC_NEGATIVE_CASES=%d FAILURES=%d\n", cases, failures);
    /* Separate actual source rejection, not a candidate-view forgery.
     * Escaping an ephemeral scoped ref should not become an authority. */
    Witness negative = {0};
    int n = read_source("tests/red_team/r435/negative_escape.nl",&negative);
    printf("SOURCE_NEGATIVE_RESULT=%d CLASS=%s\n", n,
           n == 0 ? "UNEXPECTED_ACCEPTANCE" :
           n == 1 ? "SYNTAX_OR_PROFILE" :
           n == 2 ? "DEFINITION_SEMANTIC_OR_PRECISION" :
           n == 4 ? "CALL_SEMANTIC_OR_PRECISION" : "ENVIRONMENT");
    if (n == 0) {
        ++failures;
        drop(&negative);
    }
    if (n == 99 || n == 3) ++failures;
    printf("R435_FINAL=%s\n", failures ? "PUBLIC_PROOF_FINDING" :
                                              "PUBLIC_NODE_JOIN_PROOF_PASS_TESTED_BOUNDARY");
    drop(&positive);
    return failures ? 22 : 0;
}
