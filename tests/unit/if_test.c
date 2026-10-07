#include "../../src/semantic_internal.h"
#include "../support/function_body.h"
#include "../support/ref_join.h"
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
static bool seed(NLSemanticContext *c, const char *name, NLTypeId type)
{
    NLSymbolId symbol;
    CHECK(nl_semantic_seed_value(c, name, type, NL_DEPENDENCY_FREE, &symbol) ==
          NL_CHECK_OK);
    return true;
}
static bool fixture(TestSemantic *f)
{
    CHECK(test_semantic_create(f));
    CHECK(seed(f->context, "cond",
               nl_semantic_core_type(f->context, NL_TYPE_BOOL)));
    CHECK(seed(f->context, "x", f->copy) && seed(f->context, "y", f->copy));
    return true;
}
static bool register_unit(NLSemanticContext *c, const char *text,
                          NLCheckStatus expected, const char *code)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    NLParser *parser = NULL;
    CHECK(nl_source_create(text, strlen(text), "if-function-unit", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    TestState before;
    CHECK(test_state(c, &before));
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status = nl_semantic_register_function_unit(c, inputs, 1, &d);
    if (status != expected)
        fprintf(stderr, "%s: %d wanted %d (%s)\n", text, status, expected,
                d.diagnostic.diagnostic.code);
    CHECK(status == expected);
    if (status != NL_CHECK_OK) {
        CHECK(test_unchanged(c, &before));
        CHECK(nl_source_span_valid(source, d.diagnostic.span));
        if (code != NULL) {
            if (strcmp(d.diagnostic.diagnostic.code, code) != 0)
                fprintf(stderr, "got %s wanted %s\n",
                        d.diagnostic.diagnostic.code, code);
            CHECK(strcmp(d.diagnostic.diagnostic.code, code) == 0);
        }
    }
    nl_parser_destroy(parser);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
static bool parse_reject(const char *text, const char *code)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    CHECK(nl_source_create(text, strlen(text), "if-invalid", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    for (size_t i = 0; i < 2; ++i) {
        NLSyntaxTree *tree = NULL;
        NLParseDiagnostic d = {0};
        NLParseStatus status =
            nl_parser_parse_source_fragment(parser, &tree, &d);
        CHECK(status != NL_PARSE_OK && tree == NULL &&
              nl_source_span_valid(source, d.span));
        if (code != NULL) {
            if (strcmp(d.diagnostic.code, code) != 0)
                fprintf(stderr, "%s: got %s wanted %s\n", text,
                        d.diagnostic.code, code);
            CHECK(strcmp(d.diagnostic.code, code) == 0);
        }
    }
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool grammar(void)
{
    const char *cases[][2] = {
        {"if cond {unit}else{unit}", "P13-IF-OPEN"},
        {"if (cond else{unit}", "P13-IF-CLOSE"},
        {"if (cond) unit else{unit}", "P13-IF-THEN"},
        {"if (cond) {unit}", "P13-IF-ELSE"},
        {"if (cond) {unit}else", "P13-IF-ELSE-BLOCK"},
        {"if (cond) {unit}else if(cond){unit}else{unit}", "P13-IF-ELSE-BLOCK"},
        {"else()", "P13-ELSE-CONTEXT"},
        {"if()", "P5-EXPECTED-EXPRESSION"},
        {"if (cond) {unit; else{unit}", NULL},
        {"if (cond) {unit}else{unit} junk", NULL},
        {"if ((cond)) {unit}else{unit}", NULL},
        {"{if(cond){}else{} unit}", "P5-EXPECTED-ITEM-END"}};
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        CHECK(parse_reject(cases[i][0], cases[i][1]));
    TestSemantic f = {0};
    CHECK(fixture(&f));
    CHECK(body_ok(f.context, "if\r\n(cond) {x} else\n{y}"));
    CHECK(body_ok(f.context, "{if(cond){}else{};unit}"));
    CHECK(body_ok(f.context, "if(cond){x}else{if(cond){y}else{x}}"));
    const NLAggregateField fields[] = {
        {"if", nl_semantic_core_type(f.context, NL_TYPE_BOOL)},
        {"else", f.copy}};
    NLTypeId record;
    CHECK(nl_semantic_register_aggregate(f.context, "Record", fields, 2,
                                         &record) == NL_CHECK_OK);
    /* W14: a full aggregate expression inside delimiters cannot steal an arm.
     */
    CHECK(nl_semantic_register_function(
              f.context, "is_ok", &record, 1,
              nl_semantic_core_type(f.context, NL_TYPE_BOOL), false,
              false) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "if(is_ok(Record{if:cond,else:x})){x}else{y}"));
    CHECK(test_rejected(f.context, "if(Record{if:cond,else:x}){x}else{y}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P13-IF-CONDITION"));
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(body_tree("if(cond){x}else{y}", &source, &tree));
    const NLSyntaxView *v = nl_syntax_node_view(nl_syntax_tree_root(tree));
    CHECK(v->kind == NL_SYNTAX_IF &&
          nl_syntax_node_view(v->data.conditional.then_block)->kind ==
              NL_SYNTAX_BLOCK &&
          nl_syntax_node_view(v->data.conditional.else_block)->kind ==
              NL_SYNTAX_BLOCK);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    nl_semantic_destroy(f.context);
    return true;
}
static bool bool_type(void)
{
    TestSemantic f = {0};
    CHECK(fixture(&f));
    NLTypeId boolean = nl_semantic_core_type(f.context, NL_TYPE_BOOL);
    NLSemanticTypeView t;
    CHECK(boolean != 0 && nl_semantic_type_view(f.context, boolean, &t) &&
          t.kind == NL_TYPE_BOOL && t.is_copy && t.is_discardable &&
          !t.layout_known);
    TestChecked a = {0};
    CHECK(test_run(f.context, "bool", TEST_TYPE, NL_CHECK_OK, NULL, &a));
    CHECK(test_root(&a)->type == boolean);
    test_checked_destroy(&a);
    NLTypeId collision = 999;
    CHECK(nl_semantic_nominal(f.context, "bool", true, true, &collision) ==
              NL_CHECK_SEMANTIC_ERROR &&
          collision == 999);
    CHECK(body_ok(f.context, "let flag=if(cond){cond}else{cond}"));
    CHECK(test_rejected(f.context, "if(unit){x}else{y}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P13-IF-CONDITION"));
    CHECK(test_rejected(f.context, "if(x){x}else{y}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P13-IF-CONDITION"));
    NLSymbolId b;
    CHECK(nl_semantic_seed_scalar(f.context, "octet",
                                  (NLScalarValue){.type = nl_semantic_core_type(
                                                      f.context, NL_TYPE_BYTE),
                                                  .known = true,
                                                  .value = 1},
                                  &b) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_scalar(f.context, "number",
                                  (NLScalarValue){.type = nl_semantic_core_type(
                                                      f.context, NL_TYPE_U8),
                                                  .known = true,
                                                  .value = 1},
                                  &b) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "if(octet){x}else{y}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P13-IF-CONDITION"));
    CHECK(test_rejected(f.context, "if(number){x}else{y}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P13-IF-CONDITION"));
    CHECK(test_rejected(f.context, "if(cond){x}else{unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P13-IF-RESULT"));
    NLPlaceId root;
    NLValueId value;
    CHECK(nl_semantic_seed_root(f.context, boolean, f.domain, true,
                                NL_DEPENDENCY_FREE, &root,
                                &value) == NL_CHECK_OK);
    CHECK(test_reference(f.context, "br", root, NL_TYPE_REF, NL_ACCESS_WRITE,
                         false, &b, NULL));
    CHECK(body_ok(f.context, "store(br,cond)"));
    const NLSumVariant variants[] = {{"Yes", boolean}, {"No", 0}};
    NLTypeId sum;
    CHECK(nl_semantic_register_sum(f.context, "BoolBox", variants, 2, &sum) ==
          NL_CHECK_OK);
    CHECK(body_ok(f.context, "let bb=BoolBox::Yes(cond)"));
    const NLAggregateField fields[] = {{"flag", boolean}};
    NLTypeId record;
    CHECK(nl_semantic_register_aggregate(f.context, "FlagBox", fields, 1,
                                         &record) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "let box=FlagBox{flag:cond}"));
    CHECK(register_unit(f.context, "fn id_bool(b:bool)->bool{b}", NL_CHECK_OK,
                        NULL));
    CHECK(body_ok(f.context, "id_bool(cond)"));
    CHECK(test_rejected(f.context, "if(true){unit}else{unit}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
    NLPlaceId empty;
    CHECK(nl_semantic_seed_slot(f.context, "empty", boolean, &b, &empty) ==
          NL_CHECK_OK);
    NLScopeId stable_scope;
    CHECK(test_domain_ref(&f, "stable", NL_ACCESS_READ, false, &b,
                          &stable_scope));
    CHECK(test_domain_ref(&f, "ending", NL_ACCESS_READ, true, &b, NULL));
    CHECK(body_ok(f.context, "let token=initialize(empty,cond,stable)"));
    CHECK(nl_semantic_end_scope(f.context, stable_scope) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "if(take(token,ending)){unit}else{unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P13-IF-CONDITION"));
    CHECK(body_ok(f.context, "let (taken,again)=take(token,ending)"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool flow(void)
{
    TestSemantic f = {0};
    CHECK(fixture(&f));
    CHECK(register_unit(
        f.context,
        "fn choose(cond:bool,a:CopyT,b:CopyT)->CopyT{if(cond){a}else{b}} "
        "fn early(cond:bool,x:CopyT,y:CopyT)->CopyT{if(cond){return "
        "x;}else{y}} "
        "fn both(cond:bool,x:CopyT,y:CopyT)->CopyT{if(cond){return "
        "x;}else{return y;};unknown} "
        "fn none(cond:bool)->unit{if(cond){return unit;}else{return "
        "unit;};unknown}",
        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "choose(cond,x,y)"));
    CHECK(body_ok(f.context, "early(cond,x,y)"));
    TestChecked a = {0};
    CHECK(test_run(f.context, "both(cond,x,y)", TEST_SOURCE, NL_CHECK_OK, NULL,
                   &a));
    const NLCheckedFragment *body =
        nl_checked_call_body(a.artifact, nl_checked_root(a.artifact));
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    CHECK(root->terminates && root->type == 0 && root->result_count == 0);
    const NLCheckedNodeView *conditional =
        nl_checked_node_view(body, root->first_item);
    CHECK(conditional->kind == NL_CHECKED_IF && conditional->terminates &&
          conditional->normal_arms == 0 && conditional->type == 0 &&
          conditional->result_count == 0);
    CHECK(nl_checked_if_arm(body, root->first_item, 0) != NULL &&
          nl_checked_if_arm(body, root->first_item, 1) != NULL &&
          nl_checked_if_arm(body, root->first_item, 2) == NULL);
    test_checked_destroy(&a);
    CHECK(body_ok(f.context, "none(cond)"));
    CHECK(register_unit(
        f.context,
        "fn wrong(c:bool,x:CopyT)->CopyT{if(c){return unit;}else{x}}",
        NL_CHECK_SEMANTIC_ERROR, "P9-RETURN-TYPE"));
    CHECK(test_rejected(f.context, "if(cond){return unit;}else{unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "P9-RETURN-CONTEXT"));
    CHECK(seed(f.context, "owner", f.discardable));
    CHECK(nl_semantic_register_function(f.context, "consume", &f.discardable, 1,
                                        1, false, false) == NL_CHECK_OK);
    CHECK(test_rejected(f.context, "if(cond){consume(owner);unit}else{unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P6-AVAILABILITY-JOIN"));
    CHECK(body_ok(f.context,
                  "if(cond){consume(owner);unit}else{consume(owner);unit}"));
    NLSemanticBindingView owner;
    CHECK(body_binding(f.context, "owner", &owner) &&
          owner.availability == NL_CONSUMED);
    /* Condition exactly once: consumes a unique input. Re-evaluating it would
     * fail use-after-consume. Both arm bodies still validate. */
    CHECK(seed(f.context, "test_input", f.discardable));
    CHECK(nl_semantic_register_function(
              f.context, "probe", &f.discardable, 1,
              nl_semantic_core_type(f.context, NL_TYPE_BOOL), false,
              false) == NL_CHECK_OK);
    CHECK(body_ok(f.context, "if(probe(test_input)){x}else{y}"));
    CHECK(body_binding(f.context, "test_input", &owner) &&
          owner.availability == NL_CONSUMED);
    CHECK(register_unit(
        f.context,
        "fn cond_return(c:bool)->unit{if({return unit;}){unit}else{unit}}",
        NL_CHECK_SEMANTIC_ERROR, "P13-IF-CONDITION"));
    CHECK(register_unit(f.context,
                        "fn unit_join(c:bool,u:unit)->unit{if(c){u}else{unit}}",
                        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "unit_join(cond,unit)"));
    CHECK(nl_semantic_register_function(f.context, "consume_linear", &f.linear,
                                        1, 1, false, false) == NL_CHECK_OK);
    CHECK(register_unit(
        f.context,
        "fn "
        "no_false_flow(c:bool,x:LinearT)->unit{if(c){consume_linear(x);return "
        "unit;x;}else{unit};consume_linear(x);unit}",
        NL_CHECK_OK, NULL));
    CHECK(seed(f.context, "linear_input", f.linear));
    CHECK(body_ok(f.context, "no_false_flow(cond,linear_input)"));
    CHECK(register_unit(
        f.context,
        "fn no_cleanup(c:bool,x:LinearT)->unit{if(c){return unit;}else{unit}}",
        NL_CHECK_SEMANTIC_ERROR, "P5-SCOPE-OBLIGATION"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool identity(void)
{
    TestSemantic f = {0};
    CHECK(fixture(&f));
    CHECK(seed(f.context, "owner", f.linear));
    NLSemanticBindingView before;
    CHECK(body_binding(f.context, "owner", &before));
    TestChecked a = {0};
    CHECK(test_run(f.context, "if(cond){owner}else{owner}", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &a));
    CHECK(test_root(&a)->results[0].value == before.value &&
          test_root(&a)->normal_arms == 2);
    NLSemanticValueView value;
    CHECK(nl_semantic_value_view(f.context, before.value, &value) &&
          value.carrier == NL_CARRIER_LOOSE);
    test_checked_destroy(&a);
    CHECK(seed(f.context, "first", f.discardable) &&
          seed(f.context, "second", f.discardable));
    CHECK(test_rejected(
        f.context, "if(cond){let v=first;second;v}else{let v=second;first;v}",
        TEST_SOURCE, NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P13-JOIN-PRECISION"));
    CHECK(register_unit(
        f.context,
        "fn same(c:bool,x:LinearT)->LinearT{if(c){x}else{x}} "
        "fn return_same(c:bool,x:LinearT)->LinearT{if(c){return x;}else{x}} "
        "fn both_same(c:bool,x:LinearT)->LinearT{if(c){return x;}else{return "
        "x;};x}",
        NL_CHECK_OK, NULL));
    for (size_t i = 0; i < 3; ++i) {
        const char *names[] = {"same", "return_same", "both_same"};
        char input[32], text[96];
        snprintf(input, sizeof(input), "input%zu", i);
        CHECK(seed(f.context, input, f.linear));
        CHECK(body_binding(f.context, input, &before));
        snprintf(text, sizeof(text), "%s(cond,%s)", names[i], input);
        a = (TestChecked){0};
        CHECK(test_run(f.context, text, TEST_SOURCE, NL_CHECK_OK, NULL, &a));
        CHECK(test_root(&a)->results[0].value == before.value);
        test_checked_destroy(&a);
    }
    /* Distinct return-vs-tail identity must not pick the sole normal tail as
     * the function's only possible result. */
    CHECK(register_unit(
        f.context,
        "fn distinct(c:bool,x:AffineT,y:AffineT)->AffineT{if(c){return "
        "x;}else{y}}",
        NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P13-JOIN-PRECISION"));
    CHECK(register_unit(
        f.context,
        "fn distinct_exit(c:bool,x:AffineT,y:AffineT)->AffineT{if(c){return "
        "x;}else{return y;}}",
        NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P13-JOIN-PRECISION"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool refs(void)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, true));
    CHECK(seed(f.sem.context, "cond",
               nl_semantic_core_type(f.sem.context, NL_TYPE_BOOL)));
    TestChecked a = {0};
    CHECK(test_run(f.sem.context, "if(cond){fallback}else{other}", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &a));
    NLValueId joined = test_root(&a)->results[0].value;
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(f.sem.context, joined, &v) &&
          v.reference_count == 2);
    CHECK(v.references[0].place == f.a_place &&
          v.references[1].place == f.b_place);
    CHECK(v.references[0].scope == f.a_scope &&
          v.references[1].scope == f.b_scope);
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm =
            nl_checked_if_arm(a.artifact, nl_checked_root(a.artifact), i);
        CHECK(arm && nl_checked_context(arm) != f.sem.context);
    }
    NLSymbolId symbol;
    CHECK(nl_semantic_bind_result(f.sem.context, "joined", joined, &symbol) ==
          NL_CHECK_OK);
    test_checked_destroy(&a);
    CHECK(body_ok(f.sem.context, "observe(joined)"));
    CHECK(test_rejected(f.sem.context, "ptr_from_ref(joined)", TEST_SOURCE,
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P7-SINGULAR-REF-PRECISION"));
    CHECK(nl_semantic_end_scope(f.sem.context, f.b_scope) == NL_CHECK_OK);
    CHECK(test_rejected(f.sem.context, "observe(joined)", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-DEAD-SCOPE"));
    nl_semantic_destroy(f.sem.context);
    f = (JoinFixture){0};
    CHECK(join_create(&f, true));
    CHECK(seed(f.sem.context, "cond",
               nl_semantic_core_type(f.sem.context, NL_TYPE_BOOL)));
    CHECK(body_ok(
        f.sem.context,
        "let nested=if(cond){if(cond){fallback}else{other}}else{fallback}"));
    CHECK(join_value(&f, "nested", &v) && v.reference_count == 2);
    /* W7: host-known ordinary function boundary allows a ref as a local result,
     * then discards it and returns unit. Return arm contributes no alternative.
     */
    CHECK(register_unit(f.sem.context,
                        "fn local_ref(c:bool,a:ref<read,CopyT>)->unit{let "
                        "r=if(c){return unit;}else{a};observe(r);unit}",
                        NL_CHECK_OK, NULL));
    a = (TestChecked){0};
    CHECK(test_run(f.sem.context, "local_ref(cond,fallback)", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &a));
    const NLCheckedFragment *body =
        nl_checked_call_body(a.artifact, nl_checked_root(a.artifact));
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    const NLCheckedNodeView *binding =
        nl_checked_node_view(body, root->first_item);
    const NLCheckedNodeView *conditional =
        nl_checked_node_view(body, binding->initializer);
    CHECK(conditional->normal_arms == 1 && conditional->result_count == 1);
    CHECK(nl_semantic_value_view(f.sem.context, conditional->results[0].value,
                                 &v) &&
          v.reference_count == 0 && v.reference.place == f.a_place);
    test_checked_destroy(&a);
    CHECK(register_unit(f.sem.context,
                        "fn "
                        "bad_ref(c:bool,a:ref<read,CopyT>,b:ref<read,CopyT>)->"
                        "ref<read,CopyT>{if(c){a}else{b}}",
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P8-SIGNATURE-PRECISION"));
    CHECK(test_rejected(
        f.sem.context,
        "if(cond){ptr_from_ref(fallback)}else{ptr_from_ref(other)}",
        TEST_SOURCE, NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P13-JOIN-PRECISION"));
    nl_semantic_destroy(f.sem.context);
    return true;
}
static bool effects(void)
{
    TestSemantic f = {0};
    CHECK(fixture(&f));
    NLSemanticBindingView x;
    CHECK(body_binding(f.context, "x", &x));
    NLSymbolId write;
    CHECK(test_reference(f.context, "write", x.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &write, NULL));
    NLValueFactId fact;
    NLSemanticPlaceView p;
    CHECK(nl_semantic_place_view(f.context, x.place, &p));
    fact = p.current_fact;
    CHECK(body_ok(f.context,
                  "if(cond){store(write,x);unit}else{store(write,y);unit}"));
    CHECK(nl_semantic_place_view(f.context, x.place, &p) &&
          p.current_fact != fact && p.incarnation != 0);
    /* Non-Copy current-value identities cannot be silently chosen. */
    CHECK(seed(f.context, "linear_site", f.discardable));
    NLSemanticBindingView site;
    CHECK(body_binding(f.context, "linear_site", &site));
    CHECK(test_reference(f.context, "linear_write", site.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &write, NULL));
    CHECK(seed(f.context, "a", f.discardable) &&
          seed(f.context, "b", f.discardable));
    CHECK(test_rejected(f.context,
                        "if(cond){store(linear_write,a);b;unit}else{store("
                        "linear_write,b);a;unit}",
                        TEST_SOURCE, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P6-JOIN-PRECISION"));
    /* Return mutation is absent from the IF normal continuation, yet the
     * function exit joins its possible caller-visible effects soundly. */
    CHECK(register_unit(
        f.context,
        "fn normal_state(c:bool,r:ref<write,CopyT>,a:CopyT)->unit{let "
        "v=if(c){store(r,a);return unit;}else{unit};unit}",
        NL_CHECK_OK, NULL));
    TestChecked checked = {0};
    CHECK(test_run(f.context, "normal_state(cond,write,x)", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &checked));
    const NLCheckedFragment *body = nl_checked_call_body(
        checked.artifact, nl_checked_root(checked.artifact));
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    const NLCheckedNodeView *binding =
        nl_checked_node_view(body, root->first_item);
    const NLCheckedNodeView *conditional =
        nl_checked_node_view(body, binding->initializer);
    const NLCheckedFragment *normal =
        nl_checked_if_arm(body, binding->initializer, 1);
    NLSemanticPlaceView normalp;
    CHECK(
        nl_semantic_place_view(nl_checked_context(normal), x.place, &normalp));
    CHECK(conditional->normal_arms == 1 &&
          nl_semantic_place_view(f.context, x.place, &p) &&
          p.current_fact != normalp.current_fact);
    test_checked_destroy(&checked);
    CHECK(register_unit(f.context,
                        "fn "
                        "mutate(c:bool,r:ref<write,CopyT>,a:CopyT,b:CopyT)->"
                        "unit{if(c){store(r,a);unit}else{store(r,b);unit}}",
                        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "mutate(cond,write,x,y)"));
    CHECK(register_unit(f.context,
                        "fn "
                        "early_mutate(c:bool,r:ref<write,CopyT>,a:CopyT)->unit{"
                        "if(c){store(r,a);return unit;}else{unit}}",
                        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "early_mutate(cond,write,x)"));
    /* Actual aliases retain the existing same-place swap/no-op semantics. */
    NLTypeId rw;
    CHECK(nl_semantic_compound_type(f.context, NL_TYPE_REF, f.copy,
                                    NL_ACCESS_WRITE, false,
                                    &rw) == NL_CHECK_OK);
    NLFunctionParameter params[] = {
        {"c", nl_semantic_core_type(f.context, NL_TYPE_BOOL)},
        {"a", rw},
        {"b", rw}};
    CHECK(register_body(f.context, "exchange", params, 3, 1,
                        "{if(c){swap(a,b);unit}else{unit}}", NL_CHECK_OK,
                        NULL));
    CHECK(body_ok(f.context, "exchange(cond,write,write)"));
    CHECK(nl_semantic_place_view(f.context, x.place, &p));
    fact = p.current_fact;
    CHECK(body_ok(f.context, "exchange(cond,write,write)"));
    CHECK(nl_semantic_place_view(f.context, x.place, &p) &&
          p.current_fact == fact);
    CHECK(register_unit(f.context,
                        "fn "
                        "affine_effect(c:bool,r:ref<write,AffineT>,a:AffineT)->"
                        "unit{if(c){store(r,a);return unit;}else{unit}}",
                        NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                        "P13-JOIN-PRECISION"));
    NLTypeId u8 = nl_semantic_core_type(f.context, NL_TYPE_U8);
    NLSymbolId scalar;
    CHECK(nl_semantic_seed_scalar(f.context, "octet_site",
                                  (NLScalarValue){u8, true, 0},
                                  &scalar) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_scalar(f.context, "seven",
                                  (NLScalarValue){u8, true, 7},
                                  &scalar) == NL_CHECK_OK);
    CHECK(nl_semantic_seed_scalar(f.context, "nine",
                                  (NLScalarValue){u8, true, 9},
                                  &scalar) == NL_CHECK_OK);
    NLSemanticBindingView octet;
    CHECK(body_binding(f.context, "octet_site", &octet));
    CHECK(test_reference(f.context, "octet_write", octet.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &scalar, NULL));
    CHECK(register_unit(f.context,
                        "fn "
                        "octet_effect(c:bool,r:ref<write,u8>,a:u8,b:u8)->unit{"
                        "if(c){store(r,a);return unit;}else{store(r,b);unit}}",
                        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "octet_effect(cond,octet_write,seven,nine)"));
    NLSemanticValueView current;
    CHECK(nl_semantic_place_view(f.context, octet.place, &p) &&
          nl_semantic_value_view(f.context, p.current_value, &current) &&
          !current.scalar_known);
    nl_semantic_destroy(f.context);
    return true;
}
static bool nested(void)
{
    TestSemantic f = {0};
    CHECK(fixture(&f));
    const NLSumVariant variants[] = {{"A", 0}, {"B", 0}};
    NLTypeId tag;
    CHECK(nl_semantic_register_sum(f.context, "Tag", variants, 2, &tag) ==
          NL_CHECK_OK);
    CHECK(body_ok(f.context, "let tag=Tag::A"));
    CHECK(body_ok(f.context, "if(cond){match tag{A=>{x},B=>{y}}}else{y}"));
    CHECK(body_ok(f.context, "match tag{A=>{if(cond){x}else{y}},B=>{x}}"));
    CHECK(register_unit(
        f.context,
        "fn inner(c:bool,t:Tag,x:CopyT,y:CopyT)->CopyT{if(c){match "
        "t{A=>{x},B=>{return y;}}}else{y}} "
        "fn outer(c:bool,t:Tag,x:CopyT,y:CopyT)->CopyT{match "
        "t{A=>{if(c){x}else{return y;}},B=>{return y;}}} "
        "fn nested(c:bool,x:CopyT,y:CopyT)->CopyT{if(c){if(c){return "
        "x;}else{y}}else{if(c){x}else{y}}}",
        NL_CHECK_OK, NULL));
    CHECK(body_ok(f.context, "inner(cond,tag,x,y)"));
    CHECK(body_ok(f.context, "outer(cond,tag,x,y)"));
    CHECK(body_ok(f.context, "nested(cond,x,y)"));
    /* Resource fences and all-arm validation: a bad second/nested arm is never
     * skipped; no truthiness or runtime-selected arm assumption. */
    CHECK(test_rejected(f.context, "if(cond){store_missing()}else{unit}",
                        TEST_SOURCE, NL_CHECK_SEMANTIC_ERROR,
                        "P3-UNKNOWN-CALLEE"));
    CHECK(test_rejected(f.context, "if(cond){unit}else{unknown}", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
    CHECK(register_unit(f.context,
                        "fn bad(c:bool)->unit{if(c){unit}else{unknown}}",
                        NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
    nl_semantic_destroy(f.context);
    return true;
}
static bool faults(NLSemanticContext *c, const char *text, bool unit,
                   NLCheckStatus expected)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_create(text, strlen(text), "if-oom", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    bool complete = false;
    for (fail_at = 0; fail_at < 4000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLParseStatus status =
            unit ? nl_parser_parse_function_unit(parser, &tree, NULL)
                 : nl_parser_parse_source_fragment(parser, &tree, NULL);
        injecting = false;
        if (status == NL_PARSE_OK) {
            complete = true;
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
    }
    CHECK(complete && fail_at > 0);
    nl_parser_destroy(parser);
    TestState before;
    CHECK(test_state(c, &before));
    complete = false;
    for (fail_at = 0; fail_at < 10000; ++fail_at) {
        NLCheckedFragment *artifact = NULL;
        NLCheckDiagnostic d = {0};
        NLFunctionUnitDiagnostic u = {0};
        const NLSyntaxTree *inputs[] = {tree};
        allocation_index = 0;
        injecting = true;
        NLCheckStatus status =
            unit ? nl_semantic_register_function_unit(c, inputs, 1, &u)
                 : nl_semantic_check_source_fragment(c, tree, &artifact, &d);
        injecting = false;
        if (status == expected) {
            if (status != NL_CHECK_OK)
                CHECK(test_unchanged(c, &before) && artifact == NULL);
            nl_checked_destroy(artifact);
            complete = true;
            break;
        }
        if (status != NL_CHECK_OUT_OF_MEMORY)
            fprintf(stderr, "OOM %zu got %d (%s)\n", fail_at, status,
                    unit ? u.diagnostic.diagnostic.code : d.diagnostic.code);
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && artifact == NULL &&
              test_unchanged(c, &before));
    }
    CHECK(complete && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
static bool failure(void)
{
    TestSemantic f = {0};
    CHECK(fixture(&f));
    NLSemanticBindingView x;
    CHECK(body_binding(f.context, "x", &x));
    NLSymbolId r;
    CHECK(test_reference(f.context, "write", x.place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &r, NULL));
    CHECK(faults(f.context, "if(cond){store(write,y);x}else{unknown}", false,
                 NL_CHECK_SEMANTIC_ERROR));
    CHECK(faults(f.context, "if(cond){if(cond){x}else{y}}else{x}", false,
                 NL_CHECK_OK));
    CHECK(faults(f.context,
                 "fn f(c:bool,x:CopyT)->CopyT{if(c){return x;}else{x}}", true,
                 NL_CHECK_OK));
    CHECK(faults(f.context, "f(cond,x)", false, NL_CHECK_OK));
    nl_semantic_destroy(f.context);
    JoinFixture j = {0};
    CHECK(join_create(&j, true));
    CHECK(seed(j.sem.context, "cond",
               nl_semantic_core_type(j.sem.context, NL_TYPE_BOOL)));
    CHECK(
        faults(j.sem.context,
               "let joined=if(cond){if(cond){fallback}else{other}}else{other}",
               false, NL_CHECK_OK));
    nl_semantic_destroy(j.sem.context);
    return true;
}
static bool limits(void)
{
    TestSemantic f = {0};
    CHECK(fixture(&f));
    char text[16000];
    size_t n = 0;
    for (size_t i = 0; i < 140; ++i)
        n += (size_t)snprintf(text + n, sizeof(text) - n, "if(cond){");
    n += (size_t)snprintf(text + n, sizeof(text) - n, "x");
    for (size_t i = 0; i < 140; ++i)
        n += (size_t)snprintf(text + n, sizeof(text) - n, "}else{x}");
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_create(text, strlen(text), "if-depth", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_source_fragment(parser, &tree, NULL) ==
              NL_PARSE_RESOURCE_LIMIT &&
          tree == NULL);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    /* A finite two-arm tree is bounded by existing owned-evidence budget. */
    n = 0;
    for (size_t i = 0; i < 34; ++i)
        n += (size_t)snprintf(text + n, sizeof(text) - n, "if(cond){}else{};");
    char block[16010];
    snprintf(block, sizeof(block), "{%s}", text);
    CHECK(test_rejected(f.context, block, TEST_SOURCE, NL_CHECK_RESOURCE_LIMIT,
                        NULL));
    n = 0;
    for (size_t i = 0; i < 15; ++i)
        n += (size_t)snprintf(text + n, sizeof(text) - n,
                              "if(c){return unit;}else{");
    n += (size_t)snprintf(text + n, sizeof(text) - n, "unit");
    for (size_t i = 0; i < 15; ++i)
        n += (size_t)snprintf(text + n, sizeof(text) - n, "}");
    char declaration[17000];
    snprintf(declaration, sizeof(declaration), "fn bounded(c:bool)->unit{%s}",
             text);
    CHECK(register_unit(f.context, declaration, NL_CHECK_RESOURCE_LIMIT,
                        "P13-BRANCH-WORK-LIMIT"));
    nl_semantic_destroy(f.context);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "grammar") == 0)
        return grammar() ? 0 : 1;
    if (strcmp(argv[1], "bool") == 0)
        return bool_type() ? 0 : 1;
    if (strcmp(argv[1], "flow") == 0)
        return flow() ? 0 : 1;
    if (strcmp(argv[1], "identity") == 0)
        return identity() ? 0 : 1;
    if (strcmp(argv[1], "ref") == 0)
        return refs() ? 0 : 1;
    if (strcmp(argv[1], "effects") == 0)
        return effects() ? 0 : 1;
    if (strcmp(argv[1], "nested") == 0)
        return nested() ? 0 : 1;
    if (strcmp(argv[1], "failure") == 0)
        return failure() ? 0 : 1;
    if (strcmp(argv[1], "limits") == 0)
        return limits() ? 0 : 1;
    return 2;
}
