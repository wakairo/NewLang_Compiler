#include "semantic_internal.h"

#include <stdlib.h>
#include <string.h>

#define INVALID NL_CHECK_ANALYSIS_PRECISION_LIMIT

static void changes_destroy(NLCheckedFragment *f)
{
    for (size_t i = 0; i < f->field_change_count; ++i) {
        NLCapturedChange *c = f->field_changes[i];
        if (c != NULL) {
            nl_semantic_destroy((NLSemanticContext *)c->before_origin);
            nl_semantic_destroy((NLSemanticContext *)c->after_origin);
            free(c);
        }
    }
}
NLCheckStatus nl_captured_change_begin(NLCheckedFragment *f,
                                       NLCheckedNodeId node,
                                       const NLSemanticContext *world)
{
    if (f->field_change_count >= 6)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLCapturedChange *c = calloc(1, sizeof(*c));
    if (c == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    f->destroy_field_changes = changes_destroy;
    f->field_changes[f->field_change_count++] = c;
    c->node = node;
    c->world = f->context;
    NLCheckStatus s = nl_sem_clone(world, &c->before);
    c->before_origin = c->before;
    return s;
}
NLCheckStatus nl_captured_change_end(NLCheckedFragment *f,
                                     const NLSemanticContext *world)
{
    NLCapturedChange *c = f->field_changes[f->field_change_count - 1];
    NLCheckStatus s = nl_sem_clone(world, &c->after);
    c->after_origin = c->after;
    return s;
}
size_t nl_checked_captured_change_count(const NLCheckedFragment *f)
{
    return f == NULL ? 0 : f->field_change_count;
}
bool nl_checked_captured_change_view(const NLCheckedFragment *f, size_t index,
                                     NLCapturedChangeView *out)
{
    if (f == NULL || out == NULL || index >= f->field_change_count ||
        index >= 6 || f->field_changes[index] == NULL)
        return false;
    const NLCapturedChange *c = f->field_changes[index];
    *out = (NLCapturedChangeView){c->node, c->world, c->before, c->after};
    return true;
}

static bool extent_same(NLBackingRange a, NLBackingRange b)
{
    return a.region == b.region && a.start == b.start && a.length == b.length;
}
static bool original_same(NLCapturedOriginal a, NLCapturedOriginal b)
{
    return a.origin == b.origin && extent_same(a.extent, b.extent) &&
           a.root == b.root && a.incarnation == b.incarnation &&
           a.domain == b.domain &&
           a.allocation_binding == b.allocation_binding &&
           a.domain_binding == b.domain_binding &&
           a.allocation_value == b.allocation_value &&
           a.domain_value == b.domain_value &&
           a.allocation_availability == b.allocation_availability &&
           a.domain_availability == b.domain_availability;
}

/* The deterministic order is the ancestor's original region order. Never
 * sort/identify owners using an address, static type, or another world's ID.
 * Only current complete typed occupancy + original available A/D is eligible.
 */
static NLCheckStatus collect(const NLSemanticContext *c,
                             NLCapturedClosureView *v)
{
    if (c == NULL || c->region_count > NL_CAPTURED_MAX_ORIGINALS ||
        c->domain_count != c->region_count || nl_sem_validate(c) != NL_CHECK_OK)
        return INVALID;
    NLCheckStatus s = nl_raw_validate(c);
    if (s != NL_CHECK_OK)
        return s == NL_CHECK_OUT_OF_MEMORY ? s : INVALID;
    for (size_t i = 0; i < c->scope_count; ++i)
        if (c->scopes[i].active)
            return INVALID;
    for (size_t i = 0; i < c->value_count; ++i)
        if (c->values[i].dependencies != NL_DEPENDENCY_FREE ||
            c->values[i].value_dependency_count != 0)
            return INVALID;
    *v = (NLCapturedClosureView){.ancestor = c, .count = c->region_count};
    for (size_t r = 0; r < v->count; ++r) {
        NLCapturedOriginal *o = &v->originals[r];
        if (!c->regions[r].view.live)
            return INVALID;
        o->origin = c;
        o->extent = (NLBackingRange){r + 1, 0, c->regions[r].view.size};
        for (size_t p = 0; p < c->place_count; ++p) {
            const NLSemanticPlaceView root = c->places[p];
            if (!root.live || root.placement.region != r + 1)
                continue;
            if (o->root != 0 || !root.independent_root ||
                !nl_recursive_local_type(c, root.type) ||
                !extent_same(root.placement, o->extent) ||
                root.governing_domain == 0 ||
                root.governing_domain > c->domain_count ||
                !c->domains[root.governing_domain - 1].live)
                return INVALID;
            const NLSemanticTypeView type = c->types[root.type - 1].view;
            if (!type.layout_known || type.size != o->extent.length ||
                type.alignment == 0 ||
                c->regions[r].view.alignment % type.alignment != 0)
                return INVALID;
            o->root = p + 1;
            o->incarnation = root.incarnation;
            o->domain = root.governing_domain;
        }
        if (o->root == 0 ||
            (r != 0 && c->places[o->root - 1].type !=
                           c->places[v->originals[0].root - 1].type))
            return INVALID;
        for (size_t j = 0; j < r; ++j)
            if (v->originals[j].domain == o->domain)
                return INVALID;
        for (size_t b = 0; b < c->binding_count; ++b) {
            const NLSemanticBindingView binding = c->bindings[b].view;
            if (binding.availability != NL_AVAILABLE)
                continue;
            const NLSemanticValueView value = c->values[binding.value - 1];
            const NLSemanticTypeKind kind =
                c->types[binding.type - 1].view.kind;
            if (kind == NL_TYPE_ALLOCATION &&
                value.allocation_region == r + 1) {
                if (o->allocation_binding != 0 ||
                    value.carrier != NL_CARRIER_PLACE ||
                    value.owner_place != binding.place)
                    return INVALID;
                o->allocation_binding = b + 1;
                o->allocation_value = binding.value;
                o->allocation_availability = binding.availability;
            } else if (binding.type == nl_semantic_domain_type(c) &&
                       value.domain == o->domain) {
                if (o->domain_binding != 0 ||
                    value.carrier != NL_CARRIER_PLACE ||
                    value.owner_place != binding.place)
                    return INVALID;
                o->domain_binding = b + 1;
                o->domain_value = binding.value;
                o->domain_availability = binding.availability;
            }
        }
        if (o->allocation_binding == 0 || o->domain_binding == 0 ||
            c->domains[o->domain - 1].value != o->domain_value)
            return INVALID;
    }
    for (size_t b = 0; b < c->binding_count; ++b) {
        const NLSemanticBindingView binding = c->bindings[b].view;
        if (binding.availability != NL_AVAILABLE ||
            c->types[binding.type - 1].view.is_copy)
            continue;
        bool owner = false;
        for (size_t r = 0; r < v->count; ++r)
            owner |= b + 1 == v->originals[r].allocation_binding ||
                     b + 1 == v->originals[r].domain_binding;
        if (!owner)
            return INVALID;
    }
    return NL_CHECK_OK;
}

/* A proof target, not executable cleanup/authority. Derived only from the
 * ancestor, before either arm is supplied. Actual source operations must
 * prove the entire target and their exact explicit release trace separately.
 */
static NLCheckStatus derive(const NLSemanticContext *ancestor,
                            const NLCapturedClosureView *v,
                            NLSemanticContext **out)
{
    NLSemanticContext *post = NULL;
    NLCheckStatus s = nl_sem_clone(ancestor, &post);
    if (s != NL_CHECK_OK)
        return s;
    for (size_t r = 0; r < v->count; ++r) {
        const NLCapturedOriginal o = v->originals[r];
        const NLSemanticPlaceView root = post->places[o.root - 1];
        nl_sum_detach(post, o.root);
        nl_fixed_detach(post, o.root);
        nl_sem_end_value(post, root.current_value);
        NLSemanticPlaceView *p = &post->places[o.root - 1];
        p->live = false;
        p->current_value = p->current_fact = 0;
        p->governing_domain = 0;
        p->placement = (NLBackingRange){0};
        const NLSymbolId owners[] = {o.allocation_binding, o.domain_binding};
        for (size_t j = 0; j < 2; ++j) {
            NLSemanticBindingView *b = &post->bindings[owners[j] - 1].view;
            b->availability = NL_CONSUMED;
            NLSemanticPlaceView *local = &post->places[b->place - 1];
            local->live = false;
            local->current_value = local->current_fact = 0;
            local->governing_domain = 0;
            nl_sem_end_value(post, b->value);
        }
        post->domains[o.domain - 1].live = false;
        NLRawRegionEntry *region = &post->regions[o.extent.region - 1];
        region->view.live = false;
        post->raw_interval_count -= region->count;
        free(region->intervals);
        region->intervals = NULL;
        region->count = 0;
    }
    s = nl_sem_validate(post);
    if (s == NL_CHECK_OK)
        s = nl_raw_validate(post);
    if (s != NL_CHECK_OK) {
        nl_semantic_destroy(post);
        return s == NL_CHECK_OUT_OF_MEMORY ? s : INVALID;
    }
    *out = post;
    return NL_CHECK_OK;
}

void nl_captured_closure_destroy(NLCapturedClosure *c)
{
    if (c == NULL)
        return;
    nl_semantic_destroy(c->ancestor_owned);
    nl_semantic_destroy(c->closed_owned);
    for (size_t a = 0; a < 2; ++a)
        nl_semantic_destroy((NLSemanticContext *)c->branches[a].entry_origin);
    free(c);
}
NLCheckStatus nl_captured_closure_create(const NLSemanticContext *ancestor,
                                         NLCapturedClosure **out)
{
    if (out == NULL || *out != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    NLCapturedClosureView validated = {0};
    NLCheckStatus s = collect(ancestor, &validated);
    if (s != NL_CHECK_OK)
        return s;
    NLCapturedClosure *c = calloc(1, sizeof(*c));
    if (c == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    c->parent_world = ancestor;
    NLSemanticContext *owned = NULL, *post = NULL;
    s = nl_sem_clone(ancestor, &owned);
    if (s == NL_CHECK_OK) {
        c->view = validated;
        c->view.ancestor = owned;
        c->ancestor_owned = owned;
        for (size_t r = 0; r < c->view.count; ++r)
            c->view.originals[r].origin = owned;
        s = derive(owned, &c->view, &post);
    }
    if (s == NL_CHECK_OK) {
        c->view.closed_post = post;
        c->closed_owned = post;
    } else {
        nl_semantic_destroy(owned);
        free(c);
        return s;
    }
    *out = c;
    return NL_CHECK_OK;
}

bool nl_checked_captured_closure_view(const NLCheckedFragment *f,
                                      NLCheckedNodeId match,
                                      NLCapturedClosureView *out)
{
    if (f == NULL || out == NULL || f->captured_match != match ||
        f->captured_closure == NULL)
        return false;
    *out = f->captured_closure->view;
    return true;
}

/* The isolated lifecycle trace owns no values. It verifies real checked
 * operation operands and conserves original packages through the explicit
 * EndRoot -> slot -> full Storage -> deallocate route. Parent-prefix IDs are
 * rebased only through the stored owned fork relation, never by coincidence.
 */
typedef struct {
    NLCapturedOriginal original;
    bool initialized, ended, erased, finalized, freed;
    NLValueId initial_raw, vacant, slot, storage;
    NLValueId fields[3]; /* current Copy packages, not ownership edges */
} Release;
static const NLCheckedNodeView *argument_node(const NLCheckedFragment *f,
                                              const NLCheckedNodeView *v,
                                              size_t index)
{
    NLCheckedNodeId id = v->first_argument;
    for (size_t i = 0; id != 0 && i < index; ++i) {
        const NLCheckedNodeView *a = nl_checked_node_view(f, id);
        if (a == NULL)
            return NULL;
        id = a->next_argument;
    }
    return nl_checked_node_view(f, id);
}
static const NLSemanticValueView *
input(const NLCheckedFragment *f, const NLCheckedNodeView *v, size_t index)
{
    const NLCheckedNodeView *a = argument_node(f, v, index);
    const NLSemanticContext *c = f->context;
    if (a == NULL || a->result_count != 1 || a->results[0].value == 0 ||
        a->results[0].value > c->value_count)
        return NULL;
    return &c->values[a->results[0].value - 1];
}
static NLValueId input_id(const NLCheckedFragment *f,
                          const NLCheckedNodeView *v, size_t index)
{
    const NLSemanticValueView *a = input(f, v, index);
    return a == NULL ? 0 : (size_t)(a - f->context->values) + 1;
}
static bool result_id(const NLCheckedFragment *f, const NLCheckedNodeView *v,
                      NLValueId *out)
{
    if (v->result_count != 1 || v->results[0].value == 0 ||
        v->results[0].value > f->context->value_count)
        return false;
    *out = v->results[0].value;
    return true;
}
static bool pointer_to(const NLSemanticContext *world,
                       NLSemanticValueView pointer, NLPlaceId root,
                       NLIncarnationId incarnation)
{
    return pointer.type != 0 && pointer.type <= world->type_count &&
           world->types[pointer.type - 1].view.kind == NL_TYPE_PTR &&
           world->types[pointer.type - 1].view.target ==
               world->places[root - 1].type &&
           pointer.reference_count == 0 && pointer.reference.scope == 0 &&
           pointer.reference.provenance == NL_PROVENANCE_VALID &&
           pointer.reference.place == root &&
           pointer.reference.incarnation == incarnation;
}
static bool domain_ref(const NLSemanticContext *world,
                       NLSemanticValueView reference, NLDomainId domain,
                       bool exclusive)
{
    if (reference.type == 0 || reference.type > world->type_count ||
        reference.reference_count != 0 || reference.reference.place == 0 ||
        reference.reference.place > world->place_count ||
        reference.reference.scope == 0 ||
        reference.reference.scope > world->scope_count ||
        reference.reference.provenance != NL_PROVENANCE_VALID ||
        !reference.reference.readable || domain == 0 ||
        domain > world->domain_count)
        return false;
    const NLSemanticTypeView t = world->types[reference.type - 1].view;
    const NLSemanticPlaceView p = world->places[reference.reference.place - 1];
    if (t.kind != NL_TYPE_REF || t.target != nl_semantic_domain_type(world) ||
        t.is_exclusive != exclusive || t.access != NL_ACCESS_READ ||
        p.type != nl_semantic_domain_type(world) ||
        p.incarnation != reference.reference.incarnation)
        return false;
    for (size_t b = 0; b < world->binding_count; ++b) {
        const NLSemanticBindingView binding = world->bindings[b].view;
        if (binding.place == reference.reference.place &&
            binding.value == world->domains[domain - 1].value &&
            world->values[binding.value - 1].domain == domain)
            return true;
    }
    return false;
}

static bool same_ref(NLReferenceFacts a, NLReferenceFacts b)
{
    return a.place == b.place && a.incarnation == b.incarnation &&
           a.scope == b.scope && a.provenance == b.provenance &&
           a.readable == b.readable && a.writable == b.writable &&
           a.occurrence_dependency == b.occurrence_dependency;
}

/* Recheck the actual root acquisition separately from field projection.
 * The saved world is prior to Change, while its D loan and both refs are live.
 * No C offsets or source spelling take part in this proof. */
static NLCheckStatus change_validate(const NLCheckedFragment *f,
                                     const NLCapturedChange *c)
{
    if (c == NULL || c->world != f->context || c->before == NULL ||
        c->after == NULL || c->before == c->after ||
        c->before != c->before_origin || c->after != c->after_origin ||
        nl_sem_validate(c->before) != NL_CHECK_OK ||
        nl_sem_validate(c->after) != NL_CHECK_OK)
        return INVALID;
    NLCheckStatus status = nl_raw_validate(c->before);
    if (status == NL_CHECK_OK)
        status = nl_raw_validate(c->after);
    if (status != NL_CHECK_OK)
        return status == NL_CHECK_OUT_OF_MEMORY ? status : INVALID;
    const NLSemanticContext *b = c->before, *a = c->after;
    const NLCheckedNodeView *v = nl_checked_node_view(f, c->node);
    if (v == NULL || v->kind != NL_CHECKED_REPLACE || !v->field.present ||
        !v->field.dependency_compatible || v->field.parent == 0 ||
        v->field.parent > b->place_count || v->field.child == 0 ||
        v->field.child > b->place_count || v->field.base == 0 ||
        v->field.base > b->binding_count || v->field.index >= 3 ||
        v->field.access != NL_ACCESS_WRITE)
        return INVALID;
    const NLCheckedField e = v->field;
    const NLSemanticPlaceView root = b->places[e.parent - 1];
    const NLSemanticPlaceView child = b->places[e.child - 1];
    if (!nl_recursive_local_type(b, root.type) ||
        b->types[root.type - 1].view.field_count != 4 || !root.live ||
        !root.independent_root || root.fixed_field_count != 4 ||
        root.fixed_fields[e.index] != e.child ||
        root.incarnation != e.parent_incarnation || root.type != e.nominal ||
        root.current_fact != e.parent_fact || child.type != e.type ||
        child.incarnation != e.child_incarnation ||
        child.current_value != e.old_value ||
        child.current_fact != e.child_fact ||
        child.payload_occurrence != e.payload_occurrence ||
        child.governing_domain != root.governing_domain)
        return INVALID;
    const NLCheckedNodeView *projection = argument_node(f, v, 0);
    const NLSemanticValueView *projected = input(f, v, 0);
    const NLSemanticValueView *incoming = input(f, v, 1);
    if (projection == NULL || projection->kind != NL_CHECKED_FIELD_REF ||
        v->argument_count != 2 || projection->argument_count != 1 ||
        projected == NULL || incoming == NULL ||
        input_id(f, v, 0) > b->value_count ||
        input_id(f, v, 1) > b->value_count || projected->type == 0 ||
        projected->type > b->type_count || input_id(f, v, 1) != e.new_value ||
        projection->field.base != e.base ||
        projection->field.nominal != e.nominal ||
        projection->field.type != e.type ||
        projection->field.index != e.index ||
        projection->field.parent != e.parent ||
        projection->field.child != e.child ||
        projection->field.parent_incarnation != e.parent_incarnation ||
        projection->field.child_incarnation != e.child_incarnation ||
        projection->field.parent_fact != e.parent_fact ||
        projection->field.child_fact != e.child_fact ||
        projection->field.old_value != e.old_value ||
        projection->field.payload_occurrence != e.payload_occurrence ||
        !projection->field.dependency_compatible ||
        projection->field.access != NL_ACCESS_WRITE ||
        !projection->has_reference_result ||
        projected->type != projection->type ||
        projected->reference.place != e.child ||
        projected->reference.incarnation != e.child_incarnation ||
        !same_ref(projected->reference, projection->reference_result) ||
        incoming->type != e.type || v->result_count != 1 ||
        v->results[0].value != e.old_value || v->results[0].type != e.type)
        return INVALID;
    const NLSemanticValueView saved_ref = b->values[input_id(f, v, 0) - 1];
    const NLSemanticValueView saved_incoming = b->values[input_id(f, v, 1) - 1];
    if (saved_ref.type != projected->type ||
        !same_ref(saved_ref.reference, projected->reference) ||
        saved_incoming.type != incoming->type ||
        saved_incoming.variant != incoming->variant ||
        saved_incoming.sum_payload != incoming->sum_payload)
        return INVALID;
    const NLValueId root_value = b->bindings[e.base - 1].view.value;
    if (root_value == 0 || root_value > b->value_count)
        return INVALID;
    const NLSemanticValueView root_ref = b->values[root_value - 1];
    const NLCheckedNodeView *root_operand = argument_node(f, projection, 0);
    const NLSemanticValueView *copied_root = input(f, projection, 0);
    if (root_ref.type == 0 || root_ref.type > b->type_count ||
        b->bindings[e.base - 1].view.availability != NL_AVAILABLE ||
        root_operand == NULL || root_operand->symbol != e.base ||
        copied_root == NULL || copied_root->type != root_ref.type ||
        !same_ref(copied_root->reference, root_ref.reference) ||
        root_operand->value_use != NL_VALUE_COPIED)
        return INVALID;
    const NLSemanticTypeView rt = b->types[root_ref.type - 1].view;
    if (rt.kind != NL_TYPE_REF || rt.target != root.type || rt.is_exclusive ||
        rt.access != NL_ACCESS_WRITE || root_ref.reference_count != 0 ||
        root_ref.dependencies != NL_DEPENDENCY_FREE ||
        root_ref.reference.place != e.parent ||
        root_ref.reference.incarnation != e.parent_incarnation ||
        !root_ref.reference.writable || !root_ref.reference.readable ||
        root_ref.reference.provenance != NL_PROVENANCE_VALID ||
        root_ref.reference.scope == 0 ||
        root_ref.reference.scope > b->scope_count ||
        !b->scopes[root_ref.reference.scope - 1].active)
        return INVALID;
    NLReferenceFacts derived = root_ref.reference;
    derived.place = e.child;
    derived.incarnation = e.child_incarnation;
    const NLSemanticTypeView pt = b->types[projected->type - 1].view;
    if (!same_ref(derived, projected->reference) || pt.kind != NL_TYPE_REF ||
        pt.target != e.type || pt.is_exclusive || pt.access != rt.access ||
        projected->reference_count != 0 ||
        projected->dependencies != NL_DEPENDENCY_FREE)
        return INVALID;
    const NLCheckedNodeView *acquired = NULL;
    for (size_t i = 1; i < c->node; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_REF_FROM_PTR && n->result_count == 1 &&
            n->results[0].value == root_value)
            acquired = n;
    }
    if (acquired == NULL || acquired->argument_count != 2 ||
        acquired->type != root_ref.type ||
        acquired->results[0].type != root_ref.type ||
        !acquired->has_reference_result ||
        !same_ref(acquired->reference_result, root_ref.reference) ||
        acquired->lifetime_place != e.parent ||
        acquired->lifetime_incarnation != e.parent_incarnation ||
        acquired->lifetime_domain != root.governing_domain ||
        !extent_same(acquired->lifetime_range, root.placement))
        return INVALID;
    const NLSemanticValueView *pointer = input(f, acquired, 0);
    const NLSemanticValueView *stable = input(f, acquired, 1);
    const NLCheckedNodeView *pointer_operand = argument_node(f, acquired, 0);
    const NLCheckedNodeView *stable_operand = argument_node(f, acquired, 1);
    if (pointer == NULL || stable == NULL ||
        pointer_operand->value_use != NL_VALUE_COPIED ||
        stable_operand->value_use != NL_VALUE_COPIED ||
        !pointer_to(b, *pointer, e.parent, e.parent_incarnation) ||
        nl_allocated_write_access(b, input_id(f, acquired, 0)) != NL_CHECK_OK ||
        !domain_ref(b, *stable, root.governing_domain, false) ||
        stable->reference.scope != root_ref.reference.scope ||
        stable->dependencies != NL_DEPENDENCY_FREE ||
        argument_node(f, v, 0)->value_use != NL_VALUE_USE_NONE ||
        argument_node(f, v, 1)->value_use != NL_VALUE_USE_NONE ||
        nl_fixed_change_dependencies(b, e.child, 0) != NL_CHECK_OK)
        return INVALID;
    const NLCheckedNodeView *loan = NULL;
    for (size_t i = 1; i < c->node; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_LOAN_HEADER &&
            n->loan.ref_symbol == stable_operand->symbol &&
            n->loan.scope == stable->reference.scope)
            loan = n;
    }
    if (loan == NULL || loan->loan.source == 0 ||
        loan->loan.source > b->binding_count ||
        loan->loan.domain != root.governing_domain ||
        loan->loan.place != stable->reference.place ||
        loan->loan.incarnation != stable->reference.incarnation ||
        loan->loan.access != NL_ACCESS_READ || loan->loan.is_exclusive ||
        loan->loan.from_ptr || loan->loan.implicit_local ||
        !loan->loan.body_nonescape_proved || !loan->loan.prevent_lifetime_end)
        return INVALID;
    const NLSemanticBindingView domain_binding =
        b->bindings[loan->loan.source - 1].view;
    if (domain_binding.availability != NL_AVAILABLE ||
        domain_binding.value != b->domains[root.governing_domain - 1].value ||
        domain_binding.place != loan->loan.place)
        return INVALID;
    NLSemanticContext *expected = NULL;
    status = nl_sem_clone(b, &expected);
    if (status == NL_CHECK_OK)
        status = nl_fixed_change(expected, e.child, e.new_value, false);
    if (status == NL_CHECK_OK &&
        (!nl_packet_same_entry(expected, a) ||
         a->places[e.parent - 1].current_fact != e.parent_post_fact ||
         a->places[e.child - 1].current_fact != e.child_post_fact ||
         a->places[e.child - 1].payload_occurrence !=
             e.post_payload_occurrence))
        status = INVALID;
    nl_semantic_destroy(expected);
    return status == NL_CHECK_OK || status == NL_CHECK_OUT_OF_MEMORY ? status
                                                                     : INVALID;
}

static NLCheckStatus validate(const NLCheckedFragment *, NLCheckedNodeId,
                              size_t *, size_t, bool);

static NLCheckStatus trace(const NLCheckedFragment *f,
                           const NLCapturedClosure *certificate, size_t variant,
                           size_t *histogram, size_t depth)
{
    const NLSemanticContext *world = f->context;
    if (f->field_change_count > 6)
        return INVALID;
    Release releases[NL_CAPTURED_MAX_RELEASES] = {0};
    size_t count = certificate->view.count, grants = 0;
    for (size_t r = 0; r < count; ++r) {
        releases[r].original = certificate->view.originals[r];
        releases[r].initialized = true;
        const NLSemanticPlaceView p =
            certificate->view.ancestor->places[releases[r].original.root - 1];
        if (certificate->view.ancestor->types[p.type - 1].view.field_count == 4)
            memcpy(
                releases[r].fields,
                certificate->view.ancestor->values[p.current_value - 1].fields,
                sizeof(releases[r].fields));
    }
    bool nested = false;
    size_t changes = 0;
    for (size_t i = 1; i <= f->count; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->terminates || v->kind == NL_CHECKED_TAKE ||
            v->kind == NL_CHECKED_IF || v->kind == NL_CHECKED_LOOP ||
            v->kind == NL_CHECKED_REGISTERED_CALL)
            return INVALID; /* finite lifecycle/field certificate profile */
        if (v->kind == NL_CHECKED_REPLACE) {
            if (nested || count != 5 || changes >= f->field_change_count ||
                changes >= 6 || f->field_changes[changes] == NULL ||
                f->field_changes[changes]->node != i)
                return INVALID;
            const NLCapturedChange *change = f->field_changes[changes++];
            NLCheckStatus s = change_validate(f, change);
            if (s != NL_CHECK_OK)
                return s;
            const NLSemanticContext *b = change->before;
            size_t owner = count;
            for (size_t r = 0; r < count; ++r) {
                const NLCapturedOriginal o = releases[r].original;
                if (!releases[r].initialized || releases[r].ended ||
                    o.root > b->place_count || o.domain > b->domain_count ||
                    b->places[o.root - 1].incarnation != o.incarnation ||
                    !b->places[o.root - 1].live ||
                    b->places[o.root - 1].governing_domain != o.domain ||
                    !extent_same(b->places[o.root - 1].placement, o.extent) ||
                    b->domains[o.domain - 1].value != o.domain_value ||
                    b->values[o.allocation_value - 1].allocation_region !=
                        o.extent.region)
                    return INVALID;
                size_t allocations = 0, domains = 0;
                for (size_t j = 0; j < b->binding_count; ++j) {
                    const NLSemanticBindingView owner = b->bindings[j].view;
                    if (owner.availability != NL_AVAILABLE)
                        continue;
                    if (owner.value == o.allocation_value ||
                        owner.value == o.domain_value) {
                        const NLSemanticValueView package =
                            b->values[owner.value - 1];
                        if (package.carrier != NL_CARRIER_PLACE ||
                            package.owner_place != owner.place)
                            return INVALID;
                        allocations += owner.value == o.allocation_value;
                        domains += owner.value == o.domain_value;
                    }
                }
                if (allocations != 1 || domains != 1)
                    return INVALID;
                const NLSemanticPlaceView p = b->places[o.root - 1];
                for (size_t j = 0; j < 3; ++j)
                    if (p.fixed_fields[j] == 0 ||
                        p.fixed_fields[j] > b->place_count ||
                        b->places[p.fixed_fields[j] - 1].current_value !=
                            releases[r].fields[j])
                        return INVALID;
                if (o.root == v->field.parent)
                    owner = r;
            }
            if (owner == count || v->field.index >= 3 ||
                releases[owner].fields[v->field.index] != v->field.old_value)
                return INVALID;
            releases[owner].fields[v->field.index] = v->field.new_value;
            /* A copied Some pointer must retain one current original target;
             * it contributes no Allocation/Domain/Storage responsibility. */
            const NLSemanticValueView incoming =
                b->values[v->field.new_value - 1];
            if (incoming.variant == 2) {
                bool original = false;
                if (incoming.sum_payload == 0 ||
                    incoming.sum_payload > b->value_count)
                    return INVALID;
                for (size_t r = 0; r < count; ++r)
                    original |=
                        pointer_to(b, b->values[incoming.sum_payload - 1],
                                   releases[r].original.root,
                                   releases[r].original.incarnation);
                if (!original)
                    return INVALID;
            } else if (incoming.variant != 1 || incoming.sum_payload != 0)
                return INVALID;
        }
        if (v->kind == NL_CHECKED_REF_FROM_PTR ||
            v->kind == NL_CHECKED_FIELD_REF) {
            bool proved = false;
            for (size_t j = 0; j < f->field_change_count && j < 6; ++j) {
                const NLCapturedChange *change = f->field_changes[j];
                const NLCheckedNodeView *write =
                    change == NULL ? NULL
                                   : nl_checked_node_view(f, change->node);
                if (write == NULL || change->before == NULL ||
                    change->before != change->before_origin)
                    return INVALID;
                if (v->kind == NL_CHECKED_FIELD_REF)
                    proved |= write->first_argument == i;
                else if (v->result_count == 1 && write->field.base != 0 &&
                         write->field.base <= change->before->binding_count)
                    proved |= change->before->bindings[write->field.base - 1]
                                  .view.value == v->results[0].value;
            }
            if (!proved)
                return INVALID;
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER &&
            (!v->loan.body_nonescape_proved || v->loan.scope == 0 ||
             v->loan.scope > world->scope_count ||
             world->scopes[v->loan.scope - 1].active))
            return INVALID;
        if (v->kind == NL_CHECKED_TRY_ALLOCATE_ONE && v->result_count != 0) {
            ++grants;
            if (i != nl_checked_node_view(f, f->root)->initializer ||
                grants != 1 || v->allocation_success != (variant == 2))
                return INVALID;
            if (variant == 1) {
                if (v->backing != 0 || v->allocation_authority != 0 ||
                    v->storage_authority != 0)
                    return INVALID;
                continue;
            }
            if (count >= NL_CAPTURED_MAX_RELEASES ||
                v->backing != certificate->view.ancestor->region_count + 1 ||
                v->backing > world->region_count ||
                v->allocation_authority <=
                    certificate->view.ancestor->value_count ||
                v->allocation_authority > world->value_count ||
                v->storage_authority <=
                    certificate->view.ancestor->value_count ||
                v->storage_authority > world->value_count ||
                world->values[v->allocation_authority - 1].allocation_region !=
                    v->backing ||
                !extent_same(
                    world->values[v->storage_authority - 1].occupancy,
                    (NLBackingRange){v->backing, 0, v->allocation_size}))
                return INVALID;
            releases[count++].original = (NLCapturedOriginal){
                .origin = world,
                .extent = {v->backing, 0, v->allocation_size},
                .allocation_value = v->allocation_authority};
            releases[count - 1].initial_raw = v->storage_authority;
        }
        if (v->kind == NL_CHECKED_INTO_SLOT) {
            NLValueId vacant = 0;
            if (count == 0 || nested || variant != 2 ||
                releases[count - 1].vacant != 0 ||
                input_id(f, v, 0) != releases[count - 1].initial_raw ||
                !result_id(f, v, &vacant) ||
                !extent_same(world->values[vacant - 1].occupancy,
                             releases[count - 1].original.extent) ||
                argument_node(f, v, 0)->value_use != NL_VALUE_CONSUMED)
                return INVALID;
            releases[count - 1].vacant = vacant;
        }
        if (v->kind == NL_CHECKED_DOMAIN_CREATE) {
            NLValueId domain = 0;
            if (count == 0 || nested || variant != 2 ||
                releases[count - 1].original.domain != 0 ||
                !result_id(f, v, &domain) ||
                world->values[domain - 1].domain == 0)
                return INVALID;
            releases[count - 1].original.domain =
                world->values[domain - 1].domain;
            releases[count - 1].original.domain_value = domain;
        }
        if (v->kind == NL_CHECKED_INITIALIZE) {
            if (nested || count == 0 || grants != 1 || variant != 2)
                return INVALID;
            Release *r = &releases[count - 1];
            if (r->initialized || r->vacant == 0 ||
                input_id(f, v, 0) != r->vacant ||
                argument_node(f, v, 0)->value_use != NL_VALUE_CONSUMED ||
                r->original.domain != v->lifetime_domain ||
                !extent_same(v->lifetime_range, r->original.extent) ||
                v->backing != r->original.extent.region ||
                v->lifetime_place <= certificate->view.ancestor->place_count ||
                v->lifetime_place > world->place_count ||
                v->lifetime_incarnation <=
                    certificate->view.ancestor->last_incarnation ||
                v->lifetime_domain <=
                    certificate->view.ancestor->domain_count ||
                v->lifetime_domain > world->domain_count)
                return INVALID;
            r->initialized = true;
            r->original.root = v->lifetime_place;
            r->original.incarnation = v->lifetime_incarnation;
            r->original.domain = v->lifetime_domain;
            r->original.domain_value =
                world->domains[v->lifetime_domain - 1].value;
            const NLSemanticValueView *initial = input(f, v, 1);
            if (world->types[world->places[v->lifetime_place - 1].type - 1]
                    .view.field_count == 4) {
                if (initial == NULL || initial->field_count != 4)
                    return INVALID;
                memcpy(r->fields, initial->fields, sizeof(r->fields));
            }
            const NLSemanticPlaceView p = world->places[v->lifetime_place - 1];
            NLValueId token = 0;
            const NLSemanticValueView *stable = input(f, v, 2);
            if (p.incarnation != v->lifetime_incarnation ||
                !p.independent_root ||
                world->values[r->vacant - 1].slot_place != v->lifetime_place ||
                !result_id(f, v, &token) || !v->has_reference_result ||
                !pointer_to(world, world->values[token - 1], v->lifetime_place,
                            v->lifetime_incarnation) ||
                v->reference_result.place != v->lifetime_place ||
                v->reference_result.incarnation != v->lifetime_incarnation ||
                v->reference_result.provenance != NL_PROVENANCE_VALID ||
                stable == NULL ||
                !domain_ref(world, *stable, v->lifetime_domain, false))
                return INVALID;
        }
        if (v->kind == NL_CHECKED_MATCH) {
            if (nested || f->captured_closure == NULL ||
                f->captured_match != i || variant != 2 ||
                !f->captured_closure->branches[0].entry)
                return INVALID;
            const NLCapturedClosure *child = f->captured_closure;
            if (child->parent_world != world || child->view.count != count)
                return INVALID;
            for (size_t r = 0; r < count; ++r) {
                const Release own = releases[r];
                const NLCapturedOriginal o = child->view.originals[r];
                if (!own.initialized || own.ended || own.finalized ||
                    own.freed || !extent_same(own.original.extent, o.extent) ||
                    own.original.root != o.root ||
                    own.original.incarnation != o.incarnation ||
                    own.original.domain != o.domain ||
                    own.original.allocation_value != o.allocation_value ||
                    own.original.domain_value != o.domain_value)
                    return INVALID;
            }
            NLCheckStatus s = validate(f, i, histogram, depth + 1, true);
            if (s != NL_CHECK_OK)
                return s;
            nested = true;
        }
        if (v->kind != NL_CHECKED_DESTROY && v->kind != NL_CHECKED_ERASE_SLOT &&
            v->kind != NL_CHECKED_DOMAIN_FINALIZE &&
            v->kind != NL_CHECKED_DEALLOCATE)
            continue;
        if (nested)
            return INVALID;
        const NLSemanticValueView *a = input(f, v, 0), *b = input(f, v, 1);
        if (a == NULL)
            return INVALID;
        if (v->kind != NL_CHECKED_DESTROY &&
            (argument_node(f, v, 0)->value_use != NL_VALUE_CONSUMED ||
             (v->kind == NL_CHECKED_DEALLOCATE &&
              (b == NULL ||
               argument_node(f, v, 1)->value_use != NL_VALUE_CONSUMED))))
            return INVALID;
        NLBackingRegionId region =
            v->kind == NL_CHECKED_DESTROY      ? v->lifetime_range.region
            : v->kind == NL_CHECKED_ERASE_SLOT ? a->occupancy.region
                                               : a->allocation_region;
        size_t index = count;
        for (size_t r = 0; r < count; ++r)
            if (v->kind == NL_CHECKED_DOMAIN_FINALIZE
                    ? releases[r].original.domain == a->domain
                    : releases[r].original.extent.region == region)
                index = r;
        if (index == count)
            return INVALID;
        Release *r = &releases[index];
        if (!r->initialized || r->freed)
            return INVALID;
        if (v->kind == NL_CHECKED_DESTROY) {
            NLValueId slot = 0;
            if (r->ended || b == NULL || !result_id(f, v, &slot) ||
                argument_node(f, v, 0)->value_use != NL_VALUE_COPIED ||
                argument_node(f, v, 1)->value_use != NL_VALUE_REBORROWED ||
                v->lifetime_place != r->original.root ||
                v->lifetime_incarnation != r->original.incarnation ||
                v->lifetime_domain != r->original.domain ||
                !extent_same(v->lifetime_range, r->original.extent) ||
                !pointer_to(world, *a, r->original.root,
                            r->original.incarnation) ||
                !domain_ref(world, *b, r->original.domain, true) ||
                !extent_same(world->values[slot - 1].occupancy,
                             r->original.extent))
                return INVALID;
            bool ending = false;
            for (size_t j = 0; j < world->binding_count; ++j)
                if (world->bindings[j].view.place == b->reference.place &&
                    world->values[world->bindings[j].view.value - 1].domain ==
                        r->original.domain)
                    ending = true;
            if (!ending)
                return INVALID;
            r->ended = true;
            r->slot = slot;
        } else if (v->kind == NL_CHECKED_ERASE_SLOT) {
            NLValueId storage = 0;
            if (!r->ended || r->erased || input_id(f, v, 0) != r->slot ||
                !result_id(f, v, &storage) ||
                !extent_same(a->occupancy, r->original.extent) ||
                !extent_same(world->values[storage - 1].occupancy,
                             r->original.extent))
                return INVALID;
            r->erased = true;
            r->storage = storage;
        } else if (v->kind == NL_CHECKED_DOMAIN_FINALIZE) {
            if (!r->ended || r->finalized ||
                input_id(f, v, 0) != r->original.domain_value)
                return INVALID;
            r->finalized = true;
        } else {
            if (!r->erased || !r->finalized || b == NULL ||
                input_id(f, v, 0) != r->original.allocation_value ||
                input_id(f, v, 1) != r->storage ||
                !extent_same(b->occupancy, r->original.extent))
                return INVALID;
            r->freed = true;
        }
    }
    if (grants != 1 || count != certificate->view.count + (variant == 2) ||
        changes != f->field_change_count)
        return INVALID;
    if (nested)
        return NL_CHECK_OK;
    for (size_t r = 0; r < count; ++r)
        if (!releases[r].freed)
            return INVALID;
    ++histogram[count];
    return NL_CHECK_OK;
}

