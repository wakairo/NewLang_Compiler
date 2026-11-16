/* Reuse P's already-adopted source loader, public validators and OOM apparatus.
 * The older tests/source/oracle are unchanged, and its main is never invoked.
 */
#define main p1463_retained_main
#include "../p1463/probe.c"
#undef main

static bool terminal_arms(NLCheckedFragment *f, size_t *count)
{
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->kind == NL_CHECKED_MATCH && n->normal_arms == 1) {
            const NLCheckedNodeView *trial =
                nl_checked_node_view(f, n->initializer);
            if (trial && trial->kind == NL_CHECKED_TRY_ALLOCATE_ONE) {
                NLCheckedFragment *a =
                    (NLCheckedFragment *)nl_checked_match_arm(f, id, 0);
                const NLCheckedNodeView *r =
                    nl_checked_node_view(a, nl_checked_root(a));
                CHECK(r && r->terminates && !r->type && !r->result_count &&
                      r->returned.type == 1 && !r->returned.value &&
                      !r->normal_frame_unchanged &&
                      nl_control_exits_count(a->exits) == 1);
                ++*count;
            }
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            CHECK(terminal_arms((NLCheckedFragment *)body, count));
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedFragment *a = nl_checked_match_arm(f, id, i);
            if (a)
                CHECK(terminal_arms((NLCheckedFragment *)a, count));
        }
    }
    return true;
}
static bool suite1471(const char *positive, const char *bad)
{
    Program p = {0}, foreign = {0};
    Trace t = {0}, other = {0};
    size_t returns = 0;
    CHECK(ready(positive, &p) && actual(&p) && observe(&p, 0, &t) &&
          terminal_arms(p.artifact, &returns) && returns == 2);
    CHECK(ready(positive, &foreign) && actual(&foreign) &&
          observe(&foreign, 0, &other) &&
          authentication(&p, &t, &foreign, &other));
    destroy(&p);
    destroy(&foreign);
    CHECK(negative(bad, positive, "ALLOCATED-DOMAIN-MISMATCH"));
    puts(
        "P1471 public MACHINE: exact two RETURN edges; same "
        "original/current/Copy; tamper/foreign/no-clobber/rollback/retry PASS");
    return true;
}
static bool owned1471(const char *path)
{
    Program p = {0};
    Trace t = {0};
    size_t returns = 0;
    CHECK(ready(path, &p) && actual(&p) && observe(&p, 0, &t) &&
          terminal_arms(p.artifact, &returns) && returns == 2);
    destroy(&p);
    return true;
}
static bool check_oom1471(const char *path)
{
    Program p = {0};
    NLCopyCallAnchor anchor = {0};
    CHECK(ready(path, &p) &&
          nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK);
    NLCheckDiagnostic d = {0};
    fault = 1;
    NLCheckStatus s =
        nl_semantic_check_expression(p.context, p.call, &p.artifact, &d);
    fault = 0;
    printf("CHECK OOM hits=%zu status=%d out=%p\n", hits, (int)s,
           (void *)p.artifact);
    CHECK(hits == 1 && s == NL_CHECK_OUT_OF_MEMORY && !p.artifact &&
          nl_copy_state_matches(p.context, &anchor) && actual(&p));
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "owned"))
        return owned1471(argv[2]) ? 0 : 1;
    if (argc == 3 && !strcmp(argv[1], "check-oom"))
        return check_oom1471(argv[2]) ? 0 : 1;
    if (argc == 4 && !strcmp(argv[1], "suite"))
        return suite1471(argv[2], argv[3]) ? 0 : 1;
    if (argc == 4 && !strcmp(argv[1], "oom"))
        return oom(argv[2], (unsigned)strtoul(argv[3], NULL, 10)) ? 0 : 1;
    return 2;
}
