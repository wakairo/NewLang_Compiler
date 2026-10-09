#include "checked_c_five.h"
#include "newlang/captured_closure.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This finite adapter consumes the authenticated #237 operation trace.
 * Runtime carriers have no semantic authority checks. Compiler-side local
 * availability is copied per arm and committed only against the common post.
 * Opaque projection index selects a private member; no source names/offsets
 * select authority. The previous two-root emitter remains separate. */
#define FIVE_BYTES 131072
#define FIVE_LOCALS 256
#define FIVE_NODES 2048
#define FIVE_STEPS 2048
typedef struct {
    NLSymbolId symbol;
    NLTypeId type;
    size_t frame;
    bool available;
} Carrier;
typedef struct {
    const NLCheckedFragment *fragment;
    Carrier locals[FIVE_LOCALS];
    size_t count, frame, depth;
    unsigned char
        *visited; /* borrowed same-world coverage, loan scopes share */
} World;
typedef struct {
    char *text;
    size_t used, temp, frame, steps, sites, changes;
    NLTypeId node, option, ptr, unit, u8;
    NLNodeCStatus status;
} Five;
typedef struct {
    NLTypeId type;
    size_t temp;
} Result;
static bool no(Five *e)
{
    if (e->status == NL_NODE_C_OK)
        e->status = NL_NODE_C_UNSUPPORTED;
    return false;
}
static bool put(Five *e, const char *format, ...)
{
    if (e->status != NL_NODE_C_OK)
        return false;
    va_list args;
    va_start(args, format);
    int n = vsnprintf(e->text + e->used, FIVE_BYTES - e->used, format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= FIVE_BYTES - e->used) {
        e->status = NL_NODE_C_RESOURCE_LIMIT;
        return false;
    }
    e->used += (size_t)n;
    return true;
}
static const NLCheckedNodeView *node(const World *w, NLCheckedNodeId id)
{
    return nl_checked_node_view(w->fragment, id);
}
static const NLSemanticContext *context(const World *w)
{
    return nl_checked_context(w->fragment);
}
static Carrier *local(World *w, NLSymbolId symbol, NLTypeId type)
{
    for (size_t i = 0; i < w->count; ++i)
        if (w->locals[i].symbol == symbol && w->locals[i].type == type)
            return &w->locals[i];
    return NULL;
}
static const char *ctype(const Five *e, const World *w, NLTypeId id)
{
    NLSemanticTypeView t;
    if (!nl_semantic_type_view(context(w), id, &t))
        return NULL;
    if (id == e->node)
        return "nl_node";
    if (id == e->option)
        return "nl_option";
    if (id == e->ptr)
        return "const nl_node *";
    if (id == e->u8)
        return "uint8_t";
    if (id == nl_semantic_domain_type(context(w)))
        return "nl_domain";
    if (t.kind == NL_TYPE_ALLOCATION)
        return "nl_allocation";
    if (t.kind == NL_TYPE_STORAGE)
        return "nl_storage";
    if (t.kind == NL_TYPE_SLOT && t.target == e->node)
        return "nl_slot";
    if (t.kind == NL_TYPE_REF) {
        if (t.target == e->node && !t.is_exclusive)
            return t.access == NL_ACCESS_WRITE ? "nl_node *"
                                               : "const nl_node *";
        if (t.target == e->option && !t.is_exclusive)
            return t.access == NL_ACCESS_WRITE ? "nl_option *"
                                               : "const nl_option *";
        if (t.target == nl_semantic_domain_type(context(w)) &&
            t.access == NL_ACCESS_READ)
            return "const nl_domain *";
    }
    /* OneBacking's exact fields are checked at each real primitive grant. */
    NLAggregateField a, b;
    if (t.kind == NL_TYPE_NOMINAL && t.field_count == 2 &&
        nl_semantic_aggregate_field_view(context(w), id, 0, &a) &&
        nl_semantic_aggregate_field_view(context(w), id, 1, &b) &&
        a.type == nl_semantic_core_type(context(w), NL_TYPE_ALLOCATION) &&
        b.type == nl_semantic_core_type(context(w), NL_TYPE_STORAGE))
        return "nl_backing";
    return NULL;
}
static bool bind(Five *e, World *w, NLSymbolId s, NLTypeId t)
{
    NLSemanticBindingView b;
    if (w->count >= FIVE_LOCALS) {
        e->status = NL_NODE_C_RESOURCE_LIMIT;
        return false;
    }
    if (local(w, s, t) != NULL ||
        !nl_semantic_binding_view(context(w), s, &b) || b.type != t ||
        ctype(e, w, t) == NULL)
        return no(e);
    w->locals[w->count++] = (Carrier){s, t, w->frame, true};
    return true;
}
static bool ref_same(NLReferenceFacts a, NLReferenceFacts b)
{
    return a.place == b.place && a.incarnation == b.incarnation &&
           a.scope == b.scope && a.provenance == b.provenance &&
           a.readable == b.readable && a.writable == b.writable &&
           a.occurrence_dependency == b.occurrence_dependency;
}
static bool expr(Five *, World *, NLCheckedNodeId, Result *);
static bool covered(Five *e, const World *w)
{
    for (size_t i = 1; i <= nl_checked_node_count(w->fragment); ++i) {
        NLCheckedKind k = node(w, i)->kind;
        /* These authenticated effects and operand uses may neither be skipped
         * by a poisoned block edge nor evaluated twice by emission traversal.
         */
        if ((k == NL_CHECKED_UNIT || k == NL_CHECKED_STATEMENT ||
             k == NL_CHECKED_BLOCK || k == NL_CHECKED_U8_LITERAL ||
             k == NL_CHECKED_AGGREGATE || k == NL_CHECKED_SUM_CONSTRUCTOR ||
             k == NL_CHECKED_BINDING || k == NL_CHECKED_AGGREGATE_BINDING ||
             k == NL_CHECKED_IDENTIFIER || k == NL_CHECKED_LOAN_HEADER ||
             k == NL_CHECKED_TRY_ALLOCATE_ONE || k == NL_CHECKED_MATCH ||
             k == NL_CHECKED_INITIALIZE || k == NL_CHECKED_DESTROY ||
             k == NL_CHECKED_INTO_SLOT || k == NL_CHECKED_ERASE_SLOT ||
             k == NL_CHECKED_DEALLOCATE || k == NL_CHECKED_DOMAIN_CREATE ||
             k == NL_CHECKED_DOMAIN_FINALIZE || k == NL_CHECKED_REPLACE ||
             k == NL_CHECKED_FIELD_REF || k == NL_CHECKED_REF_FROM_PTR) &&
            w->visited[i] != 1)
            return no(e);
    }
    return true;
}
static bool match(Five *e, World *w, NLCheckedNodeId id,
                  const NLCheckedNodeView *v, Result *out)
{
    NLCapturedClosureView proof;
    const NLCheckedNodeView *trial = node(w, v->initializer);
    if (trial == NULL || trial->kind != NL_CHECKED_TRY_ALLOCATE_ONE ||
        !nl_checked_captured_closure_view(w->fragment, id, &proof) ||
        proof.count != e->sites || e->sites >= 5 ||
        trial->allocation_target != e->node || trial->allocation_size != 56 ||
        trial->allocation_alignment != 8 || v->type != e->unit ||
        v->item_count != 2 || v->normal_arms != 2 ||
        ++w->visited[v->initializer] != 1)
        return no(e);
    const size_t site = e->sites++, heap = ++e->temp;
    if (!put(e,
             "void *nl_heap_%zu=malloc(56);\n"
             "NL_FIVE_TRIAL(%zu,nl_heap_%zu,56,8);\n",
             heap, site, heap))
        return false;
    unsigned variants = 0;
    for (size_t arm = 0; arm < 2; ++arm) {
        const NLCheckedFragment *f = nl_checked_match_arm(w->fragment, id, arm);
        if (f == NULL)
            return no(e);
        if (nl_checked_node_count(f) >= FIVE_NODES) {
            e->status = NL_NODE_C_RESOURCE_LIMIT;
            return false;
        }
        const NLCheckedNodeView *a =
            nl_checked_node_view(f, nl_checked_root(f));
        const NLCheckedNodeView *grant =
            a == NULL ? NULL : nl_checked_node_view(f, a->initializer);
        if (a == NULL || a->variant < 1 || a->variant > 2 ||
            (variants & (1u << a->variant)) || grant == NULL ||
            grant->allocation_success != (a->variant == 2))
            return no(e);
        variants |= 1u << a->variant;
        const bool success = a->variant == 2;
        unsigned char seen[FIVE_NODES] = {0};
        World branch = *w;
        branch.fragment = f;
        branch.frame = ++e->frame;
        branch.depth = w->depth + 1;
        branch.visited = seen;
        branch.visited[a->initializer] = 1;
        for (size_t i = 0; i < branch.count; ++i)
            if (branch.locals[i].symbol > v->match_binding_prefix)
                return no(e); /* no numeric suffix import */
        /* Classify the physical result only once, before either arm can end
         * and free it. An independent second pointer test after a Some arm
         * would load an indeterminate post-free pointer for that arm order. */
        if (!(arm == 0 ? put(e, "if(nl_heap_%zu %s NULL){\n", heap,
                             success ? "!=" : "==")
                       : put(e, "else{\n")) ||
            !put(e, "NL_FIVE_ARM(%zu,%zu);\n", site, a->variant))
            return false;
        if (success) {
            NLSemanticBindingView b;
            if (!nl_semantic_binding_view(context(&branch), a->symbol, &b) ||
                !bind(e, &branch, a->symbol, b.type) ||
                strcmp(ctype(e, &branch, b.type), "nl_backing") != 0 ||
                !put(e,
                     "nl_backing nl_b_%zu_%zu={{nl_heap_%zu},"
                     "{(unsigned char *)nl_heap_%zu,56}};\n",
                     branch.frame, a->symbol, heap, heap))
                return no(e);
        }
        Result result = {0};
        if (!expr(e, &branch, a->tail, &result) || result.type != e->unit ||
            !covered(e, &branch))
            return no(e);
        for (size_t i = 0; i < w->count; ++i) {
            NLSemanticTypeView t;
            NLSemanticBindingView post;
            if (!nl_semantic_type_view(context(w), w->locals[i].type, &t) ||
                (!t.is_copy &&
                 (!nl_semantic_binding_view(proof.closed_post,
                                            w->locals[i].symbol, &post) ||
                  branch.locals[i].available !=
                      (post.availability == NL_AVAILABLE))))
                return no(e);
        }
        if (!put(e, "}\n"))
            return false;
    }
    for (size_t i = 0; i < w->count; ++i) {
        NLSemanticTypeView t;
        NLSemanticBindingView post;
        if (!nl_semantic_type_view(context(w), w->locals[i].type, &t))
            return no(e);
        if (!t.is_copy) {
            if (!nl_semantic_binding_view(proof.closed_post,
                                          w->locals[i].symbol, &post))
                return no(e);
            w->locals[i].available = post.availability == NL_AVAILABLE;
        }
    }
    out->type = e->unit;
    return true;
}
static bool expr(Five *e, World *w, NLCheckedNodeId id, Result *out)
{
    if (id == 0 || id >= FIVE_NODES || ++e->steps > FIVE_STEPS ||
        w->depth > 12) {
        e->status = NL_NODE_C_RESOURCE_LIMIT;
        return false;
    }
    const NLCheckedNodeView *v = node(w, id);
    if (v == NULL || v->terminates || ++w->visited[id] != 1)
        return no(e);
    *out = (Result){v->type, 0};
    const char *type = ctype(e, w, v->type);
    if (type != NULL &&
        (v->result_count != 1 || v->results[0].type != v->type ||
         v->results[0].value == 0))
        return no(e);
    NLSemanticValueView semantic = {0};
    if (type != NULL &&
        (!nl_semantic_value_view(context(w), v->results[0].value, &semantic) ||
         semantic.type != v->type ||
         semantic.dependencies != NL_DEPENDENCY_FREE ||
         semantic.reference_count != 0 ||
         semantic.value_dependency_count != 0 ||
         (v->has_reference_result &&
          !ref_same(semantic.reference, v->reference_result))))
        return no(e);
    if (v->kind == NL_CHECKED_UNIT)
        return v->type == e->unit && v->result_count == 0;
    if (v->kind == NL_CHECKED_STATEMENT)
        return expr(e, w, v->initializer, out);
    if (v->kind == NL_CHECKED_BLOCK) {
        NLCheckedNodeId item = v->first_item;
        for (size_t i = 0; i < v->item_count; ++i) {
            const NLCheckedNodeView *op = node(w, item);
            Result ignored;
            if (op == NULL || !expr(e, w, item, &ignored) ||
                (ignored.temp && !put(e, "(void)nl_v_%zu;\n", ignored.temp)))
                return no(e);
            item = op->next_item;
        }
        return item == 0 && expr(e, w, v->tail, out);
    }
    if (v->kind == NL_CHECKED_MATCH)
        return match(e, w, id, v, out);
    if (v->kind == NL_CHECKED_BINDING) {
        const NLCheckedNodeView *r = node(w, v->first_argument);
        Result value;
        NLSemanticBindingView received;
        const NLCheckedNodeView *init = node(w, v->initializer);
        if (r == NULL || init == NULL || r->kind != NL_CHECKED_RECEIVER ||
            init->result_count != 1 ||
            !nl_semantic_binding_view(context(w), r->symbol, &received) ||
            received.value != init->results[0].value ||
            !expr(e, w, v->initializer, &value) || r->type != value.type ||
            !bind(e, w, r->symbol, r->type))
            return no(e);
        out->type = e->unit;
        return put(e, "%s nl_b_%zu_%zu=nl_v_%zu;\n(void)nl_b_%zu_%zu;\n",
                   ctype(e, w, r->type), w->frame, r->symbol, value.temp,
                   w->frame, r->symbol);
    }
    if (v->kind == NL_CHECKED_IDENTIFIER) {
        Carrier *l = local(w, v->symbol, v->type);
        NLSemanticBindingView b;
        NLSemanticTypeView t;
        NLSemanticValueView original, copied;
        if (l == NULL || !l->available || type == NULL ||
            !nl_semantic_binding_view(context(w), v->symbol, &b) ||
            !nl_semantic_type_view(context(w), v->type, &t) ||
            !nl_semantic_value_view(context(w), b.value, &original) ||
            !nl_semantic_value_view(context(w), v->results[0].value, &copied) ||
            original.type != v->type || copied.type != v->type ||
            original.dependencies != NL_DEPENDENCY_FREE ||
            copied.dependencies != NL_DEPENDENCY_FREE)
            return no(e);
        bool consume = v->value_use == NL_VALUE_CONSUMED;
        if (consume) {
            if (t.is_copy || b.value != v->results[0].value)
                return no(e);
            l->available = false;
        } else if (v->value_use == NL_VALUE_REBORROWED) {
            NLSemanticScopeView child;
            if (t.kind != NL_TYPE_REF || !t.is_exclusive ||
                v->reborrow_scope != copied.reference.scope ||
                !nl_semantic_scope_view(context(w), v->reborrow_scope,
                                        &child) ||
                child.parent != original.reference.scope)
                return no(e);
        } else if (v->value_use != NL_VALUE_COPIED || !t.is_copy ||
                   !ref_same(original.reference, copied.reference) ||
                   original.scalar_known != copied.scalar_known ||
                   original.scalar_value != copied.scalar_value ||
                   original.domain != copied.domain ||
                   original.allocation_region != copied.allocation_region)
            return no(e);
        out->temp = ++e->temp;
        if (!put(e, "%s nl_v_%zu=nl_b_%zu_%zu;\n(void)nl_v_%zu;\n", type,
                 out->temp, l->frame, l->symbol, out->temp))
            return false;
        return !consume ||
               put(e, "nl_b_%zu_%zu=(%s){0};\n", l->frame, l->symbol, type);
    }
    if (v->kind == NL_CHECKED_AGGREGATE_BINDING) {
        Result bundle;
        const NLCheckedNodeView *init = node(w, v->initializer);
        NLSemanticValueView backing;
        if (v->argument_count != 2 || init == NULL || init->result_count != 1 ||
            !nl_semantic_value_view(context(w), init->results[0].value,
                                    &backing) ||
            backing.field_count != 2 || !expr(e, w, v->initializer, &bundle) ||
            ctype(e, w, bundle.type) == NULL ||
            strcmp(ctype(e, w, bundle.type), "nl_backing") != 0)
            return no(e);
        NLCheckedNodeId r = v->first_argument;
        bool seen[2] = {false, false};
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedNodeView *p = node(w, r);
            NLSemanticBindingView b;
            if (p == NULL || p->field_index >= 2 || seen[p->field_index] ||
                !nl_semantic_binding_view(context(w), p->symbol, &b) ||
                b.value != backing.fields[p->field_index] ||
                !bind(e, w, p->symbol, p->type))
                return no(e);
            seen[p->field_index] = true;
            if (!put(e, "%s nl_b_%zu_%zu=nl_v_%zu.%s;\n(void)nl_b_%zu_%zu;\n",
                     ctype(e, w, p->type), w->frame, p->symbol, bundle.temp,
                     p->field_index ? "raw" : "allocation", w->frame,
                     p->symbol))
                return false;
            r = p->next_argument;
        }
        return r == 0 && put(e, "nl_v_%zu=(nl_backing){0};\n", bundle.temp);
    }
    if (v->kind == NL_CHECKED_LOAN_HEADER) {
        const NLCheckedLoanPlan *l = &v->loan;
        Carrier *source =
            local(w, l->source, nl_semantic_domain_type(context(w)));
        NLSemanticBindingView binding, owner;
        NLSemanticValueView domain_value, authority;
        NLSemanticTypeView ref_type;
        if (source == NULL || !source->available || l->domain == 0 ||
            !l->body_nonescape_proved || !l->normal_result_forwarded ||
            !l->prevent_lifetime_end || l->access != NL_ACCESS_READ ||
            l->from_ptr || l->implicit_local ||
            !nl_semantic_binding_view(context(w), l->source, &owner) ||
            !nl_semantic_value_view(context(w), owner.value, &domain_value) ||
            domain_value.domain != l->domain || owner.place != l->place ||
            !nl_semantic_binding_view(context(w), l->ref_symbol, &binding) ||
            !nl_semantic_type_view(context(w), binding.type, &ref_type) ||
            ref_type.kind != NL_TYPE_REF || ref_type.target != owner.type ||
            ref_type.access != NL_ACCESS_READ ||
            ref_type.is_exclusive != l->is_exclusive ||
            !nl_semantic_value_view(context(w), binding.value, &authority) ||
            authority.dependencies != NL_DEPENDENCY_FREE ||
            authority.reference_count != 0 ||
            authority.reference.place != l->place ||
            authority.reference.incarnation != l->incarnation ||
            authority.reference.scope != l->scope ||
            authority.reference.provenance != NL_PROVENANCE_VALID ||
            !authority.reference.readable || authority.reference.writable)
            return no(e);
        const size_t result = type == NULL ? 0 : ++e->temp;
        if (result && !put(e, "%s nl_v_%zu;\n", type, result))
            return false;
        World child = *w;
        child.frame = ++e->frame;
        child.depth++;
        if (!bind(e, &child, l->ref_symbol, binding.type) ||
            !put(e,
                 "{ const nl_domain *nl_b_%zu_%zu=&nl_b_%zu_%zu;\n"
                 "NL_FIVE_LOAN(nl_b_%zu_%zu->token,%zu,%u);\n",
                 child.frame, l->ref_symbol, source->frame, source->symbol,
                 child.frame, l->ref_symbol, l->scope,
                 l->is_exclusive ? 1u : 0u))
            return no(e);
        Result value;
        if (!expr(e, &child, v->initializer, &value) || value.type != v->type ||
            (result && !put(e, "nl_v_%zu=nl_v_%zu;\n", result, value.temp)) ||
            !put(e, "NL_FIVE_SCOPE_END(%zu);\n}\n", l->scope))
            return no(e);
        for (size_t i = 0; i < w->count; ++i)
            w->locals[i].available = child.locals[i].available;
        out->temp = result;
        return true;
    }
    if (v->kind == NL_CHECKED_U8_LITERAL) {
        if (!v->has_scalar_result || !v->scalar_result.known ||
            v->type != e->u8 || v->scalar_result.type != e->u8 ||
            v->scalar_result.value > 255 || !semantic.scalar_known ||
            semantic.scalar_value != v->scalar_result.value)
            return no(e);
        out->temp = ++e->temp;
        return put(e, "uint8_t nl_v_%zu=%zu;\n", out->temp,
                   v->scalar_result.value);
    }
    if (v->kind == NL_CHECKED_AGGREGATE) {
        if (v->type != e->node || v->argument_count != 4 ||
            semantic.field_count != 4)
            return no(e);
        Result fields[4] = {0};
        NLCheckedNodeId n = v->first_argument;
        for (size_t i = 0; i < 4; ++i) {
            const NLCheckedNodeView *f = node(w, n);
            const NLCheckedNodeView *init =
                f == NULL ? NULL : node(w, f->initializer);
            if (f == NULL || f->field_index >= 4 || init == NULL ||
                init->result_count != 1 ||
                init->results[0].value != semantic.fields[f->field_index] ||
                fields[f->field_index].temp ||
                !expr(e, w, f->initializer, &fields[f->field_index]) ||
                fields[f->field_index].type !=
                    (f->field_index == 3 ? e->u8 : e->option))
                return no(e);
            n = f->next_argument;
        }
        out->temp = ++e->temp;
        return n == 0 &&
               put(e,
                   "nl_node nl_v_%zu={nl_v_%zu,nl_v_%zu,nl_v_%zu,nl_v_%zu};\n",
                   out->temp, fields[0].temp, fields[1].temp, fields[2].temp,
                   fields[3].temp);
    }
    if (v->kind == NL_CHECKED_SUM_CONSTRUCTOR) {
        if (v->type != e->option || v->variant < 1 || v->variant > 2 ||
            semantic.variant != v->variant || v->argument_count != 0 ||
            (v->variant == 1 &&
             (v->initializer != 0 || semantic.sum_payload != 0)))
            return no(e);
        Result payload = {0};
        if (v->variant == 2) {
            const NLCheckedNodeView *p = node(w, v->initializer);
            if (p == NULL || p->result_count != 1 ||
                semantic.sum_payload != p->results[0].value ||
                !expr(e, w, v->initializer, &payload) || payload.type != e->ptr)
                return no(e);
        }
        out->temp = ++e->temp;
        return v->variant == 1
                   ? put(e, "nl_option nl_v_%zu={0,NULL};\n", out->temp)
                   : put(e, "nl_option nl_v_%zu={1,nl_v_%zu};\n", out->temp,
                         payload.temp);
    }
    if (v->kind == NL_CHECKED_DOMAIN_CREATE) {
        if (v->type != nl_semantic_domain_type(context(w)) ||
            v->lifetime_domain == 0 || v->lifetime_domain != semantic.domain)
            return no(e);
        out->temp = ++e->temp;
        return put(e, "nl_domain nl_v_%zu={%zu};\nNL_FIVE_DOMAIN(%zu);\n",
                   out->temp, v->lifetime_domain, v->lifetime_domain);
    }
    Result args[3] = {0};
    NLCheckedNodeId n = v->first_argument;
    if (v->argument_count > 3)
        return no(e);
    for (size_t i = 0; i < v->argument_count; ++i) {
        const NLCheckedNodeView *a = node(w, n);
        if (a == NULL || !expr(e, w, n, &args[i]))
            return no(e);
        n = a->next_argument;
    }
    if (n != 0)
        return no(e);
    out->temp = type == NULL ? 0 : ++e->temp;
    if (v->kind == NL_CHECKED_INTO_SLOT || v->kind == NL_CHECKED_ERASE_SLOT)
        return v->argument_count == 1 &&
               put(e,
                   "%s nl_v_%zu={nl_v_%zu.bytes,nl_v_%zu.length};\n"
                   "NL_FIVE_%s(nl_v_%zu.bytes,nl_v_%zu.length);\n",
                   type, out->temp, args[0].temp, args[0].temp,
                   v->kind == NL_CHECKED_INTO_SLOT ? "SLOT" : "RAW", out->temp,
                   out->temp);
    if (v->kind == NL_CHECKED_INITIALIZE)
        return v->argument_count == 3 &&
               put(e,
                   "*(nl_node *)nl_v_%zu.bytes=nl_v_%zu;\n"
                   "const nl_node *nl_v_%zu=(nl_node *)nl_v_%zu.bytes;\n"
                   "NL_FIVE_INIT(nl_v_%zu,nl_v_%zu->token,%zu,%zu);\n",
                   args[0].temp, args[1].temp, out->temp, args[0].temp,
                   out->temp, args[2].temp, v->lifetime_place,
                   v->lifetime_incarnation);
    if (v->kind == NL_CHECKED_REF_FROM_PTR)
        return v->argument_count == 2 && v->has_reference_result &&
               put(e,
                   "NL_FIVE_ROOT(nl_v_%zu,nl_v_%zu->token,%u,%zu,%zu,%zu);\n"
                   "%s nl_v_%zu=%snl_v_%zu;\n",
                   args[0].temp, args[1].temp,
                   v->reference_result.writable ? 1u : 0u, v->lifetime_place,
                   v->lifetime_incarnation, v->reference_result.scope, type,
                   out->temp, v->reference_result.writable ? "(nl_node *)" : "",
                   args[0].temp);
    if (v->kind == NL_CHECKED_FIELD_REF)
        return v->argument_count == 1 && v->field.index < 3 &&
               put(e,
                   "%s nl_v_%zu=&nl_v_%zu->f%zu;\n"
                   "NL_FIVE_FIELD(nl_v_%zu,nl_v_%zu,%u,%zu,%zu,%zu,%zu,%zu);\n",
                   type, out->temp, args[0].temp, v->field.index, args[0].temp,
                   out->temp, v->reference_result.writable ? 1u : 0u,
                   v->field.index, v->field.nominal, v->field.child,
                   v->field.child_incarnation, v->reference_result.scope);
    if (v->kind == NL_CHECKED_REPLACE) {
        bool certified = false;
        for (size_t i = 0; i < nl_checked_captured_change_count(w->fragment);
             ++i) {
            NLCapturedChangeView view;
            certified |=
                nl_checked_captured_change_view(w->fragment, i, &view) &&
                view.operation == id;
        }
        return certified && v->argument_count == 2 && ++e->changes <= 6 &&
               put(e,
                   "nl_option nl_v_%zu=*nl_v_%zu;\n"
                   "*nl_v_%zu=nl_v_%zu;\n"
                   "NL_FIVE_CHANGE(nl_v_%zu,&nl_v_%zu,%zu,%zu);\n",
                   out->temp, args[0].temp, args[0].temp, args[1].temp,
                   args[0].temp, out->temp, v->field.payload_occurrence,
                   v->field.post_payload_occurrence);
    }
    if (v->kind == NL_CHECKED_DESTROY)
        return v->argument_count == 2 &&
               put(e,
                   "NL_FIVE_END(nl_v_%zu,nl_v_%zu->token,%zu,%zu);\n"
                   "nl_slot nl_v_%zu={(unsigned char *)nl_v_%zu,56};\n",
                   args[0].temp, args[1].temp, v->lifetime_place,
                   v->lifetime_incarnation, out->temp, args[0].temp);
    if (v->kind == NL_CHECKED_DOMAIN_FINALIZE)
        return v->argument_count == 1 &&
               put(e, "NL_FIVE_FINALIZE(nl_v_%zu.token);\n(void)nl_v_%zu;\n",
                   args[0].temp, args[0].temp);
    if (v->kind == NL_CHECKED_DEALLOCATE)
        return v->argument_count == 2 &&
               put(e,
                   "NL_FIVE_RELEASE(nl_v_%zu.handle,nl_v_%zu.bytes,nl_v_%zu."
                   "length);\n"
                   "free(nl_v_%zu.handle);\nnl_v_%zu.handle=NULL;\n"
                   "nl_v_%zu.bytes=NULL;\n",
                   args[0].temp, args[1].temp, args[1].temp, args[0].temp,
                   args[0].temp, args[1].temp);
    return no(e);
}
NLNodeCStatus nl_checked_c_five(const NLCheckedFragment *body, char **out,
                                size_t *length)
{
    if (body == NULL || out == NULL || *out != NULL || length == NULL)
        return NL_NODE_C_UNSUPPORTED;
    if (nl_checked_node_count(body) >= FIVE_NODES)
        return NL_NODE_C_RESOURCE_LIMIT;
    NLCheckedNodeId first = 0;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i)
        if (nl_checked_node_view(body, i)->kind == NL_CHECKED_MATCH) {
            first = i;
            break;
        }
    NLCheckStatus verified = nl_checked_captured_closure_validate(body, first);
    if (verified != NL_CHECK_OK)
        return verified == NL_CHECK_OUT_OF_MEMORY ? NL_NODE_C_OUT_OF_MEMORY
                                                  : NL_NODE_C_UNSUPPORTED;
    const NLSemanticContext *c = nl_checked_context(body);
    const NLCheckedNodeView *m = nl_checked_node_view(body, first);
    const NLCheckedNodeView *trial = nl_checked_node_view(body, m->initializer);
    Five e = {.node = trial->allocation_target,
              .unit = nl_semantic_unit_type(c),
              .u8 = nl_semantic_core_type(c, NL_TYPE_U8)};
    NLSemanticTypeView h;
    NLAggregateField f;
    if (!nl_semantic_type_view(c, e.node, &h) || h.field_count != 4 ||
        !h.layout_known || h.size != 56 || h.alignment != 8 ||
        !nl_semantic_aggregate_field_view(c, e.node, 0, &f) ||
        !nl_semantic_recursive_link_types(c, e.node, &e.ptr))
        return NL_NODE_C_UNSUPPORTED;
    e.option = f.type;
    e.text = malloc(FIVE_BYTES);
    if (e.text == NULL)
        return NL_NODE_C_OUT_OF_MEMORY;
    (void)put(
        &e,
        "/* Draft17.30 bounded five-root C17; private representation, not ABI. "
        "*/\n"
        "#include <stdint.h>\n#include <stddef.h>\n#include <stdlib.h>\n"
        "typedef struct nl_node nl_node;\n"
        "typedef struct{unsigned tag;const nl_node *ptr;}nl_option;\n"
        "struct nl_node{nl_option f0,f1,f2;uint8_t f3;};\n"
        "_Static_assert(sizeof(void*)==8 && sizeof(nl_node)==56 && "
        "_Alignof(nl_node)==8,\"private H plan\");\n"
        "_Static_assert(sizeof(nl_option)==16 && offsetof(nl_node,f0)==0 && "
        "offsetof(nl_node,f1)==16 && offsetof(nl_node,f2)==32 && "
        "offsetof(nl_node,f3)==48,\"private projections\");\n"
        "typedef struct{void *handle;}nl_allocation;\n"
        "typedef struct{unsigned char *bytes;size_t length;}nl_storage;\n"
        "typedef struct{unsigned char *bytes;size_t length;}nl_slot;\n"
        "typedef struct{size_t token;}nl_domain;\n"
        "typedef struct{nl_allocation allocation;nl_storage raw;}nl_backing;\n"
        "#ifdef NEWLANG_FIVE_OBSERVER\n#include NEWLANG_FIVE_OBSERVER\n#else\n"
        "#define NL_FIVE_TRIAL(...) ((void)0)\n#define NL_FIVE_ARM(...) "
        "((void)0)\n"
        "#define NL_FIVE_SLOT(...) ((void)0)\n#define NL_FIVE_RAW(...) "
        "((void)0)\n"
        "#define NL_FIVE_DOMAIN(...) ((void)0)\n#define NL_FIVE_INIT(...) "
        "((void)0)\n"
        "#define NL_FIVE_LOAN(...) ((void)0)\n#define NL_FIVE_SCOPE_END(...) "
        "((void)0)\n"
        "#define NL_FIVE_ROOT(...) ((void)0)\n#define NL_FIVE_FIELD(...) "
        "((void)0)\n"
        "#define NL_FIVE_CHANGE(...) ((void)0)\n#define NL_FIVE_END(...) "
        "((void)0)\n"
        "#define NL_FIVE_FINALIZE(...) ((void)0)\n#define NL_FIVE_RELEASE(...) "
        "((void)0)\n"
        "#define NL_FIVE_FINISH() ((void)0)\n#endif\nint main(void){\n");
    unsigned char visited[FIVE_NODES] = {0};
    World w = {.fragment = body, .visited = visited};
    Result result;
    if (!expr(&e, &w, nl_checked_root(body), &result) ||
        result.type != e.unit || e.sites != 5 || e.changes != 6 ||
        !covered(&e, &w) || !put(&e, "NL_FIVE_FINISH();\nreturn 0;\n}\n")) {
        no(&e);
        free(e.text);
        return e.status;
    }
    *out = e.text;
    *length = e.used;
    return NL_NODE_C_OK;
}
