#include "newlang/checked_c_node.h"
#include <stdarg.h>
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
    char *text;
    size_t used, temp, steps, frames, roots;
    NLTypeId node, option, ptr, u8, unit;
    NLNodeCStatus status;
} Emit;
typedef struct Env {
    Local locals[C_LOCALS];
    size_t count, frame, depth, prefix;
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
    env->locals[env->count++] = l;
    return true;
}
static const char *ctype(const Emit *e, NLTypeId type)
{
    if (type == e->node)
        return "nl_node";
    if (type == e->option)
        return "nl_node_option";
    if (type == e->ptr)
        return "const nl_node *";
    if (type == e->u8)
        return "uint8_t";
    return NULL;
}
static bool shape(Emit *e, const Env *env, NLTypeId type)
{
    const NLSemanticContext *c = nl_checked_context(env->artifact);
    NLAggregateField link, payload;
    NLSemanticTypeView option, ptr;
    /* Registry predicate includes completed recursive-header identity and the
     * exact Option constructor metadata; names are never consulted. */
    if (!nl_semantic_recursive_local_type(c, type) ||
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
static bool block(Emit *e, Env *env, NLCheckedNodeId id, Value *out)
{
    const NLCheckedNodeView *v = get(env, id);
    if (v == NULL || v->kind != NL_CHECKED_BLOCK || v->terminates ||
        v->type != e->unit || v->result_count != 0 || env->depth >= C_DEPTH)
        return reject(e);
    NLCheckedNodeId item = v->first_item;
    ++env->depth;
    for (size_t i = 0; i < v->item_count; ++i) {
        const NLCheckedNodeView *n = get(env, item);
        Value ignored = {0};
        if (n == NULL || !expr(e, env, item, &ignored))
            return reject(e);
        if (ignored.temp != 0 && !put(e, "(void)nl_v_%zu;\n", ignored.temp))
            return false;
        item = n->next_item;
    }
    --env->depth;
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
                      .depth = env->depth + 1};
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
    if (ctype(e, v->type) != NULL &&
        (v->result_count != 1 || v->results[0].type != v->type ||
         v->results[0].value == 0))
        return reject(e);
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
            (r->type == e->node && env->parent != NULL))
            return reject(e);
        Value value = {0};
        if (!expr(e, env, v->initializer, &value) || r->type != value.type ||
            ctype(e, value.type) == NULL ||
            !add_local(e, env, r->symbol, r->type, r->type == e->node))
            return reject(e);
        if (r->type == e->node && ++e->roots > 2)
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
                   r->type == e->node || r->type == e->ptr ? "" : "const ",
                   ctype(e, r->type), r->type == e->ptr ? " const" : "",
                   env->frame, r->symbol, value.temp, env->frame, r->symbol,
                   env->frame, r->symbol, kind, env->frame, r->symbol);
    }
    if (v->kind == NL_CHECKED_AGGREGATE) {
        if (env->parent != NULL || v->argument_count != 2 ||
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
        if (l == NULL || v->value_use != NL_VALUE_COPIED ||
            (v->type != e->ptr && v->type != e->option && v->type != e->u8))
            return reject(e);
        out->temp = ++e->temp;
        return put(e, "%s nl_v_%zu = nl_b_%zu_%zu;\n", ctype(e, v->type),
                   out->temp, l->frame, l->symbol);
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
            (ctype(e, v->type) == NULL ||
             !put(e, "%s nl_v_%zu;\n", ctype(e, v->type), slot)))
            return reject(e);
        Value result = {.temp = slot};
        if (!loan(e, env, v, &result))
            return false;
        *out = result;
        return true;
    }
    return reject(e);
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
    Emit e = {.unit = nl_semantic_unit_type(c),
              .u8 = nl_semantic_core_type(c, NL_TYPE_U8)};
    if (v->type != e.unit)
        return NL_NODE_C_UNSUPPORTED;
    e.text = malloc(C_BYTES);
    if (e.text == NULL)
        return NL_NODE_C_OUT_OF_MEMORY;
    Env env = {.artifact = body};
    (void)put(
        &e,
        "/* Bounded Checked-C representation, not a NewLang ABI. */\n"
        "#include <stdint.h>\n#include <stddef.h>\n"
        "typedef struct nl_node nl_node;\n"
        "typedef struct { unsigned tag; const nl_node *ptr; } nl_node_option;\n"
        "struct nl_node { nl_node_option f0; uint8_t f1; };\n"
        "#ifdef NEWLANG_NODE_OBSERVER\n#include NEWLANG_NODE_OBSERVER\n"
        "#else\n#define NL_NODE_BIND(...) ((void)0)\n"
        "#define NL_NODE_REPLACE(...) ((void)0)\n"
        "#define NL_NODE_ARM(...) ((void)0)\n"
        "#define NL_NODE_RELOAN(...) ((void)0)\n"
        "#define NL_NODE_FINISH() ((void)0)\n#endif\n"
        "int main(void) {\n");
    Value result = {0};
    if (!block(&e, &env, nl_checked_root(body), &result) || e.roots != 2 ||
        result.type != e.unit ||
        !put(&e, "NL_NODE_FINISH();\nreturn 0;\n}\n")) {
        if (e.status == NL_NODE_C_OK)
            e.status = NL_NODE_C_UNSUPPORTED;
        free(e.text);
        return e.status;
    }
    *out = e.text;
    *length = e.used;
    return NL_NODE_C_OK;
}
