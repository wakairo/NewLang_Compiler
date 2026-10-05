#include "../support/semantic_check.h"

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

static bool run(TestSemantic *f, const char *text, TestChecked *out)
{
    return test_run(f->context, text, TEST_SOURCE, NL_CHECK_OK, NULL, out);
}
static bool ok(TestSemantic *f, const char *text)
{
    TestChecked a = {0};
    CHECK(run(f, text, &a));
    test_checked_destroy(&a);
    return true;
}
static bool rejected(TestSemantic *f, const char *text, NLCheckStatus status,
                     const char *code)
{
    return test_rejected(f->context, text, TEST_SOURCE, status, code);
}
static bool seed(TestSemantic *f, const char *name, NLTypeId type)
{
    NLSymbolId symbol;
    return nl_semantic_seed_value(f->context, name, type, NL_DEPENDENCY_FREE,
                                  &symbol) == NL_CHECK_OK;
}
static bool option(TestSemantic *f, const char *name, NLTypeId payload,
                   NLTypeId *type)
{
    const NLSumVariant variants[] = {{"Some", payload}, {"None", 0}};
    return nl_semantic_register_sum(f->context, name, variants, 2, type) ==
           NL_CHECK_OK;
}
static bool place(TestSemantic *f, const char *name, NLSemanticPlaceView *out,
                  NLPlaceId *id)
{
    NLSemanticBindingView binding;
    CHECK(nl_semantic_binding_view(
        f->context, nl_semantic_find_binding(f->context, name), &binding));
    *id = binding.place;
    return nl_semantic_place_view(f->context, binding.place, out);
}
static bool ref(TestSemantic *f, const char *name, NLPlaceId p,
                NLAccessSyntax mode)
{
    NLSymbolId symbol;
    return test_reference(f->context, name, p, NL_TYPE_REF, mode, false,
                          &symbol, NULL);
}
static bool parse(const char *text, NLParseStatus expected)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLParseDiagnostic d = {0};
    CHECK(nl_source_create(text, strlen(text), "sum-surface", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    NLParseStatus status = nl_parser_parse_source_fragment(parser, &tree, &d);
    if (status != expected)
        fprintf(stderr, "%s: parse %d expected %d (%s)\n", text, status,
                expected, d.diagnostic.code);
    CHECK(status == expected);
    if (tree != NULL)
        CHECK(nl_syntax_tree_root(tree) != NULL);
    else
        CHECK(nl_source_span_valid(source, d.span));
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool parser_tests(void)
{
    const char *valid[] = {"Option.None",
                           "Option.Some(x)",
                           "Option.Some()",
                           "Option.None(x,y)",
                           "match x {}",
                           "match x {None=>{},Some(y)=>{y},}",
                           "match f(x) {Some(_)=>{},None=>{}}",
                           "match Option.None {None=>{}}",
                           "match {x} {None=>{}}"};
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i)
        CHECK(parse(valid[i], NL_PARSE_OK));
    const char *bad[] = {"match x {None = > {}}",
                         "match x {None =\n> {}}",
                         "match x {Some(y)=>{} None=>{}}",
                         "match x {None=>x}",
                         "Option.Some(x,)",
                         "match x {None=>{}",
                         "match x {Some()=>{}}"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i)
        CHECK(parse(bad[i], NL_PARSE_SYNTAX_ERROR));
    const char *unsupported[] = {"match x {_=>{}}",
                                 "match x {Some(Some(y))=>{}}",
                                 "match x {Other.Some(y)=>{}}",
                                 "match x {Some(y) if g =>{}}",
                                 "sum Option {None,Some(T)}",
                                 "match x {Some(y)|None=>{}}"};
    for (size_t i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); ++i)
        CHECK(parse(unsupported[i], NL_PARSE_SYNTAX_UNSUPPORTED));
    return true;
}
static bool registry_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId copy, affine, linear;
    CHECK(option(&f, "CopySum", f.copy, &copy));
    CHECK(option(&f, "AffineSum", f.discardable, &affine));
    CHECK(option(&f, "LinearSum", f.linear, &linear));
    NLSemanticTypeView view;
    CHECK(nl_semantic_type_view(f.context, copy, &view) &&
          view.kind == NL_TYPE_SUM && view.is_copy && view.is_discardable &&
          view.variant_count == 2);
    CHECK(nl_semantic_type_view(f.context, affine, &view) && !view.is_copy &&
          view.is_discardable);
    CHECK(nl_semantic_type_view(f.context, linear, &view) && !view.is_copy &&
          !view.is_discardable);
    TestState before;
    CHECK(test_state(f.context, &before));
    NLTypeId sentinel = 777;
    NLSumVariant duplicate[] = {{"Same", f.copy}, {"Same", 0}};
    CHECK(nl_semantic_register_sum(f.context, "Duplicate", duplicate, 2,
                                   &sentinel) == NL_CHECK_SEMANTIC_ERROR);
    CHECK(sentinel == 777 && test_unchanged(f.context, &before));
    CHECK(nl_semantic_register_sum(f.context, "TooMany", duplicate,
                                   NL_SEMANTIC_MAX_VARIANTS + 1,
                                   &sentinel) == NL_CHECK_RESOURCE_LIMIT);
    CHECK(sentinel == 777 && test_unchanged(f.context, &before));
    CHECK(nl_semantic_register_sum(f.context, "Empty", duplicate, 0,
                                   &sentinel) == NL_CHECK_SEMANTIC_ERROR);
    NLSumVariant nested = {"Inner", copy};
    CHECK(nl_semantic_register_sum(f.context, "Nested", &nested, 1,
                                   &sentinel) == NL_CHECK_SEMANTIC_UNSUPPORTED);
    CHECK(test_unchanged(f.context, &before));
    CHECK(seed(&f, "x", f.copy));
    CHECK(ok(&f, "let c = CopySum.Some(x)"));
    TestChecked a = {0};
    CHECK(run(&f, "c", &a));
    NLValueId value = test_root(&a)->results[0].value;
    NLSemanticValueView v, original, payload;
    NLSemanticBindingView binding;
    CHECK(nl_semantic_binding_view(
        f.context, nl_semantic_find_binding(f.context, "c"), &binding));
    CHECK(nl_semantic_value_view(f.context, binding.value, &original));
    CHECK(nl_semantic_value_view(f.context, value, &v));
    CHECK(value != binding.value && v.sum_payload != original.sum_payload &&
          v.variant == 1);
    CHECK(nl_semantic_value_view(f.context, v.sum_payload, &payload) &&
          payload.carrier == NL_CARRIER_SUM && payload.sum_owner == value &&
          payload.owner_place == 0);
    CHECK(nl_semantic_register_function(f.context, "Unknown", &f.copy, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    char owned_name[] = "OwnedSum", owned_variant[] = "Only";
    NLSumVariant owned = {owned_variant, f.copy};
    NLTypeId single;
    CHECK(nl_semantic_register_sum(f.context, owned_name, &owned, 1, &single) ==
          NL_CHECK_OK);
    owned_name[0] = 'X';
    owned_variant[0] = 'X';
    CHECK(ok(&f, "match OwnedSum.Only(x) {Only(v)=>{v}}"));
    CHECK(rejected(&f, "CopySum.Only(x)", NL_CHECK_SEMANTIC_ERROR,
                   "P6-UNKNOWN-VARIANT"));
    CHECK(rejected(&f, "CopySum.Unknown(missing)", NL_CHECK_SEMANTIC_ERROR,
                   "P6-UNKNOWN-VARIANT"));
    CHECK(rejected(&f, "CopySum.Some(x,x)", NL_CHECK_SEMANTIC_ERROR,
                   "P6-CONSTRUCTOR-SHAPE"));
    CHECK(rejected(&f, "CopySum.Unknown(x)", NL_CHECK_SEMANTIC_ERROR,
                   "P6-UNKNOWN-VARIANT"));
    CHECK(rejected(&f, "CopyT.Some(x)", NL_CHECK_SEMANTIC_ERROR,
                   "P6-SUM-QUALIFIER"));
    CHECK(rejected(&f, "Unknown.Some(x)", NL_CHECK_SEMANTIC_ERROR,
                   "P6-SUM-QUALIFIER"));
    CHECK(rejected(&f, "CopySum.Some", NL_CHECK_SEMANTIC_ERROR,
                   "P6-CONSTRUCTOR-SHAPE"));
    CHECK(rejected(&f, "CopySum.Some()", NL_CHECK_SEMANTIC_ERROR,
                   "P6-CONSTRUCTOR-SHAPE"));
    CHECK(rejected(&f, "CopySum.None()", NL_CHECK_SEMANTIC_ERROR,
                   "P6-CONSTRUCTOR-SHAPE"));
    CHECK(rejected(&f, "CopySum.None(x,x)", NL_CHECK_SEMANTIC_ERROR,
                   "P6-CONSTRUCTOR-SHAPE"));
    CHECK(seed(&f, "wrong", f.discardable));
    CHECK(rejected(&f, "CopySum.Some(wrong)", NL_CHECK_SEMANTIC_ERROR,
                   "P6-PAYLOAD-TYPE"));
    test_checked_destroy(&a);
    nl_semantic_destroy(f.context);
    return true;
}
static bool consuming_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId sum;
    const NLSumVariant variants[] = {{"Ok", f.linear}, {"Err", f.discardable}};
    CHECK(nl_semantic_register_sum(f.context, "ResultHeaderError", variants, 2,
                                   &sum) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "accept", &f.linear, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "recover", &f.discardable, 1,
                                        f.copy, false, false) == NL_CHECK_OK);
    CHECK(seed(&f, "header", f.linear) && seed(&f, "error", f.discardable));
    CHECK(ok(&f, "let result = ResultHeaderError.Ok(header)"));
    TestChecked a = {0};
    CHECK(
        run(&f, "match result {Ok(h)=>{accept(h)},Err(e)=>{recover(e)},}", &a));
    CHECK(test_root(&a)->type == f.copy && test_root(&a)->result_count == 1);
    const NLCheckedFragment *arm0 =
        nl_checked_match_arm(a.artifact, nl_checked_root(a.artifact), 0);
    const NLCheckedFragment *arm1 =
        nl_checked_match_arm(a.artifact, nl_checked_root(a.artifact), 1);
    CHECK(arm0 != NULL && arm1 != NULL &&
          nl_checked_context(arm0) != f.context &&
          nl_checked_context(arm1) != f.context);
    CHECK(nl_checked_node_view(arm0, nl_checked_root(arm0))->variant == 1);
    CHECK(nl_checked_node_view(arm1, nl_checked_root(arm1))->variant == 2);
    NLSemanticBindingView binding;
    CHECK(nl_semantic_binding_view(
              f.context, nl_semantic_find_binding(f.context, "result"),
              &binding) &&
          binding.availability == NL_CONSUMED);
    CHECK(nl_semantic_find_binding(f.context, "h") == 0 &&
          nl_semantic_find_binding(f.context, "e") == 0);
    CHECK(rejected(&f, "result", NL_CHECK_SEMANTIC_ERROR,
                   "P3-USE-AFTER-CONSUME"));
    test_checked_destroy(&a);
    NLTypeId copy_sum;
    CHECK(option(&f, "Option", f.copy, &copy_sum) && seed(&f, "x", f.copy));
    CHECK(ok(&f, "let c=Option.Some(x)"));
    CHECK(ok(&f, "match c {Some(_)=>{},None=>{}}"));
    CHECK(nl_semantic_binding_view(
              f.context, nl_semantic_find_binding(f.context, "c"), &binding) &&
          binding.availability == NL_AVAILABLE);
    CHECK(rejected(&f, "match c {}", NL_CHECK_SEMANTIC_ERROR,
                   "P6-EXHAUSTIVENESS"));
    CHECK(rejected(&f, "match c {Some(_)=>{}}", NL_CHECK_SEMANTIC_ERROR,
                   "P6-EXHAUSTIVENESS"));
    CHECK(rejected(&f, "match c {None=>{},Some(_)=>{},None=>{}}",
                   NL_CHECK_SEMANTIC_ERROR, "P6-EXHAUSTIVENESS"));
    CHECK(rejected(&f, "match c {Ok(_)=>{},None=>{}}", NL_CHECK_SEMANTIC_ERROR,
                   "P6-UNKNOWN-VARIANT"));
    CHECK(rejected(&f, "match c {Some=>{},None=>{}}", NL_CHECK_SEMANTIC_ERROR,
                   "P6-PATTERN-SHAPE"));
    CHECK(rejected(&f, "match c {Some(_)=>{},None(_)=>{}}",
                   NL_CHECK_SEMANTIC_ERROR, "P6-PATTERN-SHAPE"));
    /* Known Some still checks the None arm, including diagnostics and joins. */
    CHECK(rejected(&f, "match c {Some(_)=>{},None=>{missing}}",
                   NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
    CHECK(rejected(&f, "match c {Some(_)=>{x},None=>{}}",
                   NL_CHECK_SEMANTIC_ERROR, "P6-RESULT-JOIN"));
    CHECK(seed(&f, "outer", f.discardable));
    CHECK(rejected(&f, "match c {Some(_)=>{outer;},None=>{}}",
                   NL_CHECK_SEMANTIC_ERROR, "P6-AVAILABILITY-JOIN"));
    CHECK(rejected(
        &f, "match c {Some(_)=>{},None=>{match c {Some(_)=>{},None=>{}}}}",
        NL_CHECK_SEMANTIC_UNSUPPORTED, "P6-NESTED-MATCH"));
    CHECK(ok(&f, "match c {Some(_)=>{outer;},None=>{outer;}}"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool borrowed_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId sum;
    CHECK(option(&f, "Option", f.copy, &sum) && seed(&f, "x", f.copy) &&
          seed(&f, "next", f.copy));
    CHECK(ok(&f, "let state = Option.Some(x)"));
    NLSemanticPlaceView before, after;
    NLPlaceId root;
    CHECK(place(&f, "state", &before, &root) &&
          ref(&f, "rw", root, NL_ACCESS_WRITE) &&
          ref(&f, "ro", root, NL_ACCESS_READ));
    NLSemanticOccurrenceView occurrence;
    CHECK(nl_semantic_occurrence_view(f.context, before.payload_occurrence,
                                      &occurrence) &&
          occurrence.live && occurrence.root == root);
    TestChecked a = {0};
    CHECK(run(&f, "match rw {Some(payload)=>{replace(payload,next);},None=>{}}",
              &a));
    CHECK(nl_semantic_place_view(f.context, root, &after) &&
          after.incarnation == before.incarnation &&
          after.payload_occurrence == before.payload_occurrence &&
          after.current_fact != before.current_fact);
    const NLCheckedFragment *arm =
        nl_checked_match_arm(a.artifact, nl_checked_root(a.artifact), 0);
    const NLSemanticContext *branch = nl_checked_context(arm);
    const NLCheckedNodeView *pattern =
        nl_checked_node_view(arm, nl_checked_root(arm));
    NLSemanticBindingView binding;
    NLSemanticValueView cap;
    NLSemanticTypeView t;
    CHECK(nl_semantic_binding_view(branch, pattern->symbol, &binding) &&
          nl_semantic_value_view(branch, binding.value, &cap));
    CHECK(nl_semantic_type_view(branch, cap.type, &t) &&
          t.kind == NL_TYPE_REF && t.access == NL_ACCESS_WRITE &&
          cap.reference.occurrence_dependency == before.payload_occurrence);
    NLSemanticScopeView scope;
    CHECK(nl_semantic_scope_view(branch, cap.reference.scope, &scope) &&
          scope.parent != 0 && !scope.active);
    test_checked_destroy(&a);
    CHECK(rejected(&f,
                   "match rw "
                   "{Some(payload)=>{store(rw,Option.None);},None=>{store(rw,"
                   "Option.None);}}",
                   NL_CHECK_SEMANTIC_ERROR, "P6-OCCURRENCE-CONFLICT"));
    CHECK(rejected(
        &f, "match ro {Some(payload)=>{replace(payload,next);},None=>{}}",
        NL_CHECK_SEMANTIC_ERROR, "P3-TYPE-MISMATCH"));
    CHECK(rejected(&f, "match rw {Some(payload)=>{payload},None=>{ro}}",
                   NL_CHECK_SEMANTIC_UNSUPPORTED, "P6-ESCAPING-PAYLOAD-REF"));
    CHECK(rejected(&f, "match rw {Some(_)=>{store(rw,Option.None);},None=>{}}",
                   NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P6-JOIN-PRECISION"));
    NLSymbolId ending;
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending, NULL));
    CHECK(rejected(
        &f,
        "match rw "
        "{Some(payload)=>{take(ptr_from_ref(payload),ending);},None=>{}}",
        NL_CHECK_SEMANTIC_ERROR, "P3-NOT-LIFETIME-ROOT"));
    CHECK(ok(
        &f,
        "match rw "
        "{Some(_)=>{store(rw,Option.None);},None=>{store(rw,Option.None);}}"));
    CHECK(nl_semantic_place_view(f.context, root, &after) &&
          after.payload_occurrence == 0 &&
          after.incarnation == before.incarnation);
    CHECK(nl_semantic_occurrence_view(f.context, before.payload_occurrence,
                                      &occurrence) &&
          !occurrence.live);
    /* W4: Some -> same Some resets occurrence, never the root incarnation. */
    CHECK(ok(&f, "store(rw,Option.Some(x))"));
    CHECK(nl_semantic_place_view(f.context, root, &before));
    CHECK(run(&f, "replace(rw,Option.Some(next))", &a));
    CHECK(nl_semantic_place_view(f.context, root, &after) &&
          after.incarnation == before.incarnation &&
          after.payload_occurrence != before.payload_occurrence);
    CHECK(nl_semantic_occurrence_view(f.context, before.payload_occurrence,
                                      &occurrence) &&
          !occurrence.live);
    NLSemanticValueView old, old_payload;
    CHECK(nl_semantic_value_view(f.context, test_root(&a)->results[0].value,
                                 &old) &&
          old.carrier == NL_CARRIER_LOOSE && old.variant == 1);
    CHECK(nl_semantic_value_view(f.context, old.sum_payload, &old_payload) &&
          old_payload.carrier == NL_CARRIER_SUM &&
          old_payload.owner_place == 0);
    test_checked_destroy(&a);
    /* Persistent ptr is nonblocking; the historical child place cannot revive.
     */
    NLSemanticOccurrenceView active;
    CHECK(nl_semantic_occurrence_view(f.context, after.payload_occurrence,
                                      &active));
    NLSemanticPlaceView child;
    CHECK(nl_semantic_place_view(f.context, active.payload_place, &child));
    NLTypeId payload_ref_type;
    NLScopeId child_scope;
    NLSymbolId payload_ref;
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_REF, f.copy,
                                    NL_ACCESS_READ, false,
                                    &payload_ref_type) == NL_CHECK_OK);
    CHECK(nl_semantic_scope(f.context, 0, true, &child_scope) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_reference(
              f.context, "child_ref", payload_ref_type,
              (NLReferenceFacts){.place = active.payload_place,
                                 .incarnation = child.incarnation,
                                 .scope = child_scope,
                                 .provenance = NL_PROVENANCE_VALID,
                                 .readable = true,
                                 .occurrence_dependency =
                                     after.payload_occurrence},
              &payload_ref) == NL_CHECK_OK);
    CHECK(ok(&f, "let stale=ptr_from_ref(child_ref)"));
    CHECK(rejected(&f, "match rw {Some(_)=>{},None=>{}}",
                   NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                   "P6-EXTERNAL-OCCURRENCE"));
    CHECK(nl_semantic_end_scope(f.context, child_scope) == NL_CHECK_OK);
    CHECK(ok(&f, "store(rw,Option.Some(x))"));
    CHECK(test_rejected(f.context, "loan read stale using ro as r {}",
                        TEST_LOAN, NL_CHECK_SEMANTIC_ERROR,
                        "P3-STALE-POINTER"));
    NLSymbolId ptr;
    CHECK(test_reference(f.context, "ptr", root, NL_TYPE_PTR, NL_ACCESS_READ,
                         false, &ptr, NULL));
    CHECK(rejected(&f, "match ptr {Some(_)=>{},None=>{}}",
                   NL_CHECK_SEMANTIC_ERROR, "P6-MATCH-SUM"));
    NLTypeId write_type;
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_REF, sum,
                                    NL_ACCESS_WRITE, false,
                                    &write_type) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.context, "touch", &write_type, 1, 1,
                                        false, false) == NL_CHECK_OK);
    CHECK(rejected(&f,
                   "match rw {Some(payload)=>{touch(rw);},None=>{touch(rw);}}",
                   NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P6-CALL-PRESERVATION"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool lifetime_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId sum;
    CHECK(option(&f, "AffineOption", f.discardable, &sum));
    CHECK(seed(&f, "incoming", f.discardable));
    NLSymbolId ending, slot;
    NLPlaceId target;
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &ending, NULL));
    CHECK(nl_semantic_seed_slot(f.context, "vacant", sum, &slot, &target) ==
          NL_CHECK_OK);
    CHECK(
        ok(&f, "let p=initialize(vacant,AffineOption.Some(incoming),ending)"));
    NLSemanticPlaceView before, after;
    CHECK(nl_semantic_place_view(f.context, target, &before) && before.live &&
          before.payload_occurrence != 0);
    CHECK(ok(&f, "let(old,empty)=take(p,ending)"));
    CHECK(nl_semantic_place_view(f.context, target, &after) && !after.live &&
          after.payload_occurrence == 0);
    NLSemanticOccurrenceView occurrence;
    CHECK(nl_semantic_occurrence_view(f.context, before.payload_occurrence,
                                      &occurrence) &&
          !occurrence.live);
    CHECK(rejected(&f, "take(p,ending)", NL_CHECK_SEMANTIC_ERROR,
                   "P3-STALE-POINTER"));
    CHECK(ok(&f, "let fresh=initialize(empty,old,ending)"));
    CHECK(nl_semantic_place_view(f.context, target, &after) && after.live &&
          after.incarnation != before.incarnation &&
          after.payload_occurrence != before.payload_occurrence);
    NLOccurrenceId fresh = after.payload_occurrence;
    CHECK(ok(&f, "let final_empty=destroy(fresh,ending)"));
    CHECK(nl_semantic_place_view(f.context, target, &after) && !after.live &&
          after.payload_occurrence == 0);
    CHECK(nl_semantic_occurrence_view(f.context, fresh, &occurrence) &&
          !occurrence.live);
    NLSemanticBindingView authority;
    CHECK(nl_semantic_binding_view(f.context, ending, &authority) &&
          authority.availability == NL_AVAILABLE);
    nl_semantic_destroy(f.context);
    return true;
}

