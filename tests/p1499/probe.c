/* Reuse the adopted issuer/transaction/no-clobber probe without copying it. */
#define NL_QUANTITY_PROBE_ENTRY p1492_probe_main
#include "../p1492/probe.c"

static bool composition(const char *path, bool attack)
{
    Program p = {0}, foreign = {0};
    CHECK(owned(path) && ready(path, &p));
    NLGuardedUsizePlan main, first, second;
    CHECK(nl_guarded_usize_call_valid(p.artifact, p.artifact->root, &main));
    NLCheckedFragment *caller = (NLCheckedFragment *)main.body;
    const NLCheckedNodeView *root = nl_checked_node_view(caller, caller->root);
    NLCheckedNodeId calls[2] = {0};
    size_t count = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *binding = nl_checked_node_view(caller, id);
        const NLCheckedNodeView *init =
            nl_checked_node_view(caller, binding->initializer);
        if (init->kind == NL_CHECKED_REGISTERED_CALL) {
            CHECK(count < 2);
            calls[count++] = binding->initializer;
        }
        id = binding->next_item;
    }
    CHECK(count == 2 && nl_guarded_usize_call_valid(caller, calls[0], &first) &&
          nl_guarded_usize_call_valid(caller, calls[1], &second));
    NLCheckedNodeView *call = &caller->nodes[calls[1] - 1];
    NLCheckedNodeView *arg = &caller->nodes[call->first_argument - 1];
    const NLCheckedNodeView *producer = nl_checked_node_view(caller, calls[0]);
    CHECK(arg->kind == NL_CHECKED_IDENTIFIER &&
          arg->value_use == NL_VALUE_COPIED);
    NLValueId original = producer->results[0].value;
    NLValueId actual = arg->results[0].value;
    NLValueId result = call->results[0].value;
    CHECK(original && actual && result && original != actual &&
          actual != result && original != result &&
          first.formal != second.formal && first.body != second.body &&
          p.context->bindings[second.formal - 1].view.value == actual);
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(p.context, actual, &v) && !v.scalar_known &&
          !v.scalar_value);
    printf("two genuine calls: producer result=%zu; copied actual/formal "
           "value=%zu/%zu; consumer result=%zu; Unknown preserved\n",
           original, actual, second.formal, result);
    if (attack) {
        CHECK(ready(path, &foreign));
        NLCheckedFragment *body = (NLCheckedFragment *)second.body;
        NLCheckedFragment *arm =
            (NLCheckedFragment *)nl_checked_if_arm(body, second.conditional, 0);
        size_t cases = 0;
#define MUTATE(item, replacement)                                              \
    do {                                                                       \
        unsigned char saved[sizeof(item)];                                     \
        memcpy(saved, &(item), sizeof(item));                                  \
        (item) = (replacement);                                                \
        CHECK(rejected(&p));                                                   \
        memcpy(&(item), saved, sizeof(item));                                  \
        ++cases;                                                               \
    } while (0)
        MUTATE(call->first_argument,
               caller->nodes[calls[0] - 1].first_argument);
        MUTATE(arg->symbol, first.formal);
        MUTATE(arg->results[0].value, original);
        MUTATE(call->results[0].value, original);
        MUTATE(p.context->bindings[second.formal - 1].view.value, original);
        MUTATE(p.context->values[actual - 1].scalar_known, true);
        MUTATE(
            arm->context
                ->places[arm->context->bindings[second.formal - 1].view.place -
                         1]
                .current_fact,
            0);
        MUTATE(body->context, foreign.context);
        MUTATE(body->source, foreign.source);
        MUTATE(arm->if_arm_index, 1);
        MUTATE(body->nodes[body->root - 1].results[0].value, original);
#undef MUTATE
        CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED);
        /* Destroy with a foreign display child: canonical ownership must
         * keep the other independently checked tree alive and reusable. */
        NLGuardedUsizePlan other;
        CHECK(nl_guarded_usize_call_valid(foreign.artifact,
                                          foreign.artifact->root, &other));
        caller->bodies[1] = ((NLCheckedFragment *)other.body)->bodies[1];
        CHECK(rejected(&p));
        destroy(&p);
        CHECK(nl_checked_proof_class(foreign.artifact) ==
              NL_CHECKED_MACHINE_PROVED);
        destroy(&foreign);
        printf("second-call tamper %zu + foreign-child destruction rejected "
               "before C bytes\n",
               cases);
    } else
        destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 3 &&
        (!strcmp(argv[1], "composition") || !strcmp(argv[1], "attack")))
        return composition(argv[2], !strcmp(argv[1], "attack")) ? 0 : 1;
    if (argc == 3 && !strcmp(argv[1], "changed-negative")) {
        Program p = {0};
        if (!load(argv[2], &p))
            return 1;
        NLFunctionUnitDiagnostic d = {0};
        NLCheckStatus status = define(&p, &d);
        bool ok = status == NL_CHECK_SEMANTIC_UNSUPPORTED &&
                  d.diagnostic.diagnostic.code &&
                  !strcmp(d.diagnostic.diagnostic.code, "LOCAL-LOAN-PROFILE");
        printf("changed usize formal: earlier local-loan coverage rejection "
               "status=%d code=%s\n",
               (int)status,
               d.diagnostic.diagnostic.code ? d.diagnostic.diagnostic.code
                                            : "none");
        destroy(&p);
        return ok ? 0 : 1;
    }
    return p1492_probe_main(argc, argv);
}
