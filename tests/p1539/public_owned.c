/* P1515 C1150 public-only observation of authentic source-issued artifacts.
 * No semantic seed, private headers, artifact mutation, emission or guest run.
 */
#include "newlang/checked.h"
#include "newlang/copy_body.h"
#include "newlang/parser.h"
#include "newlang/raw_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEED(x)                                                                \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "PROBE_ASSERT line=%d expression=%s\n", __LINE__,  \
                    #x);                                                       \
            return 0;                                                          \
        }                                                                      \
    } while (0)

typedef struct {
    size_t calls, copies, payloads, destroys, frees, trials, helpers;
} Count;
typedef struct {
    size_t sample, initial, sibling, copies, owners, reads, matches, loans;
    NLSymbolId original_binding;
    NLPlaceId original_root, original_child;
    NLIncarnationId original_incarnation;
    const NLCheckedFragment *copy_owners[2];
    size_t helpers;
    const NLCheckedFragment *helper_owner;
    NLSymbolId helper_binding;
    NLValueId helper_result;
    NLSourceSpan helper_span;
} Trace;

static int scalar(const NLSemanticContext *c, NLValueId id, size_t value)
{
    NLSemanticValueView v;
    return nl_semantic_value_view(c, id, &v) && v.scalar_known &&
           v.scalar_value == value && v.dependencies == NL_DEPENDENCY_FREE &&
           v.value_dependency_count == 0 && v.reference_count == 0;
}

static int closed(const NLSemanticContext *c, size_t regions)
{
    NLSemanticSnapshot s;
    NEED(nl_semantic_snapshot(c, &s) && s.backing_regions == regions &&
         s.domains == regions);
    for (size_t i = 1; i <= s.backing_regions; ++i) {
        NLSemanticBackingView r;
        NEED(nl_semantic_backing_view(c, i, &r) && !r.live);
    }
    for (size_t i = 1; i <= s.domains; ++i) {
        NLSemanticDomainView d;
        NEED(nl_semantic_domain_view(c, i, &d) && !d.live);
    }
    for (size_t i = 1; i <= s.scopes; ++i) {
        NLSemanticScopeView scope;
        NEED(nl_semantic_scope_view(c, i, &scope) && !scope.active);
    }
    return 1;
}

