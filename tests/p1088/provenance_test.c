#define main p1088_inherited_main
#include "../unit/loop_test.c"
#undef main

static bool authentic(void)
{
    Fixture f = {0}, foreign = {0};
    CHECK(setup(&f) && setup(&foreign));
    TestChecked a = {0}, b = {0};
    const char *text = "loop(){let prior=x;let sibling=y;break prior;}";
    CHECK(test_run(f.c, text, TEST_SOURCE, NL_CHECK_OK, NULL, &a));
    CHECK(test_run(foreign.c, text, TEST_SOURCE, NL_CHECK_OK, NULL, &b));
    NLCheckedNodeId id = nl_checked_root(a.artifact);
    const NLCheckedFragment *body = nl_checked_loop_body(a.artifact, id);
    const NLCheckedFragment *other =
        nl_checked_loop_body(b.artifact, nl_checked_root(b.artifact));
    CHECK(body && other && body != other && body->context != other->context);
    NLControlExitView *edge =
        (NLControlExitView *)nl_control_exit_view(body->exits, 0);
    const NLControlExitView *foreign_edge =
        nl_control_exit_view(other->exits, 0);
    CHECK(edge && foreign_edge &&
          edge->source_node == foreign_edge->source_node);
    CHECK(nl_checked_loop_break_provenance_valid(a.artifact, id));
    NLControlExitView saved = *edge;
    const char *faults[] = {
        "foreign-state", "null-state",  "foreign-target", "coedited-target",
        "foreign-owner", "zero-node",   "wrong-kind",     "wrong-count",
        "wrong-result",  "foreign-edge"};
    for (size_t i = 0; i < sizeof(faults) / sizeof(*faults); ++i) {
        switch (i) {
        case 0:
            edge->state = foreign_edge->state;
            break;
        case 1:
            edge->state = NULL;
            break;
        case 2:
            edge->target = foreign_edge->target;
            break;
        case 3:
            edge->target = edge->source_target = foreign_edge->target;
            break;
        case 4:
            edge->source_owner = other;
            break;
        case 5:
            edge->source_node = 0;
            break;
        case 6:
            edge->kind = NL_EXIT_RETURN;
            break;
        case 7:
            edge->count = 2;
            break;
        case 8:
            edge->values[0].value = 0;
            break;
        case 9:
            *edge = *foreign_edge;
            break;
        }
        CHECK(!nl_checked_loop_break_provenance_valid(a.artifact, id));
        NLControlExits *out = NULL;
        CHECK(nl_control_exits_union(&out, body->exits) ==
              NL_CHECK_INTERNAL_ERROR);
        CHECK(!out);
        *edge = saved;
        CHECK(nl_checked_loop_break_provenance_valid(a.artifact, id));
        printf("P1088 REJECT %s; union no-clobber\n", faults[i]);
    }
    NLCheckedFragment *changed = (NLCheckedFragment *)body;
    NLCheckedFragment body_saved = *body;
    for (size_t i = 0; i < 6; ++i) {
        switch (i) {
        case 0:
            changed->nodes = other->nodes;
            break;
        case 1:
            changed->context = other->context;
            break;
        case 2:
            changed->source = other->source;
            break;
        case 3:
            --changed->count;
            break;
        case 4:
            ++changed->count;
            break;
        case 5:
            changed->root = edge->source_node;
            break;
        }
        CHECK(!nl_checked_loop_break_provenance_valid(a.artifact, id));
        *changed = body_saved;
    }
    NLCheckedNodeView *br =
        (NLCheckedNodeView *)nl_checked_node_view(body, edge->source_node);
    NLCheckedNodeView node_saved = *br;
    for (size_t i = 0; i < 5; ++i) {
        switch (i) {
        case 0:
            br->kind = NL_CHECKED_CONTINUE;
            break;
        case 1:
            br->kind = NL_CHECKED_RETURN;
            break;
        case 2:
            br->initializer = 0;
            break;
        case 3:
            br->terminates = false;
            break;
        case 4:
            br->span = (NLSourceSpan){0};
            break;
        }
        CHECK(!nl_checked_loop_break_provenance_valid(a.artifact, id));
        *br = node_saved;
    }
    NLCheckedNodeView *operand =
        (NLCheckedNodeView *)nl_checked_node_view(body, br->initializer);
    NLCheckedNodeView operand_saved = *operand;
    operand->results[0].value = 0;
    CHECK(!nl_checked_loop_break_provenance_valid(a.artifact, id));
    *operand = operand_saved;
    NLCheckedNodeView *root =
        (NLCheckedNodeView *)nl_checked_node_view(body, body->root);
    NLCheckedNodeView root_saved = *root;
    root->first_item = edge->source_node;
    CHECK(!nl_checked_loop_break_provenance_valid(a.artifact, id));
    *root = root_saved;
    NLControlExits *joined = NULL;
    CHECK(nl_control_exits_union(&joined, body->exits) == NL_CHECK_OK);
    changed->exits = joined;
    CHECK(nl_checked_loop_break_provenance_valid(a.artifact, id));
    CHECK(nl_control_exits_union(&joined, joined) == NL_CHECK_OK);
    changed->exits = joined;
    CHECK(!nl_checked_loop_break_provenance_valid(a.artifact, id));
    changed->exits = body_saved.exits;
    nl_control_exits_destroy(joined);
    NLCheckedFragment shallow = *body;
    CHECK(!nl_control_source_break_body_valid(&shallow, saved.target));
    shallow = *a.artifact;
    CHECK(!nl_checked_loop_break_provenance_valid(&shallow, id));
    shallow.h_origin = NULL;
    CHECK(!nl_checked_loop_break_provenance_valid(&shallow, id));
    NLCheckedArm arm_saved = a.artifact->arms[0];
    a.artifact->arms[0].artifact = (NLCheckedFragment *)other;
    CHECK(!nl_checked_loop_break_provenance_valid(a.artifact, id));
    a.artifact->arms[0] = arm_saved;
    CHECK(nl_checked_loop_break_provenance_valid(a.artifact, id));
    /* Deliberately do not restore A's observed foreign edge. Destruction must
     * follow A's private owned handles and leave B healthy. */
    *edge = *foreign_edge;
    test_checked_destroy(&a);
    CHECK(nl_checked_loop_break_provenance_valid(b.artifact,
                                                 nl_checked_root(b.artifact)));
    puts("P1088 A destroyed with foreign view; genuine B alive");
    test_checked_destroy(&b);
    nl_semantic_destroy(f.c);
    nl_semantic_destroy(foreign.c);
    const char *selected[] = {"loop(){break x;}", "loop(){break unit;}",
                              "loop(){let old=y;break old;}"};
    Fixture cases = {0};
    CHECK(setup(&cases));
    for (size_t i = 0; i < sizeof(selected) / sizeof(*selected); ++i) {
        TestChecked c = {0};
        CHECK(
            test_run(cases.c, selected[i], TEST_SOURCE, NL_CHECK_OK, NULL, &c));
        CHECK(nl_checked_loop_break_provenance_valid(
            c.artifact, nl_checked_root(c.artifact)));
        test_checked_destroy(&c);
    }
    const char *excluded[] = {"loop(){loop(){break unit;};break x;}",
                              "loop(){if(cond){break x;}else{break y;}}",
                              "loop(p=x){break p;}", "loop(){continue();}"};
    for (size_t i = 0; i < sizeof(excluded) / sizeof(*excluded); ++i) {
        TestChecked c = {0};
        CHECK(
            test_run(cases.c, excluded[i], TEST_SOURCE, NL_CHECK_OK, NULL, &c));
        CHECK(!nl_checked_loop_break_provenance_valid(
            c.artifact, nl_checked_root(c.artifact)));
        if (i == 0) {
            const NLCheckedFragment *outer =
                nl_checked_loop_body(c.artifact, nl_checked_root(c.artifact));
            CHECK(outer && nl_control_exits_count(outer->exits) == 1);
            const NLControlExitView *outer_edge =
                nl_control_exit_view(outer->exits, 0);
            for (NLCheckedNodeId n = 1; n <= outer->count; ++n) {
                if (nl_checked_node_view(outer, n)->kind != NL_CHECKED_LOOP)
                    continue;
                const NLCheckedFragment *inner = nl_checked_loop_body(outer, n);
                CHECK(inner && nl_control_exits_count(inner->exits) == 1);
                const NLControlExitView *inner_edge =
                    nl_control_exit_view(inner->exits, 0);
                CHECK(inner_edge->source_owner == inner &&
                      outer_edge->source_owner == outer);
                CHECK(!nl_control_target_same(inner_edge->target,
                                              outer_edge->target));
                CHECK(nl_checked_loop_break_provenance_valid(outer, n));
                CHECK(!nl_control_source_break_body_valid(inner,
                                                          outer_edge->target));
            }
        }
        test_checked_destroy(&c);
    }
    CHECK(rejected(&cases, "{break x;}", NL_CHECK_SEMANTIC_ERROR,
                   "P14-CONTROL-CONTEXT"));
    nl_semantic_destroy(cases.c);
    puts("P1088 source/graph/world/target/custody negatives PASS");
    return true;
}
int main(void)
{
    return authentic() ? 0 : 1;
}