static bool storage_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId sum;
    NLTypeId storage = nl_semantic_core_type(f.context, NL_TYPE_STORAGE);
    CHECK(option(&f, "OptionStorage", storage, &sum));
    NLRawOperation allocation = {.kind = NL_RAW_ALLOCATE,
                                 .data.allocate = {.size = {true, 8},
                                                   .alignment = {true, 8},
                                                   .ordinary_read = true,
                                                   .ordinary_write = true}};
    NLCheckedFragment *raw = NULL;
    CHECK(nl_semantic_check_raw_operation(f.context, &allocation, &raw, NULL) ==
          NL_CHECK_OK);
    const NLCheckedNodeView *produced =
        nl_checked_node_view(raw, nl_checked_root(raw));
    NLSymbolId owner, bytes;
    CHECK(nl_semantic_bind_result(f.context, "allocation",
                                  produced->results[0].value,
                                  &owner) == NL_CHECK_OK);
    CHECK(nl_semantic_bind_result(f.context, "bytes",
                                  produced->results[1].value,
                                  &bytes) == NL_CHECK_OK);
    NLValueId storage_value = produced->results[1].value;
    nl_checked_destroy(raw);
    CHECK(ok(&f, "let state=OptionStorage.Some(bytes)"));
    CHECK(rejected(&f, "match state {Some(_)=>{},None=>{}}",
                   NL_CHECK_SEMANTIC_ERROR, "P6-PAYLOAD-DISCARD"));
    NLSemanticPlaceView p;
    NLPlaceId root;
    CHECK(place(&f, "state", &p, &root) &&
          ref(&f, "rw", root, NL_ACCESS_WRITE));
    CHECK(ok(&f, "match rw {Some(_)=>{},None=>{}}"));
    CHECK(rejected(&f, "store(rw,OptionStorage.None)", NL_CHECK_SEMANTIC_ERROR,
                   "P3-DISCARDABLE-REQUIRED"));
    /* End the host ref scope before consuming the source carrier. */
    NLSemanticBindingView r;
    NLSemanticValueView rv;
    CHECK(nl_semantic_binding_view(
        f.context, nl_semantic_find_binding(f.context, "rw"), &r));
    CHECK(nl_semantic_value_view(f.context, r.value, &rv));
    CHECK(nl_semantic_end_scope(f.context, rv.reference.scope) == NL_CHECK_OK);
    CHECK(ok(&f,
             "let moved=match state "
             "{Some(s)=>{OptionStorage.Some(s)},None=>{OptionStorage.None}}"));
    NLSemanticPlaceView moved;
    CHECK(place(&f, "moved", &moved, &root));
    NLSemanticValueView mv;
    CHECK(nl_semantic_value_view(f.context, moved.current_value, &mv) &&
          mv.sum_payload == storage_value);
    NLSemanticValueView member;
    CHECK(nl_semantic_value_view(f.context, storage_value, &member) &&
          member.occupancy.region != 0 && member.carrier == NL_CARRIER_PLACE);
    /* None is still statically non-Discardable. No synthetic Storage is minted.
     */
    CHECK(ok(&f, "let empty=OptionStorage.None"));
    CHECK(rejected(&f, "match empty {Some(_)=>{},None=>{}}",
                   NL_CHECK_SEMANTIC_ERROR, "P6-PAYLOAD-DISCARD"));
    CHECK(place(&f, "empty", &p, &root) &&
          ref(&f, "empty_rw", root, NL_ACCESS_WRITE));
    CHECK(rejected(&f, "store(empty_rw,OptionStorage.None)",
                   NL_CHECK_SEMANTIC_ERROR, "P3-DISCARDABLE-REQUIRED"));
    CHECK(nl_semantic_binding_view(
        f.context, nl_semantic_find_binding(f.context, "empty_rw"), &r));
    CHECK(nl_semantic_value_view(f.context, r.value, &rv));
    CHECK(nl_semantic_end_scope(f.context, rv.reference.scope) == NL_CHECK_OK);
    CHECK(ok(&f,
             "let empty2=match empty "
             "{Some(s)=>{OptionStorage.Some(s)},None=>{OptionStorage.None}}"));
    CHECK(place(&f, "empty2", &p, &root));
    CHECK(nl_semantic_value_view(f.context, p.current_value, &mv) &&
          mv.variant == 2 && mv.sum_payload == 0);
    nl_semantic_destroy(f.context);
    return true;
}
static bool check_oom(TestSemantic *f, const char *text)
{
    TestState before;
    CHECK(test_state(f->context, &before));
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_create(text, strlen(text), "sum-oom", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_source_fragment(parser, &tree, NULL) == NL_PARSE_OK);
    bool completed = false;
    for (fail_at = 0; fail_at < 4000; ++fail_at) {
        NLCheckedFragment *a = NULL;
        injecting = true;
        allocation_index = 0;
        NLCheckStatus status =
            nl_semantic_check_source_fragment(f->context, tree, &a, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(a);
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && a == NULL &&
              test_unchanged(f->context, &before));
    }
    CHECK(completed);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}