static int owner(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const NLSemanticContext *entry = nl_checked_owner_entry(f, id);
    const NLSemanticContext *post = nl_checked_context(f);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    NLTypedOwnerDefinition d;
    NLSemanticSnapshot s;
    NLSemanticPlaceView p, dead;
    NLSemanticValueView ptr, a, life;
    NLSemanticBackingView r, dead_r;
    NLSemanticDomainView domain, dead_domain;
    NEED(n && entry && entry != post && body && n->body_backed &&
         nl_checked_context(body) == post && n->owner_call.entry_proved &&
         n->owner_call.post_proved && n->argument_count == 3 &&
         !n->result_count && nl_semantic_snapshot(entry, &s) &&
         s.backing_regions == 2 && s.domains == 2);
    NEED(nl_semantic_function_applicability(post, n->function, &d) &&
         d.definition_checked && !d.live_return && !d.head_link_required &&
         d.requirements == NL_OWNER_ALL_REQUIREMENTS && d.step_count == 5 &&
         d.target == n->owner_call.definition.target);
    for (size_t i = 0; i < 5; ++i)
        NEED(d.steps[i] == (NLTypedOwnerStep)i &&
             n->owner_call.definition.steps[i] == d.steps[i]);
    NEED(nl_semantic_value_view(entry, n->owner_call.inputs[0], &ptr) &&
         nl_semantic_value_view(entry, n->owner_call.inputs[1], &a) &&
         nl_semantic_value_view(entry, n->owner_call.inputs[2], &life) &&
         ptr.reference.provenance == NL_PROVENANCE_VALID &&
         ptr.reference.place == n->owner_call.root &&
         ptr.reference.incarnation == n->owner_call.incarnation &&
         a.allocation_region == n->owner_call.range.region &&
         life.domain == n->owner_call.domain);
    NEED(nl_semantic_place_view(entry, n->owner_call.root, &p) && p.live &&
         p.independent_root && p.type == d.target &&
         p.incarnation == n->owner_call.incarnation &&
         p.governing_domain == life.domain && p.placement.start == 0 &&
         p.placement.region == a.allocation_region &&
         p.placement.length == n->owner_call.range.length &&
         nl_semantic_backing_view(entry, a.allocation_region, &r) && r.live &&
         r.size == p.placement.length && r.ordinary_read && r.ordinary_write &&
         nl_semantic_domain_view(entry, life.domain, &domain) && domain.live &&
         domain.value == n->owner_call.inputs[2]);
    size_t others = 0;
    for (size_t i = 1; i <= s.places; ++i) {
        NLSemanticPlaceView q;
        NEED(nl_semantic_place_view(entry, i, &q));
        if (i != n->owner_call.root && q.live && q.independent_root &&
            q.placement.region) {
            ++others;
            NEED(q.placement.region != a.allocation_region &&
                 q.governing_domain != life.domain);
        }
    }
    NEED(others == 1);
    NLCheckedNodeId argument = n->first_argument;
    for (size_t i = 0; i < 3; ++i) {
        NLSemanticBindingView formal, donor;
        const NLCheckedNodeView *arg = nl_checked_node_view(f, argument);
        NEED(arg && arg->kind == NL_CHECKED_IDENTIFIER &&
             arg->result_count == 1 &&
             arg->results[0].value == n->owner_call.inputs[i] &&
             arg->symbol == n->owner_call.donor[i] &&
             arg->value_use == (i ? NL_VALUE_CONSUMED : NL_VALUE_COPIED) &&
             n->owner_call.donor[i] != n->owner_call.parameters[i] &&
             nl_semantic_binding_view(post, n->owner_call.parameters[i],
                                      &formal) &&
             formal.value == n->owner_call.inputs[i] &&
             nl_semantic_binding_view(entry, n->owner_call.donor[i], &donor));
        if (i)
            NEED(donor.availability == NL_CONSUMED &&
                 formal.availability == NL_CONSUMED);
        argument = arg->next_argument;
    }
    NEED(!argument && nl_semantic_place_view(post, n->owner_call.root, &dead) &&
         !dead.live && dead.incarnation == p.incarnation &&
         !dead.current_value && !dead.placement.region &&
         nl_semantic_backing_view(post, a.allocation_region, &dead_r) &&
         !dead_r.live &&
         nl_semantic_domain_view(post, life.domain, &dead_domain) &&
         !dead_domain.live);
    size_t ends = 0, erases = 0, finals = 0, frees = 0, optional_reads = 0;
    size_t read_end = 0, destroy_start = 0;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(body, i);
        NEED(v && !v->allocation_trial && !v->allocation_success);
        if (v->kind == NL_CHECKED_REF_FROM_PTR) {
            NLSemanticValueView ref;
            NLSemanticScopeView scope;
            NEED(v->result_count == 1 &&
                 v->lifetime_place == n->owner_call.root &&
                 v->lifetime_incarnation == p.incarnation &&
                 v->lifetime_domain == life.domain &&
                 nl_semantic_value_view(post, v->results[0].value, &ref) &&
                 ref.reference.place == n->owner_call.root &&
                 ref.reference.incarnation == p.incarnation &&
                 nl_semantic_scope_view(post, ref.reference.scope, &scope) &&
                 !scope.active);
            ++optional_reads;
            read_end = v->span.end_byte;
        }
        if (v->kind == NL_CHECKED_DESTROY) {
            destroy_start = v->span.start_byte;
            ++ends;
            NEED(v->lifetime_place == n->owner_call.root &&
                 v->lifetime_incarnation == p.incarnation &&
                 v->lifetime_domain == life.domain &&
                 v->backing == a.allocation_region);
        }
        if (v->kind == NL_CHECKED_ERASE_SLOT)
            ++erases;
        if (v->kind == NL_CHECKED_DOMAIN_FINALIZE)
            ++finals;
        if (v->kind == NL_CHECKED_DEALLOCATE) {
            ++frees;
            const NLCheckedNodeView *av =
                nl_checked_node_view(body, v->first_argument);
            const NLCheckedNodeView *raw =
                av ? nl_checked_node_view(body, av->next_argument) : NULL;
            NLSemanticValueView full, avalue;
            NEED(av && raw && av->result_count == 1 && raw->result_count == 1 &&
                 nl_semantic_value_view(post, av->results[0].value, &avalue) &&
                 nl_semantic_value_view(post, raw->results[0].value, &full) &&
                 avalue.allocation_region == a.allocation_region &&
                 full.occupancy.region == a.allocation_region &&
                 full.occupancy.start == 0 && full.occupancy.length == r.size);
        }
    }
    NEED(ends == 1 && erases == 1 && finals == 1 && frees == 1 &&
         optional_reads == 1 && read_end < destroy_start);
    printf(
        "OPTIONAL same_original_root=%zu same_D=%zu closed_before_EndRoot=1\n",
        n->owner_call.root, life.domain);
    printf("OWNER actual=%zu root=%zu incarnation=%zu region=%zu domain=%zu "
           "independent_other_live=%zu definition_steps=5 full_range=%zu\n",
           id, n->owner_call.root, p.incarnation, a.allocation_region,
           life.domain, others, r.size);
    return 1;
}

