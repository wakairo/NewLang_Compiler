#include "../support/node_checked.h"
#include "newlang/copy_body.h"
#include "newlang/if_field.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const NLCheckedFragment *caller;
    NLCheckedNodeId call;
    NLCopyUnitView unit;
} R848Witness;

static bool observe(const NLCheckedFragment *fragment, R848Witness *out,
                    size_t depth)
{
    CHECK(fragment && depth < 32);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(fragment); ++id) {
        NLCopyUnitView unit;
        if (nl_checked_copy_unit_view(fragment, id, &unit)) {
            CHECK(out->caller == NULL);
            out->caller = fragment;
            out->call = id;
            out->unit = unit;
        }
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        CHECK(node != NULL);
        if (node->kind == NL_CHECKED_IF ||
            node->kind == NL_CHECKED_MATCH) {
            for (size_t arm = 0; arm < 2; ++arm) {
                const NLCheckedFragment *part =
                    node->kind == NL_CHECKED_IF
                        ? nl_checked_if_arm(fragment, id, arm)
                        : nl_checked_match_arm(fragment, id, arm);
                if (part)
                    CHECK(observe(part, out, depth + 1));
            }
        }
        const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
        if (body)
            CHECK(observe(body, out, depth + 1));
    }
    return true;
}

static bool scalar(const NLSemanticContext *world, NLValueId value,
                   unsigned expected)
{
    NLSemanticValueView result;
    return nl_semantic_value_view(world, value, &result) &&
           result.scalar_known && result.scalar_value == expected;
}

static bool positive(const char *path, size_t normal, size_t index,
                     unsigned old, unsigned incoming, unsigned adjacent)
{
    TestNode test = {0};
    R848Witness witness = {0};
    CHECK(node_checked_load(path, &test));
    CHECK(observe(test.entry, &witness, 0) && witness.caller);
    const NLCopyUnitView unit = witness.unit;
    CHECK(unit.body && unit.normal_end && unit.postcall &&
          unit.normal_end != unit.postcall &&
          nl_checked_copy_unit_validate(witness.caller, witness.call, &unit) ==
              NL_CHECK_OK &&
          nl_checked_copy_body_parameters_validate(
              witness.caller, witness.call, &unit.parameters) == NL_CHECK_OK);
    NLIfReturnView returned;
    CHECK(nl_checked_if_return_view(unit.body, unit.conditional, &returned) &&
          nl_checked_if_return_validate(unit.body, unit.conditional,
                                        &returned) == NL_CHECK_OK &&
          nl_checked_if_field_validate(unit.body, unit.conditional,
                                       &returned.field) == NL_CHECK_OK &&
          returned.normal_arm == normal &&
          returned.field.index == index &&
          returned.replay_events == 1 && returned.replay_changes == 1);
    NLIfFieldEvent event;
    CHECK(nl_checked_if_return_event(unit.body, unit.conditional, normal, 0,
                                     &event) &&
          nl_checked_if_return_event_validate(
              unit.body, unit.conditional, normal, 0, &event) == NL_CHECK_OK &&
          event.change && event.before && event.after &&
          event.field.index == index &&
          scalar(event.before, event.field.old_value, old) &&
          scalar(event.after, event.result, old) &&
          scalar(event.after, event.incoming, incoming));
    NLSemanticPlaceView arm_before, arm_after;
    CHECK(nl_semantic_place_view(event.before, event.field.child,
                                 &arm_before) &&
          nl_semantic_place_view(event.after, event.field.child, &arm_after) &&
          arm_before.incarnation == arm_after.incarnation &&
          scalar(event.after, arm_after.current_value, incoming));
    NLIfFieldEvent replay;
    CHECK(nl_checked_if_return_event(unit.body, unit.conditional, 2, 0,
                                     &replay) &&
          nl_checked_if_return_event_validate(
              unit.body, unit.conditional, 2, 0, &replay) == NL_CHECK_OK);

    const NLCheckedField field = unit.parameters.field;
    CHECK(field.present && field.index == index && field.parent &&
          field.child && unit.parameters.entry && unit.parameters.handoff &&
          unit.parameters.loan && unit.parameters.arguments[0] &&
          unit.parameters.arguments[1]);
    NLSemanticPlaceView before_root, post_root, before_child, post_child;
    CHECK(nl_semantic_place_view(unit.parameters.entry, field.parent,
                                 &before_root) &&
          nl_semantic_place_view(unit.postcall, field.parent, &post_root) &&
          nl_semantic_place_view(unit.parameters.entry, field.child,
                                 &before_child) &&
          nl_semantic_place_view(unit.postcall, field.child, &post_child) &&
          before_root.incarnation == post_root.incarnation &&
          before_child.incarnation == post_child.incarnation &&
          before_root.fixed_fields[index] == field.child &&
          post_root.fixed_fields[index] == field.child &&
          scalar(unit.parameters.entry, before_child.current_value, old));
    const NLPlaceId sibling = before_root.fixed_fields[1 - index];
    NLSemanticPlaceView before_sibling, post_sibling;
    CHECK(sibling && post_root.fixed_fields[1 - index] == sibling &&
          nl_semantic_place_view(unit.parameters.entry, sibling,
                                 &before_sibling) &&
          nl_semantic_place_view(unit.postcall, sibling, &post_sibling) &&
          before_sibling.incarnation == post_sibling.incarnation &&
          before_sibling.current_fact == post_sibling.current_fact &&
          before_sibling.current_value == post_sibling.current_value &&
          scalar(unit.postcall, post_sibling.current_value, adjacent));
    NLSemanticValueView joined;
    CHECK(nl_semantic_value_view(unit.postcall, post_child.current_value,
                                 &joined) &&
          joined.type == before_child.type && !joined.scalar_known);
    printf("R848 ORIGINAL normal=%zu index=%zu old=%u incoming=%u "
           "sibling=%u copy=VALID join=NONCONCRETE owner=SOURCE\n",
           normal, index, old, incoming, adjacent);
    node_checked_destroy(&test);
    return true;
}

