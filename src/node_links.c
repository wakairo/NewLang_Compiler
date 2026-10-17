#include "newlang/node_links.h"
#include "semantic_internal.h"
#include <string.h>

#define INVALID NL_CHECK_ANALYSIS_PRECISION_LIMIT

static bool same_name(const char *a, const char *b)
{
    return a && b ? strcmp(a, b) == 0 : a == b;
}

/* A captured type ID must retain its actual registered meaning. Core memory
 * replay alone cannot authenticate a type registry altered in BOTH snapshots.
 * This finite body creates no types after its first captured write. */
static bool same_types(const NLSemanticContext *issued,
                       const NLSemanticContext *snapshot)
{
    if (!snapshot || issued->type_count != snapshot->type_count ||
        issued->type_count > NL_SEMANTIC_MAX_ENTRIES)
        return false;
    for (size_t i = 0; i < issued->type_count; ++i) {
        const NLTypeEntry *a = &issued->types[i], *b = &snapshot->types[i];
        const NLSemanticTypeView x = a->view, y = b->view;
        if (x.kind != y.kind || x.target != y.target || x.access != y.access ||
            x.is_copy != y.is_copy || x.is_discardable != y.is_discardable ||
            x.is_exclusive != y.is_exclusive ||
            x.layout_known != y.layout_known ||
            x.variant_count != y.variant_count ||
            x.field_count != y.field_count ||
            x.element_count != y.element_count || x.size != y.size ||
            x.alignment != y.alignment ||
            x.field_count > NL_SEMANTIC_MAX_FIELDS ||
            x.variant_count > NL_SEMANTIC_MAX_VARIANTS ||
            a->incomplete != b->incomplete ||
            a->recursive_header != b->recursive_header ||
            a->option_target != b->option_target ||
            a->allocated_target != b->allocated_target ||
            a->one_backing_target != b->one_backing_target ||
            a->live_tail_target != b->live_tail_target ||
            !same_name(a->name, b->name) ||
            memcmp(a->field_types, b->field_types, sizeof(a->field_types)) ||
            memcmp(a->variant_types, b->variant_types,
                   sizeof(a->variant_types)))
            return false;
        for (size_t j = 0; j < x.field_count; ++j)
            if (!same_name(a->field_names[j], b->field_names[j]))
                return false;
        for (size_t j = 0; j < x.variant_count; ++j)
            if (!same_name(a->variant_names[j], b->variant_names[j]))
                return false;
    }
    return true;
}

static bool originals(const NLSemanticContext *c, NLPlaceId roots[3])
{
    size_t n = 0;
    if (!c || c->region_count || c->domain_count ||
        nl_sem_validate(c) != NL_CHECK_OK)
        return false;
    for (size_t i = 0; i < c->place_count; ++i) {
        const NLSemanticPlaceView p = c->places[i];
        if (!p.live || !p.independent_root ||
            !nl_recursive_local_type(c, p.type))
            continue;
        if (n == 3 || !p.implicit_local || p.governing_domain ||
            p.placement.region || p.fixed_field_count != 2)
            return false;
        roots[n++] = i + 1;
    }
    return n == 3;
}

bool nl_checked_node_links_view(const NLCheckedFragment *f,
                                NLNodeLinksView *out)
{
    NLNodeLinksView v = {.origin = f};
    if (!f || !out || f->field_change_count != 2 || !f->field_changes[0] ||
        !f->field_changes[1] ||
        !originals(f->field_changes[0]->before, v.originals))
        return false;
    for (size_t i = 0; i < 2; ++i)
        if (!nl_checked_captured_change_view(f, i, &v.changes[i]))
            return false;
    *out = v;
    return true;
}

static bool same_place(NLSemanticPlaceView a, NLSemanticPlaceView b)
{
    if (a.type != b.type || a.live != b.live ||
        a.independent_root != b.independent_root ||
        a.implicit_local != b.implicit_local ||
        a.incarnation != b.incarnation ||
        a.governing_domain != b.governing_domain ||
        a.current_fact != b.current_fact ||
        a.current_value != b.current_value ||
        a.payload_occurrence != b.payload_occurrence ||
        a.parent_aggregate != b.parent_aggregate ||
        a.parent_incarnation != b.parent_incarnation ||
        a.parent_field_index != b.parent_field_index ||
        a.parent_sum != b.parent_sum ||
        a.placement.region != b.placement.region ||
        a.placement.start != b.placement.start ||
        a.placement.length != b.placement.length ||
        a.fixed_field_count != b.fixed_field_count)
        return false;
    for (size_t i = 0; i < a.fixed_field_count; ++i)
        if (a.fixed_fields[i] != b.fixed_fields[i])
            return false;
    return true;
}