static bool failure_tests(void)
{
    TestSemantic f = {0};
    CHECK(test_semantic_create(&f));
    NLTypeId sum;
    CHECK(option(&f, "Option", f.copy, &sum) && seed(&f, "x", f.copy));
    CHECK(ok(&f, "let state=Option.Some(x)"));
    NLSemanticPlaceView p;
    NLPlaceId root;
    CHECK(place(&f, "state", &p, &root) &&
          ref(&f, "rw", root, NL_ACCESS_WRITE));
    TestState before;
    CHECK(test_state(f.context, &before));
    const NLSumVariant variants[] = {{"A", f.copy}, {"B", 0}};
    bool completed = false;
    for (fail_at = 0; fail_at < 2000; ++fail_at) {
        NLTypeId id = 999;
        injecting = true;
        allocation_index = 0;
        NLCheckStatus s =
            nl_semantic_register_sum(f.context, "OOMSum", variants, 2, &id);
        injecting = false;
        if (s == NL_CHECK_OK) {
            completed = true;
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && id == 999 &&
              test_unchanged(f.context, &before));
    }
    CHECK(completed);
    CHECK(test_state(f.context, &before));
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    const char *text =
        "match rw {Some(payload)=>{replace(payload,x);},None=>{}}";
    CHECK(nl_source_create(text, strlen(text), "oom-arms", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    completed = false;
    for (fail_at = 0; fail_at < 1000; ++fail_at) {
        NLParseDiagnostic d = {0};
        injecting = true;
        allocation_index = 0;
        NLParseStatus status =
            nl_parser_parse_source_fragment(parser, &tree, &d);
        injecting = false;
        if (status == NL_PARSE_OK) {
            completed = true;
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL &&
              nl_source_span_valid(source, d.span));
    }
    CHECK(completed);
    completed = false;
    for (fail_at = 0; fail_at < 4000; ++fail_at) {
        NLCheckedFragment *a = NULL;
        NLCheckDiagnostic d = {0};
        injecting = true;
        allocation_index = 0;
        NLCheckStatus s =
            nl_semantic_check_source_fragment(f.context, tree, &a, &d);
        injecting = false;
        if (s == NL_CHECK_OK) {
            nl_checked_destroy(a);
            completed = true;
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && a == NULL &&
              test_unchanged(f.context, &before));
        CHECK(strcmp(d.diagnostic.category, "host") == 0 &&
              nl_source_span_valid(source, d.span));
    }
    CHECK(completed);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    CHECK(rejected(
        &f, "match rw {Some(payload)=>{let partial=x;unknown;},None=>{}}",
        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
    CHECK(nl_semantic_find_binding(f.context, "partial") == 0 &&
          nl_semantic_find_binding(f.context, "payload") == 0);
    CHECK(check_oom(&f, "let fresh=Option.Some(x)"));
    CHECK(check_oom(&f, "let returned=replace(rw,Option.Some(x))"));
    CHECK(check_oom(&f, "match rw "
                        "{Some(_)=>{store(rw,Option.Some(x));},None=>{store(rw,"
                        "Option.Some(x));}}"));
    char large[4096] = "{";
    for (size_t i = 0; i < 33; ++i)
        strcat(large, "match rw {Some(_)=>{},None=>{}};");
    strcat(large, "}");
    CHECK(rejected(&f, large, NL_CHECK_RESOURCE_LIMIT, "P3-RESOURCE-LIMIT"));
    nl_semantic_destroy(f.context);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    const struct {
        const char *name;
        bool (*test)(void);
    } groups[] = {{"parser", parser_tests},       {"registry", registry_tests},
                  {"consuming", consuming_tests}, {"borrowed", borrowed_tests},
                  {"storage", storage_tests},     {"lifetime", lifetime_tests},
                  {"failure", failure_tests}};
    for (size_t i = 0; i < sizeof(groups) / sizeof(groups[0]); ++i)
        if (strcmp(argv[1], groups[i].name) == 0)
            return groups[i].test() ? 0 : 1;
    return 2;
}
