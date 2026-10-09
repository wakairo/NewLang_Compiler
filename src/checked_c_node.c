#include "newlang/checked_c_node.h"
#include "checked_c_five.h"
#include "newlang/raw_storage.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Backend-private representation. Semantic IDs select checked carriers, never
 * machine addresses. No source spelling, AST, final-state liveness replay,
 * runtime provenance policy, or reconstruction of semantic packages. */
#define C_BYTES 131072
#define C_LOCALS 128
#define C_STEPS 512
#define C_DEPTH 16
typedef struct {
    NLSymbolId symbol;
    NLTypeId type;
    size_t frame;
    NLPlaceId place;
    NLIncarnationId incarnation;
} Local;
typedef struct {
    unsigned stage;
    size_t owner_frame, domain_frame;
    NLSymbolId owner_symbol, domain_symbol;
} HeapLifecycle;
typedef struct {
    char *text;
    char *receiver;
    char *producer;
    char *recipient;
    size_t recipient_used, recipient_calls;
    NLTypeId custody_type;
    bool custody, custody_authorized;
    const NLCheckedNodeView *custody_active_call;
    size_t producer_used, producer_calls;
    NLTypeId live_tail;
    NLValueId returned_members[3];
    size_t receiver_used, owner_calls;
    bool owner_read;
    size_t used, temp, steps, frames, roots;
    bool allocated;
    bool two_heap;
    HeapLifecycle lifecycle[2]; /* backend-only carrier validation */
    bool heap_link;
    size_t projections, reads;
    NLPlaceId link_place;
    NLIncarnationId link_incarnation;
    NLValueId link_value;
    NLValueFactId root_fact, link_fact;
    NLOccurrenceId occurrence;
    size_t allocations, destroys, releases, domains, finalized, slots, erased,
        reloans, option_bindings, link_updates;
    NLTypeId bundle;
    NLTypeId node, option, ptr, u8, unit;
    NLNodeCStatus status;
} Emit;
typedef struct Env {
    Local locals[C_LOCALS];
    size_t count, frame, depth, prefix;
    NLBackingRegionId backing;
    size_t region_slot;
    NLPlaceId heap_place;
    NLIncarnationId heap_incarnation;
    NLDomainId heap_domain;
    NLSymbolId heap_symbol;
    NLScopeId stability_scope;
    NLDomainId stability_domain;
    NLPlaceId stability_place;
    const struct Env *parent;
    const NLCheckedFragment *artifact;
} Env;
typedef struct {
    NLTypeId type;
    size_t temp; /* generated carrier, not a semantic ID */
} Value;
static bool reject(Emit *e)
{
    if (e->status == NL_NODE_C_OK)
        e->status = NL_NODE_C_UNSUPPORTED;
    return false;
}

static bool put(Emit *e, const char *format, ...)
{
    if (e->status != NL_NODE_C_OK)
        return false;
    va_list args;
    va_start(args, format);
    int n = vsnprintf(e->text + e->used, C_BYTES - e->used, format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= C_BYTES - e->used) {
        e->status = NL_NODE_C_RESOURCE_LIMIT;
        return false;
    }
    e->used += (size_t)n;
    return true;
}
static const NLCheckedNodeView *get(const Env *env, NLCheckedNodeId id)
{
    return nl_checked_node_view(env->artifact, id);
}
static const Local *local(const Env *env, NLSymbolId symbol, NLTypeId type)
{
    for (size_t i = 0; i < env->count; ++i)
        if (env->locals[i].symbol == symbol && env->locals[i].type == type)
            return &env->locals[i];
    /* Public prefix mapping is authorized by the parent's checked MATCH
     * certificate, not numeric coincidence of hypothetical local IDs. */
    return env->parent != NULL && symbol <= env->prefix
               ? local(env->parent, symbol, type)
               : NULL;
}
static const Local *root(const Env *env, NLTypeId type, NLPlaceId place,
                         NLIncarnationId incarnation)
{
    for (size_t i = 0; i < env->count; ++i)
        if (env->locals[i].type == type && env->locals[i].place == place &&
            env->locals[i].incarnation == incarnation)
            return &env->locals[i];
    if (env->parent != NULL) {
        const Local *p = root(env->parent, type, place, incarnation);
        return p != NULL && p->symbol <= env->prefix ? p : NULL;
    }
    return NULL;
}
static const Env *region(const Env *, NLBackingRegionId);
static const Env *domain_heap(const Env *, NLDomainId);
static bool add_local(Emit *e, Env *env, NLSymbolId symbol, NLTypeId type,
                      bool physical_root)
{
    if (symbol == 0 || env->count == C_LOCALS ||
        local(env, symbol, type) != NULL)
        return reject(e);
    NLSemanticBindingView binding;
    if (!nl_semantic_binding_view(nl_checked_context(env->artifact), symbol,
                                  &binding) ||
        binding.type != type)
        return reject(e);
    Local l = {.symbol = symbol, .type = type, .frame = env->frame};
    if (physical_root) {
        NLSemanticBindingView b;
        NLSemanticPlaceView p;
        if (!nl_semantic_binding_view(nl_checked_context(env->artifact), symbol,
                                      &b) ||
            b.type != type ||
            !nl_semantic_place_view(nl_checked_context(env->artifact), b.place,
                                    &p) ||
            p.type != type || p.incarnation == 0 || !p.implicit_local ||
            !p.independent_root || p.governing_domain != 0)
            return reject(e);
        l.place = b.place;
        l.incarnation = p.incarnation;
    }
    if (e->two_heap) {
        const NLSemanticContext *c = nl_checked_context(env->artifact);
        NLSemanticValueView value;
        if (!nl_semantic_value_view(c, binding.value, &value))
            return reject(e);
        const bool owner = type == nl_semantic_core_type(c, NL_TYPE_ALLOCATION);
        const bool domain = type == nl_semantic_domain_type(c);
        if (owner || domain) {
            const Env *r = owner ? region(env, value.allocation_region)
                                 : domain_heap(env, value.domain);
            if (r == NULL && domain)
                r = env; /* newly created domain before INITIALIZE */
            if (r == NULL || r->region_slot > 1)
                return reject(e);
            HeapLifecycle *h = &e->lifecycle[r->region_slot];
            size_t *frame = owner ? &h->owner_frame : &h->domain_frame;
            NLSymbolId *carrier = owner ? &h->owner_symbol : &h->domain_symbol;
            if (*carrier != 0)
                return reject(e);
            *frame = l.frame;
            *carrier = l.symbol;
        }
    }
    env->locals[env->count++] = l;
    return true;
}
static const char *ctype(const Emit *e, const Env *env, NLTypeId type)
{
    if (type == e->node)
        return "nl_node";
    if (type == e->option)
        return "nl_node_option";
    if (type == e->ptr)
        return "const nl_node *";
    if (type == e->u8)
        return "uint8_t";
    if (e->allocated) {
        NLSemanticTypeView t;
        if (!nl_semantic_type_view(nl_checked_context(env->artifact), type, &t))
            return NULL;
        if (e->custody_type != 0 && type == e->custody_type)
            return "nl_custody";
        if (e->live_tail != 0 && type == e->live_tail)
            return "nl_live_tail";
        if (type == e->bundle)
            return "nl_backing";
        if (t.kind == NL_TYPE_ALLOCATION)
            return "nl_allocation";
        if (t.kind == NL_TYPE_STORAGE)
            return "nl_storage";
        if (t.kind == NL_TYPE_SLOT && t.target == e->node)
            return "nl_slot";
        if (type == nl_semantic_domain_type(nl_checked_context(env->artifact)))
            return "nl_domain";
        if (t.kind == NL_TYPE_REF) {
            if (e->custody_type != 0 && t.target == e->custody_type &&
                !t.is_exclusive && t.access == NL_ACCESS_WRITE)
                return "nl_custody *";
            if (t.target == e->node && !t.is_exclusive)
                return t.access == NL_ACCESS_WRITE ? "nl_node *"
                                                   : "const nl_node *";
            if (t.target == e->option && !t.is_exclusive)
                return t.access == NL_ACCESS_WRITE ? "nl_node_option *"
                                                   : "const nl_node_option *";
            if (t.target == nl_semantic_domain_type(
                                nl_checked_context(env->artifact)) &&
                t.access == NL_ACCESS_READ)
                return "const nl_domain *";
        }
    }
    return NULL;
}
static bool shape(Emit *e, const Env *env, NLTypeId type)
{
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    NLAggregateField link, payload;
    NLSemanticTypeView option, ptr, h;
    /* Registry predicate includes completed recursive-header identity and the
     * exact Option constructor metadata; names are never consulted. */
    if (!nl_semantic_recursive_local_type(c, type) ||
        !nl_semantic_type_view(c, type, &h) ||
        (e->allocated &&
         (!h.layout_known || h.size != 24 || h.alignment != 8)) ||
        !nl_semantic_aggregate_field_view(c, type, 0, &link) ||
        !nl_semantic_aggregate_field_view(c, type, 1, &payload) ||
        !nl_semantic_type_view(c, link.type, &option))
        return reject(e);
    NLTypeId pointer = 0;
    /* Resolve the exact committed pointer from constructor payload evidence.
     * A read-only type observation below avoids a name/structural guess. */
    if (!nl_semantic_recursive_link_types(c, type, &pointer) ||
        !nl_semantic_type_view(c, pointer, &ptr) ||
        option.kind != NL_TYPE_SUM || option.variant_count != 2 ||
        !option.is_copy || !option.is_discardable || ptr.kind != NL_TYPE_PTR ||
        ptr.target != type || payload.type != e->u8 ||
        (e->node != 0 && e->node != type))
        return reject(e);
    e->node = type;
    e->option = link.type;
    e->ptr = pointer;
    return true;
}
static bool field(Emit *e, const Env *env, const NLCheckedField *f,
                  NLAccessSyntax access)
{
    return f->present && f->dependency_compatible && f->nominal == e->node &&
                   f->type == e->option && f->index == 0 &&
                   f->access == access && f->parent != 0 && f->child != 0 &&
                   f->parent_incarnation != 0 && f->child_incarnation != 0 &&
                   f->parent_fact != 0 && f->child_fact != 0 &&
                   local(env, f->base, e->node) != NULL &&
                   root(env, e->node, f->parent, f->parent_incarnation) ==
                       local(env, f->base, e->node)
               ? true
               : reject(e);
}
static bool expr(Emit *, Env *, NLCheckedNodeId, Value *);
static bool producer_call(Emit *, Env *, NLCheckedNodeId,
                          const NLCheckedNodeView *, Value *);
static bool live_destructure(Emit *, Env *, const NLCheckedNodeView *, Value *);
static bool owner_call(Emit *, Env *, NLCheckedNodeId,
                       const NLCheckedNodeView *, Value *);
static bool allocated_match(Emit *, Env *, NLCheckedNodeId,
                            const NLCheckedNodeView *, Value *);
static bool allocated_expr(Emit *, Env *, const NLCheckedNodeView *, Value *);
static bool heap_link_expr(Emit *, Env *, const NLCheckedNodeView *, Value *);
static bool custody_expr(Emit *, Env *, NLCheckedNodeId,
                         const NLCheckedNodeView *, Value *);
static bool custody_match(Emit *, Env *, NLCheckedNodeId,
                          const NLCheckedNodeView *, Value *);