static int helper(const NLCheckedFragment *f, NLCheckedNodeId id, Trace *t)
{
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedNodeView *arg =
        call ? nl_checked_node_view(f, call->first_argument) : NULL;
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    const NLCheckedNodeView *root =
        body ? nl_checked_node_view(body, nl_checked_root(body)) : NULL;
    const NLCheckedNodeView *tail =
        root ? nl_checked_node_view(body, root->tail) : NULL;
    NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    NLSemanticBindingView input, formal, output;
    NEED(!t->helpers && t->reads == 1 && call && call->body_backed &&
         call->type == u8 && call->result_count == 1 &&
         call->argument_count == 1 && !call->owner_call.entry_proved &&
         !call->producer.entry_proved && arg &&
         arg->kind == NL_CHECKED_IDENTIFIER && arg->type == u8 &&
         arg->parameter_type == u8 && !arg->next_argument &&
         arg->value_use == NL_VALUE_COPIED && arg->result_count == 1 &&
         scalar(c, arg->results[0].value, t->sample) &&
         scalar(c, call->results[0].value, t->sample) && body &&
         nl_checked_context(body) == c && root &&
         root->kind == NL_CHECKED_BLOCK && !root->terminates &&
         !root->item_count && root->type == u8 && root->result_count == 1 &&
         tail && tail->kind == NL_CHECKED_IDENTIFIER && tail->type == u8 &&
         tail->value_use == NL_VALUE_COPIED && tail->result_count == 1 &&
         tail->symbol != arg->symbol &&
         nl_semantic_binding_view(c, tail->symbol, &formal) &&
         formal.type == u8 && formal.value == arg->results[0].value &&
         root->results[0].value == call->results[0].value &&
         tail->results[0].value == call->results[0].value &&
         nl_semantic_binding_view(c, arg->symbol, &input));
    size_t inputs = 0, outputs = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind != NL_CHECKED_BINDING)
            continue;
        const NLCheckedNodeView *init = nl_checked_node_view(f, n->initializer);
        if (n->symbol == arg->symbol) {
            const NLCheckedNodeView *block =
                init ? nl_checked_node_view(f, init->initializer) : NULL;
            const NLCheckedNodeView *read =
                block ? nl_checked_node_view(f, block->tail) : NULL;
            NLSemanticScopeView scope;
            NEED(init && init->kind == NL_CHECKED_LOAN_HEADER &&
                 init->type == u8 && init->loan.body_nonescape_proved &&
                 init->loan.normal_result_forwarded &&
                 !init->loan.is_exclusive &&
                 init->loan.access == NL_ACCESS_READ &&
                 nl_semantic_scope_view(c, init->loan.scope, &scope) &&
                 !scope.active && read && read->kind == NL_CHECKED_LINK_READ &&
                 read->field.index == 1 &&
                 init->span.end_byte < call->span.start_byte &&
                 input.value == init->results[0].value &&
                 init->results[0].value == read->results[0].value);
            ++inputs;
        }
        if (n->initializer == id) {
            NEED(nl_semantic_binding_view(c, n->symbol, &output) &&
                 output.type == u8 && output.value == call->results[0].value);
            t->helper_binding = n->symbol;
            ++outputs;
        }
    }
    NEED(inputs == 1 && outputs == 1);
    t->helper_owner = f;
    t->helper_result = call->results[0].value;
    t->helper_span = call->span;
    ++t->helpers;
    printf(
        "SCALAR_HELPER actual=%zu formal=%zu input=%zu result=%zu binding=%zu "
        "plain_u8_after_closed_caller_loan=1\n",
        id, tail->symbol, arg->results[0].value, t->helper_result,
        t->helper_binding);
    return 1;
}

