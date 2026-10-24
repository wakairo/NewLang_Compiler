#include "../support/semantic_check.h"
#include "newlang/copy_body.h"
#include "newlang/if_field.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* R856: diagnostic observations only, never a replacement public issuer. */
typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *expression;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
} R856Run;

static void release_run(R856Run *run)
{
    nl_checked_destroy(run->entry);
    nl_syntax_tree_destroy(run->expression);
    nl_syntax_tree_destroy(run->unit);
    nl_parser_destroy(run->entry_parser);
    nl_parser_destroy(run->parser);
    nl_source_destroy(run->entry_source);
    nl_source_destroy(run->source);
    nl_semantic_destroy(run->context);
    *run = (R856Run){0};
}

static bool register_run(const char *path, R856Run *run)
{
    if (nl_source_load(path, &run->source) != NL_SOURCE_OK)
        return false;
    if (nl_parser_create(run->source, &run->parser) != NL_PARSE_OK)
        return false;
    if (nl_parser_parse_function_unit(run->parser, &run->unit, NULL) !=
        NL_PARSE_OK)
        return false;
    if (nl_semantic_create(&run->context) != NL_CHECK_OK)
        return false;
    const NLSyntaxTree *units[] = {run->unit};
    if (nl_semantic_register_function_unit(run->context, units, 1, NULL) !=
        NL_CHECK_OK)
        return false;
    if (nl_source_create("main()", 6, "r856-entry", &run->entry_source) !=
        NL_SOURCE_OK)
        return false;
    if (nl_parser_create(run->entry_source, &run->entry_parser) != NL_PARSE_OK)
        return false;
    return nl_parser_parse_expression_fragment(run->entry_parser,
                                               &run->expression,
                                               NULL) == NL_PARSE_OK;
}

typedef struct {
    const NLCheckedFragment *caller;
    NLCheckedNodeId call;
    NLCopyUnitView unit;
    size_t count;
} R856Witness;

static bool walk(const NLCheckedFragment *fragment, R856Witness *w,
                 size_t depth)
{
    if (!fragment || depth >= 32)
        return false;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(fragment); ++id) {
        NLCopyUnitView unit = {0};
        if (nl_checked_copy_unit_view(fragment, id, &unit)) {
            ++w->count;
            if (w->count == 1) {
                w->caller = fragment;
                w->call = id;
                w->unit = unit;
            }
        }
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (!node)
            return false;
        if (node->kind == NL_CHECKED_IF || node->kind == NL_CHECKED_MATCH) {
            for (size_t arm = 0; arm < 2; ++arm) {
                const NLCheckedFragment *part =
                    node->kind == NL_CHECKED_IF
                        ? nl_checked_if_arm(fragment, id, arm)
                        : nl_checked_match_arm(fragment, id, arm);
                if (part && !walk(part, w, depth + 1))
                    return false;
            }
        }
        const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
        if (body && !walk(body, w, depth + 1))
            return false;
    }
    return true;
}

static bool scalar_is(const NLSemanticContext *ctx, NLValueId id,
                      unsigned value)
{
    NLSemanticValueView v = {0};
    return ctx && nl_semantic_value_view(ctx, id, &v) &&
           v.scalar_known && v.scalar_value == value;
}