static bool same_reference(NLReferenceFacts a, NLReferenceFacts b)
{
    return a.place == b.place && a.incarnation == b.incarnation &&
           a.scope == b.scope && a.provenance == b.provenance &&
           a.readable == b.readable && a.writable == b.writable &&
           a.occurrence_dependency == b.occurrence_dependency;
}
static bool selected_reference(const Env *env, const NLCheckedNodeView *arg,
                               NLSemanticValueView *value)
{
    NLSemanticBindingView b;
    NLSemanticValueView original;
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    return arg != NULL && arg->kind == NL_CHECKED_IDENTIFIER &&
           arg->value_use == NL_VALUE_COPIED && arg->result_count == 1 &&
           local(env, arg->symbol, arg->type) != NULL &&
           nl_semantic_binding_view(c, arg->symbol, &b) &&
           b.type == arg->type &&
           nl_semantic_value_view(c, b.value, &original) &&
           nl_semantic_value_view(c, arg->results[0].value, value) &&
           original.type == arg->type && value->type == arg->type &&
           original.dependencies == NL_DEPENDENCY_FREE &&
           value->dependencies == NL_DEPENDENCY_FREE &&
           original.value_dependency_count == 0 &&
           value->value_dependency_count == 0 &&
           original.reference_count == 0 && value->reference_count == 0 &&
           same_reference(original.reference, value->reference);
}
static bool block(Emit *e, Env *env, NLCheckedNodeId id, Value *out)
{
    const NLCheckedNodeView *v = get(env, id);
    if (v == NULL || v->kind != NL_CHECKED_BLOCK || v->terminates ||
        v->type != e->unit || v->result_count != 0 || env->depth >= C_DEPTH)
        return reject(e);
    NLCheckedNodeId item = v->first_item;
    bool custody_suffix = false;
    ++env->depth;
    for (size_t i = 0; i < v->item_count; ++i) {
        const NLCheckedNodeView *n = get(env, item);
        Value ignored = {0};
        if (n == NULL || !expr(e, env, item, &ignored))
            return reject(e);
        if (ignored.temp != 0 && !put(e, "(void)nl_v_%zu;\n", ignored.temp))
            return false;
        custody_suffix = n->kind == NL_CHECKED_STATEMENT &&
                         nl_checked_custody_continuation(
                             env->artifact, n->initializer, 0) != NULL;
        item = n->next_item;
    }
    --env->depth;
    if (e->custody && custody_suffix && v->tail == 0 && item == 0) {
        *out = (Value){.type = e->unit};
        return true;
    }
    return item == 0 && expr(e, env, v->tail, out);
}
static bool loan(Emit *e, Env *env, const NLCheckedNodeView *v, Value *out)
{
    const NLCheckedLoanPlan *l = &v->loan;
    const NLCheckedNodeView *body = get(env, v->initializer);
    const NLCheckedNodeView *tail = body == NULL ? NULL : get(env, body->tail);
    const bool write = l->access == NL_ACCESS_WRITE;
    if (!l->implicit_local || !l->body_nonescape_proved ||
        !l->normal_result_forwarded || !l->prevent_lifetime_end ||
        l->is_exclusive || l->scope == 0 || l->ref_symbol == 0 ||
        l->place == 0 || l->incarnation == 0 || body == NULL ||
        body->kind != NL_CHECKED_BLOCK || body->item_count != 0 ||
        body->terminates || tail == NULL || body->type != v->type ||
        body->result_count != v->result_count || tail->type != v->type ||
        tail->result_count != v->result_count ||
        (l->access != NL_ACCESS_READ && !write))
        return reject(e);
    const Local *src =
        local(env, l->source, write || !l->from_ptr ? e->node : e->ptr);
    if (src == NULL)
        return reject(e);
    NLSemanticBindingView binder;
    NLSemanticTypeView ref_type;
    if (!nl_semantic_binding_view(nl_checked_context(env->artifact),
                                  l->ref_symbol, &binder) ||
        !nl_semantic_type_view(nl_checked_context(env->artifact), binder.type,
                               &ref_type) ||
        ref_type.kind != NL_TYPE_REF || ref_type.is_exclusive ||
        ref_type.access != l->access ||
        ref_type.target != (write ? e->option : e->node))
        return reject(e);
    if (l->from_ptr) {
        NLSemanticBindingView binding;
        NLSemanticValueView pointer;
        if (!nl_semantic_binding_view(nl_checked_context(env->artifact),
                                      l->source, &binding) ||
            !nl_semantic_value_view(nl_checked_context(env->artifact),
                                    binding.value, &pointer) ||
            pointer.type != e->ptr || pointer.reference_count != 0 ||
            pointer.dependencies != NL_DEPENDENCY_FREE ||
            pointer.reference.provenance != NL_PROVENANCE_VALID ||
            !pointer.reference.readable ||
            pointer.reference.place != l->place ||
            pointer.reference.incarnation != l->incarnation)
            return reject(e);
    }
    const size_t reference = ++e->temp;
    if (write) {
        if (l->from_ptr || !field(e, env, &v->field, NL_ACCESS_WRITE) ||
            l->place != v->field.child ||
            l->incarnation != v->field.child_incarnation ||
            tail->kind != NL_CHECKED_REPLACE || tail->type != e->option ||
            tail->result_count != 1 || tail->argument_count != 2 ||
            !field(e, env, &tail->field, NL_ACCESS_WRITE) ||
            tail->field.base != l->source ||
            tail->field.parent != v->field.parent ||
            tail->field.parent_incarnation != v->field.parent_incarnation ||
            tail->field.child != v->field.child ||
            tail->field.child_incarnation != l->incarnation ||
            tail->field.old_value != tail->results[0].value ||
            tail->field.new_value == 0 || tail->field.parent_post_fact == 0 ||
            tail->field.child_post_fact == 0 ||
            tail->field.parent_fact == tail->field.parent_post_fact ||
            tail->field.child_fact == tail->field.child_post_fact)
            return reject(e);
        const NLCheckedNodeView *ref = get(env, tail->first_argument);
        NLSemanticTypeView rt;
        if (ref == NULL || ref->kind != NL_CHECKED_IDENTIFIER ||
            ref->symbol != l->ref_symbol ||
            !nl_semantic_type_view(nl_checked_context(env->artifact), ref->type,
                                   &rt) ||
            rt.kind != NL_TYPE_REF || rt.target != e->option ||
            rt.is_exclusive || rt.access != NL_ACCESS_WRITE)
            return reject(e);
        if (e->allocated) {
            const NLCheckedNodeView *replacement = get(env, ref->next_argument);
            if (e->link_updates >= 2 || e->destroys != 0 ||
                replacement == NULL ||
                replacement->kind != NL_CHECKED_SUM_CONSTRUCTOR ||
                replacement->variant != (e->link_updates == 0 ? 2u : 1u) ||
                (e->link_updates == 1 && e->reloans != 1))
                return reject(e);
            ++e->link_updates;
        }
        Value replacement = {0};
        if (!expr(e, env, ref->next_argument, &replacement) ||
            replacement.type != e->option)
            return reject(e);
        out->type = e->option;
        return put(e,
                   "{ nl_node_option *nl_ref_%zu = &nl_b_%zu_%zu.f0;\n"
                   "const nl_node_option nl_old_%zu = *nl_ref_%zu;\n"
                   "*nl_ref_%zu = nl_v_%zu;\n"
                   "NL_NODE_REPLACE(&nl_b_%zu_%zu, &nl_old_%zu);\n"
                   "nl_v_%zu = nl_old_%zu; }\n",
                   reference, src->frame, src->symbol, reference, reference,
                   reference, replacement.temp, src->frame, src->symbol,
                   reference, out->temp, reference);
    }
    if (v->field.present ||
        root(env, e->node, l->place, l->incarnation) == NULL)
        return reject(e);
    if (!l->from_ptr &&
        (src->place != l->place || src->incarnation != l->incarnation))
        return reject(e);
    if (tail->kind == NL_CHECKED_UNIT) {
        if (!l->from_ptr || v->type != e->unit || v->result_count != 0)
            return reject(e);
        out->type = e->unit;
        return put(e,
                   "{ const nl_node *nl_ref_%zu = nl_b_%zu_%zu;\n"
                   "NL_NODE_RELOAN(nl_ref_%zu); (void)nl_ref_%zu; }\n",
                   reference, src->frame, src->symbol, reference, reference);
    }
    if (l->from_ptr || tail->kind != NL_CHECKED_PTR_FROM_REF ||
        tail->type != e->ptr || tail->argument_count != 1 ||
        tail->result_count != 1 || !tail->has_reference_result ||
        tail->reference_result.scope != 0 ||
        tail->reference_result.provenance != NL_PROVENANCE_VALID ||
        !tail->reference_result.readable ||
        tail->reference_result.place != l->place ||
        tail->reference_result.incarnation != l->incarnation)
        return reject(e);
    const NLCheckedNodeView *ref = get(env, tail->first_argument);
    NLSemanticTypeView rt;
    if (ref == NULL || ref->kind != NL_CHECKED_IDENTIFIER ||
        ref->symbol != l->ref_symbol ||
        !nl_semantic_type_view(nl_checked_context(env->artifact), ref->type,
                               &rt) ||
        rt.kind != NL_TYPE_REF || rt.target != e->node || rt.is_exclusive ||
        rt.access != NL_ACCESS_READ)
        return reject(e);
    out->type = e->ptr;
    return put(e,
               "{ const nl_node *nl_ref_%zu = &nl_b_%zu_%zu;\n"
               "nl_v_%zu = nl_ref_%zu; }\n",
               reference, src->frame, src->symbol, out->temp, reference);
}
static bool match(Emit *e, Env *env, NLCheckedNodeId id,
                  const NLCheckedNodeView *v, Value *out)
{
    const NLCheckedNodeView *trial = get(env, v->initializer);
    if (trial != NULL && trial->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
        return allocated_match(e, env, id, v, out);
    if (e->custody &&
        (nl_checked_custody_continuation(env->artifact, id, 0) != NULL ||
         v->custody_selected_variant || v->custody_none_site))
        return custody_match(e, env, id, v, out);
    if (v->borrowed_match || v->terminates || v->type != e->unit ||
        v->result_count != 0 || v->item_count != 2 || v->normal_arms != 2 ||
        !v->normal_frame_unchanged || v->match_binding_prefix == 0)
        return reject(e);
    NLSemanticSnapshot snapshot;
    if (!nl_semantic_snapshot(nl_checked_context(env->artifact), &snapshot) ||
        v->match_binding_prefix > snapshot.bindings)
        return reject(e);
    Value scrutinee = {0};
    if (!expr(e, env, v->initializer, &scrutinee) ||
        scrutinee.type != e->option ||
        !put(e, "switch (nl_v_%zu.tag) {\n", scrutinee.temp))
        return reject(e);
    bool seen[2] = {false, false};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm =
            nl_checked_match_arm(env->artifact, id, i);
        const NLCheckedNodeView *a =
            nl_checked_node_view(arm, nl_checked_root(arm));
        if (a == NULL || a->kind != NL_CHECKED_MATCH_ARM || a->terminates ||
            a->type != e->unit || a->result_count != 0 || a->variant < 1 ||
            a->variant > 2 || seen[a->variant - 1] || env->depth >= C_DEPTH)
            return reject(e);
        seen[a->variant - 1] = true;
        Env branch = {.artifact = arm,
                      .parent = env,
                      .prefix = v->match_binding_prefix,
                      .frame = ++e->frames,
                      .depth = env->depth + 1,
                      .backing = env->backing};
        if (!put(e, "case %zu: {\n", a->variant - 1))
            return false;
        if (a->variant == 2) {
            if (a->symbol <= branch.prefix ||
                !add_local(e, &branch, a->symbol, e->ptr, false) ||
                !put(e,
                     "const nl_node *const nl_b_%zu_%zu = nl_v_%zu.ptr;\n"
                     "NL_NODE_ARM(2, nl_b_%zu_%zu);\n"
                     "(void)nl_b_%zu_%zu;\n",
                     branch.frame, a->symbol, scrutinee.temp, branch.frame,
                     a->symbol, branch.frame, a->symbol))
                return reject(e);
        } else {
            if (a->symbol != 0 ||
                !put(e, "NL_NODE_ARM(1, nl_v_%zu.ptr);\n", scrutinee.temp))
                return reject(e);
        }
        Value ignored = {0};
        if (!block(e, &branch, a->initializer, &ignored) ||
            !put(e, "break; }\n"))
            return reject(e);
    }
    out->type = e->unit;
    return put(e, "}\n");
}
static bool expr(Emit *e, Env *env, NLCheckedNodeId id, Value *out)
{
    const NLCheckedNodeView *v = get(env, id);
    if (++e->steps > C_STEPS || env->depth >= C_DEPTH) {
        e->status = NL_NODE_C_RESOURCE_LIMIT;
        return false;
    }
    if (v == NULL || v->terminates || v->result_count > 1)
        return reject(e);
    *out = (Value){.type = v->type};
    if (e->custody &&
        (v->custody_call.entry_proved || v->custody_extraction.present ||
         (v->kind == NL_CHECKED_REPLACE && v->type == e->custody_type) ||
         (v->kind == NL_CHECKED_SUM_CONSTRUCTOR &&
          v->type == e->custody_type) ||
         (v->kind == NL_CHECKED_LOAN_HEADER && v->loan.implicit_local)))
        return custody_expr(e, env, id, v, out);
    if (v->kind == NL_CHECKED_REGISTERED_CALL)
        return v->producer.entry_proved ? producer_call(e, env, id, v, out)
                                        : owner_call(e, env, id, v, out);
    if (v->kind == NL_CHECKED_AGGREGATE_BINDING) {
        const NLCheckedNodeView *init = get(env, v->initializer);
        if (init != NULL && e->live_tail != 0 && init->type == e->live_tail)
            return live_destructure(e, env, v, out);
    }
    if (ctype(e, env, v->type) != NULL &&
        (v->result_count != 1 || v->results[0].type != v->type ||
         v->results[0].value == 0))
        return reject(e);
    if (e->allocated &&
        (v->kind == NL_CHECKED_AGGREGATE_BINDING ||
         v->kind == NL_CHECKED_INTO_SLOT || v->kind == NL_CHECKED_ERASE_SLOT ||
         v->kind == NL_CHECKED_DOMAIN_CREATE ||
         v->kind == NL_CHECKED_DOMAIN_FINALIZE ||
         v->kind == NL_CHECKED_INITIALIZE || v->kind == NL_CHECKED_DESTROY ||
         v->kind == NL_CHECKED_DEALLOCATE ||
         v->kind == NL_CHECKED_REF_FROM_PTR ||
         (v->kind == NL_CHECKED_LOAN_HEADER && !v->loan.implicit_local)))
        return allocated_expr(e, env, v, out);
    if (e->allocated &&
        (v->kind == NL_CHECKED_FIELD_REF || v->kind == NL_CHECKED_LINK_READ ||
         v->kind == NL_CHECKED_REPLACE))
        return heap_link_expr(e, env, v, out);
    if (v->kind == NL_CHECKED_UNIT)
        return v->type == e->unit && v->result_count == 0 ? true : reject(e);
    if (v->kind == NL_CHECKED_BLOCK)
        return reject(e); /* nested root scopes require a separate gate */
    if (v->kind == NL_CHECKED_STATEMENT)
        return expr(e, env, v->initializer, out);
    if (v->kind == NL_CHECKED_MATCH)
        return match(e, env, id, v, out);
    if (v->kind == NL_CHECKED_BINDING) {
        const NLCheckedNodeView *r = get(env, v->first_argument);
        if (v->argument_count != 1 || r == NULL ||
            r->kind != NL_CHECKED_RECEIVER || r->next_argument != 0 ||
            r->symbol != v->symbol ||
            (r->type == e->node && env->parent != NULL && !e->allocated))
            return reject(e);
        Value value = {0};
        if (!expr(e, env, v->initializer, &value) || r->type != value.type ||
            ctype(e, env, value.type) == NULL ||
            !add_local(e, env, r->symbol, r->type, r->type == e->node))
            return reject(e);
        if (e->allocated && r->type == e->ptr && env->heap_place != 0 &&
            env->heap_symbol == 0)
            env->heap_symbol = r->symbol;
        if (r->type == e->node && ++e->roots > 2)
            return reject(e);
        if (e->allocated && r->type == e->option && ++e->option_bindings > 3)
            return reject(e);
        const size_t kind = r->type == e->node     ? 1
                            : r->type == e->ptr    ? 2
                            : r->type == e->option ? 3
                                                   : 4;
        out->type = e->unit;
        return put(e,
                   "%s%s%s nl_b_%zu_%zu = nl_v_%zu;\n"
                   "(void)nl_b_%zu_%zu;\n"
                   "NL_NODE_BIND(%zu, %zu, %zu, &nl_b_%zu_%zu);\n",
                   e->allocated || r->type == e->node || r->type == e->ptr
                       ? ""
                       : "const ",
                   ctype(e, env, r->type), r->type == e->ptr ? " const" : "",
                   env->frame, r->symbol, value.temp, env->frame, r->symbol,
                   env->frame, r->symbol, kind, env->frame, r->symbol);
    }
    if (v->kind == NL_CHECKED_AGGREGATE) {
        if ((env->parent != NULL && !e->allocated) || v->argument_count != 2 ||
            !shape(e, env, v->type))
            return reject(e);
        Value fields[2] = {0};
        bool seen[2] = {false, false};
        NLCheckedNodeId item = v->first_argument;
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedNodeView *f = get(env, item);
            if (f == NULL || f->kind != NL_CHECKED_AGGREGATE_FIELD ||
                f->field_index >= 2 || seen[f->field_index])
                return reject(e);
            seen[f->field_index] = true;
            const NLCheckedNodeView *init = get(env, f->initializer);
            if (f->field_index == 0 &&
                (init == NULL || init->kind != NL_CHECKED_SUM_CONSTRUCTOR ||
                 init->variant != 1 || init->type != e->option))
                return reject(e);
            if (!expr(e, env, f->initializer, &fields[f->field_index]))
                return false;
            item = f->next_argument;
        }
        if (item != 0 || fields[0].type != e->option || fields[1].type != e->u8)
            return reject(e);
        out->temp = ++e->temp;
        return put(e, "nl_node nl_v_%zu = { nl_v_%zu, nl_v_%zu };\n", out->temp,
                   fields[0].temp, fields[1].temp);
    }
    if (v->kind == NL_CHECKED_IDENTIFIER) {
        const Local *l = local(env, v->symbol, v->type);
        if (l == NULL ||
            (v->value_use != NL_VALUE_COPIED &&
             !(e->allocated && v->value_use == NL_VALUE_CONSUMED)) ||
            ctype(e, env, v->type) == NULL)
            return reject(e);
        NLSemanticTypeView t;
        NLSemanticBindingView binding;
        if (!nl_semantic_binding_view(nl_checked_context(env->artifact),
                                      v->symbol, &binding) ||
            (v->value_use == NL_VALUE_CONSUMED &&
             binding.value != v->results[0].value))
            return reject(e);
        if ((!e->allocated && v->type != e->ptr && v->type != e->option &&
             v->type != e->u8) ||
            !nl_semantic_type_view(nl_checked_context(env->artifact), v->type,
                                   &t) ||
            (v->value_use == NL_VALUE_COPIED && !t.is_copy) ||
            (v->value_use == NL_VALUE_CONSUMED && t.is_copy))
            return reject(e);
        if (e->two_heap && v->value_use == NL_VALUE_CONSUMED &&
            (t.kind == NL_TYPE_ALLOCATION ||
             v->type ==
                 nl_semantic_domain_type(nl_checked_context(env->artifact)))) {
            NLSemanticValueView value;
            if (!nl_semantic_value_view(nl_checked_context(env->artifact),
                                        binding.value, &value))
                return reject(e);
            const bool owner = t.kind == NL_TYPE_ALLOCATION;
            const Env *r = owner ? region(env, value.allocation_region)
                                 : domain_heap(env, value.domain);
            if (r == NULL || r->region_slot > 1)
                return reject(e);
            HeapLifecycle *h = &e->lifecycle[r->region_slot];
            size_t *frame = owner ? &h->owner_frame : &h->domain_frame;
            NLSymbolId *carrier = owner ? &h->owner_symbol : &h->domain_symbol;
            if (*frame != l->frame || *carrier != l->symbol)
                return reject(e);
            *frame = 0;
            *carrier = 0;
        }
        out->temp = ++e->temp;
        if (!put(e, "%s nl_v_%zu = nl_b_%zu_%zu;\n", ctype(e, env, v->type),
                 out->temp, l->frame, l->symbol))
            return false;
        return v->value_use != NL_VALUE_CONSUMED ||
               put(e, "nl_b_%zu_%zu = (%s){0};\n", l->frame, l->symbol,
                   ctype(e, env, v->type));
    }
    if (v->kind == NL_CHECKED_U8_LITERAL) {
        if (v->type != e->u8 || !v->has_scalar_result ||
            v->scalar_result.type != e->u8 || !v->scalar_result.known ||
            v->scalar_result.value > 255)
            return reject(e);
        out->temp = ++e->temp;
        return put(e, "uint8_t nl_v_%zu = %zu;\n", out->temp,
                   v->scalar_result.value);
    }
    if (v->kind == NL_CHECKED_SUM_CONSTRUCTOR) {
        if (v->type != e->option || v->result_count != 1 ||
            v->results[0].type != e->option || v->variant < 1 || v->variant > 2)
            return reject(e);
        Value pointer = {0};
        if (v->variant == 2) {
            if (!expr(e, env, v->initializer, &pointer) ||
                pointer.type != e->ptr)
                return reject(e);
        } else if (v->initializer != 0)
            return reject(e);
        out->temp = ++e->temp;
        return v->variant == 1
                   ? put(e, "nl_node_option nl_v_%zu = { 0, NULL };\n",
                         out->temp)
                   : put(e, "nl_node_option nl_v_%zu = { 1, nl_v_%zu };\n",
                         out->temp, pointer.temp);
    }
    if (v->kind == NL_CHECKED_FIELD_READ) {
        if (v->type != e->option || v->result_count != 1 ||
            !field(e, env, &v->field, NL_ACCESS_READ))
            return reject(e);
        const Local *l = local(env, v->field.base, e->node);
        out->temp = ++e->temp;
        return put(e, "nl_node_option nl_v_%zu = nl_b_%zu_%zu.f0;\n", out->temp,
                   l->frame, l->symbol);
    }
    if (v->kind == NL_CHECKED_LOAN_HEADER) {
        /* Staged outer carrier lives beyond the operation's C scope. */
        size_t slot = ++e->temp;
        if (v->type != e->unit &&
            (ctype(e, env, v->type) == NULL ||
             !put(e, "%s nl_v_%zu;\n", ctype(e, env, v->type), slot)))
            return reject(e);
        Value result = {.temp = slot};
        if (!loan(e, env, v, &result))
            return false;
        *out = result;
        return true;
    }
    return reject(e);
}
/* Allocated profile shares the lexical Node representation, expression carriers
 * and prefix-fenced environments. No source/AST access or final-state liveness
 * replay. New branch-local IDs are resolved only in their owned artifact. */
