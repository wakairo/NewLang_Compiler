#include "../support/node_checked.h"
#include "newlang/copy_body.h"

#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);

static bool inject_failure;
static size_t allocation_index, fail_index;

void *__wrap_malloc(size_t n)
{
    return inject_failure && allocation_index++ == fail_index ? NULL
                                                             : __real_malloc(n);
}

void *__wrap_calloc(size_t n, size_t count)
{
    return inject_failure && allocation_index++ == fail_index
               ? NULL
               : __real_calloc(n, count);
}

void *__wrap_realloc(void *old, size_t n)
{
    return inject_failure && allocation_index++ == fail_index
               ? NULL
               : __real_realloc(old, n);
}

typedef struct {
    TestNode node;
    const NLCheckedFragment *arm;
    NLCheckedNodeId call;
    NLCopyBodyView body;
    NLCopyBodyParameters args;
} R824Case;

static bool scalar(const NLSemanticContext *world, NLValueId id,
                   unsigned expected)
{
    NLSemanticValueView value;
    return nl_semantic_value_view(world, id, &value) && value.scalar_known &&
           value.scalar_value == expected;
}

static bool open_case(const char *path, size_t normal_arm, R824Case *out)
{
    CHECK(node_checked_load(path, &out->node));
    const NLCheckedFragment *main_body = nl_checked_call_body(
        out->node.entry, nl_checked_root(out->node.entry));
    CHECK(main_body != NULL);
    size_t matching_if = 0, matching_call = 0;
    for (size_t id = 1; id <= nl_checked_node_count(main_body); ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(main_body, id);
        if (!node || node->kind != NL_CHECKED_IF)
            continue;
        const NLCheckedFragment *arm =
            nl_checked_if_arm(main_body, id, normal_arm);
        const NLCheckedFragment *terminal =
            nl_checked_if_arm(main_body, id, 1 - normal_arm);
        CHECK(arm && terminal && !nl_checked_node_view(
                                     arm, nl_checked_root(arm))->terminates &&
              nl_checked_node_view(
                  terminal, nl_checked_root(terminal))->terminates &&
              node->normal_arms == 1);
        ++matching_if;
        for (size_t call = 1; call <= nl_checked_node_count(arm); ++call) {
            NLCopyBodyView body;
            NLCopyBodyParameters parameters;
            if (!nl_checked_copy_body_view(arm, call, &body) ||
                !nl_checked_copy_body_parameters(arm, call, &parameters))
                continue;
            CHECK(!matching_call);
            out->arm = arm;
            out->call = call;
            out->body = body;
            out->args = parameters;
            ++matching_call;
        }
    }
    CHECK(matching_if == 1 && matching_call == 1);
    return true;
}

static bool valid(const R824Case *item)
{
    return nl_checked_copy_body_validate(item->arm, item->call, &item->body) ==
               NL_CHECK_OK &&
           nl_checked_copy_body_parameters_validate(item->arm, item->call,
                                                    &item->args) == NL_CHECK_OK;
}

static void close_case(R824Case *item)
{
    node_checked_destroy(&item->node);
}

static bool positive(const char *path, size_t arm, size_t index, unsigned old,
                     unsigned current, unsigned sibling)
{
    R824Case item = {0};
    CHECK(open_case(path, arm, &item) && valid(&item));
    const NLCopyBodyParameters *p = &item.args;
    const NLCopyBodyView *body = &item.body;
    CHECK(p->field.present && p->field.index == index &&
          p->field.dependency_compatible &&
          body->body == nl_checked_call_body(item.arm, item.call) &&
          scalar(body->live_exit, body->result.value, old));
    NLSemanticPlaceView before_root, after_root, before_child, after_child,
        before_sibling, after_sibling;
    CHECK(nl_semantic_place_view(p->entry, p->field.parent, &before_root) &&
          nl_semantic_place_view(p->handoff, p->field.parent, &after_root) &&
          nl_semantic_place_view(p->entry, p->field.child, &before_child) &&
          nl_semantic_place_view(p->handoff, p->field.child, &after_child) &&
          before_root.incarnation == after_root.incarnation &&
          before_child.incarnation == after_child.incarnation &&
          before_root.current_fact != after_root.current_fact &&
          before_child.current_fact != after_child.current_fact &&
          scalar(p->entry, before_child.current_value, old) &&
          scalar(p->handoff, after_child.current_value, current));
    NLPlaceId sibling_id = before_root.fixed_fields[1 - index];
    CHECK(sibling_id != p->field.child &&
          nl_semantic_place_view(p->entry, sibling_id, &before_sibling) &&
          nl_semantic_place_view(p->handoff, sibling_id, &after_sibling) &&
          before_sibling.current_fact == after_sibling.current_fact &&
          before_sibling.current_value == after_sibling.current_value &&
          scalar(p->handoff, after_sibling.current_value, sibling));
    NLCopyBodyEventView event;
    CHECK(body->event_count == 1 &&
          nl_checked_copy_body_event(item.arm, item.call, 0, &event) &&
          event.kind == NL_COPY_BODY_CHANGE &&
          nl_checked_copy_body_event_validate(item.arm, item.call, 0,
                                              &event) == NL_CHECK_OK);
    printf("R824 original arm=%zu index=%zu old=%u new=%u sibling=%u "
           "body/parameters/event=OK original-world=OK\n",
           arm, index, old, current, sibling);
    close_case(&item);
    return true;
}

