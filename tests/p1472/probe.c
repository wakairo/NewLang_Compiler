/* Adopted P apparatus, never independent F/R source or oracle. */
#define main p1463_retained_main
#include "../p1463/probe.c"
#undef main

static bool terminal_authentication_at(Program *p, Program *foreign,
                                       NLCheckedFragment *body,
                                       const NLCheckedFragment *other,
                                       size_t *tested)
{
    for (size_t i = 0; i < body->arm_count; ++i) {
        NLCheckedFragment *arm = body->arms[i].artifact;
        const NLCheckedNodeView *root =
            nl_checked_node_view(arm, nl_checked_root(arm));
        CHECK(root && i < other->arm_count);
        if (!root->terminates || root->variant != 1) {
            CHECK(terminal_authentication_at(p, foreign, arm,
                                             other->arms[i].artifact, tested));
            continue;
        }
        NLCheckedNodeView *read = NULL;
        for (NLCheckedNodeId n = 1; n <= nl_checked_node_count(arm); ++n) {
            NLCheckedNodeView *v =
                (NLCheckedNodeView *)nl_checked_node_view(arm, n);
            if (v->kind == NL_CHECKED_FIELD_READ) {
                read = v;
                break;
            }
        }
        CHECK(read && read->field.present &&
              read->field.access == NL_ACCESS_READ);
#define DAMAGE1472(target, value, label)                                       \
    do {                                                                       \
        unsigned char saved[sizeof(target)];                                   \
        memcpy(saved, &(target), sizeof(target));                              \
        (target) = (value);                                                    \
        bool rejected = refused(p, label);                                     \
        memcpy(&(target), saved, sizeof(target));                              \
        CHECK(rejected && nl_checked_proof_class(p->artifact) ==               \
                              NL_CHECKED_MACHINE_PROVED);                      \
    } while (0)
        DAMAGE1472(read->field.parent, read->field.parent + 1,
                   "terminal wrong original");
        DAMAGE1472(read->field.child_fact, read->field.child_fact + 1,
                   "terminal wrong current fact");
        DAMAGE1472(read->field.parent_incarnation,
                   read->field.parent_incarnation + 1,
                   "terminal wrong incarnation");
        DAMAGE1472(arm->source, foreign->source, "terminal foreign source");
        CHECK(i < other->arm_count);
        DAMAGE1472(arm->context, other->arms[i].artifact->context,
                   "terminal foreign world");
        DAMAGE1472(body->arms[i].artifact, other->arms[i].artifact,
                   "terminal foreign canonical arm");
#undef DAMAGE1472
        ++*tested;
    }
    return true;
}
static bool terminal_authentication(Program *p, Program *foreign)
{
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        p->artifact, nl_checked_root(p->artifact));
    const NLCheckedFragment *other = nl_checked_call_body(
        foreign->artifact, nl_checked_root(foreign->artifact));
    size_t tested = 0;
    CHECK(terminal_authentication_at(p, foreign, body, other, &tested) &&
          tested == 2 && emit(p, NL_NODE_C_OK));
    puts("terminal ancestor/current/source/foreign arm negative PASS");
    return true;
}
static bool suite1472(const char *old_dir, const char *inherited_dir,
                      const char *new_dir)
{
    char source[1024], wrong[1024], missing[1024];
    CHECK(snprintf(source, sizeof(source), "%s/P1468-B1-failure-return.nl",
                   inherited_dir) > 0);
    CHECK(snprintf(wrong, sizeof(wrong), "%s/P1468-N1-wrong-live-domain.nl",
                   inherited_dir) > 0);
    CHECK(snprintf(missing, sizeof(missing),
                   "%s/N-terminal-missing-deallocate.nl", new_dir) > 0);
    Program p = {0}, foreign = {0};
    Trace a = {0}, b = {0};
    CHECK(ready(source, &p) && actual(&p) && observe(&p, 0, &a));
    CHECK(ready(source, &foreign) && actual(&foreign) &&
          observe(&foreign, 0, &b));
    CHECK(terminal_authentication(&p, &foreign) &&
          authentication(&p, &a, &foreign, &b));
    destroy(&p);
    destroy(&foreign);
    CHECK(negative(wrong, source, "ALLOCATED-DOMAIN-MISMATCH") &&
          negative(missing, source, "P5-SCOPE-OBLIGATION") && suite(old_dir));
    puts("P1472 genuine public MACHINE/Copy/RETURN, old regression, "
         "negative/rollback/retry PASS");
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "suite"))
        return suite1472(argv[2], argv[3], argv[4]) ? 0 : 1;
    if (argc == 4 && !strcmp(argv[1], "oom"))
        return oom(argv[2], (unsigned)strtoul(argv[3], NULL, 10)) ? 0 : 1;
    return 2;
}