static bool range_value(const Env *env, NLBackingRegionId backing,
                        NLValueId value)
{
    NLSemanticValueView v;
    return nl_semantic_value_view(nl_checked_context(env->artifact), value,
                                  &v) &&
           v.occupancy.region == backing && v.occupancy.start == 0 &&
           v.occupancy.length == 24;
}
static const Env *heap(const Env *env, const NLCheckedNodeView *v)
{
    if (env->heap_place == v->lifetime_place &&
        env->heap_incarnation == v->lifetime_incarnation &&
        env->heap_domain == v->lifetime_domain)
        return env;
    if (env->parent == NULL)
        return NULL;
    if (env->artifact != env->parent->artifact &&
        (env->parent->heap_symbol == 0 ||
         env->parent->heap_symbol > env->prefix))
        return NULL;
    return heap(env->parent, v);
}
/* A backing carrier is inherited only through a checked ancestor prefix.
 * Equal arm-local numeric IDs never search sibling environments. */
static const Env *region(const Env *env, NLBackingRegionId backing)
{
    if (backing != 0 && env->backing == backing)
        return env;
    if (env->parent == NULL || (env->artifact != env->parent->artifact &&
                                (env->parent->heap_symbol == 0 ||
                                 env->parent->heap_symbol > env->prefix)))
        return NULL;
    return region(env->parent, backing);
}
static const Env *domain_heap(const Env *env, NLDomainId domain)
{
    if (domain != 0 && env->heap_domain == domain)
        return env;
    if (env->parent == NULL || (env->artifact != env->parent->artifact &&
                                (env->parent->heap_symbol == 0 ||
                                 env->parent->heap_symbol > env->prefix)))
        return NULL;
    return domain_heap(env->parent, domain);
}
static bool stage(Emit *e, const Env *env, unsigned before, unsigned after)
{
    if (!e->two_heap)
        return true;
    if (env == NULL || env->region_slot > 1 ||
        e->lifecycle[env->region_slot].stage != before)
        return reject(e);
    e->lifecycle[env->region_slot].stage = after;
    return true;
}
/* Existing immutable captured-post certificate only. Closed claims select
 * ancestor carriers; they never generate implicit runtime cleanup. */
static bool closed_capture(Emit *e, const Env *env, const NLCheckedNodeView *v,
                           const NLSemanticContext *post)
{
    NLSemanticBackingView r;
    NLSemanticDomainView d;
    NLSemanticPlaceView p;
    NLSemanticBindingView a, life;
    NLSemanticSnapshot snapshot;
    NLSemanticValueView owner, domain;
    const Env *h =
        heap(env, &(NLCheckedNodeView){.lifetime_place = v->captured_root,
                                       .lifetime_incarnation =
                                           v->captured_incarnation,
                                       .lifetime_domain = v->captured_domain});
    const Local *allocation =
        post == NULL ? NULL
                     : local(env, v->captured_allocation,
                             nl_semantic_core_type(post, NL_TYPE_ALLOCATION));
    const Local *domain_carrier = post == NULL
                                      ? NULL
                                      : local(env, v->captured_domain_binding,
                                              nl_semantic_domain_type(post));
    if (h == NULL || h->region_slot > 1 || allocation == NULL ||
        domain_carrier == NULL)
        return false;
    const HeapLifecycle current = e->lifecycle[h->region_slot];
    return current.stage == 4 && current.owner_symbol == allocation->symbol &&
           current.owner_frame == allocation->frame &&
           current.domain_symbol == domain_carrier->symbol &&
           current.domain_frame == domain_carrier->frame && post != NULL &&
           nl_semantic_snapshot(post, &snapshot) &&
           v->match_binding_prefix == snapshot.bindings &&
           v->captured_frame_closed && !v->normal_frame_unchanged &&
           h->backing == v->captured_backing &&
           v->captured_allocation <= v->match_binding_prefix &&
           v->captured_domain_binding <= v->match_binding_prefix &&
           nl_semantic_backing_view(post, h->backing, &r) && !r.live &&
           nl_semantic_domain_view(post, h->heap_domain, &d) && !d.live &&
           nl_semantic_place_view(post, h->heap_place, &p) && !p.live &&
           p.incarnation == h->heap_incarnation && p.governing_domain == 0 &&
           p.placement.region == 0 &&
           nl_semantic_binding_view(post, v->captured_allocation, &a) &&
           nl_semantic_binding_view(post, v->captured_domain_binding, &life) &&
           a.availability == NL_CONSUMED && life.availability == NL_CONSUMED &&
           nl_semantic_value_view(post, a.value, &owner) &&
           owner.allocation_region == h->backing &&
           owner.dependencies == NL_DEPENDENCY_FREE &&
           owner.value_dependency_count == 0 &&
           nl_semantic_value_view(post, life.value, &domain) &&
           domain.domain == h->heap_domain;
}
static bool closed_arm(const NLCheckedNodeView *v, const NLSemanticContext *c)
{
    NLSemanticBackingView r;
    NLSemanticDomainView d;
    NLSemanticPlaceView p;
    NLSemanticBindingView a, life;
    NLSemanticValueView owner;
    return nl_semantic_backing_view(c, v->captured_backing, &r) && !r.live &&
           nl_semantic_domain_view(c, v->captured_domain, &d) && !d.live &&
           nl_semantic_place_view(c, v->captured_root, &p) && !p.live &&
           p.incarnation == v->captured_incarnation && p.current_value == 0 &&
           p.current_fact == 0 && p.placement.region == 0 &&
           p.governing_domain == 0 &&
           nl_semantic_binding_view(c, v->captured_allocation, &a) &&
           nl_semantic_binding_view(c, v->captured_domain_binding, &life) &&
           a.availability == NL_CONSUMED && life.availability == NL_CONSUMED &&
           nl_semantic_value_view(c, a.value, &owner) &&
           owner.dependencies == NL_DEPENDENCY_FREE &&
           owner.allocation_region == v->captured_backing;
}
/* Point-in-time certificates select carriers; final liveness is not replayed.
 * Historical child/parent identity is inspectable even after EndRoot. */
