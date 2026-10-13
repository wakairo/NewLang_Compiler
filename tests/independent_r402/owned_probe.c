/* R405 NON-BLIND calibrated owned second pass.
 * Original R402 independent evidence is preserved in PR403. */
#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    NLSource *source;
    NLSyntaxTree *tree;
} Input;

static void release_input(Input *input)
{
    nl_syntax_tree_destroy(input->tree);
    nl_source_destroy(input->source);
    input->tree = NULL;
    input->source = NULL;
}

static int parse_input(const char *text, bool function_unit, Input *out)
{
    NLParser *parser = NULL;
    NLParseDiagnostic diagnostic = {0};
    out->source = NULL;
    out->tree = NULL;
    NLSourceStatus source_status =
        nl_source_create(text, strlen(text), "r402-independent", &out->source);
    if (source_status != NL_SOURCE_OK) {
        printf("R402 owned source_create=%d\n", (int)source_status);
        return 0;
    }
    NLParseStatus status = nl_parser_create(out->source, &parser);
    if (status != NL_PARSE_OK) {
        printf("R402 owned parser_create=%d\n", (int)status);
        release_input(out);
        return 0;
    }
    NLSyntaxTree **destination = &out->tree;
    NLParseDiagnostic *error = &diagnostic;
    if (function_unit) {
        status = nl_parser_parse_function_unit(parser, destination, error);
    } else {
        status = nl_parser_parse_source_fragment(parser, destination, error);
    }
    nl_parser_destroy(parser);
    if (status != NL_PARSE_OK) {
        printf("R402 owned parse=%d code=%s\n", (int)status,
               diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
        release_input(out);
        return 0;
    }
    return 1;
}

static void required(int result, const char *condition, int line)
{
    if (!result) {
        printf("R405 FAIL line=%d condition=%s\n", line, condition);
        fflush(stdout);
        exit(1);
    }
}
#define REQUIRE(expr) required(!!(expr), #expr, __LINE__)

static int equal_locator(NLReferenceFacts x, NLReferenceFacts y)
{
    return x.place == y.place && x.incarnation == y.incarnation &&
           x.provenance == y.provenance && x.readable == y.readable &&
           x.writable == y.writable && x.scope == y.scope &&
           x.occurrence_dependency == y.occurrence_dependency;
}

/* Read only source-generated public views. No semantic seeding or private IDs.
 * Arm-local facts are NEVER treated as ancestor identities; the two original
 * roots below are obtained from the common caller's checked binding nodes. */
static int observe_owned(const NLCheckedFragment *call, int same)
{
    const NLCheckedFragment *body = NULL;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(call); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(call, i);
        if (n != NULL && n->kind == NL_CHECKED_REGISTERED_CALL &&
            n->body_backed) {
            REQUIRE(body == NULL);
            body = nl_checked_call_body(call, i);
        }
    }
    REQUIRE(body != NULL);
    const NLSemanticContext *parent = nl_checked_context(body);
    REQUIRE(parent != NULL && parent == nl_checked_context(call));
    NLPlaceId original[2] = {0};
    NLIncarnationId incarnation[2] = {0};
    NLCheckedNodeId match = 0;
    size_t roots = 0, post_reads = 0, second_writes = 0;
    const NLCheckedNodeView *post_read = NULL;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, i);
        REQUIRE(n != NULL);
        if (n->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *received =
                nl_checked_node_view(body, n->first_argument);
            NLSemanticTypeView type;
            if (received == NULL ||
                !nl_semantic_type_view(parent, received->type, &type))
                continue;
            if (type.kind == NL_TYPE_NOMINAL && type.field_count == 2) {
                NLSemanticBindingView binding;
                NLSemanticPlaceView root;
                REQUIRE(roots < 2);
                NLSymbolId symbol = received->symbol;
                REQUIRE(nl_semantic_binding_view(parent, symbol, &binding));
                REQUIRE(nl_semantic_place_view(parent, binding.place, &root));
                REQUIRE(root.independent_root && root.implicit_local &&
                        root.placement.region == 0 && root.incarnation != 0);
                original[roots] = binding.place;
                incarnation[roots++] = root.incarnation;
            }
        }
        if (n->kind == NL_CHECKED_MATCH && !n->normal_frame_unchanged) {
            REQUIRE(match == 0 && n->normal_arms == 2 && !n->terminates);
            match = i;
        }
        if (match != 0 && i > match && n->kind == NL_CHECKED_FIELD_READ) {
            if (n->field.parent == original[0]) {
                NLSemanticValueView v;
                REQUIRE(n->field.present);
                REQUIRE(n->field.parent_incarnation == incarnation[0]);
                REQUIRE(n->field.parent_fact != 0);
                REQUIRE(n->field.child_fact != 0);
                NLValueId value = n->results[0].value;
                REQUIRE(nl_semantic_value_view(parent, value, &v));
                if (post_read == NULL) {
                    post_read = n;
                    REQUIRE(v.variant == 0 && v.sum_payload == 0);
                }
                ++post_reads;
            }
        }
        if (match != 0 && i > match && n->kind == NL_CHECKED_REPLACE &&
            n->field.parent == original[0]) {
            REQUIRE(n->field.present && n->field.parent_fact != 0 &&
                    n->field.parent_fact != n->field.parent_post_fact &&
                    n->field.child_fact != n->field.child_post_fact &&
                    n->field.parent_incarnation == incarnation[0]);
            ++second_writes;
        }
    }
    REQUIRE(roots == 2 && original[0] != original[1]);
    REQUIRE(incarnation[0] != 0 && incarnation[1] != 0 && match != 0);
    REQUIRE(post_read != NULL && post_reads >= 2 && second_writes >= 1);
    size_t replacements[2] = {0};
    size_t variants[2] = {0};
    NLValueFactId sibling_fact = 0;
    const NLSemanticContext *worlds[2] = {0};
    for (size_t arm_index = 0; arm_index < 2; ++arm_index) {
        const NLCheckedFragment *arm =
            nl_checked_match_arm(body, match, arm_index);
        REQUIRE(arm != NULL);
        worlds[arm_index] = nl_checked_context(arm);
        REQUIRE(worlds[arm_index] != NULL && worlds[arm_index] != parent);
        const NLCheckedNodeView *aroot =
            nl_checked_node_view(arm, nl_checked_root(arm));
        REQUIRE(aroot != NULL && aroot->kind == NL_CHECKED_MATCH_ARM &&
                !aroot->terminates);
        for (size_t r = 0; r < 2; ++r) {
            NLSemanticPlaceView p;
            REQUIRE(nl_semantic_place_view(worlds[arm_index], original[r], &p));
            REQUIRE(p.independent_root && p.incarnation == incarnation[r]);
            REQUIRE(p.fixed_field_count == 2 && p.placement.region == 0);
        }
        NLSemanticPlaceView root, link, sibling;
        NLSemanticValueView payload;
        REQUIRE(nl_semantic_place_view(worlds[arm_index], original[0], &root));
        REQUIRE(nl_semantic_place_view(worlds[arm_index], root.fixed_fields[0],
                                       &link));
        REQUIRE(nl_semantic_place_view(worlds[arm_index], root.fixed_fields[1],
                                       &sibling));
        REQUIRE(link.parent_aggregate == original[0] &&
                link.parent_incarnation == incarnation[0]);
        REQUIRE(sibling.parent_aggregate == original[0]);
        NLValueId sibling_value = sibling.current_value;
        const NLSemanticContext *world = worlds[arm_index];
        REQUIRE(nl_semantic_value_view(world, sibling_value, &payload));
        REQUIRE(payload.scalar_known && payload.scalar_value == 1);
        if (sibling_fact == 0)
            sibling_fact = sibling.current_fact;
        REQUIRE(sibling.current_fact == sibling_fact);
        NLSemanticValueView sum;
        NLValueId current_value = link.current_value;
        REQUIRE(nl_semantic_value_view(worlds[arm_index], current_value, &sum));
        variants[arm_index] = sum.variant;
        REQUIRE((arm_index == 0 ? 2u : (same ? 2u : 1u)) == sum.variant);
        for (NLCheckedNodeId k = 1; k <= nl_checked_node_count(arm); ++k) {
            const NLCheckedNodeView *n = nl_checked_node_view(arm, k);
            REQUIRE(n != NULL);
            if (n->kind != NL_CHECKED_REPLACE)
                continue;
            REQUIRE(n->field.present);
            REQUIRE(n->field.parent == original[0] &&
                    n->field.child == root.fixed_fields[0] &&
                    n->field.parent_incarnation == incarnation[0] &&
                    n->field.child_incarnation == link.incarnation &&
                    n->field.parent_fact != n->field.parent_post_fact &&
                    n->field.child_fact != n->field.child_post_fact);
            REQUIRE(n->field.parent_post_fact != 0 &&
                    n->field.child_post_fact != 0);
            NLSemanticValueView old, next;
            REQUIRE(nl_semantic_value_view(worlds[arm_index],
                                           n->field.old_value, &old));
            REQUIRE(nl_semantic_value_view(worlds[arm_index],
                                           n->field.new_value, &next));
            REQUIRE(next.variant == variants[arm_index]);
            if (same) {
                /* Exact same Copy locator before/after the whole Option change
                 * must NOT revive the ended Some occurrence or old facts. */
                NLSemanticValueView op, np;
                NLSemanticOccurrenceView ended, fresh;
                REQUIRE(old.variant == 2 && next.variant == 2);
                REQUIRE(old.sum_payload != 0 && next.sum_payload != 0);
                REQUIRE(nl_semantic_value_view(worlds[arm_index],
                                               old.sum_payload, &op));
                REQUIRE(nl_semantic_value_view(worlds[arm_index],
                                               next.sum_payload, &np));
                REQUIRE(equal_locator(op.reference, np.reference));
                REQUIRE(n->field.payload_occurrence != 0 &&
                        n->field.post_payload_occurrence != 0 &&
                        n->field.payload_occurrence !=
                            n->field.post_payload_occurrence);
                REQUIRE(nl_semantic_occurrence_view(
                    worlds[arm_index], n->field.payload_occurrence, &ended));
                REQUIRE(!ended.live);
                NLOccurrenceId new_occ = n->field.post_payload_occurrence;
                REQUIRE(nl_semantic_occurrence_view(worlds[arm_index], new_occ,
                                                    &fresh));
                REQUIRE(fresh.live && fresh.variant == 2 &&
                        fresh.root == root.fixed_fields[0]);
                printf("R405 same-locator arm=%zu old-occur=%zu ended=%d "
                       "new-occur=%zu live=%d child-fact=%zu->%zu "
                       "parent-fact=%zu->%zu\n",
                       arm_index, n->field.payload_occurrence, !ended.live,
                       n->field.post_payload_occurrence, fresh.live,
                       n->field.child_fact, n->field.child_post_fact,
                       n->field.parent_fact, n->field.parent_post_fact);
            } else if (arm_index == 0) {
                REQUIRE(old.variant == 1 && next.variant == 2 &&
                        n->field.post_payload_occurrence != 0);
            } else {
                REQUIRE(old.variant == 1 && next.variant == 1);
            }
            ++replacements[arm_index];
        }
        REQUIRE(replacements[arm_index] == 1);
    }
    REQUIRE(worlds[0] != worlds[1] && sibling_fact != 0);
    REQUIRE(post_read->field.parent == original[0]);
    REQUIRE(post_read->field.child != 0);
    REQUIRE(post_read->field.parent_incarnation == incarnation[0]);
    REQUIRE(post_read->field.child_fact != 0);
    REQUIRE(post_read->field.parent_fact != 0);
    printf("R405 OWNED PASS same=%d original_roots=%zu,%zu "
           "incarnations=%zu,%zu normal_arms=2 worlds=separate "
           "arm_variants=%zu,%zu child_join_fact=%zu parent_join_fact=%zu "
           "sibling_fact=%zu parent_second_writes=%zu post_reads=%zu\n",
           same, original[0], original[1], incarnation[0], incarnation[1],
           variants[0], variants[1], post_read->field.child_fact,
           post_read->field.parent_fact, sibling_fact, second_writes,
           post_reads);
    return 1;
}