static bool diagnostic(const char *path, size_t expected_arm,
                       size_t expected_index, unsigned old,
                       unsigned incoming, unsigned sibling)
{
    R856Run run = {0};
    R856Witness w = {0};
    bool result = false;
    if (!register_run(path, &run)) {
        fprintf(stderr, "R856 REGISTER_FAIL path=%s\n", path);
        goto done;
    }
    NLCheckStatus check =
        nl_semantic_check_expression(run.context, run.expression, &run.entry,
                                     NULL);
    printf("R856 SOURCE registered=1 check=%d entry=%p\n", (int)check,
           (void *)run.entry);
    if (check != NL_CHECK_OK || !run.entry)
        goto done;
    if (!walk(run.entry, &w, 0) || w.count != 1 || !w.caller) {
        fprintf(stderr, "R856 WITNESS count=%zu traversal_fail=1\n", w.count);
        goto done;
    }
    const NLCopyUnitView *u = &w.unit;
    const NLCheckedNodeView *call =
        nl_checked_node_view(w.caller, w.call);
    const NLCheckedNodeView *if_node =
        u->body ? nl_checked_node_view(u->body, u->conditional) : NULL;
    const bool body = u->body != NULL;
    const bool normal = u->normal_end != NULL;
    const bool post = u->postcall != NULL;
    const bool separate_pointer = u->normal_end != u->postcall;
    const NLCheckStatus valid_unit =
        nl_checked_copy_unit_validate(w.caller, w.call, u);
    const NLCheckStatus valid_params = nl_checked_copy_body_parameters_validate(
        w.caller, w.call, &u->parameters);
    printf("R856 OWNER caller=%p call=%zu kind=%d body=%p "
           "conditional=%zu if_kind=%d function=%zu root=%zu\n",
           (const void *)w.caller, (size_t)w.call,
           call ? (int)call->kind : -1, (const void *)u->body,
           (size_t)u->conditional, if_node ? (int)if_node->kind : -1,
           u->function, u->body ? (size_t)nl_checked_root(u->body) : 0);
    printf("R856 L63 body=%d normal=%d post=%d pointers_distinct=%d "
           "unit_validate=%d params_validate=%d normal_ptr=%p post_ptr=%p\n",
           body, normal, post, separate_pointer, (int)valid_unit,
           (int)valid_params, (const void *)u->normal_end,
           (const void *)u->postcall);
    printf("R856 PARAMS loan=%zu actual0=%zu actual1=%zu "
           "formal0=%zu formal1=%zu entry=%p handoff=%p "
           "field_index=%zu parent=%zu child=%zu\n",
           (size_t)u->parameters.loan,
           (size_t)u->parameters.arguments[0],
           (size_t)u->parameters.arguments[1],
           (size_t)u->parameters.parameters[0],
           (size_t)u->parameters.parameters[1],
           (const void *)u->parameters.entry,
           (const void *)u->parameters.handoff,
           u->parameters.field.index, (size_t)u->parameters.field.parent,
           (size_t)u->parameters.field.child);
    if (!body || !normal || !post || valid_unit != NL_CHECK_OK ||
        valid_params != NL_CHECK_OK) {
        fprintf(stderr, "R856 STOP unmodified unit/parameters rejection\n");
        goto done;
    }
    NLIfReturnView view = {0};
    const bool has_view =
        nl_checked_if_return_view(u->body, u->conditional, &view);
    const int valid_return =
        has_view ? (int)nl_checked_if_return_validate(u->body,
                                                      u->conditional, &view)
                 : -1;
    const int valid_field =
        has_view ? (int)nl_checked_if_field_validate(u->body,
                                                     u->conditional,
                                                     &view.field)
                 : -1;
    printf("R856 L70 view=%d return_validate=%d field_validate=%d "
           "normal_arm=%zu expected_arm=%zu field_index=%zu "
           "expected_index=%zu replay_events=%zu replay_changes=%zu "
           "arm0_events=%zu arm1_events=%zu "
           "arm0_changes=%zu arm1_changes=%zu "
           "return_node=%zu replay_root=%zu return_live=%p ended=%p\n",
           has_view, (int)valid_return, (int)valid_field,
           view.normal_arm, expected_arm, view.field.index, expected_index,
           view.replay_events, view.replay_changes,
           view.field.arms[0].event_count, view.field.arms[1].event_count,
           view.field.arms[0].change_count, view.field.arms[1].change_count,
           (size_t)view.return_node, (size_t)view.replay_root,
           (const void *)view.return_live, (const void *)view.return_ended);
    if (!has_view || valid_return != NL_CHECK_OK ||
        valid_field != NL_CHECK_OK) {
        fprintf(stderr, "R856 STOP unmodified IF public proof rejection\n");
        goto done;
    }
    if (view.normal_arm != expected_arm ||
        view.field.index != expected_index) {
        fprintf(stderr, "R856 STOP original owner/arm mismatch\n");
        goto done;
    }
    NLIfFieldEvent event = {0};
    const bool has_original = nl_checked_if_return_event(
        u->body, u->conditional, expected_arm, 0, &event);
    const int event_status =
        has_original ? (int)nl_checked_if_return_event_validate(
                           u->body, u->conditional, expected_arm, 0, &event)
                     : -1;
    printf("R856 ORIGINAL view=%d status=%d change=%d index=%zu "
           "before=%p after=%p child=%zu old=%zu result=%zu incoming=%zu "
           "old_matches=%d copy_matches=%d incoming_matches=%d\n",
           has_original, (int)event_status, event.change,
           event.field.index, (const void *)event.before,
           (const void *)event.after, (size_t)event.field.child,
           (size_t)event.field.old_value, (size_t)event.result,
           (size_t)event.incoming,
           scalar_is(event.before, event.field.old_value, old),
           scalar_is(event.after, event.result, old),
           scalar_is(event.after, event.incoming, incoming));
    if (!has_original || event_status != NL_CHECK_OK) {
        fprintf(stderr, "R856 STOP original event rejection\n");
        goto done;
    }
    NLIfFieldEvent replay = {0};
    const bool has_replay = nl_checked_if_return_event(
        u->body, u->conditional, 2, 0, &replay);
    const int replay_status =
        has_replay ? (int)nl_checked_if_return_event_validate(
                         u->body, u->conditional, 2, 0, &replay)
                   : -1;
    printf("R856 REPLAY view=%d status=%d change=%d "
           "replay_events=%zu (no exact-count assertion)\n",
           has_replay, (int)replay_status, replay.change,
           view.replay_events);
    if (!has_replay || replay_status != NL_CHECK_OK)
        goto done;
    NLSemanticPlaceView pre = {0}, joined = {0}, pre_sibling = {0};
    NLSemanticPlaceView joined_sibling = {0};
    NLValueId child = u->parameters.field.child;
    const NLPlaceId parent = u->parameters.field.parent;
    NLSemanticPlaceView pre_root = {0};
    const bool pre_ok =
        nl_semantic_place_view(u->parameters.entry, parent, &pre_root) &&
        nl_semantic_place_view(u->parameters.entry, child, &pre);
    const NLPlaceId sibling_id =
        pre_ok && expected_index < 2
            ? pre_root.fixed_fields[1 - expected_index]
            : 0;
    const bool joined_ok =
        nl_semantic_place_view(u->postcall, child, &joined);
    const bool sibling_ok =
        sibling_id &&
        nl_semantic_place_view(u->parameters.entry, sibling_id,
                               &pre_sibling) &&
        nl_semantic_place_view(u->postcall, sibling_id, &joined_sibling);
    NLSemanticValueView value = {0};
    const bool joined_value =
        joined_ok && nl_semantic_value_view(u->postcall,
                                            joined.current_value, &value);
    printf("R856 JOIN pre=%d post=%d sibling=%d joined_value=%d "
           "known=%d scalar=%zu pre_old=%d sibling_old=%d sibling_post=%d "
           "root=%zu child=%zu sibling_id=%zu\n",
           pre_ok, joined_ok, sibling_ok, joined_value,
           joined_value && value.scalar_known,
           joined_value ? (size_t)value.scalar_value : 0,
           pre_ok && scalar_is(u->parameters.entry, pre.current_value, old),
           sibling_ok && scalar_is(u->parameters.entry,
                                   pre_sibling.current_value, sibling),
           sibling_ok && scalar_is(u->postcall,
                                   joined_sibling.current_value, sibling),
           (size_t)parent, (size_t)child, (size_t)sibling_id);
    result = event.change && event.field.index == expected_index &&
             scalar_is(event.before, event.field.old_value, old) &&
             scalar_is(event.after, event.result, old) &&
             scalar_is(event.after, event.incoming, incoming) &&
             pre_ok && joined_ok && sibling_ok && joined_value &&
             !value.scalar_known &&
             scalar_is(u->parameters.entry, pre.current_value, old) &&
             scalar_is(u->postcall, joined_sibling.current_value, sibling);
    printf("R856 PUBLIC_DIAGNOSTIC final=%s\n",
           result ? "VALID" : "CONTRACT_OR_OBSERVER_UNRESOLVED");
done:
    release_run(&run);
    return result;
}