static bool heap_projection(Emit *e, const Env *env, const NLCheckedNodeView *v)
{
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    const NLCheckedField f = v->field;
    const NLCheckedNodeView *a = get(env, v->first_argument);
    NLSemanticTypeView pt, t;
    NLSemanticValueView p, r;
    NLSemanticPlaceView child;
    if (!e->heap_link || (!e->two_heap && e->destroys != 0) || !f.present ||
        !f.dependency_compatible || f.nominal != e->node ||
        f.type != e->option || f.index != 0 || f.parent_fact == 0 ||
        f.child_fact == 0 || f.old_value != e->link_value ||
        f.payload_occurrence != e->occurrence ||
        (e->root_fact != 0 && f.parent_fact != e->root_fact) ||
        (e->link_fact != 0 && f.child_fact != e->link_fact) ||
        heap(env, v) == NULL || f.parent != v->lifetime_place ||
        f.parent_incarnation != v->lifetime_incarnation ||
        v->lifetime_domain != env->stability_domain ||
        v->lifetime_range.region != env->backing ||
        v->lifetime_range.start != 0 || v->lifetime_range.length != 24 ||
        !nl_semantic_place_view(c, f.child, &child) ||
        child.parent_aggregate != f.parent ||
        child.parent_incarnation != f.parent_incarnation ||
        child.parent_field_index != f.index || child.type != f.type ||
        child.incarnation != f.child_incarnation ||
        child.governing_domain != v->lifetime_domain ||
        (e->link_place != 0 && (f.child != e->link_place ||
                                f.child_incarnation != e->link_incarnation)) ||
        v->argument_count != 1 || a == NULL || a->next_argument != 0 ||
        a->kind != NL_CHECKED_IDENTIFIER || a->symbol != f.base ||
        a->value_use != NL_VALUE_COPIED ||
        local(env, a->symbol, a->type) == NULL ||
        !nl_semantic_type_view(c, a->type, &pt) || pt.kind != NL_TYPE_REF ||
        pt.is_exclusive || pt.target != e->node || pt.access != f.access ||
        !nl_semantic_type_view(c, v->type, &t) || t.kind != NL_TYPE_REF ||
        t.is_exclusive || t.target != e->option || t.access != f.access ||
        !selected_reference(env, a, &p) ||
        !nl_semantic_value_view(c, v->results[0].value, &r) ||
        p.dependencies != NL_DEPENDENCY_FREE ||
        r.dependencies != p.dependencies || p.value_dependency_count != 0 ||
        r.value_dependency_count != 0 || p.reference_count != 0 ||
        r.reference_count != 0 || p.reference.place != f.parent ||
        p.reference.incarnation != f.parent_incarnation ||
        p.reference.scope != env->stability_scope ||
        p.reference.provenance != NL_PROVENANCE_VALID ||
        !p.reference.readable ||
        p.reference.writable != (f.access == NL_ACCESS_WRITE) ||
        r.reference.place != f.child ||
        r.reference.incarnation != f.child_incarnation ||
        r.reference.scope != p.reference.scope ||
        r.reference.provenance != p.reference.provenance ||
        r.reference.readable != p.reference.readable ||
        r.reference.writable != p.reference.writable ||
        r.reference.occurrence_dependency !=
            p.reference.occurrence_dependency ||
        !v->has_reference_result ||
        !same_reference(v->reference_result, r.reference))
        return reject(e);
    e->link_place = f.child;
    e->link_incarnation = f.child_incarnation;
    e->root_fact = f.parent_fact;
    e->link_fact = f.child_fact;
    return true;
}
static bool heap_link_expr(Emit *e, Env *env, const NLCheckedNodeView *v,
                           Value *out)
{
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    if (v->kind == NL_CHECKED_FIELD_REF) {
        Value parent = {0};
        if (e->projections >= 3 || !heap_projection(e, env, v) ||
            !expr(e, env, v->first_argument, &parent))
            return reject(e);
        ++e->projections;
        out->temp = ++e->temp;
        return put(e,
                   "%s nl_v_%zu = &nl_v_%zu->f0;\n"
                   "NL_HEAP_FIELD(nl_v_%zu,nl_v_%zu,%u,%zu,%zu,%zu,%zu,%zu);\n",
                   ctype(e, env, v->type), out->temp, parent.temp, parent.temp,
                   out->temp, v->field.access == NL_ACCESS_WRITE ? 1u : 0u,
                   v->field.nominal, v->field.index, v->field.child,
                   v->field.child_incarnation, v->reference_result.scope);
    }
    const NLCheckedNodeView *ref = get(env, v->first_argument);
    if (ref == NULL || ref->kind != NL_CHECKED_FIELD_REF ||
        v->type != e->option || v->result_count != 1 ||
        v->field.old_value != e->link_value ||
        ref->field.old_value != v->field.old_value || !v->field.present ||
        !v->field.dependency_compatible ||
        ref->field.nominal != v->field.nominal ||
        ref->field.type != v->field.type ||
        ref->field.index != v->field.index ||
        ref->field.access != v->field.access ||
        ref->field.parent_incarnation != v->field.parent_incarnation ||
        ref->field.child_incarnation != v->field.child_incarnation ||
        ref->field.parent_fact != v->field.parent_fact ||
        ref->field.child_fact != v->field.child_fact ||
        ref->field.payload_occurrence != v->field.payload_occurrence ||
        ref->field.parent != v->field.parent ||
        ref->field.child != v->field.child || ref->field.base != v->field.base)
        return reject(e);
    Value projected = {0};
    if (!expr(e, env, v->first_argument, &projected))
        return false;
    if (v->kind == NL_CHECKED_LINK_READ) {
        NLSemanticValueView original, copy, p, q;
        if (e->reads++ != 0 || e->link_updates != 1 || v->argument_count != 1 ||
            ref->next_argument != 0 || v->value_use != NL_VALUE_COPIED ||
            !nl_semantic_value_view(c, v->field.old_value, &original) ||
            !nl_semantic_value_view(c, v->results[0].value, &copy) ||
            original.variant != 2 || copy.variant != original.variant ||
            copy.type != e->option || copy.dependencies != NL_DEPENDENCY_FREE ||
            original.dependencies != NL_DEPENDENCY_FREE ||
            !nl_semantic_value_view(c, original.sum_payload, &p) ||
            !nl_semantic_value_view(c, copy.sum_payload, &q) ||
            original.sum_payload == copy.sum_payload || p.type != e->ptr ||
            q.type != e->ptr || !same_reference(p.reference, q.reference) ||
            q.reference.scope != 0 ||
            q.reference.provenance != NL_PROVENANCE_VALID ||
            p.reference_count != 0 || q.reference_count != 0 ||
            p.dependencies != NL_DEPENDENCY_FREE ||
            q.dependencies != NL_DEPENDENCY_FREE)
            return reject(e);
        out->temp = ++e->temp;
        return put(e,
                   "nl_node_option nl_v_%zu = *nl_v_%zu;\n"
                   "NL_HEAP_COPY(nl_v_%zu,&nl_v_%zu);\n",
                   out->temp, projected.temp, projected.temp, out->temp);
    }
    const NLCheckedField f = v->field;
    const NLCheckedNodeView *next = get(env, ref->next_argument);
    NLSemanticOccurrenceView occurrence;
    NLSemanticValueView old;
    if (v->kind != NL_CHECKED_REPLACE || v->argument_count != 2 ||
        f.access != NL_ACCESS_WRITE || ref->field.access != NL_ACCESS_WRITE ||
        !f.dependency_compatible || f.nominal != e->node || f.index != 0 ||
        f.parent_fact != e->root_fact || f.child_fact != e->link_fact ||
        f.parent_incarnation != ref->field.parent_incarnation ||
        f.child_incarnation != ref->field.child_incarnation ||
        f.payload_occurrence != e->occurrence || f.parent_post_fact == 0 ||
        f.child_post_fact == 0 || f.parent_post_fact == f.parent_fact ||
        f.child_post_fact == f.child_fact ||
        f.old_value != v->results[0].value || next == NULL ||
        next->kind != NL_CHECKED_SUM_CONSTRUCTOR || next->next_argument != 0 ||
        next->results[0].value != f.new_value || e->link_updates >= 2 ||
        next->variant != (e->link_updates == 0 ? 2u : 1u) ||
        !nl_semantic_value_view(c, f.old_value, &old) ||
        old.variant != (e->link_updates == 0 ? 1u : 2u) ||
        (e->link_updates == 0 ? f.post_payload_occurrence == 0
                              : f.post_payload_occurrence != 0) ||
        (e->link_updates == 1 && e->reads != 1) ||
        !nl_semantic_occurrence_view(c,
                                     e->link_updates == 0
                                         ? f.post_payload_occurrence
                                         : f.payload_occurrence,
                                     &occurrence) ||
        occurrence.root != f.child || occurrence.variant != 2)
        return reject(e);
    Value replacement = {0};
    if (!expr(e, env, ref->next_argument, &replacement))
        return false;
    e->link_value = f.new_value;
    e->root_fact = f.parent_post_fact;
    e->link_fact = f.child_post_fact;
    e->occurrence = f.post_payload_occurrence;
    ++e->link_updates;
    out->temp = ++e->temp;
    return put(e,
               "nl_node_option nl_v_%zu = *nl_v_%zu;\n"
               "*nl_v_%zu = nl_v_%zu;\n"
               "NL_HEAP_CHANGE(nl_v_%zu,&nl_v_%zu,%zu);\n",
               out->temp, projected.temp, projected.temp, replacement.temp,
               projected.temp, out->temp, e->link_updates);
}
static bool allocated_body(Emit *e, Env *env, NLCheckedNodeId id, Value *out)
{
    const NLCheckedNodeView *body = get(env, id);
    if (body == NULL || body->kind != NL_CHECKED_BLOCK || body->terminates ||
        body->result_count > 1 || env->depth >= C_DEPTH)
        return reject(e);
    NLCheckedNodeId item = body->first_item;
    for (size_t i = 0; i < body->item_count; ++i) {
        const NLCheckedNodeView *op = get(env, item);
        Value ignored = {0};
        if (op == NULL || !expr(e, env, item, &ignored))
            return reject(e);
        if (ignored.temp != 0 && !put(e, "(void)nl_v_%zu;\n", ignored.temp))
            return false;
        item = op->next_item;
    }
    return item == 0 && expr(e, env, body->tail, out) &&
           out->type == body->type;
}
static bool allocated_match(Emit *e, Env *env, NLCheckedNodeId id,
                            const NLCheckedNodeView *v, Value *out)
{
    const NLCheckedNodeView *trial = get(env, v->initializer);
    if (!e->allocated || e->allocations >= (e->two_heap ? 2u : 1u) ||
        trial == NULL || !trial->allocation_trial || trial->result_count != 0 ||
        trial->backing != 0 || trial->allocation_authority != 0 ||
        trial->storage_authority != 0 || trial->allocation_size != 24 ||
        trial->allocation_alignment != 8 ||
        !shape(e, env, trial->allocation_target) || v->item_count != 2 ||
        v->normal_arms != 2 ||
        (e->two_heap && e->allocations == 1
             ? !closed_capture(e, env, v,
                               nl_checked_captured_post(env->artifact, id))
             : (!v->normal_frame_unchanged || v->captured_frame_closed)) ||
        v->type != e->unit || v->result_count != 0 || v->terminates)
        return reject(e);
    const size_t site = e->allocations++;
    HeapLifecycle incoming[2] = {e->lifecycle[0], e->lifecycle[1]};
    if (e->two_heap &&
        ((site == 0 && incoming[0].stage != 0) ||
         (site == 1 && (incoming[0].stage != 4 || incoming[1].stage != 0))))
        return reject(e);
    size_t allocation = ++e->temp;
    if (!put(e,
             "void *nl_heap_%zu = "
             "malloc(24);\nNL_HEAP_TRIAL(nl_heap_%zu,24,8);\n",
             allocation, allocation))
        return false;
    bool seen[2] = {false, false};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm =
            nl_checked_match_arm(env->artifact, id, i);
        const NLCheckedNodeView *a =
            nl_checked_node_view(arm, nl_checked_root(arm));
        if (a == NULL || a->kind != NL_CHECKED_MATCH_ARM || a->variant < 1 ||
            a->variant > 2 || seen[a->variant - 1] || a->terminates ||
            a->type != e->unit ||
            (v->normal_frame_unchanged
                 ? !a->normal_frame_unchanged
                 : (a->normal_frame_unchanged ||
                    !closed_arm(v, nl_checked_context(arm)))))
            return reject(e);
        seen[a->variant - 1] = true;
        const NLCheckedNodeView *grant =
            nl_checked_node_view(arm, a->initializer);
        if (grant == NULL || grant->kind != NL_CHECKED_TRY_ALLOCATE_ONE ||
            !grant->allocation_trial ||
            grant->allocation_success != (a->variant == 2) ||
            grant->allocation_target != e->node ||
            grant->allocation_size != 24 || grant->allocation_alignment != 8 ||
            grant->result_count != 1)
            return reject(e);
        e->lifecycle[0] = incoming[0];
        e->lifecycle[1] = incoming[1];
        Env branch = {.artifact = arm,
                      .region_slot = a->variant == 2 ? site : env->region_slot,
                      .frame = ++e->frames,
                      .parent = env,
                      .prefix = v->match_binding_prefix,
                      .depth = env->depth + 1,
                      .backing =
                          a->variant == 2 ? grant->backing : env->backing};
        if (!put(e, "if (nl_heap_%zu %s NULL) {\nNL_HEAP_ARM(%zu);\n",
                 allocation, a->variant == 2 ? "!=" : "==", a->variant))
            return false;
        if (a->variant == 2) {
            NLSemanticBindingView binder;
            NLAggregateField f0, f1;
            NLSemanticValueView owner, raw;
            if (a->symbol <= branch.prefix || grant->backing == 0 ||
                !nl_semantic_binding_view(nl_checked_context(arm), a->symbol,
                                          &binder) ||
                !nl_semantic_aggregate_field_view(nl_checked_context(arm),
                                                  binder.type, 0, &f0) ||
                !nl_semantic_aggregate_field_view(nl_checked_context(arm),
                                                  binder.type, 1, &f1) ||
                f0.type != nl_semantic_core_type(nl_checked_context(arm),
                                                 NL_TYPE_ALLOCATION) ||
                f1.type != nl_semantic_core_type(nl_checked_context(arm),
                                                 NL_TYPE_STORAGE) ||
                !nl_semantic_value_view(nl_checked_context(arm),
                                        grant->allocation_authority, &owner) ||
                owner.allocation_region != grant->backing ||
                !nl_semantic_value_view(nl_checked_context(arm),
                                        grant->storage_authority, &raw) ||
                raw.occupancy.region != grant->backing ||
                raw.occupancy.start != 0 || raw.occupancy.length != 24)
                return reject(e);
            if (e->two_heap && (site == 1 && grant->backing == env->backing))
                return reject(e);
            e->bundle = binder.type;
            if (!stage(e, &branch, 0, 1))
                return false;
            if (!add_local(e, &branch, a->symbol, binder.type, false) ||
                !put(e,
                     "nl_backing nl_b_%zu_%zu = { {nl_heap_%zu}, {(unsigned "
                     "char *)nl_heap_%zu,24} };\n",
                     branch.frame, a->symbol, allocation, allocation))
                return reject(e);
        } else if (a->symbol != 0 || grant->backing != 0 ||
                   grant->allocation_authority != 0 ||
                   grant->storage_authority != 0)
            return reject(e);
        Value result = {0};
        if (!block(e, &branch, a->tail, &result) ||
            (e->two_heap &&
             ((site == 1 && e->lifecycle[0].stage != 8) ||
              (a->variant == 2 && e->lifecycle[site].stage != 8))) ||
            !put(e, "}\n"))
            return reject(e);
    }
    /* Both owned paths were separately validated, then project only the
     * common ancestor carrier lifecycle. No branch-local IDs escape. */
    e->lifecycle[0] = site == 1 ? (HeapLifecycle){.stage = 8} : incoming[0];
    e->lifecycle[1] = incoming[1];
    out->type = e->unit;
    return true;
}
static bool allocated_expr(Emit *e, Env *env, const NLCheckedNodeView *v,
                           Value *out)
{
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    if (v->kind == NL_CHECKED_AGGREGATE_BINDING) {
        if (v->argument_count != 2 || e->bundle == 0)
            return reject(e);
        Value bundle = {0};
        if (!expr(e, env, v->initializer, &bundle) || bundle.type != e->bundle)
            return reject(e);
        NLCheckedNodeId id = v->first_argument;
        bool seen[2] = {false, false};
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedNodeView *r = get(env, id);
            NLTypeId expected = nl_semantic_core_type(
                c, i == 0 ? NL_TYPE_ALLOCATION : NL_TYPE_STORAGE);
            if (r == NULL || r->kind != NL_CHECKED_RECEIVER ||
                r->field_index >= 2 || seen[r->field_index])
                return reject(e);
            expected = nl_semantic_core_type(
                c, r->field_index == 0 ? NL_TYPE_ALLOCATION : NL_TYPE_STORAGE);
            if (r->type != expected ||
                !add_local(e, env, r->symbol, r->type, false))
                return reject(e);
            seen[r->field_index] = true;
            if (!put(e, "%s nl_b_%zu_%zu = nl_v_%zu.%s;\n(void)nl_b_%zu_%zu;\n",
                     ctype(e, env, r->type), env->frame, r->symbol, bundle.temp,
                     r->field_index == 0 ? "allocation" : "raw", env->frame,
                     r->symbol))
                return false;
            id = r->next_argument;
        }
        out->type = e->unit;
        return id == 0 && put(e, "nl_v_%zu = (nl_backing){0};\n", bundle.temp);
    }
    if (v->kind == NL_CHECKED_LOAN_HEADER) {
        const NLCheckedLoanPlan *l = &v->loan;
        NLSemanticBindingView binder;
        NLSemanticTypeView type;
        NLSemanticBindingView db;
        NLSemanticValueView dv, reference;
        const Local *src = local(env, l->source, nl_semantic_domain_type(c));
        if (src == NULL || l->domain == 0 || l->scope == 0 ||
            l->access != NL_ACCESS_READ || !l->body_nonescape_proved ||
            !l->normal_result_forwarded || !l->prevent_lifetime_end ||
            !nl_semantic_binding_view(c, l->ref_symbol, &binder) ||
            !nl_semantic_type_view(c, binder.type, &type) ||
            type.kind != NL_TYPE_REF ||
            type.target != nl_semantic_domain_type(c) ||
            type.is_exclusive != l->is_exclusive ||
            !nl_semantic_binding_view(c, l->source, &db) ||
            !nl_semantic_value_view(c, db.value, &dv) ||
            dv.domain != l->domain ||
            !nl_semantic_value_view(c, binder.value, &reference) ||
            reference.reference.place != db.place ||
            reference.reference.scope != l->scope)
            return reject(e);
        size_t result = 0;
        if (v->type != e->unit) {
            result = ++e->temp;
            if (ctype(e, env, v->type) == NULL ||
                !put(e, "%s nl_v_%zu;\n", ctype(e, env, v->type), result))
                return reject(e);
        }
        const Env *selected = domain_heap(env, l->domain);
        if (e->two_heap) {
            const HeapLifecycle h =
                e->lifecycle[selected == NULL ? env->region_slot
                                              : selected->region_slot];
            if (h.domain_frame != src->frame || h.domain_symbol != src->symbol)
                return reject(e);
        }
        Env body = {.artifact = env->artifact,
                    .parent = env,
                    .prefix = SIZE_MAX,
                    .frame = ++e->frames,
                    .depth = env->depth + 1,
                    .backing =
                        selected == NULL ? env->backing : selected->backing,
                    .region_slot = selected == NULL ? env->region_slot
                                                    : selected->region_slot,
                    .stability_scope = l->scope,
                    .stability_domain = l->domain,
                    .stability_place = db.place};
        if (!add_local(e, &body, l->ref_symbol, binder.type, false) ||
            !put(e,
                 "{ const nl_domain *nl_b_%zu_%zu = "
                 "&nl_b_%zu_%zu;\n(void)nl_b_%zu_%zu;\n",
                 body.frame, l->ref_symbol, src->frame, src->symbol, body.frame,
                 l->ref_symbol))
            return reject(e);
        Value value = {0};
        if (!allocated_body(e, &body, v->initializer, &value) ||
            value.type != v->type)
            return reject(e);
        if (result != 0 &&
            !put(e, "nl_v_%zu = nl_v_%zu;\n", result, value.temp))
            return false;
        if (body.heap_place != 0) {
            env->heap_place = body.heap_place;
            env->heap_incarnation = body.heap_incarnation;
            env->heap_domain = body.heap_domain;
        }
        out->temp = result;
        if (e->heap_link && !l->is_exclusive &&
            !put(e, "NL_HEAP_SCOPE_END(%zu);\n", l->scope))
            return false;
        return put(e, "}\n");
    }
    if (v->kind == NL_CHECKED_DOMAIN_CREATE) {
        NLSemanticValueView domain_value;
        if (!nl_semantic_value_view(c, v->results[0].value, &domain_value) ||
            domain_value.domain != v->lifetime_domain ||
            v->lifetime_domain == 0 || v->type != nl_semantic_domain_type(c) ||
            e->domains >= (e->two_heap ? 2u : 1u) || !stage(e, env, 2, 3))
            return reject(e);
        ++e->domains;
        out->temp = ++e->temp;
        return put(
            e, "nl_domain nl_v_%zu = {%zu};\nNL_HEAP_DOMAIN(nl_v_%zu.token);\n",
            out->temp, v->lifetime_domain, out->temp);
    }
    const NLCheckedNodeView *a = get(env, v->first_argument);
    if (a == NULL || a->kind != NL_CHECKED_IDENTIFIER || a->symbol == 0)
        return reject(e);
    if (v->kind == NL_CHECKED_REF_FROM_PTR) {
        const NLCheckedNodeView *st = get(env, a->next_argument);
        Value p = {0}, stable = {0};
        NLSemanticTypeView t;
        NLSemanticTypeView rt;
        NLSemanticValueView pv, rv, sv;
        NLSemanticBackingView backing;
        const bool write = v->reference_result.writable;
        if (e->reloans == 0)
            e->heap_link = write;
        const Env *target = heap(env, v);
        if ((!e->two_heap && e->destroys != 0) ||
            e->reloans >= (e->two_heap    ? 4u + (e->live_tail != 0 ? 1u : 0u) +
                                                (e->owner_read ? 1u : 0u)
                           : e->heap_link ? 3u
                                          : 1u) ||
            (e->heap_link &&
             write != (e->custody    ? e->reloans < 2
                       : e->two_heap ? (e->reloans == 0 || e->reloans == 3)
                                     : e->reloans != 1)) ||
            v->argument_count != 2 || a->value_use != NL_VALUE_COPIED ||
            a->type != e->ptr || st == NULL ||
            st->value_use != NL_VALUE_COPIED || st->next_argument != 0 ||
            !nl_semantic_type_view(c, st->type, &t) || t.kind != NL_TYPE_REF ||
            t.target != nl_semantic_domain_type(c) || t.is_exclusive ||
            !v->has_reference_result || heap(env, v) == NULL ||
            !nl_semantic_type_view(c, v->type, &rt) || rt.kind != NL_TYPE_REF ||
            rt.is_exclusive || rt.target != e->node ||
            rt.access != (write ? NL_ACCESS_WRITE : NL_ACCESS_READ) ||
            !selected_reference(env, a, &pv) ||
            !selected_reference(env, st, &sv) ||
            !nl_semantic_value_view(c, v->results[0].value, &rv) ||
            pv.dependencies != NL_DEPENDENCY_FREE || pv.reference_count != 0 ||
            sv.dependencies != NL_DEPENDENCY_FREE ||
            rv.dependencies != NL_DEPENDENCY_FREE ||
            pv.reference.provenance != NL_PROVENANCE_VALID ||
            !pv.reference.readable || (write && !pv.reference.writable) ||
            pv.reference.place != v->lifetime_place ||
            pv.reference.incarnation != v->lifetime_incarnation ||
            rv.reference.place != pv.reference.place ||
            rv.reference.incarnation != pv.reference.incarnation ||
            rv.reference.scope != env->stability_scope ||
            rv.reference.scope != sv.reference.scope ||
            sv.reference.place != env->stability_place ||
            rv.reference.scope != v->reference_result.scope ||
            v->lifetime_domain != env->stability_domain ||
            !rv.reference.readable || rv.reference.writable != write ||
            rv.reference.provenance != NL_PROVENANCE_VALID ||
            !same_reference(v->reference_result, rv.reference) ||
            !nl_semantic_backing_view(c, target == NULL ? 0 : target->backing,
                                      &backing) ||
            !backing.ordinary_read || backing.size != 24 ||
            v->lifetime_range.region != target->backing ||
            v->lifetime_range.start != 0 || v->lifetime_range.length != 24 ||
            (write && (!backing.ordinary_write || backing.alignment < 8 ||
                       v->lifetime_range.region != target->backing ||
                       v->lifetime_range.start != 0 ||
                       v->lifetime_range.length != 24)) ||
            !expr(e, env, v->first_argument, &p) ||
            !expr(e, env, a->next_argument, &stable))
            return reject(e);
        ++e->reloans;
        out->temp = ++e->temp;
        if (e->heap_link)
            return put(
                e,
                "%s nl_v_%zu = %snl_v_%zu;\n"
                "NL_HEAP_ROOT(nl_v_%zu,nl_v_%zu->token,%u,%zu,%zu,%zu);\n"
                "(void)nl_v_%zu;\n",
                ctype(e, env, v->type), out->temp, write ? "(nl_node *)" : "",
                p.temp, out->temp, stable.temp, write ? 1u : 0u,
                v->lifetime_place, v->lifetime_incarnation, rv.reference.scope,
                stable.temp);
        return put(e,
                   "const nl_node *nl_v_%zu = "
                   "nl_v_%zu;\nNL_HEAP_RELOAN(nl_v_%zu,nl_v_%zu->token);\n("
                   "void)nl_v_%zu;\n",
                   out->temp, p.temp, out->temp, stable.temp, stable.temp);
    }
    if (v->kind == NL_CHECKED_INITIALIZE) {
        const NLCheckedNodeView *n = get(env, a->next_argument),
                                *st = n == NULL ? NULL
                                                : get(env, n->next_argument);
        Value slot = {0}, node = {0}, stable = {0};
        NLSemanticTypeView t;
        NLSemanticValueView pointer, stability;
        if (!v->has_reference_result ||
            !nl_semantic_value_view(c, v->results[0].value, &pointer) ||
            !same_reference(pointer.reference, v->reference_result) ||
            pointer.reference.place != v->lifetime_place ||
            pointer.reference.incarnation != v->lifetime_incarnation ||
            pointer.reference.provenance != NL_PROVENANCE_VALID ||
            pointer.reference.scope != 0 ||
            pointer.dependencies != NL_DEPENDENCY_FREE ||
            v->argument_count != 3 || n == NULL || st == NULL ||
            st->next_argument != 0 ||
            !selected_reference(env, st, &stability) ||
            stability.reference.scope != env->stability_scope ||
            stability.reference.place != env->stability_place ||
            !nl_semantic_type_view(c, a->type, &t) || t.kind != NL_TYPE_SLOT ||
            t.target != e->node ||
            !range_value(env, env->backing, a->results[0].value) ||
            v->type != e->ptr || v->backing != env->backing ||
            v->lifetime_range.region != env->backing ||
            v->lifetime_range.length != 24 || v->lifetime_range.start != 0 ||
            v->lifetime_domain == 0 ||
            v->lifetime_domain != env->stability_domain ||
            v->lifetime_place == 0 || v->lifetime_incarnation == 0 ||
            ++e->roots > 2 || env->heap_place != 0 ||
            !expr(e, env, v->first_argument, &slot) ||
            !expr(e, env, a->next_argument, &node) || node.type != e->node ||
            !expr(e, env, n->next_argument, &stable))
            return reject(e);
        env->heap_place = v->lifetime_place;
        env->heap_incarnation = v->lifetime_incarnation;
        env->heap_domain = v->lifetime_domain;
        NLSemanticValueView initial;
        if (!nl_semantic_value_view(c, n->results[0].value, &initial) ||
            initial.field_count != 2)
            return reject(e);
        if (!e->two_heap || env->region_slot == 0)
            e->link_value = initial.fields[0];
        if (!stage(e, env, 3, 4))
            return false;
        out->temp = ++e->temp;
        return put(e,
                   "*(nl_node *)nl_v_%zu.bytes = nl_v_%zu;\nconst nl_node "
                   "*nl_v_%zu = (nl_node "
                   "*)nl_v_%zu.bytes;\nNL_HEAP_INITIALIZE(nl_v_%zu,nl_v_%zu->"
                   "token);\n(void)nl_v_%zu;\n",
                   slot.temp, node.temp, out->temp, slot.temp, out->temp,
                   stable.temp, stable.temp);
    }
    if (v->kind == NL_CHECKED_DESTROY) {
        const NLCheckedNodeView *ending = get(env, a->next_argument);
        Value p = {0};
        NLSemanticValueView pointer;
        NLSemanticTypeView ending_type;
        NLSemanticBindingView ending_binding;
        NLSemanticValueView ending_value;
        const Local *authority =
            ending == NULL ? NULL : local(env, ending->symbol, ending->type);
        if (v->argument_count != 2 || ending == NULL ||
            ending->next_argument != 0 ||
            ending->value_use != NL_VALUE_REBORROWED || authority == NULL ||
            a->type != e->ptr || heap(env, v) == NULL ||
            !selected_reference(env, a, &pointer) ||
            pointer.reference.place != v->lifetime_place ||
            pointer.reference.incarnation != v->lifetime_incarnation ||
            pointer.reference.provenance != NL_PROVENANCE_VALID ||
            v->lifetime_domain != env->stability_domain ||
            v->lifetime_range.region != heap(env, v)->backing ||
            v->lifetime_range.start != 0 || v->lifetime_range.length != 24 ||
            !range_value(env, heap(env, v)->backing, v->results[0].value) ||
            !nl_semantic_type_view(c, ending->type, &ending_type) ||
            ending_type.kind != NL_TYPE_REF || !ending_type.is_exclusive ||
            ending_type.target != nl_semantic_domain_type(c) ||
            ending_type.access != NL_ACCESS_READ ||
            !nl_semantic_binding_view(c, ending->symbol, &ending_binding) ||
            !nl_semantic_value_view(c, ending_binding.value, &ending_value) ||
            ending_value.reference.scope != env->stability_scope ||
            ending_value.reference.place != env->stability_place ||
            ending_value.dependencies != NL_DEPENDENCY_FREE ||
            (!e->two_heap && (e->link_updates != 2 || e->destroys != 0)) ||
            !stage(e, heap(env, v), 4, 5) ||
            !expr(e, env, v->first_argument, &p))
            return reject(e);
        ++e->destroys;
        out->temp = ++e->temp;
        return put(e,
                   "NL_HEAP_END(nl_v_%zu,nl_b_%zu_%zu->token);\nnl_slot "
                   "nl_v_%zu = {(unsigned char *)nl_v_%zu,24};\n",
                   p.temp, authority->frame, authority->symbol, out->temp,
                   p.temp);
    }
    if (v->kind == NL_CHECKED_INTO_SLOT || v->kind == NL_CHECKED_ERASE_SLOT) {
        Value raw = {0};
        NLSemanticTypeView t;
        NLSemanticValueView source;
        if (!nl_semantic_value_view(c, a->results[0].value, &source))
            return reject(e);
        const Env *r = region(env, source.occupancy.region);
        if (r == NULL ||
            (!e->two_heap &&
             (v->kind == NL_CHECKED_INTO_SLOT ? e->slots != 0
                                              : e->erased != 0)) ||
            !stage(e, r, v->kind == NL_CHECKED_INTO_SLOT ? 1u : 5u,
                   v->kind == NL_CHECKED_INTO_SLOT ? 2u : 6u))
            return reject(e);
        NLTypeId slot = v->kind == NL_CHECKED_INTO_SLOT ? v->type : a->type;
        if (v->argument_count != 1 || a->next_argument != 0 ||
            !nl_semantic_type_view(c, slot, &t) || t.kind != NL_TYPE_SLOT ||
            t.target != e->node ||
            !range_value(env, r->backing, a->results[0].value) ||
            !expr(e, env, v->first_argument, &raw))
            return reject(e);
        if (v->kind == NL_CHECKED_INTO_SLOT)
            ++e->slots;
        else
            ++e->erased;
        out->temp = ++e->temp;
        return put(e,
                   "%s nl_v_%zu = "
                   "{nl_v_%zu.bytes,nl_v_%zu.length};\nNL_HEAP_%s(nl_v_%zu."
                   "bytes,nl_v_%zu.length);\n",
                   ctype(e, env, v->type), out->temp, raw.temp, raw.temp,
                   v->kind == NL_CHECKED_INTO_SLOT ? "SLOT" : "RAW", out->temp,
                   out->temp);
    }
    if (v->kind == NL_CHECKED_DOMAIN_FINALIZE) {
        Value domain = {0};
        NLSemanticValueView d;
        if (!nl_semantic_value_view(c, a->results[0].value, &d))
            return reject(e);
        const Env *r = domain_heap(env, d.domain);
        if ((!e->two_heap && (e->finalized != 0 || e->destroys != 1)) ||
            !stage(e, r, 6, 7) || v->argument_count != 1 ||
            a->next_argument != 0 || a->type != nl_semantic_domain_type(c) ||
            !expr(e, env, v->first_argument, &domain))
            return reject(e);
        ++e->finalized;
        return put(e, "NL_HEAP_FINALIZE(nl_v_%zu.token);\n(void)nl_v_%zu;\n",
                   domain.temp, domain.temp);
    }
    if (v->kind == NL_CHECKED_DEALLOCATE) {
        const NLCheckedNodeView *raw = get(env, a->next_argument);
        NLSemanticValueView owner;
        Value allocation = {0}, storage = {0};
        if (v->argument_count != 2 || raw == NULL || raw->next_argument != 0 ||
            !nl_semantic_value_view(c, a->results[0].value, &owner) ||
            region(env, owner.allocation_region) == NULL ||
            !range_value(env, owner.allocation_region, raw->results[0].value) ||
            (!e->two_heap && (e->destroys != 1 || e->releases != 0)) ||
            !stage(e, region(env, owner.allocation_region), 7, 8) ||
            !expr(e, env, v->first_argument, &allocation) ||
            !expr(e, env, a->next_argument, &storage))
            return reject(e);
        ++e->releases;
        return put(e,
                   "NL_HEAP_RELEASE(nl_v_%zu.handle,nl_v_%zu.bytes,nl_v_%zu."
                   "length);\nfree(nl_v_%zu.handle);\nnl_v_%zu.handle=NULL;"
                   "\nnl_v_%zu.bytes=NULL;\n(void)nl_v_%zu;\n",
                   allocation.temp, storage.temp, storage.temp, allocation.temp,
                   allocation.temp, storage.temp, storage.temp);
    }
    return reject(e);
}