static NLCheckStatus validate(const NLCheckedFragment *f, NLCheckedNodeId match,
                              size_t *histogram, size_t depth, bool sealed)
{
    if (f == NULL || f->context == NULL || f->captured_match != match ||
        depth > 5 || f->captured_closure == NULL)
        return INVALID;
    const NLCapturedClosure *c = f->captured_closure;
    const NLCheckedNodeView *m = nl_checked_node_view(f, match);
    if (c->parent_world != f->context || m == NULL ||
        m->kind != NL_CHECKED_MATCH || m->normal_arms != 2 ||
        m->item_count != 2 || m->terminates || c->view.ancestor == NULL ||
        c->view.closed_post == NULL || c->view.ancestor != c->ancestor_owned ||
        c->view.closed_post != c->closed_owned ||
        m->captured_frame_closed != (c->view.count != 0) ||
        c->view.ancestor == c->view.closed_post ||
        m->match_binding_prefix != c->view.ancestor->binding_count ||
        m->normal_frame_unchanged != (c->view.count == 0))
        return INVALID;
    const NLCheckedNodeView *trial = nl_checked_node_view(f, m->initializer);
    if (trial == NULL || trial->kind != NL_CHECKED_TRY_ALLOCATE_ONE ||
        !trial->allocation_trial || trial->result_count != 0 ||
        trial->allocation_target == 0 ||
        trial->allocation_target > c->view.ancestor->type_count ||
        !nl_recursive_local_type(c->view.ancestor, trial->allocation_target) ||
        trial->allocation_size !=
            c->view.ancestor->types[trial->allocation_target - 1].view.size ||
        trial->allocation_alignment !=
            c->view.ancestor->types[trial->allocation_target - 1]
                .view.alignment)
        return INVALID;
    NLCapturedClosureView recomputed = {0};
    NLCheckStatus s = collect(c->view.ancestor, &recomputed);
    if (s != NL_CHECK_OK)
        return s;
    if (recomputed.count != c->view.count)
        return INVALID;
    for (size_t r = 0; r < NL_CAPTURED_MAX_ORIGINALS; ++r)
        if (!original_same(recomputed.originals[r], c->view.originals[r]))
            return INVALID;
    NLSemanticContext *expected = NULL;
    s = derive(c->view.ancestor, &recomputed, &expected);
    if (s != NL_CHECK_OK)
        return s;
    bool post = nl_packet_same_entry(expected, c->view.closed_post);
    nl_semantic_destroy(expected);
    if (!post)
        return INVALID;
    size_t actual[NL_CAPTURED_MAX_RELEASES + 1] = {0};
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_match_arm(f, match, a);
        const NLCheckedNodeView *root =
            arm == NULL ? NULL : nl_checked_node_view(arm, arm->root);
        if (arm == NULL || root == NULL || arm->context == NULL ||
            c->branches[a].entry == NULL || arm != c->branches[a].arm ||
            arm->context != c->branches[a].world ||
            arm->context == f->context || arm->context == c->view.ancestor ||
            arm->context == c->view.closed_post ||
            arm->context->region_count != c->view.ancestor->region_count + a ||
            arm->context->domain_count != c->view.ancestor->domain_count + a ||
            root->kind != NL_CHECKED_MATCH_ARM || root->variant != a + 1 ||
            c->branches[a].entry != c->branches[a].entry_origin ||
            c->branches[a].entry == c->view.ancestor ||
            c->branches[a].entry == c->view.closed_post ||
            !nl_packet_same_entry(c->view.ancestor, c->branches[a].entry) ||
            !nl_allocated_post_matches(c->view.closed_post, arm->context, root))
            return INVALID;
        s = nl_raw_validate(arm->context);
        if (s != NL_CHECK_OK)
            return s == NL_CHECK_OUT_OF_MEMORY ? s : INVALID;
        for (size_t scope = 0; scope < arm->context->scope_count; ++scope)
            if (arm->context->scopes[scope].active)
                return INVALID;
        for (size_t value = 0; value < arm->context->value_count; ++value)
            if (arm->context->values[value].dependencies !=
                    NL_DEPENDENCY_FREE ||
                arm->context->values[value].value_dependency_count != 0)
                return INVALID;
        const NLCheckedNodeView *grant =
            nl_checked_node_view(arm, root->initializer);
        NLValueId outcome = 0;
        if (grant == NULL || !grant->allocation_trial ||
            grant->kind != NL_CHECKED_TRY_ALLOCATE_ONE ||
            !result_id(arm, grant, &outcome) ||
            grant->allocation_target != trial->allocation_target ||
            grant->allocation_size != trial->allocation_size ||
            grant->allocation_alignment != trial->allocation_alignment ||
            grant->type != trial->type ||
            grant->results[0].type != trial->type ||
            arm->context->values[outcome - 1].type != trial->type ||
            arm->context->values[outcome - 1].variant != a + 1 ||
            root->normal_frame_unchanged != (c->view.count == 0))
            return INVALID;
        if (a == 0) {
            if (root->symbol != 0)
                return INVALID;
        } else {
            if (root->symbol == 0 ||
                root->symbol > arm->context->binding_count ||
                grant->backing == 0 ||
                grant->backing > arm->context->region_count)
                return INVALID;
            const NLSemanticBindingView binding =
                arm->context->bindings[root->symbol - 1].view;
            const NLSemanticValueView bundle =
                arm->context->values[binding.value - 1];
            const NLSemanticBackingView backing =
                arm->context->regions[grant->backing - 1].view;
            if (bundle.type !=
                    arm->context->types[trial->type - 1].variant_types[1] ||
                bundle.field_count != 2 ||
                bundle.fields[0] != grant->allocation_authority ||
                bundle.fields[1] != grant->storage_authority ||
                backing.size != grant->allocation_size ||
                backing.alignment != grant->allocation_alignment)
                return INVALID;
        }
        s = trace(arm, c, a + 1, actual, depth);
        if (s != NL_CHECK_OK)
            return s;
    }
    if (c->branches[0].world == c->branches[1].world ||
        c->branches[0].entry == c->branches[1].entry)
        return INVALID;
    if (sealed && memcmp(actual, c->view.release_worlds, sizeof(actual)) != 0)
        return INVALID;
    for (size_t i = 0; i <= NL_CAPTURED_MAX_RELEASES; ++i)
        histogram[i] += actual[i];
    return NL_CHECK_OK;
}
NLCheckStatus nl_captured_closure_finish(NLCheckedFragment *f,
                                         NLCheckedNodeId match)
{
    size_t histogram[NL_CAPTURED_MAX_RELEASES + 1] = {0};
    NLCheckStatus s = validate(f, match, histogram, 0, false);
    if (s == NL_CHECK_OK)
        memcpy(f->captured_closure->view.release_worlds, histogram,
               sizeof(histogram));
    return s;
}
NLCheckStatus nl_checked_captured_closure_validate(const NLCheckedFragment *f,
                                                   NLCheckedNodeId match)
{
    size_t histogram[NL_CAPTURED_MAX_RELEASES + 1] = {0};
    return validate(f, match, histogram, 0, true);
}