static int copy(const NLCheckedFragment *f, NLCheckedNodeId id, Trace *t)
{
    NLCopyBodyParameters p;
    NLCopyBodyView v;
    NLCopyBodyEventView event;
    NLSemanticPlaceView before, after, parent, sibling;
    NLSemanticBindingView binding;
    NEED(t->copies < 2 && nl_checked_copy_body_parameters(f, id, &p) &&
         nl_checked_copy_body_view(f, id, &v) &&
         nl_checked_copy_body_event(f, id, 0, &event));
    if (!t->copies)
        t->original_binding = p.field.base;
    NLCheckStatus bv = nl_checked_copy_body_validate(f, id, &v);
    NLCheckStatus pv = nl_checked_copy_body_parameters_validate(f, id, &p);
    NLCheckStatus ev = nl_checked_copy_body_event_validate(f, id, 0, &event);
    printf("COPY call=%zu ordinal=%zu body=%d parameters=%d event=%d\n", id,
           t->copies + 1, (int)bv, (int)pv, (int)ev);
    NEED(bv == NL_CHECK_OK && pv == NL_CHECK_OK && ev == NL_CHECK_OK &&
         v.event_count == 1 && event.kind == NL_COPY_BODY_CHANGE &&
         p.field.index == 0 && p.entry && p.handoff &&
         nl_semantic_place_view(p.entry, p.field.child, &before) &&
         nl_semantic_place_view(p.handoff, p.field.child, &after) &&
         nl_semantic_place_view(p.handoff, p.field.parent, &parent) &&
         nl_semantic_place_view(p.handoff, parent.fixed_fields[1], &sibling) &&
         scalar(p.entry, before.current_value, t->copies ? 0 : t->initial) &&
         scalar(p.handoff, after.current_value, t->copies ? t->sample : 0) &&
         scalar(p.handoff, sibling.current_value, t->sibling) &&
         scalar(v.live_exit, v.result.value, t->copies ? 0 : t->initial) &&
         before.incarnation == after.incarnation &&
         before.current_fact != after.current_fact &&
         nl_semantic_binding_view(p.entry, t->original_binding, &binding) &&
         binding.place == p.field.parent);
    if (t->copies) {
        const NLCheckedNodeView *call = nl_checked_node_view(f, id);
        const NLCheckedNodeView *first =
            call ? nl_checked_node_view(f, call->first_argument) : NULL;
        const NLCheckedNodeView *second =
            first ? nl_checked_node_view(f, first->next_argument) : NULL;
        NLSemanticBindingView result, formal;
        const NLCheckedNodeView *change =
            nl_checked_node_view(v.body, event.node);
        const NLCheckedNodeView *dst =
            change ? nl_checked_node_view(v.body, change->first_argument)
                   : NULL;
        const NLCheckedNodeView *incoming =
            dst ? nl_checked_node_view(v.body, dst->next_argument) : NULL;
        NEED(t->helpers == 1 && t->helper_owner == f && second &&
             second->kind == NL_CHECKED_IDENTIFIER &&
             second->value_use == NL_VALUE_COPIED &&
             second->symbol == t->helper_binding &&
             nl_semantic_binding_view(p.entry, second->symbol, &result) &&
             result.value == t->helper_result &&
             p.arguments[1] == first->next_argument &&
             nl_semantic_binding_view(p.entry, p.parameters[1], &formal) &&
             formal.value == second->results[0].value &&
             scalar(p.entry, formal.value, t->sample) && incoming &&
             incoming->kind == NL_CHECKED_IDENTIFIER &&
             incoming->symbol == p.parameters[1] &&
             incoming->value_use == NL_VALUE_COPIED &&
             incoming->results[0].value == event.incoming &&
             scalar(event.before, event.incoming, t->sample));
        printf("HELPER_RESULT_CONSUMED same_original_T=%zu child=%zu "
               "actual_result=%zu\n",
               p.field.parent, p.field.child, t->helper_result);
    }
    if (!t->copies) {
        t->original_root = p.field.parent;
        t->original_child = p.field.child;
        t->original_incarnation = p.field.parent_incarnation;
    } else {
        /* Prefix identity was authenticated at each actual owned match edge.
         * These equal IDs alone would not authenticate a foreign world. */
        NEED(t->original_root == p.field.parent &&
             t->original_child == p.field.child &&
             t->original_incarnation == p.field.parent_incarnation &&
             t->copy_owners[0] != f);
    }
    t->copy_owners[t->copies++] = f;
    return 1;
}

