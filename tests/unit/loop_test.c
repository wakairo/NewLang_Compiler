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
typedef struct {
    NLSemanticContext *c;
    NLTypeId copy, owner, boolean;
} Fixture;
static bool seed(Fixture *f, const char *name, NLTypeId type)
{
    NLSymbolId symbol;
    CHECK(nl_semantic_seed_value(f->c, name, type, NL_DEPENDENCY_FREE,
                                 &symbol) == NL_CHECK_OK);
    return true;
}
static bool setup(Fixture *f)
{
    CHECK(nl_semantic_create(&f->c) == NL_CHECK_OK);
    CHECK(nl_semantic_nominal(f->c, "Copy", true, true, &f->copy) ==
          NL_CHECK_OK);
    f->boolean = nl_semantic_core_type(f->c, NL_TYPE_BOOL);
    CHECK(nl_semantic_nominal(f->c, "Owner", false, false, &f->owner) ==
          NL_CHECK_OK);
    CHECK(seed(f, "x", f->copy) && seed(f, "y", f->copy));
    CHECK(seed(f, "cond", f->boolean) && seed(f, "other", f->boolean));
    CHECK(seed(f, "owner", f->owner) && seed(f, "second", f->owner));
    CHECK(nl_semantic_register_function(f->c, "next", &f->copy, 1, f->copy,
                                        false, false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f->c, "done", &f->copy, 1, f->boolean,
                                        false, false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f->c, "make", NULL, 0, f->owner, false,
                                        false) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f->c, "consume", &f->owner, 1, 1, false,
                                        false) == NL_CHECK_OK);
    return true;
}
static bool rejected(Fixture *f, const char *text, NLCheckStatus status,
                     const char *code)
{
    return test_rejected(f->c, text, TEST_SOURCE, status, code);
}
static bool accepted(Fixture *f, const char *text)
{
    return body_ok(f->c, text);
}
static bool grammar(const char *text, const char *code)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    CHECK(nl_source_create(text, strlen(text), "loop-invalid", &source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    for (size_t i = 0; i < 2; ++i) {
        NLSyntaxTree *tree = NULL;
        NLParseDiagnostic d = {0};
        CHECK(nl_parser_parse_source_fragment(parser, &tree, &d) !=
              NL_PARSE_OK);
        CHECK(tree == NULL && nl_source_span_valid(source, d.span));
        if (code != NULL) {
            if (strcmp(d.diagnostic.code, code) != 0)
                fprintf(stderr, "%s got %s wanted %s\n", text,
                        d.diagnostic.code, code);
            CHECK(strcmp(d.diagnostic.code, code) == 0);
        }
    }
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool evidence(Fixture *f, const char *text, size_t cont, size_t brk,
                     size_t ret)
{
    TestChecked checked = {0};
    CHECK(test_run(f->c, text, TEST_SOURCE, NL_CHECK_OK, NULL, &checked));
    const NLCheckedNodeView *root = test_root(&checked);
    CHECK(root->kind == NL_CHECKED_LOOP && root->header_inductive);
    CHECK(root->continue_edges == cont && root->break_edges == brk &&
          root->return_edges == ret);
    CHECK(root->terminates == (brk == 0));
    const NLCheckedFragment *body = nl_checked_loop_body(
        checked.artifact, nl_checked_root(checked.artifact));
    CHECK(body != NULL && nl_checked_context(body) != f->c);
    CHECK(nl_control_exits_count(nl_checked_control_exits(body)) ==
          cont + brk + ret);
    if (brk == 0)
        CHECK(root->type == 0 && root->result_count == 0 &&
              root->returned.type == 0);
    /* Continue/Break are owned only under the loop, never function exits. */
    CHECK(nl_control_exits_count(nl_checked_control_exits(checked.artifact)) ==
          ret);
    test_checked_destroy(&checked);
    return true;
}
static bool ref_alternatives(Fixture *f)
{
    NLSemanticBindingView x, y;
    CHECK(body_binding(f->c, "x", &x) && body_binding(f->c, "y", &y));
    NLSymbolId r;
    CHECK(test_reference(f->c, "a", x.place, NL_TYPE_REF, NL_ACCESS_READ, false,
                         &r, NULL));
    CHECK(test_reference(f->c, "b", y.place, NL_TYPE_REF, NL_ACCESS_READ, false,
                         &r, NULL));
    CHECK(accepted(f, "let result=loop () {if(cond){break a;}else{break b;}}"));
    NLSemanticBindingView result;
    CHECK(body_binding(f->c, "result", &result));
    NLSemanticValueView value;
    CHECK(nl_semantic_value_view(f->c, result.value, &value));
    CHECK(value.reference_count == 2 &&
          value.references[0].place != value.references[1].place);
    CHECK(value.references[0].scope != 0 && value.references[1].scope != 0);
    return true;
}
static bool workload(size_t w)
{
    Fixture f = {0};
    CHECK(setup(&f));
    bool ok = false;
    switch (w) {
    case 1:
        ok = evidence(&f, "loop () {continue();}", 1, 0, 0);
        break;
    case 2:
        ok = grammar("loop x {continue();}", "P14-LOOP-OPEN");
        break;
    case 3:
        ok = grammar("loop {continue();}", "P14-LOOP-OPEN");
        break;
    case 4:
        ok = grammar("loop (i=x,) {continue(i);}", "P14-TRAILING-COMMA");
        break;
    case 5:
        ok = rejected(&f, "loop(i=x,i=y){continue(i,i);}",
                      NL_CHECK_SEMANTIC_ERROR, "P14-DUPLICATE-PARAMETER");
        break;
    case 6:
        ok = rejected(&f, "loop(loop=x){continue(x);}", NL_CHECK_SEMANTIC_ERROR,
                      "P12-RESERVED-STRUCTURAL-NAME");
        CHECK(rejected(&f, "loop(unit=x){continue(x);}",
                       NL_CHECK_SEMANTIC_ERROR, "P10-RESERVED-NAME"));
        break;
    case 7:
        ok = accepted(&f, "loop(x=owner,y=x){consume(x);break y;}");
        CHECK(rejected(&f, "loop(fresh=y,k=fresh){continue(fresh,k);}",
                       NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
        break;
    case 8:
        ok = rejected(&f, "loop(a=owner,b=owner){continue(a,b);}",
                      NL_CHECK_SEMANTIC_ERROR, "P3-USE-AFTER-CONSUME");
        break;
    case 9:
        ok = evidence(&f, "loop(){if(cond){continue();}else{continue();}}", 2,
                      0, 0);
        break;
    case 10:
        ok = rejected(&f, "loop(i=x){continue();}", NL_CHECK_SEMANTIC_ERROR,
                      "P14-CONTINUE-ARITY");
        break;
    case 11:
        ok = rejected(&f, "loop(i=x){continue(cond);}", NL_CHECK_SEMANTIC_ERROR,
                      "P14-CONTINUE-TYPE");
        break;
    case 12:
        ok = rejected(&f, "{continue();}", NL_CHECK_SEMANTIC_ERROR,
                      "P14-CONTROL-CONTEXT");
        break;
    case 13:
        ok = rejected(&f, "{break x;}", NL_CHECK_SEMANTIC_ERROR,
                      "P14-CONTROL-CONTEXT");
        break;
    case 14:
        ok = grammar("loop(){continue()}", "P14-CONTROL-SEMICOLON");
        break;
    case 15:
        ok = grammar("loop(){break unit}", "P14-CONTROL-SEMICOLON");
        break;
    case 16:
        ok = grammar("loop(){break;}", "P14-BARE-BREAK");
        break;
    case 17:
        ok = accepted(&f, "loop(){break unit;}");
        break;
    case 18:
        ok = rejected(&f, "loop(){unit}", NL_CHECK_SEMANTIC_ERROR,
                      "P14-BODY-FALLTHROUGH");
        CHECK(rejected(&f, "loop(){if(cond){continue();}else{unit}}",
                       NL_CHECK_SEMANTIC_ERROR, "P14-BODY-FALLTHROUGH"));
        break;
    case 19: {
        NLSemanticBindingView x;
        CHECK(body_binding(f.c, "x", &x));
        f.c->values[x.value - 1].scalar_known = true;
        f.c->values[x.value - 1].scalar_value = 7;
    }
        ok = evidence(
            &f, "loop(i=x){if(done(i)){break i;}else{continue(next(i));}}", 1,
            1, 0);
        {
            TestChecked a = {0};
            CHECK(test_run(f.c,
                           "loop(i=x){if(cond){break i;}else{continue(y);}}",
                           TEST_SOURCE, NL_CHECK_OK, NULL, &a));
            NLValueId output = test_root(&a)->results[0].value;
            CHECK(output != 0 && !f.c->values[output - 1].scalar_known);
            test_checked_destroy(&a);
        }
        break;
    case 20:
        ok = evidence(&f, "loop(o=owner){continue(o);}", 1, 0, 0);
        CHECK(accepted(
            &f,
            "let result=loop(o=second){if(cond){break o;}else{continue(o);}}"));
        {
            NLSemanticBindingView b;
            CHECK(body_binding(f.c, "result", &b));
            CHECK(b.availability == NL_AVAILABLE);
        }
        break;
    case 21:
        ok = rejected(&f, "loop(o=owner){consume(o);continue(make());}",
                      NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P14-LOOP-PRECISION");
        break;
    case 22:
        ok = rejected(&f, "loop(){consume(owner);continue();}",
                      NL_CHECK_SEMANTIC_ERROR, "P14-CONTINUE-AVAILABILITY");
        break;
    case 23: {
        NLSemanticBindingView x;
        CHECK(body_binding(f.c, "x", &x));
        NLSymbolId symbol;
        CHECK(test_reference(f.c, "r", x.place, NL_TYPE_REF, NL_ACCESS_READ,
                             false, &symbol, NULL));
        ok = rejected(&f, "loop(rp=r){continue(rp);}",
                      NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P14-LOOP-PRECISION");
        /* Rich local-derived capabilities are rejected, never dep-free top. */
        CHECK(rejected(&f, "loop(rp=r){continue(rp);}",
                       NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                       "P14-LOOP-PRECISION"));
        /* A loose capability derived from an ended local place cannot be
         * hidden by projection, independently of carried-ref precision. */
        NLControlState *entry = NULL, *post = NULL;
        CHECK(nl_control_state_create(f.c, &entry) == NL_CHECK_OK);
        NLSemanticContext *b = NULL;
        CHECK(nl_sem_clone(f.c, &b) == NL_CHECK_OK);
        NLValueId local = 0;
        NLSymbolId local_symbol = 0;
        CHECK(nl_sem_new_value(b, (NLSemanticValueView){.type = f.copy},
                               &local) == NL_CHECK_OK);
        CHECK(nl_sem_bind_in_scope(b, "local", local, b->binding_count,
                                   &local_symbol) == NL_CHECK_OK);
        NLPlaceId place = b->bindings[local_symbol - 1].view.place;
        CHECK(test_reference(b, "escaping", place, NL_TYPE_REF, NL_ACCESS_READ,
                             false, &symbol, NULL));
        NLValueId capability = b->bindings[symbol - 1].view.value;
        b->values[capability - 1].carrier = NL_CARRIER_LOOSE;
        b->values[capability - 1].owner_place = 0;
        CHECK(nl_control_state_project(entry, b, &post) ==
                  NL_CHECK_SEMANTIC_ERROR &&
              post == NULL);
        nl_semantic_destroy(b);
        nl_control_state_destroy(entry);
        break;
    }
    case 24:
        ok = evidence(&f, "loop(){break x;}", 0, 1, 0);
        break;
    case 25:
        ok = rejected(&f, "loop(){if(cond){break x;}else{break cond;}}",
                      NL_CHECK_SEMANTIC_ERROR, "P14-BREAK-TYPE");
        break;
    case 26:
        ok = ref_alternatives(&f);
        break;
    case 27:
        ok = rejected(&f, "loop(){if(cond){break make();}else{break make();}}",
                      NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P14-LOOP-PRECISION");
        break;
    case 28:
        ok = evidence(&f, "loop(){continue();consume(owner);}", 1, 0, 0);
        CHECK(register_body(f.c, "diverge", NULL, 0, f.owner,
                            "{loop(){continue();}}", NL_CHECK_OK, NULL));
        {
            TestChecked a = {0};
            CHECK(
                test_run(f.c, "diverge()", TEST_SOURCE, NL_CHECK_OK, NULL, &a));
            CHECK(test_root(&a)->terminates && test_root(&a)->type == 0 &&
                  test_root(&a)->result_count == 0);
            test_checked_destroy(&a);
        }
        break;
    case 29: {
        const NLFunctionParameter p[] = {
            {"flag", f.boolean}, {"flag2", f.boolean}, {"v", f.copy}};
        CHECK(
            register_body(f.c, "separate", p, 3, f.copy,
                          "{loop(i=v){if(flag){return i;}else{if(flag2){break "
                          "i;}else{continue(next(i));}}}}",
                          NL_CHECK_OK, NULL));
        ok = accepted(&f, "separate(cond,other,x)");
        const NLFunctionParameter q[] = {
            {"resource", f.owner}, {"flag", f.boolean}, {"v", f.copy}};
        CHECK(register_body(
            f.c, "return_only", q, 3, f.copy,
            "{loop(){if(flag){consume(resource);return v;}else{continue();}}}",
            NL_CHECK_OK, NULL));
        NLSemanticBindingView owner;
        CHECK(body_binding(f.c, "owner", &owner));
        NLValueId package = owner.value;
        CHECK(accepted(&f, "return_only(owner,cond,x)"));
        NLSemanticValueView ended;
        CHECK(nl_semantic_value_view(f.c, package, &ended) &&
              ended.carrier == NL_CARRIER_ENDED);
        CHECK(register_body(f.c, "init_return", NULL, 0, 1,
                            "{loop(p={return unit;}){continue(p);}}",
                            NL_CHECK_OK, NULL));
        CHECK(accepted(&f, "init_return()"));
        break;
    }
    case 30:
        ok = evidence(&f, "loop(){if(cond){{continue();}}else{{break x;}}}", 1,
                      1, 0);
        break;
    case 31: {
        NLTypeId choice;
        const NLSumVariant variants[] = {{"A", 0}, {"B", 0}};
        CHECK(nl_semantic_register_sum(f.c, "Choice", variants, 2, &choice) ==
              NL_CHECK_OK);
        ok = evidence(&f,
                      "loop(){match Choice.A{A=>{continue();},B=>{break x;}}}",
                      1, 1, 0);
        CHECK(evidence(&f,
                       "loop(i=x){match "
                       "Choice.A{A=>{continue(i);},B=>{unit}};continue(y);}",
                       2, 0, 0));
        break;
    }
    case 32:
        ok = evidence(&f, "loop(){loop(){break unit;};continue();}", 1, 0, 0);
        CHECK(rejected(&f, "loop(){loop(){continue();}}",
                       NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                       "P14-LOOP-PRECISION"));
        break;
    case 33:
        ok = register_body(f.c, "invalid", NULL, 0, 1, "{continue();}",
                           NL_CHECK_SEMANTIC_ERROR, "P14-CONTROL-CONTEXT");
        CHECK(register_body(f.c, "ordinary", NULL, 0, 1,
                            "{loop(){break unit;}}", NL_CHECK_OK, NULL));
        CHECK(accepted(&f, "loop(){ordinary();continue();}"));
        break;
    case 34:
        ok = rejected(&f,
                      "loop(o=owner){if(cond){consume(o);continue(make());}"
                      "else{consume(o);continue(make());}}",
                      NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P14-LOOP-PRECISION");
        break;
    case 35: {
        NLSemanticBindingView b;
        CHECK(body_binding(f.c, "cond", &b));
        f.c->values[b.value - 1].scalar_known = true;
        f.c->values[b.value - 1].scalar_value = 1;
    }
        ok = evidence(
            &f, "loop(i=x){if(cond){continue(next(i));}else{continue(y);}}", 2,
            0, 0);
        CHECK(rejected(
            &f, "loop(){if(cond){continue();}else{consume(owner);continue();}}",
            NL_CHECK_SEMANTIC_ERROR, "P14-CONTINUE-AVAILABILITY"));
        CHECK(nl_semantic_register_function(f.c, "blocked", NULL, 0, f.copy,
                                            false, true) == NL_CHECK_OK);
        CHECK(rejected(
            &f, "loop(i=x){if(cond){continue(i);}else{continue(blocked());}}",
            NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P3-DEPENDENCIES-UNSUPPORTED"));
        break;
    case 36: {
        NLSemanticBindingView x;
        NLSymbolId r;
        CHECK(body_binding(f.c, "x", &x));
        CHECK(test_reference(f.c, "rw", x.place, NL_TYPE_REF, NL_ACCESS_WRITE,
                             false, &r, NULL));
        ok = rejected(&f, "loop(){store(rw,y);continue();}",
                      NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P14-LOOP-PRECISION");
        break;
    }
    case 37:
        ok = grammar("continue()", "P14-CONTROL-ITEM");
        CHECK(grammar("break x", "P14-CONTROL-ITEM"));
        CHECK(grammar("loop(x)", "P14-LOOP-INITIALIZER"));
        CHECK(grammar("loop(){continue(x,);}", "P14-TRAILING-COMMA"));
        CHECK(grammar("loop(){continue;}", "P14-CONTINUE-OPEN"));
        break;
    case 38: {
        NLTypeId record;
        const NLAggregateField fields[] = {
            {"loop", f.copy}, {"continue", f.copy}, {"break", f.copy}};
        CHECK(nl_semantic_register_aggregate(f.c, "Labels", fields, 3,
                                             &record) == NL_CHECK_OK);
        ok = accepted(&f, "Labels{loop:x,continue:y,break:x}");
        break;
    }
    case 39:
        ok = accepted(&f, "{let loops=x;let Continue=y;let breaks=x;unit}");
        break;
    default:
        CHECK(false);
    }
    nl_semantic_destroy(f.c);
    return ok;
}
static bool failure(void)
{
    const char *texts[] = {
        "loop(a=x,b=y){if(cond){continue(b,a);}else{break a;}}",
        "loop(o=owner){if(cond){continue(o);}else{break o;}}",
        "loop(){loop(){break unit;};break x;}",
        "loop(){if(cond){break a;}else{break b;}}", "return_path(cond,x)"};
    for (size_t t = 0; t < sizeof(texts) / sizeof(texts[0]); ++t) {
        Fixture f = {0};
        CHECK(setup(&f));
        if (t == 3) {
            NLSemanticBindingView x, y;
            NLSymbolId r;
            CHECK(body_binding(f.c, "x", &x) && body_binding(f.c, "y", &y));
            CHECK(test_reference(f.c, "a", x.place, NL_TYPE_REF, NL_ACCESS_READ,
                                 false, &r, NULL));
            CHECK(test_reference(f.c, "b", y.place, NL_TYPE_REF, NL_ACCESS_READ,
                                 false, &r, NULL));
        }
        if (t == 4) {
            const NLFunctionParameter params[] = {{"flag", f.boolean},
                                                  {"v", f.copy}};
            CHECK(register_body(
                f.c, "return_path", params, 2, f.copy,
                "{loop(i=v){if(flag){return i;}else{continue(next(i));}}}",
                NL_CHECK_OK, NULL));
        }
        NLSource *source = NULL;
        NLSyntaxTree *tree = NULL;
        CHECK(body_tree(texts[t], &source, &tree));
        TestState before;
        CHECK(test_state(f.c, &before));
        bool reached = false;
        size_t failures = 0;
        for (size_t n = 0; n < 10000; ++n) {
            NLSemanticContext *candidate = NULL;
            CHECK(nl_sem_clone(f.c, &candidate) == NL_CHECK_OK);
            NLCheckedFragment *a = NULL;
            NLCheckDiagnostic d = {0};
            fail_at = n;
            allocation_index = 0;
            injecting = true;
            NLCheckStatus status =
                nl_semantic_check_source_fragment(candidate, tree, &a, &d);
            injecting = false;
            if (status == NL_CHECK_OK) {
                CHECK(a != NULL);
                nl_checked_destroy(a);
                reached = true;
                nl_semantic_destroy(candidate);
                break;
            }
            CHECK(status == NL_CHECK_OUT_OF_MEMORY && a == NULL);
            CHECK(test_unchanged(candidate, &before));
            CHECK(d.diagnostic.code != NULL);
            ++failures;
            nl_semantic_destroy(candidate);
        }
        CHECK(reached && failures > 50);
        nl_syntax_tree_destroy(tree);
        nl_source_destroy(source);
        source = NULL;
        /* Parser's flat node ownership must survive every allocation failure.
         */
        CHECK(nl_source_create(texts[t], strlen(texts[t]), "parser-oom",
                               &source) == NL_SOURCE_OK);
        reached = false;
        for (size_t n = 0; n < 1000; ++n) {
            NLParser *parser = NULL;
            CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
            tree = NULL;
            fail_at = n;
            allocation_index = 0;
            injecting = true;
            NLParseStatus status =
                nl_parser_parse_source_fragment(parser, &tree, NULL);
            injecting = false;
            if (status == NL_PARSE_OK) {
                reached = true;
                nl_syntax_tree_destroy(tree);
                nl_parser_destroy(parser);
                break;
            }
            CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
            nl_parser_destroy(parser);
        }
        CHECK(reached);
        nl_source_destroy(source);
        nl_semantic_destroy(f.c);
    }
    Fixture f = {0};
    CHECK(setup(&f));
    char text[2048] = "loop(";
    size_t at = strlen(text);
    for (size_t i = 0; i < 17; ++i)
        at += (size_t)snprintf(text + at, sizeof(text) - at, "%sp%zu=x",
                               i == 0 ? "" : ",", i);
    (void)snprintf(text + at, sizeof(text) - at, "){continue();}");
    CHECK(rejected(&f, text, NL_CHECK_RESOURCE_LIMIT, "P3-RESOURCE-LIMIT"));
    nl_semantic_destroy(f.c);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    size_t w = (size_t)strtoul(argv[1], NULL, 10);
    return (w == 40 ? failure() : workload(w)) ? 0 : 1;
}