/* Exact two-item producer body. Its scope/relational certificate is checked
 * first; this traversal validates each actual operand and emits in checked
 * source order. It is deliberately not a general aggregate/return backend. */
static bool producer_call(Emit *e, Env *env, NLCheckedNodeId id,
                          const NLCheckedNodeView *v, Value *out)
{
    const NLCheckedFragment *f = nl_checked_call_body(env->artifact, id);
    const NLCheckedNodeView *body = nl_checked_node_view(f, nl_checked_root(f));
    const Env *tail = region(env, v->producer.range.region);
    if (!e->two_heap || e->live_tail == 0 || v->type != e->live_tail ||
        e->producer_calls != 0 || e->owner_calls != 0 ||
        !nl_checked_producer_valid(env->artifact, id) ||
        !v->producer.head.dependency_compatible || tail == NULL ||
        tail->region_slot != 1 || e->lifecycle[1].stage != 4 ||
        tail->heap_place != v->producer.root ||
        tail->heap_incarnation != v->producer.incarnation ||
        tail->heap_domain != v->producer.domain || e->link_updates != 1 ||
        e->reloans != (e->custody ? 2u : 4u) ||
        e->reads != (e->custody ? 0u : 1u) ||
        e->projections != (e->custody ? 1u : 2u) || body == NULL ||
        body->kind != NL_CHECKED_BLOCK || !body->terminates ||
        body->item_count != 2 || body->tail != 0 ||
        body->returned.type != e->live_tail ||
        body->returned.value != v->producer.result)
        return reject(e);
    const NLCheckedNodeView *binding =
        nl_checked_node_view(f, body->first_item);
    const NLCheckedNodeView *ret =
        binding == NULL ? NULL : nl_checked_node_view(f, binding->next_item);
    const NLCheckedNodeView *replace =
        binding == NULL ? NULL : nl_checked_node_view(f, binding->initializer);
    const NLCheckedNodeView *aggregate =
        ret == NULL ? NULL : nl_checked_node_view(f, ret->initializer);
    if (binding == NULL || binding->kind != NL_CHECKED_BINDING ||
        binding->argument_count != 1 || ret == NULL ||
        ret->kind != NL_CHECKED_RETURN || !ret->terminates ||
        ret->next_item != 0 || ret->returned.type != e->live_tail ||
        ret->returned.value != v->producer.result || replace == NULL ||
        replace->kind != NL_CHECKED_REPLACE || replace->argument_count != 2 ||
        replace->type != e->option || replace->result_count != 1 ||
        aggregate == NULL || aggregate->kind != NL_CHECKED_AGGREGATE ||
        aggregate->type != e->live_tail || aggregate->argument_count != 3 ||
        aggregate->result_count != 1 ||
        aggregate->results[0].value != v->producer.result)
        return reject(e);
    const NLCheckedNodeView *receiver =
        nl_checked_node_view(f, binding->first_argument);
    if (receiver == NULL || receiver->kind != NL_CHECKED_RECEIVER ||
        receiver->symbol != binding->symbol || receiver->type != e->option ||
        receiver->next_argument != 0 || binding->type != e->unit)
        return reject(e);
    NLSemanticValueView package;
    if (!nl_semantic_value_view(nl_checked_producer_return(env->artifact, id),
                                v->producer.result, &package) ||
        package.field_count != 3)
        return reject(e);
    const NLCheckedNodeView *ref =
        nl_checked_node_view(f, replace->first_argument);
    const NLCheckedNodeView *none =
        ref == NULL ? NULL : nl_checked_node_view(f, ref->next_argument);
    const NLCheckedField field = replace->field;
    if (ref == NULL || ref->kind != NL_CHECKED_IDENTIFIER ||
        ref->symbol != v->producer.parameters[0] ||
        ref->value_use != NL_VALUE_COPIED || none == NULL ||
        none->kind != NL_CHECKED_SUM_CONSTRUCTOR || none->variant != 1 ||
        none->type != e->option || none->next_argument != 0 || !field.present ||
        !field.dependency_compatible || field.nominal != e->node ||
        field.type != e->option || field.index != 0 ||
        field.access != NL_ACCESS_WRITE ||
        field.parent != v->producer.head.parent ||
        field.child != v->producer.head.child ||
        field.parent_incarnation != v->producer.head.parent_incarnation ||
        field.child_incarnation != v->producer.head.child_incarnation ||
        field.parent_fact != e->root_fact || field.child_fact != e->link_fact ||
        field.old_value != e->link_value ||
        field.old_value != v->producer.head_before ||
        field.child_fact != v->producer.head_before_fact ||
        field.new_value != v->producer.head_after ||
        field.child_post_fact != v->producer.head_after_fact ||
        field.parent_post_fact == 0 ||
        field.parent_post_fact == field.parent_fact ||
        field.child_post_fact == field.child_fact ||
        field.payload_occurrence != e->occurrence ||
        field.post_payload_occurrence != 0 ||
        replace->results[0].value != field.old_value ||
        none->result_count != 1 || none->results[0].value != field.new_value)
        return reject(e);
    Value args[4] = {0};
    const Local *donors[4] = {0};
    NLCheckedNodeId arg = v->first_argument;
    for (size_t i = 0; i < 4; ++i) {
        const NLCheckedNodeView *a = get(env, arg);
        if (a == NULL || a->result_count != 1 ||
            a->results[0].value != v->producer.inputs[i] ||
            (i == 0 ? a->kind != NL_CHECKED_FIELD_REF
                    : (a->kind != NL_CHECKED_IDENTIFIER ||
                       a->symbol != v->producer.donor[i])))
            return reject(e);
        if (i == 0 && (a->field.child != v->producer.head.child ||
                       a->field.parent != v->producer.head.parent ||
                       a->reference_result.scope == 0))
            return reject(e);
        if (i != 0)
            donors[i] = local(env, a->symbol, a->type);
        if (!expr(e, env, arg, &args[i]))
            return false;
        arg = a->next_argument;
    }
    if (arg != 0 || donors[2] == NULL || donors[3] == NULL ||
        e->projections != (e->custody ? 2u : 3u))
        return reject(e);
    Env callee = {.artifact = f,
                  .frame = ++e->frames,
                  .backing = tail->backing,
                  .region_slot = 1,
                  .heap_place = tail->heap_place,
                  .heap_incarnation = tail->heap_incarnation,
                  .heap_domain = tail->heap_domain};
    for (size_t i = 0; i < 4; ++i)
        if (!add_local(e, &callee, v->producer.parameters[i], args[i].type,
                       false))
            return false;
    e->producer = malloc(C_BYTES);
    if (e->producer == NULL) {
        e->status = NL_NODE_C_OUT_OF_MEMORY;
        return false;
    }
    char *main_text = e->text;
    size_t main_used = e->used;
    e->text = e->producer;
    e->used = 0;
    ++e->producer_calls;
    bool ok = put(e, "static nl_live_tail nl_producer_%zu(", v->function);
    for (size_t i = 0; ok && i < 4; ++i)
        ok = put(e, "%s%s nl_b_%zu_%zu", i == 0 ? "" : ",",
                 ctype(e, &callee, args[i].type), callee.frame,
                 v->producer.parameters[i]);
    if (ok)
        ok = put(e,
                 ") "
                 "{\nNL_LIVE_ENTER(nl_b_%zu_%zu,nl_b_%zu_%zu,nl_b_%zu_%zu."
                 "handle,nl_b_%zu_%zu.token);\n",
                 callee.frame, v->producer.parameters[0], callee.frame,
                 v->producer.parameters[1], callee.frame,
                 v->producer.parameters[2], callee.frame,
                 v->producer.parameters[3]);
    Value projected = {0}, replacement = {0};
    NLSemanticValueView selected, input;
    if (ok)
        ok =
            selected_reference(&callee, ref, &selected) &&
            nl_semantic_value_view(nl_checked_producer_entry(env->artifact, id),
                                   v->producer.inputs[0], &input) &&
            same_reference(selected.reference, input.reference) &&
            expr(e, &callee, replace->first_argument, &projected) &&
            expr(e, &callee, ref->next_argument, &replacement);
    size_t old = ++e->temp;
    if (ok)
        ok = put(e,
                 "nl_node_option nl_v_%zu = *nl_v_%zu;\n*nl_v_%zu = nl_v_%zu;\n"
                 "NL_HEAP_CHANGE(nl_v_%zu,&nl_v_%zu,2);\n(void)nl_v_%zu;\n",
                 old, projected.temp, projected.temp, replacement.temp,
                 projected.temp, old, old);
    Value values[3] = {0};
    bool seen[3] = {false, false, false};
    NLCheckedNodeId item = aggregate->first_argument;
    for (size_t i = 0; ok && i < 3; ++i) {
        const NLCheckedNodeView *field_node = nl_checked_node_view(f, item);
        const NLCheckedNodeView *a =
            field_node == NULL
                ? NULL
                : nl_checked_node_view(f, field_node->initializer);
        if (field_node == NULL ||
            field_node->kind != NL_CHECKED_AGGREGATE_FIELD ||
            field_node->field_index >= 3 || seen[field_node->field_index] ||
            a == NULL || a->kind != NL_CHECKED_IDENTIFIER ||
            a->symbol != v->producer.parameters[field_node->field_index + 1] ||
            a->result_count != 1 ||
            a->results[0].value != package.fields[field_node->field_index] ||
            field_node->type != a->type ||
            a->type != args[field_node->field_index + 1].type) {
            ok = reject(e);
            break;
        }
        seen[field_node->field_index] = true;
        ok = expr(e, &callee, field_node->initializer,
                  &values[field_node->field_index]);
        item = field_node->next_argument;
    }
    size_t packed = ++e->temp;
    if (ok)
        ok = item == 0 &&
             put(e,
                 "nl_live_tail nl_v_%zu = {nl_v_%zu,nl_v_%zu,nl_v_%zu};\n"
                 "NL_LIVE_RETURN(nl_v_%zu);\nreturn nl_v_%zu;\n}\n",
                 packed, values[0].temp, values[1].temp, values[2].temp, packed,
                 packed);
    e->producer_used = e->used;
    e->text = main_text;
    e->used = main_used;
    if (!ok)
        return reject(e);
    for (size_t i = 0; i < 3; ++i)
        e->returned_members[i] = package.fields[i];
    e->link_value = field.new_value;
    e->root_fact = field.parent_post_fact;
    e->link_fact = field.child_post_fact;
    e->occurrence = 0;
    ++e->link_updates;
    out->type = e->live_tail;
    out->temp = ++e->temp;
    return put(e,
               "NL_LIVE_SEND(nl_v_%zu,nl_v_%zu.handle,nl_v_%zu.token,&nl_b_%zu_"
               "%zu,&nl_b_%zu_%zu);\n"
               "nl_live_tail nl_v_%zu = "
               "nl_producer_%zu(nl_v_%zu,nl_v_%zu,nl_v_%zu,nl_v_%zu);\n"
               "NL_LIVE_RECEIVE(nl_v_%zu,&nl_b_%zu_%zu,&nl_b_%zu_%zu);\n",
               args[1].temp, args[2].temp, args[3].temp, donors[2]->frame,
               donors[2]->symbol, donors[3]->frame, donors[3]->symbol,
               out->temp, v->function, args[0].temp, args[1].temp, args[2].temp,
               args[3].temp, out->temp, donors[2]->frame, donors[2]->symbol,
               donors[3]->frame, donors[3]->symbol);
}
static bool live_destructure(Emit *e, Env *env, const NLCheckedNodeView *v,
                             Value *out)
{
    if (v->argument_count != 3 ||
        (e->custody ? (!e->custody_authorized || e->producer_calls != 1)
                    : e->producer_calls != 0))
        return reject(e);
    Value package = {0};
    if (!expr(e, env, v->initializer, &package) ||
        package.type != e->live_tail || e->producer_calls != 1)
        return reject(e);
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    const NLTypeId types[3] = {e->ptr,
                               nl_semantic_core_type(c, NL_TYPE_ALLOCATION),
                               nl_semantic_domain_type(c)};
    const char *members[3] = {"ptr", "allocation", "domain"};
    bool seen[3] = {false, false, false};
    NLCheckedNodeId id = v->first_argument;
    for (size_t i = 0; i < 3; ++i) {
        const NLCheckedNodeView *r = get(env, id);
        if (r == NULL || r->kind != NL_CHECKED_RECEIVER ||
            r->field_index >= 3 || seen[r->field_index] ||
            r->type != types[r->field_index] ||
            !add_local(e, env, r->symbol, r->type, false))
            return reject(e);
        NLSemanticBindingView binding;
        if (!nl_semantic_binding_view(c, r->symbol, &binding) ||
            binding.value != e->returned_members[r->field_index])
            return reject(e);
        seen[r->field_index] = true;
        if (!put(e, "%s nl_b_%zu_%zu = nl_v_%zu.%s;\n(void)nl_b_%zu_%zu;\n",
                 ctype(e, env, r->type), env->frame, r->symbol, package.temp,
                 members[r->field_index], env->frame, r->symbol))
            return false;
        id = r->next_argument;
    }
    out->type = e->unit;
    return id == 0 && put(e, "nl_v_%zu = (nl_live_tail){0};\n", package.temp);
}