static bool same_reference(NLReferenceFacts a, NLReferenceFacts b)
{
    return a.place == b.place && a.incarnation == b.incarnation &&
           a.scope == b.scope && a.provenance == b.provenance &&
           a.readable == b.readable && a.writable == b.writable &&
           a.occurrence_dependency == b.occurrence_dependency;
}

/* Historical Copy operand content must agree with the source artifact's
 * actual issued package, not merely a same-typed ID in a modified snapshot.
 * Carrier bookkeeping may legitimately have ended in the final context. */
static bool issued_option(const NLCheckedFragment *f,
                          const NLSemanticContext *before, NLValueId id)
{
    if (!id || id > before->value_count || id > f->context->value_count)
        return false;
    const NLSemanticValueView a = before->values[id - 1],
                              b = f->context->values[id - 1];
    if (a.type != b.type || a.variant != b.variant ||
        a.sum_payload != b.sum_payload || a.reference_count ||
        a.dependencies != NL_DEPENDENCY_FREE || a.value_dependency_count)
        return false;
    if (!a.sum_payload)
        return a.variant == 1;
    if (a.variant != 2 || a.sum_payload > before->value_count ||
        a.sum_payload > f->context->value_count)
        return false;
    const NLSemanticValueView x = before->values[a.sum_payload - 1],
                              y = f->context->values[a.sum_payload - 1];
    return x.type == y.type && !x.reference_count &&
           x.dependencies == NL_DEPENDENCY_FREE && !x.value_dependency_count &&
           !x.domain && !x.allocation_region && !x.slot_place &&
           !x.reference.scope && !x.reference.occurrence_dependency &&
           x.reference.provenance == NL_PROVENANCE_VALID &&
           x.reference.readable && same_reference(x.reference, y.reference);
}

static bool registered(const NLCheckedFragment *f)
{
    if (!f->body_owner || f->source != f->body_owner->source || !f->context)
        return false;
    for (size_t i = 0; i < f->context->function_count; ++i) {
        const NLFunctionEntry fn = f->context->functions[i];
        if (fn.body == f->body_owner && fn.kind == NL_CHECKED_REGISTERED_CALL &&
            !fn.count && fn.result == 1 && !fn.caller_effects &&
            !fn.hidden_dependencies)
            return true;
    }
    return false;
}

/* Compare actual current package contents as well as place/ValueFact IDs.
 * Equal Copy pointer bits or IDs alone do not establish a bridge. The admitted
 * two-field Node -> Option -> ptr/u8 packages have depth at most two. */
static bool same_package(const NLSemanticContext *a, const NLSemanticContext *b,
                         NLValueId id, size_t depth)
{
    if (!id || id > a->value_count || id > b->value_count || depth > 2)
        return false;
    const NLSemanticValueView v = a->values[id - 1];
    if (!nl_node_value_same(v, b->values[id - 1]) || v.field_count > 2)
        return false;
    for (size_t i = 0; i < v.field_count; ++i)
        if (!same_package(a, b, v.fields[i], depth + 1))
            return false;
    return !v.sum_payload || same_package(a, b, v.sum_payload, depth + 1);
}