static const char good[] =
    "struct Node { next: Option<ptr<Node>>, payload: u8, }\n"
    "fn main()->unit {\n"
    " let a=Node{next:Option<ptr<Node>>::None,payload:u8(1)};\n"
    " let b=Node{next:Option<ptr<Node>>::None,payload:u8(2)};\n"
    " let pa=loan_read(a){|ra|ptr_from_ref(ra)};\n"
    " let pb=loan_read(b){|rb|ptr_from_ref(rb)};\n"
    " let option=Option<ptr<Node>>::Some(pa);\n"
    " match option {\n"
    "   Some(q)=>{loan_write(a@next){|w|replace(w,"
    "Option<ptr<Node>>::Some(pb))};unit},\n"
    "   None=>{loan_write(a@next){|w|replace(w,"
    "Option<ptr<Node>>::None)};unit}\n"
    " };\n"
    " let after=a@next;\n"
    " loan_write(a@next){|w|replace(w,Option<ptr<Node>>::None)};\n"
    " let final_copy=a@next;\n"
    " unit\n"
    "}\n";
static const char same_locator[] =
    "struct Node { next: Option<ptr<Node>>, payload: u8, }\n"
    "fn main()->unit {\n"
    " let a=Node{next:Option<ptr<Node>>::None,payload:u8(1)};\n"
    " let b=Node{next:Option<ptr<Node>>::None,payload:u8(2)};\n"
    " let pa=loan_read(a){|ra|ptr_from_ref(ra)};\n"
    " let pb=loan_read(b){|rb|ptr_from_ref(rb)};\n"
    " let seeded=loan_write(a@next){|w|replace(w,"
    "Option<ptr<Node>>::Some(pa))};\n"
    " let option=Option<ptr<Node>>::Some(pa);\n"
    " match option {\n"
    "   Some(q)=>{loan_write(a@next){|w|replace(w,"
    "Option<ptr<Node>>::Some(pa))};unit},\n"
    "   None=>{loan_write(a@next){|w|replace(w,"
    "Option<ptr<Node>>::Some(pa))};unit}\n"
    " };\n"
    " let after=a@next;\n"
    " loan_write(a@next){|w|replace(w,Option<ptr<Node>>::None)};\n"
    " let final_copy=a@next;\n"
    " unit\n"
    "}\n";