/* Passive link-stage counters. A wrapped symbol is not itself a proof of the
 * private issuer's allocation phase, so never inject faults at this gate. */
static bool tracking, inside_seal;
static size_t seal_calls, arm_roots, malloc_calls, calloc_calls, realloc_calls;
static int first_root_kind = -1, last_root_kind = -1;

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *ptr, size_t size);
NLCheckStatus __real_nl_copy_body_seal_calls(NLCheckedFragment *fragment);

void *__wrap_malloc(size_t size)
{
    if (tracking && inside_seal)
        ++malloc_calls;
    return __real_malloc(size);
}

void *__wrap_calloc(size_t count, size_t size)
{
    if (tracking && inside_seal)
        ++calloc_calls;
    return __real_calloc(count, size);
}

void *__wrap_realloc(void *ptr, size_t size)
{
    if (tracking && inside_seal)
        ++realloc_calls;
    return __real_realloc(ptr, size);
}

NLCheckStatus __wrap_nl_copy_body_seal_calls(NLCheckedFragment *fragment)
{
    const NLCheckedNodeView *root =
        nl_checked_node_view(fragment, nl_checked_root(fragment));
    const int kind = root ? (int)root->kind : -1;
    if (tracking) {
        if (!seal_calls)
            first_root_kind = kind;
        last_root_kind = kind;
        ++seal_calls;
        if (kind == (int)NL_CHECKED_IF_ARM)
            ++arm_roots;
    }
    const bool previous = inside_seal;
    inside_seal = tracking;
    NLCheckStatus status = __real_nl_copy_body_seal_calls(fragment);
    inside_seal = previous;
    return status;
}