static bool forgery(const char *path, size_t arm, const char *other,
                    size_t foreign_arm)
{
    R824Case a = {0}, b = {0};
    CHECK(open_case(path, arm, &a) && open_case(other, foreign_arm, &b) &&
          valid(&a) && valid(&b) && a.arm != b.arm);
    size_t rejects = 0;
#define REJECT_BODY(change)                                                    \
    do {                                                                       \
        NLCopyBodyView altered = a.body;                                       \
        change;                                                                \
        CHECK(nl_checked_copy_body_validate(a.arm, a.call, &altered) !=        \
              NL_CHECK_OK);                                                    \
        ++rejects;                                                             \
    } while (0)
#define REJECT_ARGS(change)                                                    \
    do {                                                                       \
        NLCopyBodyParameters altered = a.args;                                 \
        change;                                                                \
        CHECK(nl_checked_copy_body_parameters_validate(                      \
                  a.arm, a.call, &altered) != NL_CHECK_OK);                     \
        ++rejects;                                                             \
    } while (0)
    REJECT_BODY(altered.body = b.body.body);
    REJECT_BODY(altered.live_exit = b.body.live_exit);
    REJECT_BODY(altered.root++);
    REJECT_BODY(altered.event_count++);
    REJECT_BODY(altered.result.value++);
    REJECT_ARGS(altered.entry = b.args.entry);
    REJECT_ARGS(altered.handoff = b.args.handoff);
    REJECT_ARGS(altered.field.parent++);
    REJECT_ARGS(altered.field.child++);
    REJECT_ARGS(altered.field.index = 1 - a.args.field.index);
    REJECT_ARGS(altered.field.parent_fact++);
    REJECT_ARGS(altered.field.child_fact++);
    REJECT_ARGS(altered.field.old_value++);
    REJECT_ARGS(altered.loan++);
    REJECT_ARGS(altered.parameters[0]++);
    REJECT_ARGS(altered.arguments[0]++);
#undef REJECT_BODY
#undef REJECT_ARGS
    NLCopyBodyEventView event;
    CHECK(nl_checked_copy_body_event(a.arm, a.call, 0, &event));
    NLCopyBodyEventView bad = event;
    bad.before = b.args.entry;
    CHECK(nl_checked_copy_body_event_validate(a.arm, a.call, 0, &bad) !=
          NL_CHECK_OK);
    ++rejects;
    CHECK(valid(&a) && valid(&b) && rejects == 17);
    printf("R824 foreign-world/index/facts/loan/body/event rejects=%zu "
           "original-restored=OK\n",
           rejects);
    close_case(&b);
    close_case(&a);
    return true;
}

static bool fault(const char *path)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL, *call = NULL;
    NLSemanticContext *world = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    parser = NULL;
    CHECK(nl_semantic_create(&world) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {unit};
    CHECK(nl_semantic_register_function_unit(world, units, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_source_destroy(source);
    CHECK(nl_source_create("main()", 6, "r824-entry", &entry_source) ==
              NL_SOURCE_OK &&
          nl_parser_create(entry_source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(parser, &call, NULL) ==
              NL_PARSE_OK);
    nl_parser_destroy(parser);
    TestState *before = malloc(sizeof(*before));
    CHECK(before != NULL && test_state(world, before));
    bool completed = false;
    size_t refused = 0;
    for (fail_index = 0; fail_index < 4096; ++fail_index) {
        allocation_index = 0;
        inject_failure = true;
        NLCheckedFragment *issued = NULL;
        NLCheckStatus status =
            nl_semantic_check_expression(world, call, &issued, NULL);
        inject_failure = false;
        if (status == NL_CHECK_OK) {
            CHECK(issued != NULL);
            nl_checked_destroy(issued);
            completed = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && issued == NULL &&
              test_unchanged(world, before));
        ++refused;
    }
    CHECK(completed && refused > 1);
    printf("R824 actual registered-source check OOM failures=%zu "
           "unchanged=OK retry=OK second-child-location=NOT_PROVEN\n",
           refused);
    free(before);
    nl_syntax_tree_destroy(call);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(world);
    return true;
}

static bool scope_and_fault(const char *path, size_t arm)
{
    R824Case item = {0};
    CHECK(open_case(path, arm, &item) && valid(&item));
    const NLCheckedNodeView *loan =
        nl_checked_node_view(item.arm, item.args.loan);
    NLSemanticScopeView before, after;
    CHECK(loan && loan->loan.scope &&
          nl_semantic_scope_view(item.args.handoff, loan->loan.scope,
                                 &before) &&
          nl_semantic_scope_view(nl_checked_context(item.arm),
                                 loan->loan.scope, &after) &&
          before.active && !after.active);
    NLCopyBodyParameters invalid = item.args;
    invalid.loan++;
    CHECK(nl_checked_copy_body_parameters_validate(item.arm, item.call,
                                                   &invalid) != NL_CHECK_OK);
    close_case(&item);
    return fault(path);
}

int main(int argc, char **argv)
{
    if (argc == 8 && strcmp(argv[1], "positive") == 0)
        return positive(argv[2], (size_t)strtoul(argv[3], NULL, 10),
                        (size_t)strtoul(argv[4], NULL, 10),
                        (unsigned)strtoul(argv[5], NULL, 10),
                        (unsigned)strtoul(argv[6], NULL, 10),
                        (unsigned)strtoul(argv[7], NULL, 10))
                   ? 0
                   : 1;
    if (argc == 6 && strcmp(argv[1], "forgery") == 0)
        return forgery(argv[2], (size_t)strtoul(argv[3], NULL, 10), argv[4],
                       (size_t)strtoul(argv[5], NULL, 10))
                   ? 0
                   : 1;
    if (argc == 4 && strcmp(argv[1], "scope-fault") == 0)
        return scope_and_fault(argv[2],
                               (size_t)strtoul(argv[3], NULL, 10))
                   ? 0
                   : 1;
    return 2;
}