static NLCheckStatus replay(const NLCheckedFragment *f, size_t index,
                            const NLPlaceId roots[3])
{
    const NLCapturedChange *change = f->field_changes[index];
    if (!change || change->world != f->context || !change->before ||
        !change->after || change->before != change->before_origin ||
        change->after != change->after_origin ||
        change->before == change->after)
        return INVALID;
    const NLSemanticContext *b = change->before, *a = change->after;
    if (!same_types(f->context, b) || !same_types(f->context, a))
        return INVALID;
    NLPlaceId br[3], ar[3];
    if (!originals(b, br) || !originals(a, ar))
        return INVALID;
    for (size_t i = 0; i < 3; ++i)
        if (roots[i] != br[i] || br[i] != ar[i])
            return INVALID;
    const NLCheckedNodeView *op = nl_checked_node_view(f, change->node);
    if (!op || op->kind != NL_CHECKED_REPLACE || op->argument_count != 2 ||
        op->result_count != 1 || !op->field.present ||
        !op->field.dependency_compatible || op->field.index != 0 ||
        op->field.access != NL_ACCESS_WRITE)
        return INVALID;
    const NLCheckedField p = op->field;
    if (!p.parent || p.parent > b->place_count || !p.child ||
        p.child > b->place_count || !p.new_value ||
        p.new_value > b->value_count || !p.base || p.base > b->binding_count)
        return INVALID;
    const NLSemanticPlaceView parent = b->places[p.parent - 1],
                              child = b->places[p.child - 1];
    if (p.parent != roots[index + 1] ||
        b->bindings[p.base - 1].view.place != p.parent ||
        parent.type != p.nominal ||
        parent.incarnation != p.parent_incarnation ||
        parent.fixed_fields[0] != p.child ||
        parent.current_fact != p.parent_fact ||
        child.parent_aggregate != p.parent || child.parent_field_index != 0 ||
        child.parent_incarnation != p.parent_incarnation ||
        child.incarnation != p.child_incarnation || child.type != p.type ||
        child.current_fact != p.child_fact ||
        child.current_value != p.old_value ||
        child.payload_occurrence != p.payload_occurrence ||
        op->results[0].value != p.old_value || op->results[0].type != p.type)
        return INVALID;
    if (!issued_option(f, b, p.old_value) || !issued_option(f, b, p.new_value))
        return INVALID;
    const NLCheckedNodeView *arg = nl_checked_node_view(f, op->first_argument);
    const NLCheckedNodeView *incoming =
        arg ? nl_checked_node_view(f, arg->next_argument) : NULL;
    if (!arg || !incoming || arg->kind != NL_CHECKED_IDENTIFIER ||
        arg->result_count != 1 || !arg->results[0].value ||
        arg->results[0].value > b->value_count || incoming->result_count != 1 ||
        incoming->results[0].value != p.new_value)
        return INVALID;
    const NLSemanticValueView ref = b->values[arg->results[0].value - 1];
    if (!ref.type || ref.type > b->type_count ||
        b->types[ref.type - 1].view.kind != NL_TYPE_REF ||
        b->types[ref.type - 1].view.access != NL_ACCESS_WRITE ||
        b->types[ref.type - 1].view.is_exclusive ||
        b->types[ref.type - 1].view.target != p.type || arg->type != ref.type ||
        arg->results[0].type != ref.type || ref.reference.place != p.child ||
        ref.reference.incarnation != p.child_incarnation ||
        ref.reference.provenance != NL_PROVENANCE_VALID ||
        !ref.reference.readable || !ref.reference.writable ||
        ref.reference_count || ref.dependencies != NL_DEPENDENCY_FREE ||
        ref.value_dependency_count || !ref.reference.scope ||
        ref.reference.scope > b->scope_count ||
        !b->scopes[ref.reference.scope - 1].active)
        return INVALID;
    size_t loans = 0;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *loan = nl_checked_node_view(f, id);
        if (loan->kind != NL_CHECKED_LOAN_HEADER ||
            loan->loan.ref_symbol != arg->symbol)
            continue;
        if (loan->loan.access != NL_ACCESS_WRITE ||
            !loan->loan.implicit_local || !loan->loan.body_nonescape_proved ||
            !loan->loan.normal_result_forwarded ||
            !loan->loan.prevent_lifetime_end || loan->loan.from_ptr ||
            loan->loan.source != p.base || loan->loan.place != p.child ||
            loan->loan.incarnation != p.child_incarnation ||
            loan->loan.scope != ref.reference.scope ||
            loan->field.parent_fact != p.parent_fact ||
            loan->field.child_fact != p.child_fact ||
            loan->span.start_byte > op->span.start_byte ||
            loan->span.end_byte < op->span.end_byte)
            return INVALID;
        ++loans;
    }
    if (loans != 1)
        return INVALID;
    NLSemanticContext *expected = NULL;
    NLCheckStatus status = nl_sem_clone(b, &expected);
    if (status != NL_CHECK_OK)
        return status;
    status = nl_fixed_change(expected, p.child, p.new_value, false);
    if (status == NL_CHECK_OK) {
        for (size_t i = 0; i < expected->binding_count; ++i)
            if (expected->bindings[i].view.place == p.parent &&
                expected->bindings[i].view.availability == NL_AVAILABLE)
                expected->bindings[i].view.value =
                    expected->places[p.parent - 1].current_value;
        if (!nl_packet_same_entry(expected, a) ||
            expected->places[p.parent - 1].current_fact != p.parent_post_fact ||
            expected->places[p.child - 1].current_fact != p.child_post_fact ||
            expected->places[p.child - 1].payload_occurrence !=
                p.post_payload_occurrence)
            status = INVALID;
    }
    nl_semantic_destroy(expected);
    return status == NL_CHECK_INTERNAL_ERROR ? INVALID : status;
}