/* This gate consumes only the existing §18.1a proof. A separate parameter
 * placement never supplies a heap root or an allocation handle. */
static bool owner_evidence(Emit *e, Env *env, NLCheckedNodeId id,
                           const NLCheckedNodeView *v, const Env **target)
{
    const NLSemanticContext *post = nl_checked_context(env->artifact),
                            *entry = nl_checked_owner_entry(env->artifact, id);
    const NLCheckedFragment *body = nl_checked_call_body(env->artifact, id);
    NLTypedOwnerDefinition definition;
    NLSemanticSnapshot snap;
    NLSemanticPlaceView before, after;
    NLSemanticBackingView r, dead_r;
    NLSemanticDomainView d, dead_d;
    const Env *h = region(env, v->owner_call.range.region);
    if (!e->two_heap || e->owner_calls != 0 || e->allocations != 2 ||
        e->reloans != (e->custody ? 2u : 4u + (e->live_tail != 0 ? 1u : 0u)) ||
        e->link_updates != 2 || e->projections != (e->custody ? 2u : 3u) ||
        e->reads != (e->custody ? 0u : 1u) || h == NULL ||
        h->region_slot != 1 || v->kind != NL_CHECKED_REGISTERED_CALL ||
        !v->body_backed || v->argument_count != 3 || v->type != e->unit ||
        v->result_count != 0 || !v->owner_call.entry_proved ||
        !v->owner_call.post_proved || entry == NULL || body == NULL ||
        nl_checked_context(body) != post ||
        !nl_semantic_function_applicability(post, v->function, &definition) ||
        !definition.definition_checked || definition.target != e->node ||
        definition.requirements != NL_OWNER_ALL_REQUIREMENTS ||
        definition.step_count < 4 || definition.step_count > 5 ||
        definition.definition_checked !=
            v->owner_call.definition.definition_checked ||
        definition.target != v->owner_call.definition.target ||
        definition.requirements != v->owner_call.definition.requirements ||
        definition.step_count != v->owner_call.definition.step_count ||
        h->heap_place != v->owner_call.root ||
        h->heap_incarnation != v->owner_call.incarnation ||
        h->heap_domain != v->owner_call.domain ||
        v->owner_call.range.start != 0 || v->owner_call.range.length != 24 ||
        e->lifecycle[1].stage != 4 || !nl_semantic_snapshot(entry, &snap) ||
        !nl_semantic_place_view(entry, h->heap_place, &before) ||
        !before.live || !before.independent_root || before.type != e->node ||
        before.parent_aggregate != 0 || before.parent_sum != 0 ||
        before.incarnation != h->heap_incarnation ||
        before.governing_domain != h->heap_domain ||
        before.current_value == 0 || before.current_fact == 0 ||
        before.placement.region != h->backing || before.placement.start != 0 ||
        before.placement.length != 24 ||
        !nl_semantic_place_view(post, h->heap_place, &after) || after.live ||
        after.incarnation != before.incarnation || after.current_value != 0 ||
        after.current_fact != 0 || after.placement.region != 0 ||
        after.governing_domain != 0 ||
        !nl_semantic_backing_view(entry, h->backing, &r) || !r.live ||
        r.size != 24 || r.alignment < 8 || !r.ordinary_read ||
        !r.ordinary_write ||
        !nl_semantic_backing_view(post, h->backing, &dead_r) || dead_r.live ||
        !nl_semantic_domain_view(entry, h->heap_domain, &d) || !d.live ||
        d.value != v->owner_call.inputs[2] ||
        !nl_semantic_domain_view(post, h->heap_domain, &dead_d) || dead_d.live)
        return reject(e);
    for (size_t i = 0; i < definition.step_count; ++i)
        if (definition.steps[i] !=
                (NLTypedOwnerStep)(i + (definition.step_count == 4)) ||
            definition.steps[i] != v->owner_call.definition.steps[i])
            return reject(e);
    /* This closed backend requires all source loans ended at transfer. An
     * unsupported surviving unrelated loan is not a new language error. */
    for (NLScopeId s = 1; s <= snap.scopes; ++s) {
        NLSemanticScopeView scope;
        if (!nl_semantic_scope_view(entry, s, &scope) || scope.active)
            return reject(e);
    }
    size_t roots = 0;
    for (NLPlaceId p = 1; p <= snap.places; ++p) {
        NLSemanticPlaceView place;
        if (!nl_semantic_place_view(entry, p, &place))
            return reject(e);
        if (place.live && place.placement.region == h->backing) {
            if (p != h->heap_place)
                return reject(e);
            ++roots;
        }
        if (place.live && place.independent_root &&
            place.governing_domain == h->heap_domain && p != h->heap_place)
            return reject(e);
    }
    if (roots != 1)
        return reject(e);
    for (NLValueId i = 1; i <= snap.values; ++i) {
        NLSemanticValueView value;
        if (!nl_semantic_value_view(entry, i, &value))
            return reject(e);
        if (value.carrier == NL_CARRIER_ENDED)
            continue;
        if (value.dependencies != NL_DEPENDENCY_FREE ||
            value.value_dependency_count != 0 ||
            value.occupancy.region == h->backing ||
            (value.allocation_region == h->backing &&
             i != v->owner_call.inputs[1]) ||
            (value.domain == h->heap_domain && i != v->owner_call.inputs[2]))
            return reject(e);
    }
    const NLTypeId types[] = {e->ptr,
                              nl_semantic_core_type(post, NL_TYPE_ALLOCATION),
                              nl_semantic_domain_type(post)};
    NLCheckedNodeId arg_id = v->first_argument;
    for (size_t i = 0; i < 3; ++i) {
        const NLCheckedNodeView *arg = get(env, arg_id);
        NLSemanticBindingView donor, parameter;
        NLSemanticValueView a, b;
        if (arg == NULL || arg->kind != NL_CHECKED_IDENTIFIER ||
            arg->type != types[i] || arg->symbol != v->owner_call.donor[i] ||
            arg->result_count != 1 ||
            arg->results[0].value != v->owner_call.inputs[i] ||
            arg->results[0].type != types[i] ||
            arg->value_use != (i == 0 ? NL_VALUE_COPIED : NL_VALUE_CONSUMED) ||
            local(env, arg->symbol, arg->type) == NULL ||
            v->owner_call.parameters[i] <= snap.bindings ||
            !nl_semantic_binding_view(entry, arg->symbol, &donor) ||
            donor.type != types[i] ||
            !nl_semantic_binding_view(post, v->owner_call.parameters[i],
                                      &parameter) ||
            parameter.type != types[i] ||
            parameter.value != v->owner_call.inputs[i] ||
            parameter.place == donor.place ||
            parameter.place == h->heap_place ||
            !nl_semantic_value_view(entry, v->owner_call.inputs[i], &a) ||
            a.type != types[i] ||
            !nl_semantic_value_view(post, parameter.value, &b) ||
            b.type != types[i] || a.dependencies != NL_DEPENDENCY_FREE ||
            a.value_dependency_count != 0 ||
            b.dependencies != NL_DEPENDENCY_FREE ||
            b.value_dependency_count != 0)
            return reject(e);
        if (i == 0) {
            NLSemanticValueView original;
            if (!selected_reference(env, arg, &original) ||
                !same_reference(a.reference, b.reference) ||
                a.reference_count != 0 || a.reference.place != h->heap_place ||
                a.reference.incarnation != h->heap_incarnation ||
                a.reference.provenance != NL_PROVENANCE_VALID ||
                !a.reference.readable || a.reference.scope != 0 ||
                a.reference.occurrence_dependency != 0)
                return reject(e);
        } else if (donor.availability != NL_CONSUMED ||
                   parameter.availability != NL_CONSUMED ||
                   donor.value != v->owner_call.inputs[i] ||
                   a.carrier != NL_CARRIER_LOOSE ||
                   b.carrier != NL_CARRIER_ENDED ||
                   (i == 1 ? (a.allocation_region != h->backing ||
                              b.allocation_region != h->backing)
                           : (a.domain != h->heap_domain ||
                              b.domain != h->heap_domain)))
            return reject(e);
        arg_id = arg->next_argument;
    }
    if (arg_id != 0)
        return reject(e);
    *target = h;
    return true;
}
static bool owner_call(Emit *e, Env *env, NLCheckedNodeId id,
                       const NLCheckedNodeView *v, Value *out)
{
    const Env *target = NULL;
    if (!owner_evidence(e, env, id, v, &target))
        return false;
    const Local *donors[3];
    Value args[3] = {0};
    NLCheckedNodeId arg = v->first_argument;
    for (size_t i = 0; i < 3; ++i) {
        const NLCheckedNodeView *a = get(env, arg);
        donors[i] = local(env, a->symbol, a->type);
        if (!expr(e, env, arg, &args[i]))
            return false;
        arg = a->next_argument;
    }
    const HeapLifecycle head = e->lifecycle[0];
    const NLCheckedFragment *body = nl_checked_call_body(env->artifact, id);
    Env receiver = {.artifact = body,
                    .frame = ++e->frames,
                    .region_slot = 1,
                    .backing = target->backing,
                    .heap_place = target->heap_place,
                    .heap_incarnation = target->heap_incarnation,
                    .heap_domain = target->heap_domain,
                    .heap_symbol = v->owner_call.parameters[0]};
    for (size_t i = 0; i < 3; ++i)
        if (!add_local(e, &receiver, v->owner_call.parameters[i], args[i].type,
                       false))
            return false;
    if (e->receiver == NULL)
        e->receiver = malloc(C_BYTES);
    if (e->receiver == NULL) {
        e->status = NL_NODE_C_OUT_OF_MEMORY;
        return false;
    }
    char *main_text = e->text;
    size_t main_used = e->used;
    e->text = e->receiver;
    e->used = e->receiver_used;
    ++e->owner_calls;
    e->owner_read = v->owner_call.definition.step_count == 5;
    bool ok = put(e, "static void nl_owner_%zu", v->function) &&
              (!e->custody || put(e, "_%zu", receiver.frame)) &&
              put(e,
                  "(const nl_node *nl_b_%zu_%zu,nl_allocation "
                  "nl_b_%zu_%zu,nl_domain nl_b_%zu_%zu) {\n"
                  "NL_HEAP_RECEIVER_ENTER(nl_b_%zu_%zu,nl_b_%zu_%zu.handle,nl_"
                  "b_%zu_%zu."
                  "token,%zu,%zu,%zu,%zu,&nl_b_%zu_%zu,&nl_b_%zu_%zu);\n",
                  receiver.frame, v->owner_call.parameters[0], receiver.frame,
                  v->owner_call.parameters[1], receiver.frame,
                  v->owner_call.parameters[2], receiver.frame,
                  v->owner_call.parameters[0], receiver.frame,
                  v->owner_call.parameters[1], receiver.frame,
                  v->owner_call.parameters[2], receiver.frame,
                  v->owner_call.parameters[0], v->owner_call.parameters[1],
                  v->owner_call.parameters[2], receiver.frame,
                  v->owner_call.parameters[1], receiver.frame,
                  v->owner_call.parameters[2]);
    Value result = {0};
    if (ok)
        ok = block(e, &receiver, nl_checked_root(body), &result) &&
             result.type == e->unit && e->lifecycle[1].stage == 8 &&
             e->lifecycle[1].owner_symbol == 0 &&
             e->lifecycle[1].domain_symbol == 0 &&
             head.stage == e->lifecycle[0].stage &&
             head.owner_frame == e->lifecycle[0].owner_frame &&
             head.owner_symbol == e->lifecycle[0].owner_symbol &&
             head.domain_frame == e->lifecycle[0].domain_frame &&
             head.domain_symbol == e->lifecycle[0].domain_symbol &&
             put(e, "NL_HEAP_RECEIVER_EXIT();\n}\n");
    e->receiver_used = e->used;
    e->text = main_text;
    e->used = main_used;
    if (!ok)
        return reject(e);
    out->type = e->unit;
    return put(e,
               "NL_HEAP_HANDOFF(nl_v_%zu,nl_v_%zu.handle,nl_v_%zu.token,&nl_b_%"
               "zu_%zu,&nl_b_%zu_%zu);\n"
               "nl_owner_%zu",
               args[0].temp, args[1].temp, args[2].temp, donors[1]->frame,
               donors[1]->symbol, donors[2]->frame, donors[2]->symbol,
               v->function) &&
           (!e->custody || put(e, "_%zu", receiver.frame)) &&
           put(e,
               "(nl_v_%zu,nl_v_%zu,nl_v_%zu);\n"
               "NL_HEAP_RETURNED(&nl_b_%zu_%zu,&nl_b_%zu_%zu);\n",
               args[0].temp, args[1].temp, args[2].temp, donors[1]->frame,
               donors[1]->symbol, donors[2]->frame, donors[2]->symbol);
}

