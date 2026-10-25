/* P-owned NON-BLIND calibrated regression. Reuse only the P896 harness,
 * never R899's probe/oracle/rollback assertion. */
#define main p896_reference_main
#include "../p896/private.c"
#undef main
static unsigned expected_old, expected_sibling, expected_incoming[2];
static size_t expected_index, extra_faults, unrelated_faults;
static void p901_observe(const NLDefinitionCallCapture *p,
                         const NLDefinitionCallView *v, NLCheckStatus status,
                         void *user)
{
    /* Requires a successful positive replay BEFORE running inherited mutants.
     * Wrong RETURN sources must get PRE verified, not fail at AST lookup. */
    observe(p, v, status, user);
    if (status != NL_CHECK_OK || expect_bad)
        return;
    NLDefinitionCallResult result;
    REQUIRE(nl_definition_call_validate(p, v, &result) == NL_CHECK_OK &&
            result.pre_verified && result.return_old_verified &&
            result.loan_closed && result.old_value == expected_old &&
            result.sibling == expected_sibling &&
            result.incoming == expected_incoming[v->arm] &&
            v->field.index == expected_index);
    printf("P901 root%zu child%zu index%zu inc%zu/%zu scope%zu arm%zu "
           "PREold%llu new%llu sibling%llu verified\n",
           v->field.parent, v->field.child, v->field.index,
           v->field.parent_incarnation, v->field.child_incarnation, v->scope,
           v->arm, (unsigned long long)result.old_value,
           (unsigned long long)result.incoming,
           (unsigned long long)result.sibling);
    if (!mutate)
        return;
    NLCheckedFragment *parent = (NLCheckedFragment *)v->parent;
    NLCheckedFragment *caller = (NLCheckedFragment *)v->caller;
    NLCheckedNodeView *choice = &parent->nodes[caller->if_parent_node - 1];
    const NLSourceSpan saved = choice->span;
    ++choice->span.start_byte;
    refuses(p, *v);
    ++extra_faults;
    choice->span = saved;
    for (size_t i = 1; i <= parent->count; ++i) {
        const NLCheckedNodeView *other = nl_checked_node_view(parent, i);
        if (other->kind == NL_CHECKED_IF && i != caller->if_parent_node) {
            /* A real unrelated IF, not a fake invalid byte offset. */
            choice->span = other->span;
            refuses(p, *v);
            ++unrelated_faults;
            choice->span = saved;
        }
    }
    NLSemanticContext *pre = (NLSemanticContext *)v->pre;
    NLFunctionBody *owner = pre->functions[v->caller_function - 1].body;
    pre->functions[v->caller_function - 1].body =
        pre->functions[v->function - 1].body;
    refuses(p, *v);
    ++extra_faults;
    pre->functions[v->caller_function - 1].body = owner;
    const NLSource *source = caller->source;
    caller->source = v->callee->source;
    refuses(p, *v);
    ++extra_faults;
    caller->source = source;
    for (size_t i = 0; i < parent->arm_count; ++i) {
        if (parent->arms[i].match != caller->if_parent_node ||
            parent->arms[i].artifact == caller)
            continue;
        NLCheckedFragment *foreign = parent->arms[i].artifact;
        REQUIRE(foreign->body_count == 1 && foreign->body_calls[0] == v->call);
        NLCheckedFragment *body = caller->bodies[0];
        caller->bodies[0] = foreign->bodies[0];
        refuses(p, *v);
        ++extra_faults;
        caller->bodies[0] = body;
    }
    REQUIRE(nl_definition_call_validate(p, v, &result) == NL_CHECK_OK);
}
static bool p901_run_case(const char *path)
{
    bool none = strstr(path, "no_call") != NULL;
    expect_bad = strstr(path, "negative_return_") != NULL;
    if (strstr(path, "meridian")) {
        expected_old = 37;
        expected_sibling = 163;
        expected_index = 0;
        expected_incoming[0] = 81;
        expected_incoming[1] = 118;
    } else if (strstr(path, "relay_pair")) {
        expected_old = 212;
        expected_sibling = 43;
        expected_index = 1;
        expected_incoming[0] = 65;
        expected_incoming[1] = 147;
    } else {
        expected_old = 56;
        expected_sibling = 11;
        expected_index = 1;
        expected_incoming[0] = 126;
        expected_incoming[1] = 209;
    }
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *context = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&context) == NL_CHECK_OK);
    TestState *before = calloc(1, sizeof(*before));
    CHECK(before && test_state(context, before));
    records = verified = rejected = oom = 0;
    world[0] = world[1] = 0;
    nl_definition_call_observer(p901_observe, NULL);
    const NLSyntaxTree *units[] = {tree};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(context, units, 1, &diagnostic);
    nl_definition_call_observer(NULL, NULL);
    if (none) {
        /* Success commits the registered function, it is NOT rollback. */
        CHECK(status == NL_CHECK_OK && records == 0 && oom == 0 &&
              context->function_count > before->counts.functions);
    } else {
        CHECK(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
              !strcmp(diagnostic.diagnostic.diagnostic.code,
                      "P6-JOIN-PRECISION") &&
              test_unchanged(context, before) && records == 2 && !oom);
        CHECK(expect_bad ? rejected == 2 : verified == 2);
        if (!expect_bad)
            CHECK(world[0] && world[1] && world[0] != world[1]);
    }
    free(before);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    if (argc < 3)
        return 2;
    mutate = !strcmp(argv[1], "negative");
    for (int i = 2; i < argc; ++i)
        if (!p901_run_case(argv[i]))
            return 1;
    REQUIRE(!mutate || (extra_faults >= 24 && unrelated_faults >= 4));
    printf("P901 calibrated private rejection count%zu (extra owner/span%zu, "
           "unrelated IF%zu); source CHECK3 unchanged\n",
           faults, extra_faults, unrelated_faults);
    return 0;
}