static bool negative(const char *first, const char *second)
{
    TestNode a = {0}, foreign = {0}, b = {0};
    R848Witness one = {0}, alien = {0}, other = {0};
    CHECK(node_checked_load(first, &a) &&
          node_checked_load(first, &foreign) &&
          node_checked_load(second, &b) &&
          observe(a.entry, &one, 0) &&
          observe(foreign.entry, &alien, 0) &&
          observe(b.entry, &other, 0) &&
          one.caller && alien.caller && other.caller &&
          one.call == alien.call &&
          one.caller != alien.caller);
    NLCopyUnitView value = one.unit, bad = value;
    CHECK(nl_checked_copy_unit_validate(one.caller, one.call, &value) ==
          NL_CHECK_OK);
    CHECK(nl_checked_copy_unit_validate(one.caller, one.call,
                                        &alien.unit) != NL_CHECK_OK);
    CHECK(nl_checked_copy_unit_validate(one.caller, one.call,
                                        &other.unit) != NL_CHECK_OK);
    bad.postcall = value.normal_end;
    CHECK(nl_checked_copy_unit_validate(one.caller, one.call, &bad) !=
          NL_CHECK_OK);
    NLCopyBodyParameters parameters = value.parameters;
    CHECK(nl_checked_copy_body_parameters_validate(
              one.caller, one.call, &parameters) == NL_CHECK_OK);
    parameters.arguments[0] = parameters.arguments[1];
    CHECK(nl_checked_copy_body_parameters_validate(
              one.caller, one.call, &parameters) != NL_CHECK_OK);
    parameters = value.parameters;
    parameters.loan = parameters.arguments[1];
    CHECK(nl_checked_copy_body_parameters_validate(
              one.caller, one.call, &parameters) != NL_CHECK_OK);
    NLIfReturnView proof;
    CHECK(nl_checked_if_return_view(value.body, value.conditional, &proof));
    NLIfReturnView changed = proof;
    changed.normal_arm = 1 - proof.normal_arm;
    CHECK(nl_checked_if_return_validate(value.body, value.conditional,
                                        &changed) != NL_CHECK_OK);
    changed = proof;
    changed.field.child = changed.field.sibling;
    CHECK(nl_checked_if_return_validate(value.body, value.conditional,
                                        &changed) != NL_CHECK_OK);
    NLIfFieldEvent event;
    CHECK(nl_checked_if_return_event(value.body, value.conditional,
                                     proof.normal_arm, 0, &event));
    NLIfFieldEvent forged = event;
    forged.incoming++;
    CHECK(nl_checked_if_return_event_validate(
              value.body, value.conditional, proof.normal_arm, 0,
              &forged) != NL_CHECK_OK);
    forged = event;
    forged.field.child_fact++;
    CHECK(nl_checked_if_return_event_validate(
              value.body, value.conditional, proof.normal_arm, 0,
              &forged) != NL_CHECK_OK);
    printf("R848 NEGATIVE foreign-equal-numeric=REJECT "
           "wrong-body=REJECT postcall-fork=REJECT formal-loan=REJECT "
           "return-index=REJECT sibling=REJECT stale-copy=REJECT\n");
    node_checked_destroy(&b);
    node_checked_destroy(&foreign);
    node_checked_destroy(&a);
    return true;
}

