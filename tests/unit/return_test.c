#include "../../src/semantic_internal.h"
#include "../support/function_body.h"
#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
static bool syntax_test(void)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree("{return unit; unknown; unknown}", &source, &tree));
    const NLSyntaxView *block = nl_syntax_node_view(nl_syntax_tree_root(tree));
    const NLSyntaxView *item = nl_syntax_node_view(block->data.block.items);
    CHECK(item->kind == NL_SYNTAX_RETURN && item->span.start_byte == 1);
    NLSourceView bytes;
    CHECK(nl_source_view(source, item->span, &bytes) && bytes.length == 12 &&
          memcmp(bytes.bytes, "return unit;", 12) == 0);
    CHECK(block->data.block.tail != NULL &&
          nl_syntax_next_argument(block->data.block.items) != NULL);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    source = NULL;
    tree = NULL;
    CHECK(body_tree(
        "{let header=match parse(input) {Ok(h)=>{h},Err(e)=>{return "
        "ResultPacketError.Err(e);}};return ResultPacketError.Ok(header);}",
        &source, &tree));
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    const char *invalid[] = {"{return;}",
                             "{return unit}",
                             "{return unit\n}",
                             "{return unit;return;}",
                             "return unit;",
                             "{let x=return unit;}",
                             "{sink(return unit);}",
                             "{|| {return unit;}}",
                             "{loan read x as r {return unit;}}"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        NLParser *parser = NULL;
        source = NULL;
        tree = NULL;
        NLParseDiagnostic d = {0};
        CHECK(nl_source_create(invalid[i], strlen(invalid[i]), "return-invalid",
                               &source) == NL_SOURCE_OK);
        CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
        CHECK(nl_parser_parse_source_fragment(parser, &tree, &d) !=
                  NL_PARSE_OK &&
              tree == NULL && nl_source_span_valid(source, d.span));
        if (i == 0)
            CHECK(strcmp(d.diagnostic.code, "P9-BARE-RETURN") == 0);
        if (i == 1 || i == 2)
            CHECK(strcmp(d.diagnostic.code, "P9-RETURN-SEMICOLON") == 0);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
    }
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    CHECK(test_rejected(f.context, "{return unit;}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "P9-RETURN-CONTEXT"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool transfer_test(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLFunctionParameter p[] = {{"x", f.linear}};
    const char *bodies[] = {"{return x; x; unknown}", "{{{return x;}; x;}; x}",
                            "{return {return x;}; x;}",
                            "{let never={return x;}; never}"};
    for (size_t i = 0; i < sizeof(bodies) / sizeof(bodies[0]); ++i) {
        char fn[32], input[32], call[100];
        (void)snprintf(fn, sizeof(fn), "identity%zu", i);
        (void)snprintf(input, sizeof(input), "input%zu", i);
        CHECK(register_body(f.context, fn, p, 1, f.linear, bodies[i],
                            NL_CHECK_OK, NULL));
        NLSymbolId symbol;
        CHECK(nl_semantic_seed_value(f.context, input, f.linear,
                                     NL_DEPENDENCY_FREE,
                                     &symbol) == NL_CHECK_OK);
        NLSemanticBindingView before, after;
        CHECK(nl_semantic_binding_view(f.context, symbol, &before));
        (void)snprintf(call, sizeof(call), "let out%zu=%s(%s)", i, fn, input);
        TestChecked a = {0};
        CHECK(test_run(f.context, call, TEST_SOURCE, NL_CHECK_OK, NULL, &a));
        const NLCheckedNodeId call_id = test_root(&a)->initializer;
        const NLCheckedNodeView *called =
            nl_checked_node_view(a.artifact, call_id);
        CHECK(!called->terminates && called->results[0].value == before.value);
        const NLCheckedFragment *body =
            nl_checked_call_body(a.artifact, call_id);
        const NLCheckedNodeView *root =
            nl_checked_node_view(body, nl_checked_root(body));
        CHECK(root->terminates && root->type == 0 && root->result_count == 0);
        CHECK(nl_semantic_binding_view(f.context, symbol, &after) &&
              after.availability == NL_CONSUMED);
        test_checked_destroy(&a);
    }
    NLFunctionParameter cp[] = {{"x", f.copy}};
    CHECK(register_body(f.context, "copy_result", cp, 1, f.copy,
                        "{return x; unknown}", NL_CHECK_OK, NULL));
    NLSymbolId copy;
    CHECK(nl_semantic_seed_value(f.context, "copy", f.copy, NL_DEPENDENCY_FREE,
                                 &copy) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "let copied=copy_result(copy)"));
    NLSemanticBindingView original, returned;
    CHECK(body_binding(f.context, "copy", &original) &&
          body_binding(f.context, "copied", &returned) &&
          original.availability == NL_AVAILABLE &&
          returned.value != original.value);
    CHECK(register_body(f.context, "unit_result", NULL, 0, 1,
                        "{return unit; unknown}", NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "unit_result()"));
    CHECK(register_body(f.context, "wrong", cp, 1, f.linear, "{return x;}",
                        NL_CHECK_SEMANTIC_ERROR, "P9-RETURN-TYPE"));
    CHECK(register_body(f.context, "no_cleanup", p, 1, 1, "{return unit; x;}",
                        NL_CHECK_SEMANTIC_ERROR, "P5-SCOPE-OBLIGATION"));
    NLTypeId ref;
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_REF, f.copy,
                                    NL_ACCESS_READ, false,
                                    &ref) == NL_CHECK_OK);
    NLFunctionParameter rp[] = {{"r", ref}};
    CHECK(register_body(f.context, "illegal_ref", rp, 1, 1, "{return r;}",
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "P9-REF-RETURN"));
    CHECK(register_body(f.context, "ref_signature", rp, 1, ref, "{return r;}",
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P8-SIGNATURE-PRECISION"));
    CHECK(nl_semantic_register_function(f.context, "sink", &f.copy, 1, 1, false,
                                        false) == NL_CHECK_OK);
    CHECK(register_body(
        f.context, "pending", cp, 1, f.copy, "{sink({return x;});x}",
        NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P9-OPERAND-TERMINATION"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool result_type(TestSemantic *f, NLTypeId *result)
{
    const NLSumVariant v[] = {{"Ok", f->copy}, {"Err", f->linear}};
    CHECK(nl_semantic_register_sum(f->context, "ResultPacketError", v, 2,
                                   result) == NL_CHECK_OK);
    return true;
}
static const char decoder_body[] =
    "{let header=match parsed {Ok(h)=>{h},"
    "Err(e)=>{return ResultPacketError.Err(e); e; unknown}};"
    "return ResultPacketError.Ok(header); unknown}";
static bool workload_test(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId result;
    CHECK(result_type(&f, &result));
    NLFunctionParameter p[] = {{"parsed", result}};
    CHECK(register_body(f.context, "decode", p, 1, result, decoder_body,
                        NL_CHECK_OK, NULL));
    CHECK(
        register_body(f.context, "both_return", p, 1, result,
                      "{match parsed {Ok(h)=>{return ResultPacketError.Ok(h);},"
                      "Err(e)=>{{return ResultPacketError.Err(e);}}}; unknown}",
                      NL_CHECK_OK, NULL));
    CHECK(register_body(f.context, "bad_other_arm", p, 1, result,
                        "{match parsed {Ok(h)=>{return h;},"
                        "Err(e)=>{return ResultPacketError.Err(e);}}}",
                        NL_CHECK_SEMANTIC_ERROR, "P9-RETURN-TYPE"));
    CHECK(nl_semantic_register_function(f.context, "make_packet", NULL, 0,
                                        f.copy, false, false) == NL_CHECK_OK);
    CHECK(
        register_body(f.context, "missing_obligation", p, 1, result,
                      "{match parsed {Ok(h)=>{return ResultPacketError.Ok(h);},"
                      "Err(e)=>{return ResultPacketError.Ok(make_packet());}}}",
                      NL_CHECK_SEMANTIC_ERROR, "P5-SCOPE-OBLIGATION"));
    NLFunctionParameter error_param[] = {{"error", f.linear}};
    CHECK(register_body(f.context, "wrap_error", error_param, 1, result,
                        "{return ResultPacketError.Err(error);}", NL_CHECK_OK,
                        NULL));
    NLSymbolId wrapped_symbol;
    CHECK(nl_semantic_seed_value(f.context, "wrapped_error", f.linear,
                                 NL_DEPENDENCY_FREE,
                                 &wrapped_symbol) == NL_CHECK_OK);
    NLSemanticBindingView original_error, wrapped;
    CHECK(nl_semantic_binding_view(f.context, wrapped_symbol, &original_error));
    CHECK(body_ok(f.context, "let wrapped=wrap_error(wrapped_error)"));
    NLSemanticValueView wrapper, retained;
    CHECK(body_binding(f.context, "wrapped", &wrapped) &&
          nl_semantic_value_view(f.context, wrapped.value, &wrapper) &&
          wrapper.sum_payload == original_error.value &&
          nl_semantic_value_view(f.context, wrapper.sum_payload, &retained) &&
          retained.carrier != NL_CARRIER_ENDED);
    const char *functions[] = {"decode", "both_return"};
    for (size_t fn = 0; fn < 2; ++fn) {
        for (size_t variant = 1; variant <= 2; ++variant) {
            char seed[80], text[128], output[32];
            NLSymbolId symbol;
            (void)snprintf(seed, sizeof(seed), "payload%zu%zu", fn, variant);
            CHECK(nl_semantic_seed_value(
                      f.context, seed, variant == 1 ? f.copy : f.linear,
                      NL_DEPENDENCY_FREE, &symbol) == NL_CHECK_OK);
            NLSemanticBindingView payload;
            CHECK(nl_semantic_binding_view(f.context, symbol, &payload));
            (void)snprintf(text, sizeof(text),
                           "let parsed%zu%zu=ResultPacketError.%s(%s)", fn,
                           variant, variant == 1 ? "Ok" : "Err", seed);
            CHECK(body_ok(f.context, text));
            (void)snprintf(output, sizeof(output), "out%zu%zu", fn, variant);
            (void)snprintf(text, sizeof(text), "let %s=%s(parsed%zu%zu)",
                           output, functions[fn], fn, variant);
            TestChecked a = {0};
            CHECK(
                test_run(f.context, text, TEST_SOURCE, NL_CHECK_OK, NULL, &a));
            NLSemanticBindingView out;
            NLSemanticValueView v, child;
            CHECK(body_binding(f.context, output, &out) &&
                  nl_semantic_value_view(f.context, out.value, &v) &&
                  v.variant == variant && v.type == result &&
                  nl_semantic_value_view(f.context, v.sum_payload, &child) &&
                  child.type == (variant == 1 ? f.copy : f.linear));
            if (variant == 2)
                CHECK(v.sum_payload == payload.value);
            const NLCheckedFragment *body =
                nl_checked_call_body(a.artifact, test_root(&a)->initializer);
            size_t matches = 0;
            for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
                const NLCheckedNodeView *n = nl_checked_node_view(body, i);
                if (n->kind != NL_CHECKED_MATCH)
                    continue;
                ++matches;
                CHECK(n->normal_arms == (fn == 0 ? 1 : 0));
                if (fn == 1)
                    CHECK(n->terminates && n->type == 0 &&
                          n->result_count == 0);
                for (size_t arm = 0; arm < 2; ++arm) {
                    const NLCheckedFragment *owned =
                        nl_checked_match_arm(body, i, arm);
                    CHECK(owned != NULL &&
                          nl_checked_context(owned) != f.context);
                    const NLCheckedNodeView *root =
                        nl_checked_node_view(owned, nl_checked_root(owned));
                    CHECK(root->terminates == (fn == 1 || arm == 1));
                }
            }
            CHECK(matches == 1 && nl_sum_validate(f.context) == NL_CHECK_OK);
            test_checked_destroy(&a);
        }
    }
    nl_semantic_destroy(f.context);
    return true;
}
/* W2: current source cannot construct/install a local-scope ref into a
 * caller-visible carrier. Real constructors (no private writes) create that
 * setup; the trusted boundary runs the SAME source block/return/exit checker.
 */
static bool exit_test(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId root, ref;
    CHECK(nl_semantic_seed_value(f.context, "root", f.copy, NL_DEPENDENCY_FREE,
                                 &root) == NL_CHECK_OK);
    NLSemanticBindingView binding;
    CHECK(nl_semantic_binding_view(f.context, root, &binding));
    CHECK(test_reference(f.context, "global", binding.place, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &ref, NULL));
    CHECK(nl_semantic_binding_view(f.context, ref, &binding));
    NLSemanticValueView global;
    CHECK(nl_semantic_value_view(f.context, binding.value, &global));
    NLValueId original;
    NLPlaceId carrier;
    CHECK(nl_sem_copy_value(f.context, binding.value, &original) ==
          NL_CHECK_OK);
    CHECK(nl_sem_new_place(f.context, binding.type, 0, true, original,
                           &carrier) == NL_CHECK_OK);
    NLSemanticSnapshot boundary;
    CHECK(nl_semantic_snapshot(f.context, &boundary));
    NLScopeId local_scope;
    CHECK(nl_semantic_scope(f.context, global.reference.scope, true,
                            &local_scope) == NL_CHECK_OK);
    global.reference.scope = local_scope;
    NLValueId local;
    CHECK(nl_sem_new_value(f.context, global, &local) == NL_CHECK_OK);
    nl_sem_end_value(f.context, original);
    CHECK(nl_sem_install(f.context, carrier, local, 0) == NL_CHECK_OK);
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree("{return unit;}", &source, &tree));
    const NLFunctionBoundary exit = {1, boundary.bindings, boundary.scopes,
                                     boundary.places};
    TestState before;
    CHECK(test_state(f.context, &before));
    NLCheckedFragment *artifact = NULL;
    NLCheckDiagnostic diagnostic = {0};
    CHECK(nl_sem_check_function_block(f.context, tree, exit, &artifact,
                                      &diagnostic) == NL_CHECK_SEMANTIC_ERROR &&
          artifact == NULL);
    CHECK(test_unchanged(f.context, &before) &&
          strcmp(diagnostic.diagnostic.code, "P9-RETURN-DEPENDENCY") == 0 &&
          nl_source_span_valid(source, diagnostic.span));
    CHECK(nl_sem_function_exit(f.context, boundary.scopes, boundary.places) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_semantic_end_scope(f.context, local_scope) == NL_CHECK_OK);
    CHECK(nl_sem_function_exit(f.context, boundary.scopes, boundary.places) ==
          NL_CHECK_SEMANTIC_ERROR);
    nl_sem_end_value(f.context, local);
    CHECK(nl_sem_copy_value(f.context, binding.value, &original) ==
          NL_CHECK_OK);
    CHECK(nl_sem_install(f.context, carrier, original, 0) == NL_CHECK_OK);
    CHECK(nl_sem_function_exit(f.context, boundary.scopes, boundary.places) ==
          NL_CHECK_OK);
    CHECK(test_state(f.context, &before));
    bool complete = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            nl_sem_check_function_block(f.context, tree, exit, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            complete = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(f.context, &before));
    }
    CHECK(complete && fail_at > 0);
    CHECK(
        nl_checked_node_view(artifact, nl_checked_root(artifact))->terminates);
    nl_checked_destroy(artifact);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    CHECK(register_body(f.context, "restored_unit", NULL, 0, 1,
                        "{return unit;}", NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "restored_unit()"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool failure_test(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId result;
    CHECK(result_type(&f, &result));
    NLFunctionParameter p[] = {{"parsed", result}};
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree(decoder_body, &source, &tree));
    TestState before;
    CHECK(test_state(f.context, &before));
    bool complete = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status = nl_semantic_register_function_body(
            f.context, "decode", p, 1, result, tree, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            complete = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
              test_unchanged(f.context, &before));
    }
    CHECK(complete && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    NLSymbolId error;
    CHECK(nl_semantic_seed_value(f.context, "error", f.linear,
                                 NL_DEPENDENCY_FREE, &error) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "let parsed=ResultPacketError.Err(error)"));
    source = NULL;
    tree = NULL;
    CHECK(body_tree("let out=decode(parsed)", &source, &tree));
    CHECK(test_state(f.context, &before));
    complete = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        NLCheckedFragment *a = NULL;
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_check_source_fragment(f.context, tree, &a, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(a);
            complete = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && a == NULL &&
              test_unchanged(f.context, &before));
    }
    CHECK(complete && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    source = NULL;
    /* Parser owns every allocated RETURN/expression/block node on failure. */
    CHECK(nl_source_create(decoder_body, strlen(decoder_body), "oom-return",
                           &source) == NL_SOURCE_OK);
    complete = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        NLParser *parser = NULL;
        tree = NULL;
        allocation_index = 0;
        injecting = true;
        NLParseStatus status = nl_parser_create(source, &parser);
        if (status == NL_PARSE_OK)
            status = nl_parser_parse_source_fragment(parser, &tree, NULL);
        injecting = false;
        nl_parser_destroy(parser);
        if (status == NL_PARSE_OK) {
            nl_syntax_tree_destroy(tree);
            complete = true;
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
    }
    CHECK(complete && fail_at > 0);
    nl_source_destroy(source);
    nl_semantic_destroy(f.context);
    return true;
}
static bool effects_test(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLSymbolId x, y, ref;
    CHECK(nl_semantic_seed_value(f.context, "x", f.copy, NL_DEPENDENCY_FREE,
                                 &x) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_value(f.context, "y", f.copy, NL_DEPENDENCY_FREE,
                                 &y) == NL_CHECK_OK);
    NLSemanticBindingView before, by, r;
    CHECK(body_binding(f.context, "x", &before) &&
          body_binding(f.context, "y", &by));
    CHECK(test_reference(f.context, "rw", before.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &ref, NULL));
    CHECK(nl_semantic_binding_view(f.context, ref, &r));
    NLFunctionParameter p[] = {{"dst", r.type}, {"v", f.copy}};
    CHECK(register_body(f.context, "once", p, 2, f.copy,
                        "{return replace(dst,v); store(dst,v); unknown}",
                        NL_CHECK_OK, NULL));
    TestChecked a = {0};
    CHECK(test_run(f.context, "let old=once(rw,y)", TEST_SOURCE, NL_CHECK_OK,
                   NULL, &a));
    NLSemanticBindingView returned;
    NLSemanticPlaceView installed;
    CHECK(body_binding(f.context, "old", &returned) &&
          returned.value == before.value);
    CHECK(nl_semantic_place_view(f.context, before.place, &installed) &&
          installed.current_value != returned.value);
    const NLCheckedFragment *body =
        nl_checked_call_body(a.artifact, test_root(&a)->initializer);
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    CHECK(root->item_count == 1 && root->tail == 0);
    const NLCheckedNodeView *ret = nl_checked_node_view(body, root->first_item);
    const NLCheckedNodeView *expr =
        nl_checked_node_view(body, ret->initializer);
    CHECK(ret->kind == NL_CHECKED_RETURN && expr->kind == NL_CHECKED_REPLACE &&
          ret->returned.value == returned.value && ret->result_count == 0);
    size_t replaces = 0, stores = 0;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, i);
        replaces += n->kind == NL_CHECKED_REPLACE;
        stores += n->kind == NL_CHECKED_STORE;
    }
    CHECK(replaces == 1 && stores == 0);
    test_checked_destroy(&a);
    /* Registration succeeds on disjoint formal sites; an actual joined write
     * capability fails only AFTER store. Entire caller state must roll back. */
    NLFunctionParameter late[] = {
        {"dst", r.type}, {"other", r.type}, {"v", f.copy}};
    CHECK(register_body(f.context, "late", late, 3, f.copy,
                        "{store(dst,v);return replace(other,v);}", NL_CHECK_OK,
                        NULL));
    NLValueId joined;
    CHECK(nl_semantic_join_references(f.context, &r.value, 1, &joined) ==
          NL_CHECK_OK);
    NLSymbolId joined_symbol;
    CHECK(nl_semantic_bind_result(f.context, "joined", joined,
                                  &joined_symbol) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "late(rw,joined,y)", TEST_SOURCE,
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P7-SINGULAR-REF-PRECISION"));
    /* Every allocation failure after a caller-visible write and before owned
     * return-body attachment must leave the public state exactly unchanged. */
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree("let old2=once(rw,y)", &source, &tree));
    TestState state;
    CHECK(test_state(f.context, &state));
    bool complete = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            nl_semantic_check_source_fragment(f.context, tree, &artifact, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(artifact);
            complete = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(f.context, &state));
    }
    CHECK(complete && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    nl_semantic_destroy(f.context);
    return true;
}
static bool availability_test(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId choice;
    const NLSumVariant variants[] = {{"Early", 0}, {"Later", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Choice", variants, 2, &choice) ==
          NL_CHECK_OK);
    NLFunctionParameter p[] = {{"choice", choice}, {"x", f.linear}};
    CHECK(register_body(
        f.context, "choose", p, 2, f.linear,
        "{match choice {Early=>{return x; x;},Later=>{unit}};return x;}",
        NL_CHECK_OK, NULL));
    CHECK(register_body(
        f.context, "all", p, 2, f.linear,
        "{match choice {Early=>{return x;},Later=>{return x;}};x;}",
        NL_CHECK_OK, NULL));
    const char *functions[] = {"choose", "all"};
    const char *cases[] = {"Early", "Later"};
    for (size_t fn = 0; fn < 2; ++fn) {
        for (size_t arm = 0; arm < 2; ++arm) {
            char input[32], text[128], output[32];
            NLSymbolId symbol;
            (void)snprintf(input, sizeof(input), "linear%zu%zu", fn, arm);
            CHECK(nl_semantic_seed_value(f.context, input, f.linear,
                                         NL_DEPENDENCY_FREE,
                                         &symbol) == NL_CHECK_OK);
            NLSemanticBindingView before, after;
            CHECK(nl_semantic_binding_view(f.context, symbol, &before));
            (void)snprintf(output, sizeof(output), "chosen%zu%zu", fn, arm);
            (void)snprintf(text, sizeof(text), "let %s=%s(Choice.%s,%s)",
                           output, functions[fn], cases[arm], input);
            CHECK(body_ok(f.context, text));
            CHECK(body_binding(f.context, output, &after) &&
                  after.value == before.value);
        }
    }
    NLSymbolId root, read_ref;
    CHECK(nl_semantic_seed_value(f.context, "root", f.copy, NL_DEPENDENCY_FREE,
                                 &root) == NL_CHECK_OK);
    NLSemanticBindingView root_binding, read_binding;
    CHECK(nl_semantic_binding_view(f.context, root, &root_binding));
    NLScopeId read_scope;
    CHECK(test_reference(f.context, "read_ref", root_binding.place, NL_TYPE_REF,
                         NL_ACCESS_READ, false, &read_ref, &read_scope));
    CHECK(nl_semantic_binding_view(f.context, read_ref, &read_binding));
    CHECK(nl_semantic_register_function(f.context, "observe",
                                        &read_binding.type, 1, 1, false,
                                        false) == NL_CHECK_OK);
    NLFunctionParameter refs[] = {{"choice", choice}, {"r", read_binding.type}};
    CHECK(register_body(f.context, "observe_later", refs, 2, 1,
                        "{let kept=match choice {Early=>{return "
                        "unit;},Later=>{r}};observe(kept);return unit;}",
                        NL_CHECK_OK, NULL));
    TestChecked a = {0};
    CHECK(test_run(f.context, "observe_later(Choice.Later,read_ref)",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    const NLCheckedFragment *body =
        nl_checked_call_body(a.artifact, nl_checked_root(a.artifact));
    size_t matches = 0;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, i);
        if (n->kind != NL_CHECKED_MATCH)
            continue;
        ++matches;
        NLSemanticValueView capability;
        CHECK(n->normal_arms == 1 && n->result_count == 1 &&
              nl_semantic_value_view(f.context, n->results[0].value,
                                     &capability));
        CHECK(capability.reference.place == root_binding.place &&
              capability.reference.scope == read_scope &&
              capability.reference.provenance == NL_PROVENANCE_VALID);
    }
    CHECK(matches == 1);
    test_checked_destroy(&a);
    CHECK(nl_semantic_end_scope(f.context, read_scope) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "observe_later(Choice.Later,read_ref)",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR, "P3-DEAD-SCOPE"));
    /* Owned guarded proof artifacts have an explicit bounded capacity. */
    char many[4096] = "{";
    for (size_t i = 0; i < 33; ++i)
        strcat(many, "match choice {Early=>{return unit;},Later=>{unit}};");
    strcat(many, "return unit;}");
    NLFunctionParameter only_choice[] = {{"choice", choice}};
    CHECK(register_body(f.context, "many_arms", only_choice, 1, 1, many,
                        NL_CHECK_RESOURCE_LIMIT, "P3-RESOURCE-LIMIT"));
    /* Outside-function matches keep the P6/P7 path. Inside this bounded
     * body profile two normal states are rejected, never picked silently. */
    CHECK(register_body(
        f.context, "two_normal", p, 2, f.linear,
        "{match choice {Early=>{unit},Later=>{unit}};return x;}",
        NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P9-CONTINUATION-PRECISION"));
    nl_semantic_destroy(f.context);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "effects") == 0)
        return effects_test() ? 0 : 1;
    if (strcmp(argv[1], "availability") == 0)
        return availability_test() ? 0 : 1;
    if (strcmp(argv[1], "syntax") == 0)
        return syntax_test() ? 0 : 1;
    if (strcmp(argv[1], "transfer") == 0)
        return transfer_test() ? 0 : 1;
    if (strcmp(argv[1], "workload") == 0)
        return workload_test() ? 0 : 1;
    if (strcmp(argv[1], "exit") == 0)
        return exit_test() ? 0 : 1;
    if (strcmp(argv[1], "failure") == 0)
        return failure_test() ? 0 : 1;
    return 2;
}