static int walk(const NLCheckedFragment *f, Trace *t, Count *out, size_t depth)
{
    NEED(f && depth < 16);
    const NLSemanticContext *c = nl_checked_context(f);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        NEED(n);
        if (n->kind == NL_CHECKED_LOAN_HEADER) {
            NLSemanticScopeView scope;
            NEED(n->loan.body_nonescape_proved &&
                 nl_semantic_scope_view(c, n->loan.scope, &scope) &&
                 !scope.active);
            ++t->loans;
        }
        out->destroys += n->kind == NL_CHECKED_DESTROY;
        out->frees += n->kind == NL_CHECKED_DEALLOCATE;
        out->trials += n->allocation_trial;
        if (n->kind == NL_CHECKED_REGISTERED_CALL && n->argument_count == 1 &&
            n->type == nl_semantic_core_type(c, NL_TYPE_U8)) {
            NEED(helper(f, id, t));
            ++out->helpers;
        }
        if (n->owner_call.entry_proved) {
            NEED(t->helpers == 1 && t->helper_owner == f &&
                 t->helper_span.end_byte < n->span.start_byte);
            NEED(owner(f, id));
            ++out->calls;
            ++t->owners;
        }
        NLCopyBodyParameters parameters;
        if (nl_checked_copy_body_parameters(f, id, &parameters)) {
            NEED(copy(f, id, t));
            ++out->copies;
        }
        if (n->kind == NL_CHECKED_LINK_READ && n->field.index == 1) {
            const NLCheckedNodeView *field =
                nl_checked_node_view(f, n->first_argument);
            const NLCheckedNodeView *ref =
                field ? nl_checked_node_view(f, field->first_argument) : NULL;
            NLSemanticValueView rv;
            NEED(n->result_count == 1 &&
                 scalar(c, n->results[0].value, t->sample) && field && ref &&
                 ref->result_count == 1 &&
                 nl_semantic_value_view(c, ref->results[0].value, &rv));
            const NLCheckedNodeView *receiver = NULL, *acquisition = NULL;
            for (size_t j = 1; j <= nl_checked_node_count(f); ++j) {
                const NLCheckedNodeView *candidate = nl_checked_node_view(f, j);
                if (candidate->owner_call.entry_proved)
                    receiver = candidate;
                if (candidate->kind == NL_CHECKED_REF_FROM_PTR &&
                    candidate->lifetime_place == n->field.parent)
                    acquisition = candidate;
            }
            NEED(receiver && acquisition &&
                 receiver->owner_call.root == n->field.parent &&
                 acquisition->lifetime_domain == receiver->owner_call.domain &&
                 acquisition->lifetime_incarnation ==
                     receiver->owner_call.incarnation &&
                 acquisition->span.end_byte < receiver->span.start_byte);
            printf("PAYLOAD child=%zu original=%zu sample=%zu ref_scope=%zu\n",
                   n->field.child, n->field.parent, t->sample,
                   rv.reference.scope);
            ++out->payloads;
            ++t->reads;
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            NEED(walk(body, t, out, depth + 1));
        if (n->kind == NL_CHECKED_MATCH) {
            const NLCheckedNodeView *trial =
                nl_checked_node_view(f, n->initializer);
            NEED(trial && trial->kind == NL_CHECKED_TRY_ALLOCATE_ONE &&
                 n->item_count == 2);
            ++t->matches;
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *arm = nl_checked_match_arm(f, id, a);
                const NLSemanticContext *ac = nl_checked_context(arm);
                NLSemanticBindingView parent_binding, child_binding;
                NEED(arm && ac != c &&
                     n->match_binding_prefix >= t->original_binding &&
                     nl_semantic_binding_view(c, t->original_binding,
                                              &parent_binding) &&
                     nl_semantic_binding_view(ac, t->original_binding,
                                              &child_binding) &&
                     parent_binding.place == child_binding.place);
                Count branch = {0};
                NEED(walk(arm, t, &branch, depth + 1));
                if (!a) {
                    size_t resources = n->captured_frame_closed ? 1 : 0;
                    NEED(closed(ac, resources) && !branch.calls &&
                         !branch.copies && !branch.payloads &&
                         !branch.helpers && branch.destroys == resources &&
                         branch.frees == resources);
                    printf("NORMAL H%zu receiver=0 second_T_write=0 "
                           "releases=%zu\n",
                           resources, branch.frees);
                } else if (n->captured_frame_closed) {
                    NEED(closed(ac, 2) && branch.calls == 1 &&
                         branch.copies == 1 && branch.payloads == 1 &&
                         branch.helpers == 1 && branch.destroys == 2 &&
                         branch.frees == 2);
                    printf("NORMAL H2 receiver=1 second_T_write=1 sample=%zu "
                           "releases=2\n",
                           t->sample);
                }
                out->helpers += branch.helpers;
                out->calls += branch.calls;
                out->copies += branch.copies;
                out->payloads += branch.payloads;
                out->destroys += branch.destroys;
                out->frees += branch.frees;
                out->trials += branch.trials;
            }
        }
    }
    return 1;
}