typedef struct {
    NLSemanticContext *context;
    NLSource *entry_source;
    NLSyntaxTree *entry;
} R848Registered;

static bool register_source(const char *path, R848Registered *r)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&r->context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    CHECK(nl_semantic_register_function_unit(r->context, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    CHECK(nl_source_create("main()", 6, "r848-real-entry",
                           &r->entry_source) == NL_SOURCE_OK);
    parser = NULL;
    CHECK(nl_parser_create(r->entry_source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(parser, &r->entry, NULL) ==
              NL_PARSE_OK);
    nl_parser_destroy(parser);
    return true;
}

static void unregister_source(R848Registered *r)
{
    nl_syntax_tree_destroy(r->entry);
    nl_source_destroy(r->entry_source);
    nl_semantic_destroy(r->context);
    *r = (R848Registered){0};
}

/* The linker hook brackets the actual completed-arm issuer, not parsing or
 * registration. A failure outside this narrow stage is never counted. */
static bool injecting, inside_issuer, actual_failure;
static size_t allocation_count, failure_index, issuer_calls;
void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    if (injecting && inside_issuer && allocation_count++ == failure_index) {
        actual_failure = true;
        return NULL;
    }
    return __real_malloc(size);
}
NLCheckStatus __real_nl_copy_body_seal_calls(NLCheckedFragment *);
NLCheckStatus __wrap_nl_copy_body_seal_calls(NLCheckedFragment *fragment)
{
    const NLCheckedNodeView *root =
        nl_checked_node_view(fragment, nl_checked_root(fragment));
    const bool selected =
        injecting && root && root->kind == NL_CHECKED_IF_ARM;
    bool previous = inside_issuer;
    if (selected) {
        inside_issuer = true;
        ++issuer_calls;
    }
    NLCheckStatus result = __real_nl_copy_body_seal_calls(fragment);
    inside_issuer = previous;
    return result;
}

static bool oom(const char *path)
{
    R848Registered baseline = {0};
    CHECK(register_source(path, &baseline));
    NLCheckedFragment *out = NULL;
    allocation_count = issuer_calls = 0;
    failure_index = SIZE_MAX;
    actual_failure = false;
    injecting = true;
    NLCheckStatus status = nl_semantic_check_expression(
        baseline.context, baseline.entry, &out, NULL);
    injecting = false;
    const size_t positions = allocation_count;
    CHECK(status == NL_CHECK_OK && out && !actual_failure &&
          issuer_calls && positions > 0 && positions <= 32);
    nl_checked_destroy(out);
    unregister_source(&baseline);
    for (size_t i = 0; i < positions; ++i) {
        R848Registered unit = {0};
        CHECK(register_source(path, &unit));
        TestState *snapshot = malloc(sizeof(*snapshot));
        CHECK(snapshot && test_state(unit.context, snapshot));
        out = NULL;
        failure_index = i;
        allocation_count = issuer_calls = 0;
        actual_failure = false;
        injecting = true;
        status = nl_semantic_check_expression(
            unit.context, unit.entry, &out, NULL);
        injecting = false;
        CHECK(actual_failure && issuer_calls &&
              status == NL_CHECK_OUT_OF_MEMORY && out == NULL &&
              test_unchanged(unit.context, snapshot));
        CHECK(nl_semantic_check_expression(unit.context, unit.entry,
                                           &out, NULL) == NL_CHECK_OK &&
              out);
        R848Witness witness = {0};
        CHECK(observe(out, &witness, 0) && witness.caller &&
              nl_checked_copy_unit_validate(
                  witness.caller, witness.call, &witness.unit) ==
                  NL_CHECK_OK);
        printf("R848 ISSUER_OOM index=%zu/%zu reached=1 "
               "output=NULL rollback=1 clean-retry=1\n",
               i, positions);
        nl_checked_destroy(out);
        free(snapshot);
        unregister_source(&unit);
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc == 8 && strcmp(argv[1], "public") == 0)
        return positive(argv[2], (size_t)strtoul(argv[3], NULL, 10),
                        (size_t)strtoul(argv[4], NULL, 10),
                        (unsigned)strtoul(argv[5], NULL, 10),
                        (unsigned)strtoul(argv[6], NULL, 10),
                        (unsigned)strtoul(argv[7], NULL, 10))
                   ? 0
                   : 1;
    if (argc == 4 && strcmp(argv[1], "negative") == 0)
        return negative(argv[2], argv[3]) ? 0 : 1;
    if (argc == 3 && strcmp(argv[1], "oom") == 0)
        return oom(argv[2]) ? 0 : 1;
    return 2;
}