/* §18.1c only. The complete owned certificate authorizes the finite world
 * mapping; every reachable policy arm and its own continuation is emitted.
 * These are compile-time carrier maps, never an ownership runtime. */
static bool custody_loan(Emit *e, Env *env, const NLCheckedNodeView *v,
                         Value *out)
{
    const NLCheckedLoanPlan l = v->loan;
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    const Local *src = local(env, l.source, e->custody_type);
    NLSemanticBindingView b;
    NLSemanticTypeView t;
    if (!e->custody_authorized || src == NULL || !l.implicit_local ||
        l.is_exclusive || l.from_ptr || l.access != NL_ACCESS_WRITE ||
        l.scope == 0 || !l.body_nonescape_proved ||
        !l.normal_result_forwarded || !l.prevent_lifetime_end ||
        !nl_semantic_binding_view(c, l.ref_symbol, &b) ||
        !nl_semantic_type_view(c, b.type, &t) || t.kind != NL_TYPE_REF ||
        t.target != e->custody_type || t.is_exclusive ||
        t.access != NL_ACCESS_WRITE)
        return reject(e);
    Env body = {.artifact = env->artifact,
                .parent = env,
                .prefix = SIZE_MAX,
                .frame = ++e->frames,
                .depth = env->depth + 1};
    if (!add_local(e, &body, l.ref_symbol, b.type, false))
        return false;
    size_t result = 0;
    if (v->type != e->unit) {
        if (v->type != e->custody_type)
            return reject(e);
        result = ++e->temp;
        if (!put(e, "nl_custody nl_v_%zu;\n", result))
            return false;
    }
    if (!put(e, "{ nl_custody *nl_b_%zu_%zu = &nl_b_%zu_%zu;\n", body.frame,
             l.ref_symbol, src->frame, src->symbol))
        return false;
    if (!put(e, "NL_CUSTODY_LOAN(nl_b_%zu_%zu,%zu,1);\n", body.frame,
             l.ref_symbol, l.scope))
        return false;
    Value value = {0};
    if (!allocated_body(e, &body, v->initializer, &value) ||
        value.type != v->type ||
        (result && !put(e, "nl_v_%zu = nl_v_%zu;\n", result, value.temp)) ||
        !put(e, "NL_CUSTODY_LOAN(nl_b_%zu_%zu,%zu,0);\n}\n", body.frame,
             l.ref_symbol, l.scope))
        return reject(e);
    *out = (Value){.type = v->type, .temp = result};
    return true;
}
static bool custody_recipient(Emit *e, Env *env, NLCheckedNodeId id,
                              const NLCheckedNodeView *v, Value *out)
{
    const NLCheckedFragment *f = nl_checked_call_body(env->artifact, id);
    const NLCheckedNodeView *body = nl_checked_node_view(f, nl_checked_root(f));
    const NLCheckedNodeView *ref = get(env, v->first_argument),
                            *packet = ref ? get(env, ref->next_argument) : NULL;
    NLSemanticValueView pv, sv;
    NLSemanticBindingView rb;
    NLSemanticTypeView rt;
    if (!e->custody_authorized || e->recipient_calls != 0 ||
        !v->custody_call.entry_proved || !v->custody_call.post_proved ||
        !v->body_backed || v->argument_count != 2 || v->type != e->unit ||
        v->custody_call.world != nl_checked_context(env->artifact) ||
        body == NULL || body->kind != NL_CHECKED_BLOCK ||
        body->item_count != 2 || body->terminates || ref == NULL ||
        packet == NULL || packet->next_argument != 0 ||
        ref->kind != NL_CHECKED_IDENTIFIER ||
        packet->kind != NL_CHECKED_IDENTIFIER || packet->type != e->live_tail ||
        packet->symbol != v->custody_call.donor ||
        packet->value_use != NL_VALUE_CONSUMED ||
        packet->results[0].value != v->custody_call.packet ||
        !nl_semantic_type_view(nl_checked_context(env->artifact), ref->type,
                               &rt) ||
        rt.kind != NL_TYPE_REF || rt.target != e->custody_type ||
        rt.is_exclusive || rt.access != NL_ACCESS_WRITE ||
        !nl_semantic_value_view(v->custody_call.entry_world,
                                packet->results[0].value, &pv) ||
        pv.field_count != 3 ||
        !nl_semantic_binding_view(v->custody_call.entry_world, ref->symbol,
                                  &rb) ||
        !nl_semantic_value_view(v->custody_call.entry_world, rb.value, &sv) ||
        sv.reference.place != v->custody_call.sink ||
        sv.reference.incarnation != v->custody_call.sink_incarnation ||
        sv.reference.scope == 0 ||
        sv.reference.provenance != NL_PROVENANCE_VALID ||
        !sv.reference.writable)
        return reject(e);
    for (size_t i = 0; i < 3; ++i)
        if (pv.fields[i] != e->returned_members[i])
            return reject(e);
    const Local *donor = local(env, packet->symbol, packet->type);
    Value args[2] = {0};
    if (donor == NULL || !expr(e, env, v->first_argument, &args[0]) ||
        !expr(e, env, ref->next_argument, &args[1]))
        return reject(e);
    Env callee = {.artifact = f, .frame = ++e->frames, .depth = env->depth + 1};
    for (size_t i = 0; i < 2; ++i)
        if (!add_local(e, &callee, v->custody_call.parameters[i], args[i].type,
                       false))
            return false;
    e->recipient = malloc(C_BYTES);
    if (e->recipient == NULL) {
        e->status = NL_NODE_C_OUT_OF_MEMORY;
        return false;
    }
    char *main_text = e->text;
    size_t main_used = e->used;
    e->text = e->recipient;
    e->used = 0;
    ++e->recipient_calls;
    bool ok = put(e,
                  "static void nl_recipient_%zu(nl_custody *nl_b_%zu_%zu,"
                  "nl_live_tail nl_b_%zu_%zu) {\n"
                  "NL_CUSTODY_ENTER(nl_b_%zu_%zu,nl_b_%zu_%zu);\n",
                  v->function, callee.frame, v->custody_call.parameters[0],
                  callee.frame, v->custody_call.parameters[1], callee.frame,
                  v->custody_call.parameters[0], callee.frame,
                  v->custody_call.parameters[1]);
    e->custody_active_call = v;
    /* Source-body traversal, not a fabricated recipient builtin. */
    Value result = {0};
    if (ok)
        ok = block(e, &callee, nl_checked_root(f), &result) &&
             result.type == e->unit &&
             put(e, "NL_CUSTODY_STORED(nl_b_%zu_%zu,&nl_b_%zu_%zu);\n}\n",
                 callee.frame, v->custody_call.parameters[0], callee.frame,
                 v->custody_call.parameters[1]);
    e->custody_active_call = NULL;
    e->recipient_used = e->used;
    e->text = main_text;
    e->used = main_used;
    if (!ok)
        return reject(e);
    *out = (Value){.type = e->unit};
    return put(e,
               "nl_recipient_%zu(nl_v_%zu,nl_v_%zu);\n"
               "NL_CUSTODY_RETURNED(nl_v_%zu,&nl_b_%zu_%zu);\n",
               v->function, args[0].temp, args[1].temp, args[0].temp,
               donor->frame, donor->symbol);
}
static bool custody_expr(Emit *e, Env *env, NLCheckedNodeId id,
                         const NLCheckedNodeView *v, Value *out)
{
    if (v->custody_call.entry_proved)
        return custody_recipient(e, env, id, v, out);
    if (v->kind == NL_CHECKED_LOAN_HEADER)
        return custody_loan(e, env, v, out);
    if (v->kind == NL_CHECKED_SUM_CONSTRUCTOR) {
        if (v->type != e->custody_type || v->result_count != 1 ||
            v->results[0].type != e->custody_type || v->variant < 1 ||
            v->variant > 2)
            return reject(e);
        out->type = e->custody_type;
        out->temp = ++e->temp;
        if (v->variant == 1)
            return v->initializer == 0 &&
                   put(e, "nl_custody nl_v_%zu = {0};\n", out->temp);
        Value payload = {0};
        if (!e->custody_authorized || !expr(e, env, v->initializer, &payload) ||
            payload.type != e->live_tail)
            return reject(e);
        return put(e, "nl_custody nl_v_%zu = {1,nl_v_%zu};\n", out->temp,
                   payload.temp);
    }
    const NLCheckedNodeView *a = get(env, v->first_argument),
                            *b = a ? get(env, a->next_argument) : NULL;
    if (v->kind != NL_CHECKED_REPLACE || !e->custody_authorized ||
        v->type != e->custody_type || v->argument_count != 2 || a == NULL ||
        b == NULL || b->next_argument != 0 ||
        b->kind != NL_CHECKED_SUM_CONSTRUCTOR || b->type != e->custody_type ||
        (v->custody_extraction.present &&
         (b->variant != 1 ||
          b->results[0].value != v->custody_extraction.new_sum ||
          v->results[0].value != v->custody_extraction.old_sum)))
        return reject(e);
    if (!v->custody_extraction.present) {
        const NLCheckedNodeView *call = e->custody_active_call;
        const NLCheckedNodeView *payload = get(env, b->initializer);
        if (call == NULL || a->kind != NL_CHECKED_IDENTIFIER ||
            a->symbol != call->custody_call.parameters[0] || b->variant != 2 ||
            payload == NULL || payload->kind != NL_CHECKED_IDENTIFIER ||
            payload->symbol != call->custody_call.parameters[1] ||
            payload->results[0].value != call->custody_call.packet ||
            v->results[0].value != call->custody_call.old_none ||
            b->results[0].value != call->custody_call.new_some)
            return reject(e);
    }
    Value ref = {0}, replacement = {0};
    if (!expr(e, env, v->first_argument, &ref) ||
        !expr(e, env, a->next_argument, &replacement))
        return reject(e);
    out->type = e->custody_type;
    out->temp = ++e->temp;
    return put(e,
               "nl_custody nl_v_%zu = *nl_v_%zu;\n"
               "*nl_v_%zu = nl_v_%zu;\n",
               out->temp, ref.temp, ref.temp, replacement.temp) &&
           (!v->custody_extraction.present ||
            put(e, "NL_CUSTODY_EXTRACT(nl_v_%zu,nl_v_%zu);\n", ref.temp,
                out->temp));
}
static bool custody_match(Emit *e, Env *env, NLCheckedNodeId id,
                          const NLCheckedNodeView *v, Value *out)
{
    const NLCheckedFragment *suffix0 =
        nl_checked_custody_continuation(env->artifact, id, 0);
    if (suffix0 != NULL) {
        if (e->custody_authorized ||
            !nl_checked_custody_valid(env->artifact, id) ||
            v->normal_frame_unchanged || v->normal_arms != 2 ||
            v->item_count != 2)
            return reject(e);
        e->custody_authorized = true;
        Value scrutinee = {0};
        if (!expr(e, env, v->initializer, &scrutinee) ||
            scrutinee.type != e->option ||
            !put(e, "switch(nl_v_%zu.tag) {\n", scrutinee.temp))
            return reject(e);
        HeapLifecycle incoming[2] = {e->lifecycle[0], e->lifecycle[1]};
        size_t starting_reloans = e->reloans;
        bool seen[2] = {false, false};
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedFragment *f = nl_checked_match_arm(env->artifact, id,
                                                              i),
                                    *suffix = nl_checked_custody_continuation(
                                        env->artifact, id, i);
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, nl_checked_root(f));
            if (suffix == NULL || a == NULL ||
                a->kind != NL_CHECKED_MATCH_ARM || a->variant < 1 ||
                a->variant > 2 || seen[a->variant - 1])
                return reject(e);
            seen[a->variant - 1] = true;
            e->lifecycle[0] = incoming[0];
            e->lifecycle[1] = incoming[1];
            e->owner_calls = 0;
            e->reloans = starting_reloans;
            Env arm = {.artifact = f,
                       .parent = env,
                       .prefix = v->match_binding_prefix,
                       .frame = ++e->frames,
                       .depth = env->depth + 1};
            if (!put(e, "case %zu: {\nNL_CUSTODY_POLICY(%zu);\n",
                     a->variant - 1, a->variant))
                return false;
            if (a->variant == 2 &&
                (!add_local(e, &arm, a->symbol, e->ptr, false) ||
                 !put(e,
                      "const nl_node *nl_b_%zu_%zu = nl_v_%zu.ptr;\n"
                      "(void)nl_b_%zu_%zu;\n",
                      arm.frame, a->symbol, scrutinee.temp, arm.frame,
                      a->symbol)))
                return reject(e);
            Value ignored = {0};
            if (!block(e, &arm, a->initializer, &ignored))
                return reject(e);
            /* Prefix refers to parent public carriers; never sibling numeric
             * IDs. suffix is owned by this arm's certificate, independently
             * checked. */
            Env continuation = {.artifact = suffix,
                                .parent = env,
                                .prefix = v->match_binding_prefix,
                                .frame = ++e->frames,
                                .depth = env->depth + 1};
            if (!block(e, &continuation, nl_checked_root(suffix), &ignored) ||
                e->lifecycle[0].stage != 8 || e->lifecycle[1].stage != 8 ||
                e->owner_calls != 1 || e->reloans != starting_reloans + 1 ||
                !put(e, "break; }\n"))
                return reject(e);
        }
        e->reloans =
            starting_reloans + 2; /* count both emitted receiver bodies */
        e->lifecycle[0] = (HeapLifecycle){.stage = 8};
        e->lifecycle[1] = (HeapLifecycle){.stage = 8};
        *out = (Value){.type = e->unit};
        return put(e, "}\n");
    }
    if (!e->custody_authorized || v->type != e->unit || v->result_count != 0)
        return reject(e);
    Value scrutinee = {0};
    if (!expr(e, env, v->initializer, &scrutinee) ||
        scrutinee.type != e->custody_type)
        return reject(e);
    *out = (Value){.type = e->unit};
    if (v->custody_none_site) {
        if (v->item_count != 1 ||
            (v->custody_none_site != 1 && v->custody_none_site != 2))
            return reject(e);
        /* Only the source-proven exact None is erased. There is no runtime
         * cleanup or runtime test supplying the omitted-arm proof. */
        return put(e, "NL_CUSTODY_NONE(%u,nl_v_%zu);\n(void)nl_v_%zu;\n",
                   v->custody_none_site, scrutinee.temp, scrutinee.temp) &&
               block(e, env,
                     v->custody_none_site == 1 ? v->tail : v->first_item, out);
    }
    if (v->item_count != 2 || v->custody_selected_variant < 1 ||
        v->custody_selected_variant > 2)
        return reject(e);
    if (!put(e, "if(nl_v_%zu.tag == %u) {\n", scrutinee.temp,
             v->custody_selected_variant - 1))
        return false;
    if (v->custody_selected_variant == 2) {
        const NLCheckedNodeView *b = get(env, v->first_item),
                                *d = b ? get(env, b->first_item) : NULL,
                                *p = d ? get(env, d->initializer) : NULL;
        if (p == NULL || p->kind != NL_CHECKED_IDENTIFIER ||
            p->type != e->live_tail ||
            !add_local(e, env, p->symbol, p->type, false) ||
            !put(e,
                 "nl_live_tail nl_b_%zu_%zu = nl_v_%zu.packet;\n"
                 "nl_v_%zu.packet = (nl_live_tail){0};\n",
                 env->frame, p->symbol, scrutinee.temp, scrutinee.temp))
            return reject(e);
    }
    return block(e, env, v->first_item, out) && put(e, "}\n");
}