static int run(const char *path, size_t sample, size_t initial, size_t sibling)
{
    NLSource *source = NULL, *call_source = NULL;
    NLParser *parser = NULL, *call_parser = NULL;
    NLSyntaxTree *unit = NULL, *call = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *artifact = NULL;
    NLParseDiagnostic pd = {0};
    NLFunctionUnitDiagnostic ud = {0};
    NLCheckDiagnostic cd = {0};
    NEED(nl_source_load(path, &source) == NL_SOURCE_OK &&
         nl_parser_create(source, &parser) == NL_PARSE_OK);
    NLParseStatus ps = nl_parser_parse_function_unit(parser, &unit, &pd);
    printf("PARSE status=%d code=%s\n", (int)ps,
           pd.diagnostic.code ? pd.diagnostic.code : "none");
    NEED(ps == NL_PARSE_OK && nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    NLCheckStatus ds = nl_semantic_register_function_unit(c, inputs, 1, &ud);
    printf("DEFINITION status=%d code=%s\n", (int)ds,
           ud.diagnostic.diagnostic.code ? ud.diagnostic.diagnostic.code
                                         : "none");
    NEED(ds == NL_CHECK_OK);
    NLSemanticSnapshot s;
    NEED(nl_semantic_snapshot(c, &s) && !s.bindings && !s.values && !s.places &&
         !s.domains && !s.backing_regions);
    size_t definitions = 0;
    for (size_t i = 1; i <= s.functions; ++i) {
        NLTypedOwnerDefinition d;
        if (nl_semantic_function_applicability(c, i, &d)) {
            NEED(d.definition_checked && !d.live_return &&
                 !d.head_link_required &&
                 d.requirements == NL_OWNER_ALL_REQUIREMENTS &&
                 d.step_count == 5);
            ++definitions;
        }
    }
    NEED(definitions == 1);
    /* Destroy the source AST before using the owned registered definition. */
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    NEED(nl_source_create("main()", 6, "P1515-actual-main", &call_source) ==
             NL_SOURCE_OK &&
         nl_parser_create(call_source, &call_parser) == NL_PARSE_OK &&
         nl_parser_parse_expression_fragment(call_parser, &call, NULL) ==
             NL_PARSE_OK);
    NLCheckStatus actual =
        nl_semantic_check_expression(c, call, &artifact, &cd);
    printf("ACTUAL status=%d code=%s\n", (int)actual,
           cd.diagnostic.code ? cd.diagnostic.code : "none");
    NEED(actual == NL_CHECK_OK && artifact);
    NLCheckedProofClass proof = nl_checked_proof_class(artifact);
    printf("PROOF_CLASS value=%d MACHINE=%d\n", (int)proof,
           (int)NL_CHECKED_MACHINE_PROVED);
    NEED(proof == NL_CHECKED_MACHINE_PROVED);
    const NLCheckedFragment *main_body =
        nl_checked_call_body(artifact, nl_checked_root(artifact));
    NEED(main_body);
    Trace t = {.sample = sample, .initial = initial, .sibling = sibling};
    Count count = {0};
    NEED(walk(main_body, &t, &count, 0));
    NEED(t.copies == 2 && t.owners == 1 && t.reads == 1 && t.matches == 2 &&
         count.calls == 1 && count.copies == 2 && count.helpers == 1 &&
         t.helpers == 1 && count.destroys == 3 && count.frees == 3);
    printf("AUTHENTICATED copies=2 owner=1 read=1 matches=2 closed_loans=%zu "
           "root=%zu child=%zu incarnation=%zu sample=%zu\n",
           t.loans, t.original_root, t.original_child, t.original_incarnation,
           sample);
    nl_checked_destroy(artifact);
    nl_syntax_tree_destroy(call);
    nl_parser_destroy(call_parser);
    nl_source_destroy(call_source);
    nl_semantic_destroy(c);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 5)
        return 2;
    setvbuf(stdout, NULL, _IONBF, 0);
    return run(argv[1], (size_t)strtoul(argv[2], NULL, 10),
               argc == 5 ? (size_t)strtoul(argv[3], NULL, 10) : 91,
               argc == 5 ? (size_t)strtoul(argv[4], NULL, 10) : 23)
               ? 0
               : 1;
}
