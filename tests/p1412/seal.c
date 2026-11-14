/* Reuse P1408's unchanged real-source loader and ptr-fact oracle. */
#define main p1408_preserved_main
#include "../p1408/owned.c"
#undef main

static bool in_seal, oom_armed, oom_hit;
static size_t seal_allocations, fail_at;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t n)
{
    if (in_seal && ++seal_allocations == fail_at && oom_armed) {
        oom_hit = true;
        return NULL;
    }
    return __real_malloc(n);
}
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *p,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId call,
                                              const NLSemanticContext *world)
{
    in_seal = world != NULL;
    if (in_seal)
        seal_allocations = 0;
    NLCheckStatus s = __real_nl_h_custody_seal_result(p, child, call, world);
    in_seal = false;
    if (world && s != NL_CHECK_OK)
        fprintf(stderr, "issuer status=%d allocations=%zu\n", (int)s,
                seal_allocations);
    return s;
}
static bool prepare(const char *path, Program *p)
{
    CHECK(load(path, p) && define(p, NULL) == NL_CHECK_OK);
    /* Definition input is not an authority receipt and need not survive. */
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    p->unit = NULL;
    p->parser = NULL;
    p->source = NULL;
    return true;
}
static bool actual(Program *p)
{
    NLCheckDiagnostic d = {0};
    NLCheckStatus status =
        nl_semantic_check_expression(p->context, p->call, &p->artifact, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr,
                "actual status=%d seal_allocations=%zu code=%s message=%s\n",
                (int)status, seal_allocations,
                d.diagnostic.code ? d.diagnostic.code : "(none)",
                d.diagnostic.message ? d.diagnostic.message : "(none)");
    CHECK(status == NL_CHECK_OK);
    CHECK(nl_h_custody_result_valid(p->artifact, nl_checked_root(p->artifact)));
    CHECK(!nl_h_custody_validate_candidate(p->artifact, p->context));
    CHECK(nl_h_custody_validate(p->artifact) &&
          nl_h_origin_validate(p->artifact));
    return true;
}
static NLCheckedFragment *find_if(NLCheckedFragment *f, NLCheckedNodeId *id)
{
    for (size_t i = 1; i <= f->count; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_IF) {
            *id = i;
            return f;
        }
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a) {
                NLCheckedFragment *child =
                    (NLCheckedFragment *)nl_checked_match_arm(f, i, a);
                NLCheckedFragment *found = find_if(child, id);
                if (found)
                    return found;
            }
    }
    return NULL;
}
static NLCheckedFragment *top(Program *p)
{
    return (NLCheckedFragment *)nl_checked_call_body(
        p->artifact, nl_checked_root(p->artifact));
}
static bool rejects(Program *p, const char *fault)
{
    bool valid =
        nl_h_custody_result_valid(p->artifact, nl_checked_root(p->artifact));
    printf("fault=%s result_valid=%d\n", fault, valid);
    CHECK(!valid);
    return true;
}
static bool authentic(const char *path)
{
    Program p = {0};
    CHECK(prepare(path, &p) && actual(&p));
    size_t choices = 0;
    CHECK(walk(top(&p), &choices) && choices == 1);
    printf("S actual-call result_valid=1, CHECK/owned/ptr facts PASS; "
           "seal allocations=%zu\n",
           seal_allocations);
    destroy(&p);
    return true;
}
static bool faults(const char *path)
{
    Program p = {0}, foreign = {0};
    CHECK(prepare(path, &p) && actual(&p));
    CHECK(prepare(path, &foreign) && actual(&foreign));
    const NLCheckedNodeId call = nl_checked_root(p.artifact);
    NLCheckedNodeId id = 0, other_id = 0;
    NLCheckedFragment *f = find_if(top(&p), &id),
                      *g = find_if(top(&foreign), &other_id);
    CHECK(f && g);
    NLCheckedFragment *arm = (NLCheckedFragment *)nl_checked_if_arm(f, id, 0);
    CHECK(arm);
    CHECK(!nl_h_custody_result_valid(p.artifact, 0) &&
          !nl_h_custody_result_valid(p.artifact, call + 1) &&
          !nl_h_custody_result_valid(top(&p), call));
    NLCheckedFragment unissued = {0}, replay = *p.artifact;
    CHECK(!nl_h_custody_result_valid(&unissued, call) &&
          !nl_h_custody_result_valid(&replay, call));
#define DAMAGE(target, value, label)                                           \
    do {                                                                       \
        unsigned char saved[sizeof(target)];                                   \
        memcpy(saved, &(target), sizeof(target));                              \
        (target) = (value);                                                    \
        bool denied = rejects(&p, label);                                      \
        memcpy(&(target), saved, sizeof(target));                              \
        CHECK(denied && nl_h_custody_result_valid(p.artifact, call));          \
    } while (0)
    DAMAGE(f->nodes[id - 1].results[0].value, 0, "IF result node");
    DAMAGE(f->nodes[id - 1].initializer, 0, "IF condition node");
    DAMAGE(arm->if_parent, g, "foreign IF parent");
    DAMAGE(arm->if_parent_node, id + 1, "wrong IF node key");
    DAMAGE(arm->if_arm_index, 1, "arm index");
    DAMAGE(arm->branch_value_prefix, 0, "branch value prefix");
    DAMAGE(arm->branch_fact_prefix, 0, "branch fact prefix");
    DAMAGE(f->arm_count, 0, "arm cardinality");
    DAMAGE(f->arm_count, f->arm_count + 1, "arm cardinality growth");
    DAMAGE(f->context, g->context, "same-shaped foreign world");
    DAMAGE(f->source, g->source, "foreign source");
    NLCheckedNodeView short_node = {0};
    DAMAGE(p.artifact->nodes, &short_node, "short foreign call array");
    DAMAGE(p.artifact->count, p.artifact->count + 1, "call array count");
    NLSemanticContext *c = (NLSemanticContext *)arm->context;
    const NLCheckedNodeView *root = nl_checked_node_view(arm, arm->root);
    NLValueId value = root->results[0].value;
    NLReferenceFacts ref = c->values[value - 1].reference;
    NLPlaceId place = ref.place;
    NLDomainId domain = c->places[place - 1].governing_domain;
    CHECK(place && domain && c->region_count == 2);
    DAMAGE(c->values[value - 1].reference.readable, false, "permission");
    DAMAGE(c->values[value - 1].reference.writable, false, "write fact");
    DAMAGE(c->values[value - 1].reference.provenance, NL_PROVENANCE_UNKNOWN,
           "provenance");
    DAMAGE(c->values[value - 1].reference.incarnation, ref.incarnation + 1,
           "wrong incarnation");
    DAMAGE(c->values[value - 1].reference.place, place + 1, "wrong original");
    DAMAGE(c->values[value - 1].domain, domain, "ptr as Domain authority");
    DAMAGE(c->values[value - 1].allocation_region, 1, "ptr as Allocation");
    DAMAGE(c->values[value - 1].dependencies, NL_DEPENDENCIES_UNKNOWN,
           "unproved dependency");
    DAMAGE(c->values[value - 1].reference.scope, 1, "escaped loan");
    DAMAGE(c->places[place - 1].live, false, "stale original");
    DAMAGE(c->places[place - 1].governing_domain, 0, "wrong Domain");
    DAMAGE(c->domains[domain - 1].live, false, "ended Domain");
    DAMAGE(c->scopes[0].active, !c->scopes[0].active, "loan state");
    DAMAGE(c->regions[0].view.live, false, "ended backing");
    DAMAGE(c->values, NULL, "NULL value array");
    NLSemanticValueView short_value = {0};
    DAMAGE(c->values, &short_value, "short foreign value array");
    DAMAGE(c->value_count, c->value_count + 1, "value array count");
    DAMAGE(c->functions, NULL, "NULL function array");
    NLFunctionEntry short_function = {0};
    DAMAGE(c->functions, &short_function, "short foreign functions");
    DAMAGE(c->functions[0].count, c->functions[0].count + 1,
           "signature extent");
    NLTypeId short_parameter = 0;
    DAMAGE(c->functions[0].parameters, &short_parameter,
           "foreign parameter storage");
    DAMAGE(c->regions[0].intervals, NULL, "raw interval base");
    CHECK(c->regions[0].count);
    DAMAGE(c->regions[0].intervals[0].state.value,
           (unsigned char)(c->regions[0].intervals[0].state.value ^ 1u),
           "raw state");
#undef DAMAGE
    /* Actual A/B canonical child substitution, deliberately unrestored on A.
     * Its canonical custody, not this display view, must destroy A. */
    NLCheckedFragment *b_arm =
        (NLCheckedFragment *)nl_checked_if_arm(g, other_id, 0);
    size_t slot = 0;
    while (slot < f->arm_count && f->arms[slot].artifact != arm)
        ++slot;
    CHECK(slot < f->arm_count && b_arm);
    f->arms[slot].artifact = b_arm;
    CHECK(rejects(&p, "genuine foreign B arm"));
    destroy(&p);
    CHECK(nl_h_custody_result_valid(foreign.artifact,
                                    nl_checked_root(foreign.artifact)));
    destroy(&foreign);
    puts("A unrestored destroy, B healthy/destroy PASS");
    return true;
}
static bool seal_oom(const char *path, size_t point)
{
    Program p = {0};
    CHECK(prepare(path, &p));
    NLSemanticSnapshot before = {0}, after = {0};
    NLCopyCallAnchor anchor = {0};
    CHECK(nl_semantic_snapshot(p.context, &before) &&
          nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK);
    fail_at = point;
    oom_hit = false;
    oom_armed = true;
    NLCheckStatus s =
        nl_semantic_check_expression(p.context, p.call, &p.artifact, NULL);
    oom_armed = false;
    CHECK(oom_hit && s == NL_CHECK_OUT_OF_MEMORY && p.artifact == NULL &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof(before)) &&
          nl_copy_state_matches(p.context, &anchor));
    nl_copy_state_destroy(&anchor);
    CHECK(actual(&p));
    destroy(&p);
    printf("seal malloc ordinal=%zu OUT_OF_MEMORY/rollback/out/retry PASS\n",
           point);
    return true;
}
int main(int argc, char **argv)
{
    if (argc < 3)
        return 2;
    bool ok = !strcmp(argv[1], "positive")   ? authentic(argv[2])
              : !strcmp(argv[1], "faults")   ? faults(argv[2])
              : !strcmp(argv[1], "negative") ? negative(argv[2])
              : !strcmp(argv[1], "oom") && argc == 4
                  ? seal_oom(argv[2], (size_t)strtoull(argv[3], NULL, 10))
                  : false;
    return ok ? 0 : 1;
}