static const char bad_registration[] =
    "fn rejected()->unit {\n"
    " let a=Node{next:Option<ptr<Node>>::None,payload:u8(1)};\n"
    " let b=Node{next:Option<ptr<Node>>::None,payload:u8(2)};\n"
    " let pa=loan_read(a){|ra|ptr_from_ref(ra)};\n"
    " let opt=Option<ptr<Node>>::Some(pa);\n"
    " match opt {Some(q)=>{unit}, None=>{r405_undefined_name;unit}};\n"
    " unit\n"
    "}\n";

static int run_case(const char *unit_text, int same)
{
    Input unit = {0}, invocation = {0}, bad = {0};
    NLSemanticContext *ctx = NULL;
    NLCheckedFragment *checked = NULL;
    NLCheckDiagnostic diagnostic = {0};
    NLFunctionUnitDiagnostic unit_diag = {0};
    int passed = 0;
    if (!parse_input(unit_text, true, &unit) ||
        !parse_input("main()", false, &invocation) ||
        !parse_input(bad_registration, true, &bad))
        goto cleanup;
    if (nl_semantic_create(&ctx) != NL_CHECK_OK)
        goto cleanup;
    const NLSyntaxTree *units[] = {unit.tree};
    NLCheckStatus status =
        nl_semantic_register_function_unit(ctx, units, 1, &unit_diag);
    const char *reg_code = unit_diag.diagnostic.diagnostic.code;
    printf("R405 register=%d code=%s\n", (int)status,
           reg_code ? reg_code : "-");
    if (status != NL_CHECK_OK)
        goto cleanup;
    status = nl_semantic_check_source_fragment(ctx, invocation.tree, &checked,
                                               &diagnostic);
    const char *check_code = diagnostic.diagnostic.code;
    printf("R405 actual main() fragment=%d code=%s owned=%d\n", (int)status,
           check_code ? check_code : "-", checked != NULL);
    if (status != NL_CHECK_OK || checked == NULL)
        goto cleanup;
    if (!observe_owned(checked, same))
        goto cleanup;
    nl_checked_destroy(checked);
    checked = NULL;
    NLSemanticSnapshot before, after;
    if (!nl_semantic_snapshot(ctx, &before))
        goto cleanup;
    const NLSyntaxTree *invalid[] = {bad.tree};
    unit_diag = (NLFunctionUnitDiagnostic){0};
    status = nl_semantic_register_function_unit(ctx, invalid, 1, &unit_diag);
    const char *invalid_code = unit_diag.diagnostic.diagnostic.code;
    printf("R405 rejected unselected arm=%d code=%s\n", (int)status,
           invalid_code ? invalid_code : "-");
    if (status != NL_CHECK_SEMANTIC_ERROR || invalid_code == NULL ||
        strcmp(invalid_code, "P3-UNKNOWN-BINDING") != 0 ||
        !nl_semantic_snapshot(ctx, &after))
        goto cleanup;
    if (before.types != after.types || before.bindings != after.bindings ||
        before.values != after.values || before.places != after.places ||
        before.domains != after.domains || before.scopes != after.scopes ||
        before.functions != after.functions ||
        before.last_incarnation != after.last_incarnation ||
        before.last_value_fact != after.last_value_fact ||
        before.occurrences != after.occurrences ||
        before.backing_regions != after.backing_regions)
        goto cleanup;
    diagnostic = (NLCheckDiagnostic){0};
    status = nl_semantic_check_source_fragment(ctx, invocation.tree, &checked,
                                               &diagnostic);
    printf("R405 rollback status=%d owned=%d\n", (int)status, checked != NULL);
    if (status != NL_CHECK_OK || checked == NULL ||
        !observe_owned(checked, same))
        goto cleanup;
    passed = 1;
cleanup:
    printf("R405 same=%d outcome=%s\n", same, passed ? "PASS" : "FAIL");
    nl_checked_destroy(checked);
    nl_semantic_destroy(ctx);
    release_input(&bad);
    release_input(&invocation);
    release_input(&unit);
    return passed;
}
int main(void)
{
    /* Both are R402-derived sources; no P positive fixtures are imported.
     * Fail closed: precision/NULL owned never equals a successful R405 run. */
    const int first = run_case(good, 0);
    const int second = run_case(same_locator, 1);
    return first && second ? 0 : 1;
}