static bool two_profile(const NLCheckedFragment *f, size_t depth)
{
    if (depth >= C_DEPTH)
        return false;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind != NL_CHECKED_MATCH)
            continue;
        if (n->item_count > 2)
            return false;
        if (n->captured_frame_closed)
            return true;
        for (size_t a = 0; a < n->item_count; ++a) {
            const NLCheckedFragment *arm = nl_checked_match_arm(f, i, a);
            if (arm != NULL && two_profile(arm, depth + 1))
                return true;
        }
    }
    return false;
}

static bool return_profile(Emit *e, const NLCheckedFragment *f, size_t depth)
{
    if (depth >= C_DEPTH)
        return reject(e);
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_MATCH &&
            nl_checked_custody_continuation(f, i, 0) != NULL) {
            if (e->custody || !nl_checked_custody_valid(f, i))
                return reject(e);
            e->custody = true;
            const NLCheckedFragment *suffix =
                nl_checked_custody_continuation(f, i, 0);
            for (NLCheckedNodeId n = 1; n <= nl_checked_node_count(suffix);
                 ++n) {
                const NLCheckedNodeView *op = nl_checked_node_view(suffix, n);
                if (op->custody_extraction.present)
                    e->custody_type = op->type;
            }
            if (e->custody_type == 0)
                return reject(e);
        }
        if (v->producer.entry_proved) {
            if (e->live_tail != 0 || !nl_checked_producer_valid(f, i))
                return reject(e);
            e->live_tail = v->type;
        }
        const NLCheckedFragment *b = nl_checked_call_body(f, i);
        if (b != NULL && !return_profile(e, b, depth + 1))
            return false;
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a)
                if (!return_profile(e, nl_checked_match_arm(f, i, a),
                                    depth + 1))
                    return false;
    }
    return true;
}

NLNodeCStatus nl_checked_c_node(const NLCheckedFragment *entry, char **out,
                                size_t *length)
{
    if (entry == NULL || out == NULL || length == NULL || *out != NULL)
        return NL_NODE_C_UNSUPPORTED;
    const NLCheckedNodeId call = nl_checked_root(entry);
    const NLCheckedNodeView *v = nl_checked_node_view(entry, call);
    const NLCheckedFragment *body = nl_checked_call_body(entry, call);
    if (v == NULL || v->kind != NL_CHECKED_REGISTERED_CALL || !v->body_backed ||
        v->argument_count != 0 || v->terminates || v->result_count != 0 ||
        body == NULL)
        return NL_NODE_C_UNSUPPORTED;
    const NLSemanticContext *c = nl_checked_context(body);
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *op = nl_checked_node_view(body, i);
        NLSemanticTypeView h;
        if (op->kind == NL_CHECKED_TRY_ALLOCATE_ONE &&
            nl_semantic_type_view(c, op->allocation_target, &h) &&
            h.field_count == 4) {
            if (v->type != nl_semantic_unit_type(c))
                return NL_NODE_C_UNSUPPORTED;
            return nl_checked_c_five(body, out, length);
        }
    }
    Emit e = {.unit = nl_semantic_unit_type(c),
              .u8 = nl_semantic_core_type(c, NL_TYPE_U8)};
    if (v->type != e.unit)
        return NL_NODE_C_UNSUPPORTED;
    e.text = malloc(C_BYTES);
    if (e.text == NULL)
        return NL_NODE_C_OUT_OF_MEMORY;
    Env env = {.artifact = body};
    e.two_heap = two_profile(body, 0);
    if (!return_profile(&e, body, 0)) {
        free(e.text);
        return e.status;
    }
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *op = get(&env, i);
        if (op->kind == NL_CHECKED_TRY_ALLOCATE_ONE && op->result_count == 0) {
            e.allocated = true;
            if (!shape(&e, &env, op->allocation_target)) {
                free(e.text);
                return e.status;
            }
        }
    }
    (void)put(
        &e,
        "/* Bounded Checked-C representation, not a NewLang ABI. */\n"
        "#include <stdint.h>\n#include <stddef.h>\n#include <stdlib.h>\n"
        "typedef struct nl_node nl_node;\n"
        "typedef struct { unsigned tag; const nl_node *ptr; } nl_node_option;\n"
        "struct nl_node { nl_node_option f0; uint8_t f1; };\n"
        "#ifdef NEWLANG_NODE_OBSERVER\n#include NEWLANG_NODE_OBSERVER\n"
        "#else\n#define NL_NODE_BIND(...) ((void)0)\n"
        "#define NL_NODE_REPLACE(...) ((void)0)\n"
        "#define NL_NODE_ARM(...) ((void)0)\n"
        "#define NL_NODE_RELOAN(...) ((void)0)\n"
        "#define NL_NODE_FINISH() ((void)0)\n#endif\n"
        "");
    if (e.allocated) {
        (void)put(
            &e,
            "_Static_assert(sizeof(nl_node)==24 && _Alignof(nl_node)==8, "
            "\"checked H target layout\");\n"
            "_Static_assert(sizeof(void*)==8 && offsetof(nl_node,f0)==0 && "
            "offsetof(nl_node,f1)==16, \"private member plan\");\n"
            "_Static_assert(sizeof(nl_node_option)==16 && "
            "_Alignof(nl_node_option)==8 && "
            "_Generic(((nl_node *)0)->f0,nl_node_option:1,default:0) && "
            "_Generic(((nl_node *)0)->f1,uint8_t:1,default:0) && "
            "_Generic(((nl_node_option *)0)->ptr,const nl_node *:1,default:0), "
            "\"private member types\");\n"
            "typedef struct {void *handle;} nl_allocation;\n"
            "typedef struct {unsigned char *bytes;size_t length;} nl_storage;\n"
            "typedef struct {unsigned char *bytes;size_t length;} nl_slot;\n"
            "typedef struct {size_t token;} nl_domain;\n"
            "typedef struct {const nl_node *ptr;nl_allocation "
            "allocation;nl_domain domain;} nl_live_tail;\n"
            "typedef struct {unsigned tag;nl_live_tail packet;} nl_custody;\n"
            "typedef struct {nl_allocation allocation;nl_storage raw;} "
            "nl_backing;\n"
            "#ifdef NEWLANG_HEAP_OBSERVER\n#include "
            "NEWLANG_HEAP_OBSERVER\n#else\n"
            "#define NL_HEAP_TRIAL(...) ((void)0)\n#define NL_HEAP_ARM(...) "
            "((void)0)\n"
            "#define NL_HEAP_DOMAIN(...) ((void)0)\n#define NL_HEAP_SLOT(...) "
            "((void)0)\n"
            "#define NL_HEAP_INITIALIZE(...) ((void)0)\n#define "
            "NL_HEAP_RELOAN(...) ((void)0)\n"
            "#define NL_HEAP_ROOT(...) ((void)0)\n"
            "#define NL_HEAP_FIELD(...) ((void)0)\n"
            "#define NL_HEAP_COPY(...) ((void)0)\n"
            "#define NL_HEAP_CHANGE(...) ((void)0)\n"
            "#define NL_HEAP_SCOPE_END(...) ((void)0)\n"
            "#define NL_HEAP_END(...) ((void)0)\n#define NL_HEAP_RAW(...) "
            "((void)0)\n"
            "#define NL_HEAP_FINALIZE(...) ((void)0)\n#define "
            "NL_HEAP_RELEASE(...) ((void)0)\n"
            "#define NL_HEAP_FINISH() ((void)0)\n"
            "#define NL_HEAP_HANDOFF(...) ((void)0)\n"
            "#define NL_HEAP_RECEIVER_ENTER(...) ((void)0)\n"
            "#define NL_HEAP_RECEIVER_EXIT() ((void)0)\n"
            "#define NL_HEAP_RETURNED(...) ((void)0)\n"
            "#define NL_LIVE_SEND(...) ((void)0)\n"
            "#define NL_LIVE_ENTER(...) ((void)0)\n"
            "#define NL_LIVE_RETURN(...) ((void)0)\n"
            "#define NL_LIVE_RECEIVE(...) ((void)0)\n#endif\n"
            "#ifndef NL_CUSTODY_ENTER\n"
            "#define NL_CUSTODY_ENTER(...) ((void)0)\n"
            "#define NL_CUSTODY_STORED(...) ((void)0)\n"
            "#define NL_CUSTODY_RETURNED(...) ((void)0)\n"
            "#define NL_CUSTODY_EXTRACT(...) ((void)0)\n"
            "#define NL_CUSTODY_NONE(...) ((void)0)\n"
            "#define NL_CUSTODY_POLICY(...) ((void)0)\n"
            "#define NL_CUSTODY_LOAN(...) ((void)0)\n#endif\n");
    }
    const size_t main_offset = e.used;
    (void)put(&e, "int main(void) {\n");
    Value result = {0};
    if (!block(&e, &env, nl_checked_root(body), &result) || e.roots != 2 ||
        result.type != e.unit ||
        (e.custody && (!e.custody_authorized || e.recipient_calls != 1)) ||
        (e.allocated &&
         (e.allocations != (e.two_heap ? 2u : 1u) ||
          e.destroys != (e.custody    ? 5u
                         : e.two_heap ? 3u
                                      : 1u) ||
          e.releases != (e.custody    ? 5u
                         : e.two_heap ? 3u
                                      : 1u) ||
          e.domains != (e.two_heap ? 2u : 1u) ||
          e.finalized != (e.custody    ? 5u
                          : e.two_heap ? 3u
                                       : 1u) ||
          e.slots != (e.two_heap ? 2u : 1u) ||
          e.erased != (e.custody    ? 5u
                       : e.two_heap ? 3u
                                    : 1u) ||
          e.reloans != (e.custody     ? 4u
                        : e.two_heap  ? 4u + (e.live_tail != 0 ? 1u : 0u) +
                                            (e.owner_read ? 1u : 0u)
                        : e.heap_link ? 3u
                                      : 1u) ||
          (e.heap_link && (e.projections != (e.custody ? 2u : 3u) ||
                           e.reads != (e.custody ? 0u : 1u))) ||
          e.option_bindings != (e.live_tail != 0 ? 2u : 3u) ||
          e.link_updates != 2)) ||
        !put(&e, e.allocated ? "NL_HEAP_FINISH();\nreturn 0;\n}\n"
                             : "NL_NODE_FINISH();\nreturn 0;\n}\n")) {
        if (e.status == NL_NODE_C_OK)
            e.status = NL_NODE_C_UNSUPPORTED;
        free(e.recipient);
        free(e.receiver);
        free(e.producer);
        free(e.text);
        return e.status;
    }
    if (e.producer != NULL) {
        if (e.producer_used >= C_BYTES - e.used) {
            free(e.recipient);
            free(e.receiver);
            free(e.producer);
            free(e.text);
            return NL_NODE_C_RESOURCE_LIMIT;
        }
        memmove(e.text + main_offset + e.producer_used, e.text + main_offset,
                e.used - main_offset + 1);
        memcpy(e.text + main_offset, e.producer, e.producer_used);
        e.used += e.producer_used;
        free(e.producer);
    }
    if (e.recipient != NULL) {
        if (e.recipient_used >= C_BYTES - e.used) {
            free(e.recipient);
            free(e.receiver);
            free(e.text);
            return NL_NODE_C_RESOURCE_LIMIT;
        }
        memmove(e.text + main_offset + e.recipient_used, e.text + main_offset,
                e.used - main_offset + 1);
        memcpy(e.text + main_offset, e.recipient, e.recipient_used);
        e.used += e.recipient_used;
        free(e.recipient);
        e.recipient = NULL;
    }
    if (e.receiver != NULL) {
        if (e.receiver_used >= C_BYTES - e.used) {
            free(e.recipient);
            free(e.receiver);
            free(e.text);
            return NL_NODE_C_RESOURCE_LIMIT;
        }
        memmove(e.text + main_offset + e.receiver_used, e.text + main_offset,
                e.used - main_offset + 1);
        memcpy(e.text + main_offset, e.receiver, e.receiver_used);
        e.used += e.receiver_used;
        free(e.recipient);
        free(e.receiver);
    }
    *out = e.text;
    *length = e.used;
    return NL_NODE_C_OK;
}