NLCheckStatus nl_checked_node_links_validate(const NLCheckedFragment *f,
                                             const NLNodeLinksView *candidate)
{
    NLNodeLinksView v;
    if (!nl_checked_node_links_view(f, &v) || !registered(f))
        return INVALID;
    const NLCheckedNodeView *body = nl_checked_node_view(f, nl_checked_root(f));
    if (!body || body->kind != NL_CHECKED_BLOCK || body->type != 1 ||
        body->terminates || body->result_count)
        return INVALID;
    if (candidate) {
        if (candidate->origin != f)
            return INVALID;
        for (size_t i = 0; i < 3; ++i)
            if (candidate->originals[i] != v.originals[i])
                return INVALID;
        for (size_t i = 0; i < 2; ++i)
            if (candidate->changes[i].operation != v.changes[i].operation ||
                candidate->changes[i].world != v.changes[i].world ||
                candidate->changes[i].before != v.changes[i].before ||
                candidate->changes[i].after != v.changes[i].after)
                return INVALID;
    }
    size_t changes = 0, bindings = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_MATCH || n->kind == NL_CHECKED_IF ||
            n->kind == NL_CHECKED_LOOP || n->kind == NL_CHECKED_RETURN ||
            n->kind == NL_CHECKED_STORE ||
            n->kind == NL_CHECKED_REGISTERED_CALL ||
            n->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
            return INVALID;
        if (n->kind == NL_CHECKED_REPLACE) {
            if (changes == 2 || i != v.changes[changes++].operation)
                return INVALID;
        }
        if (n->kind == NL_CHECKED_BINDING &&
            nl_recursive_local_type(f->context, n->type))
            return INVALID; /* binding nodes themselves must have unit type */
        if (n->kind == NL_CHECKED_RECEIVER &&
            nl_recursive_local_type(f->context, n->type)) {
            NLSemanticBindingView b;
            if (bindings == 3 ||
                !nl_semantic_binding_view(f->context, n->symbol, &b) ||
                b.place != v.originals[bindings++])
                return INVALID;
        }
    }
    if (changes != 2 || bindings != 3)
        return INVALID;
    for (size_t i = 0; i < 2; ++i) {
        NLCheckStatus s = replay(f, i, v.originals);
        if (s != NL_CHECK_OK)
            return s;
    }
    /* The intervening source creates/ends lexical ref/Copy bindings. It may
     * not change any original object between the two issued operations. */
    const NLSemanticContext *a = v.changes[0].after, *b = v.changes[1].before;
    for (size_t i = 0; i < 3; ++i) {
        const NLPlaceId root = v.originals[i];
        if (!same_place(a->places[root - 1], b->places[root - 1]))
            return INVALID;
        if (!same_package(a, b, a->places[root - 1].current_value, 0))
            return INVALID;
        for (size_t j = 0; j < 2; ++j) {
            const NLPlaceId child = a->places[root - 1].fixed_fields[j];
            if (!same_place(a->places[child - 1], b->places[child - 1]))
                return INVALID;
            const NLOccurrenceId occurrence =
                a->places[child - 1].payload_occurrence;
            if (occurrence) {
                if (occurrence > a->occurrence_count ||
                    occurrence > b->occurrence_count)
                    return INVALID;
                const NLSemanticOccurrenceView
                    x = a->occurrences[occurrence - 1],
                    y = b->occurrences[occurrence - 1];
                if (!x.live || !y.live || x.root != y.root ||
                    x.variant != y.variant ||
                    x.payload_place != y.payload_place ||
                    !same_place(a->places[x.payload_place - 1],
                                b->places[y.payload_place - 1]))
                    return INVALID;
            }
        }
    }
    return NL_CHECK_OK;
}