static bool baseline(const char *path)
{
    R856Run run = {0};
    bool result = false;
    if (!register_run(path, &run)) {
        fprintf(stderr, "R856 OOM REGISTER_FAIL path=%s\n", path);
        goto done;
    }
    seal_calls = arm_roots = malloc_calls = calloc_calls = realloc_calls = 0;
    first_root_kind = last_root_kind = -1;
    tracking = true;
    NLCheckStatus status =
        nl_semantic_check_expression(run.context, run.expression, &run.entry,
                                     NULL);
    tracking = false;
    printf("R856 OOM_BASELINE status=%d out=%p actual_failure=0 "
           "issuer_calls=%zu if_arm_roots=%zu first_root_kind=%d "
           "last_root_kind=%d malloc=%zu calloc=%zu realloc=%zu "
           "positions=%zu index=NOT_ATTEMPTED\n",
           (int)status, (void *)run.entry, seal_calls, arm_roots,
           first_root_kind, last_root_kind, malloc_calls, calloc_calls,
           realloc_calls, malloc_calls + calloc_calls + realloc_calls);
    printf("R856 OOM_CLASSIFICATION genuine_fault=NOT_TESTED "
           "rollback=NOT_TESTED clean_retry=NOT_TESTED\n");
    result = status == NL_CHECK_OK && run.entry != NULL;
done:
    tracking = false;
    release_run(&run);
    return result;
}

int main(int argc, char **argv)
{
    if (argc == 8 && strcmp(argv[1], "public") == 0)
        return diagnostic(argv[2], (size_t)strtoul(argv[3], NULL, 10),
                          (size_t)strtoul(argv[4], NULL, 10),
                          (unsigned)strtoul(argv[5], NULL, 10),
                          (unsigned)strtoul(argv[6], NULL, 10),
                          (unsigned)strtoul(argv[7], NULL, 10))
                   ? 0
                   : 1;
    if (argc == 3 && strcmp(argv[1], "baseline") == 0)
        return baseline(argv[2]) ? 0 : 1;
    return 2;
}
