#include "producer_local_internal.h"
#include <stdlib.h>

#include <string.h>

/* A finite conditional type/ownership transfer, not a hypothetical semantic
 * world. Roles refer to uncorrelated signature origins p/a/d. Slot/raw roles
 * denote conditional operation results; no Storage or root is minted here. */
typedef enum {
    U,
    P,
    A,
    D,
    STABLE,
    ENDING,
    REF,
    EMPTY,
    RAW,
    HEAD,
    OPTION,
    LIVE_TAIL
} Role;
typedef struct {
    const char *parameter; /* owned by immutable body plan, borrowed here */
    NLSourceSpan name;
    Role role;
    bool available;
} Symbol;
typedef struct {
    const NLSemanticContext *registry;
    const NLFunctionBody *body;
    NLTypedOwnerDefinition definition;
    Symbol symbols[128];
    size_t count, depth;
    bool ended, finalized, released, returned, detached;
    NLCheckStatus status;
    NLCheckDiagnostic diagnostic;
} Infer;

bool nl_owner_signature(const NLSemanticContext *c, const NLTypeId *types,
                        size_t count, NLTypeId result)
{
    if (count != 3 || result != nl_semantic_unit_type(c))
        return false;
    const NLSemanticTypeView p = c->types[types[0] - 1].view;
    return p.kind == NL_TYPE_PTR && nl_recursive_local_type(c, p.target) &&
           c->types[types[1] - 1].view.kind == NL_TYPE_ALLOCATION &&
           types[2] == nl_semantic_domain_type(c);
}

bool nl_producer_signature(const NLSemanticContext *c, const NLTypeId *types,
                           size_t count, NLTypeId result)
{
    if (count != 4 || c->types[result - 1].live_tail_target == 0)
        return false;
    const NLTypeId h = c->types[result - 1].live_tail_target;
    const NLSemanticTypeView head = c->types[types[0] - 1].view;
    return head.kind == NL_TYPE_REF && !head.is_exclusive &&
           head.access == NL_ACCESS_WRITE &&
           c->types[head.target - 1].option_target == types[1] &&
           c->types[types[1] - 1].view.kind == NL_TYPE_PTR &&
           c->types[types[1] - 1].view.target == h &&
           c->types[types[2] - 1].view.kind == NL_TYPE_ALLOCATION &&
           types[3] == nl_semantic_domain_type(c);
}

static Role error(Infer *i, NLSourceSpan span, NLCheckStatus status,
                  const char *code, const char *message)
{
    if (i->status == NL_CHECK_OK) {
        i->status = status;
        i->diagnostic = (NLCheckDiagnostic){
            {NL_DIAG_ERROR,
             status == NL_CHECK_ANALYSIS_PRECISION_LIMIT ? "precision"
                                                         : "semantic",
             code, message, NULL, NULL, 0},
            span};
    }
    return U;
}
static bool text(const Infer *i, NLSourceSpan s, const char *name)
{
    NLSourceView v;
    return nl_source_view(i->body->source, s, &v) && v.length == strlen(name) &&
           memcmp(v.bytes, name, v.length) == 0;
}
static bool same(const Infer *i, NLSourceSpan a, NLSourceSpan b)
{
    NLSourceView x, y;
    return nl_source_view(i->body->source, a, &x) &&
           nl_source_view(i->body->source, b, &y) && x.length == y.length &&
           memcmp(x.bytes, y.bytes, x.length) == 0;
}
static bool affine(Role r)
{
    return r == A || r == D || r == EMPTY || r == RAW || r == LIVE_TAIL;
}
static Role use(Infer *i, const NLSyntaxNode *n, bool consume)
{
    const NLSyntaxView *v = nl_syntax_node_view(n);
    if (v == NULL || v->kind != NL_SYNTAX_EXPR_NAME)
        return error(i, v == NULL ? (NLSourceSpan){0} : v->span,
                     NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                     "P193-DEFINITION-PROFILE",
                     "operand needs a symbolic name");
    for (size_t j = i->count; j != 0; --j) {
        Symbol *s = &i->symbols[j - 1];
        if (s->parameter != NULL ? text(i, v->data.name, s->parameter)
                                 : same(i, v->data.name, s->name)) {
            if (!s->available)
                return error(i, v->span, NL_CHECK_SEMANTIC_ERROR,
                             "P193-DEFINITION-CONSUMED",
                             "symbolic non-Copy responsibility was consumed");
            if (consume && affine(s->role))
                s->available = false;
            return s->role;
        }
    }
    if (text(i, v->data.name, "unit"))
        return U;
    return error(i, v->span, NL_CHECK_SEMANTIC_ERROR, "P193-DEFINITION-NAME",
                 "name has no symbolic definition");
}
static void bind(Infer *i, NLSourceSpan name, Role role, size_t floor)
{
    NLSourceView v;
    if (!nl_source_view(i->body->source, name, &v) ||
        !nl_sem_lexical_name_admissible(v.bytes, v.length)) {
        (void)error(i, name, NL_CHECK_SEMANTIC_ERROR, "P193-DEFINITION-NAME",
                    "inadmissible lexical binder");
        return;
    }
    for (size_t j = floor; j < i->count; ++j) {
        const Symbol s = i->symbols[j];
        if (s.parameter != NULL ? text(i, name, s.parameter)
                                : same(i, name, s.name)) {
            (void)error(i, name, NL_CHECK_SEMANTIC_ERROR,
                        "P193-DEFINITION-NAME", "duplicate lexical binder");
            return;
        }
    }
    if (i->count == 128) {
        (void)error(i, name, NL_CHECK_RESOURCE_LIMIT, "P193-DEFINITION-LIMIT",
                    "symbolic binder budget exceeded");
        return;
    }
    i->symbols[i->count++] = (Symbol){NULL, name, role, true};
}
static void step(Infer *i, NLTypedOwnerStep s, NLSourceSpan span)
{
    const size_t count = i->definition.step_count;
    if ((count == 0 && s != NL_OWNER_READ_ROOT && s != NL_OWNER_END_ROOT) ||
        (count != 0 && s != i->definition.steps[count - 1] + 1)) {
        (void)error(i, span, NL_CHECK_SEMANTIC_ERROR, "P193-DEFINITION-ORDER",
                    "lifecycle primitive order invalid");
        return;
    }
    if (i->definition.step_count == 5) {
        (void)error(i, span, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                    "P193-DEFINITION-PROFILE", "outside finite receiver steps");
        return;
    }
    i->definition.steps[i->definition.step_count++] = s;
}
static Role expr(Infer *, const NLSyntaxNode *);
static Role block(Infer *i, const NLSyntaxView *v, size_t floor)
{
    if (v == NULL || v->kind != NL_SYNTAX_BLOCK)
        return error(i, (NLSourceSpan){0}, NL_CHECK_SEMANTIC_ERROR,
                     "P193-DEFINITION-BLOCK",
                     "receiver requires lexical block");
    for (const NLSyntaxNode *n = v->data.block.items;
         n != NULL && i->status == NL_CHECK_OK && !i->returned;
         n = nl_syntax_next_argument(n)) {
        const NLSyntaxView *item = nl_syntax_node_view(n);
        if (item->kind == NL_SYNTAX_BINDING) {
            Role r = expr(i, item->data.binding.initializer);
            bind(i, item->data.binding.name, r, floor);
        } else if (item->kind == NL_SYNTAX_STATEMENT ||
                   item->kind == NL_SYNTAX_RETURN) {
            Role r = expr(i, item->data.statement.expression);
            if (affine(r) && !(item->kind == NL_SYNTAX_RETURN &&
                               i->definition.live_return && r == LIVE_TAIL))
                (void)error(i, item->span, NL_CHECK_SEMANTIC_ERROR,
                            "P193-DEFINITION-OBLIGATION",
                            "non-Discardable conditional result is discarded");
            if (item->kind == NL_SYNTAX_RETURN) {
                if (i->depth != 0 ||
                    (i->definition.live_return ? r != LIVE_TAIL || !i->detached
                                               : r != U))
                    (void)error(i, item->span, NL_CHECK_SEMANTIC_ERROR,
                                "P193-DEFINITION-ESCAPE",
                                "return cannot escape a loan or capability");
                i->returned = true;
            }
        } else {
            return error(i, item->span, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                         "P193-DEFINITION-PROFILE",
                         "control/receiving form outside bounded receiver");
        }
    }
    Role result = U;
    if (!i->returned && v->data.block.tail != NULL)
        result = expr(i, v->data.block.tail);
    for (size_t j = floor; j < i->count; ++j)
        if (i->symbols[j].available && affine(i->symbols[j].role))
            (void)error(i, v->span, NL_CHECK_SEMANTIC_ERROR,
                        "P193-DEFINITION-OBLIGATION",
                        "normal exit retains non-Discardable responsibility");
    return result;
}
static Role expr(Infer *i, const NLSyntaxNode *node)
{
    const NLSyntaxView *v = nl_syntax_node_view(node);
    if (v == NULL || i->status != NL_CHECK_OK)
        return U;
    if (v->kind == NL_SYNTAX_EXPR_NAME)
        return use(i, node, true);
    if (i->definition.live_return && v->kind == NL_SYNTAX_SUM_CONSTRUCTOR) {
        const NLSyntaxView *type =
            nl_syntax_node_view(v->data.constructor.type);
        const NLSyntaxView *ptr =
            type == NULL ? NULL
                         : nl_syntax_node_view(type->data.ptr_type.target);
        const char *h = i->registry->types[i->definition.target - 1].name;
        if (type == NULL || type->kind != NL_SYNTAX_OPTION_PTR || ptr == NULL ||
            ptr->kind != NL_SYNTAX_TYPE_NAME || !text(i, ptr->data.name, h) ||
            !text(i, v->data.constructor.variant, "None") ||
            v->data.constructor.parentheses ||
            v->data.constructor.argument_count != 0)
            return error(i, v->span, NL_CHECK_SEMANTIC_ERROR,
                         "P208-DEFINITION-TYPE",
                         "detach requires exact H Option None");
        return OPTION;
    }
    if (i->definition.live_return && v->kind == NL_SYNTAX_AGGREGATE) {
        if (!i->detached || !text(i, v->data.aggregate.type_name, "LiveTail") ||
            v->data.aggregate.count != 3)
            return error(i, v->span, NL_CHECK_SEMANTIC_ERROR,
                         "P208-DEFINITION-RETURN",
                         "complete LiveTail requires prior detach");
        const char *names[] = {"owned_ptr", "owned_allocation", "owned_domain"};
        const Role roles[] = {P, A, D};
        bool seen[3] = {false};
        for (const NLSyntaxNode *n = v->data.aggregate.fields; n != NULL;
             n = nl_syntax_next_argument(n)) {
            const NLSyntaxView *f = nl_syntax_node_view(n);
            size_t index = 3;
            for (size_t j = 0; j < 3; ++j)
                if (text(i, f->data.binding.name, names[j]))
                    index = j;
            if (index == 3 || seen[index] ||
                use(i, f->data.binding.initializer, true) != roles[index])
                return error(i, f->span, NL_CHECK_SEMANTIC_ERROR,
                             "P208-DEFINITION-RETURN",
                             "field must transfer its original symbolic input "
                             "role once");
            seen[index] = true;
        }
        for (size_t j = 0; j < 3; ++j)
            if (!seen[j])
                return error(i, v->span, NL_CHECK_SEMANTIC_ERROR,
                             "P208-DEFINITION-RETURN", "missing field");
        return LIVE_TAIL;
    }
    if (i->definition.live_return && v->kind == NL_SYNTAX_EXPR_CALL &&
        text(i, v->data.call.callee, "replace")) {
        const NLSyntaxNode *a = v->data.call.arguments;
        if (i->detached || v->data.call.argument_count != 2 ||
            use(i, a, false) != HEAD ||
            expr(i, nl_syntax_next_argument(a)) != OPTION)
            return error(
                i, v->span, NL_CHECK_SEMANTIC_ERROR, "P208-DEFINITION-DETACH",
                "producer must detach the input head ref exactly once");
        i->detached = true;
        return OPTION;
    }
    if (i->definition.live_return)
        return error(
            i, v->span, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
            "P208-DEFINITION-PROFILE",
            "operation needs obligations outside finite producer definition");
    if (v->kind == NL_SYNTAX_LOCAL_READ_LOAN) {
        if (use(i, v->data.loan.source, false) != D || i->finalized ||
            i->depth != 0)
            return error(i, v->span, NL_CHECK_SEMANTIC_ERROR,
                         "P193-DEFINITION-ORDER", "invalid domain loan source");
        const size_t floor = i->count;
        bind(i, v->data.loan.binding,
             v->data.loan.is_exclusive ? ENDING : STABLE, floor);
        ++i->depth;
        const Role r = block(i, nl_syntax_node_view(v->data.loan.body), floor);
        --i->depth;
        i->count = floor;
        if (r != (v->data.loan.is_exclusive ? EMPTY : U))
            return error(i, v->span, NL_CHECK_SEMANTIC_ERROR,
                         "P193-DEFINITION-ESCAPE",
                         "loan must return only unit or recovered empty slot");
        return r;
    }
    if (v->kind == NL_SYNTAX_ALLOCATED_REF) {
        const NLSyntaxNode *p = v->data.call.arguments;
        if (v->data.call.argument_count != 2 ||
            v->data.call.access != NL_ACCESS_READ || i->ended ||
            use(i, p, false) != P ||
            use(i, nl_syntax_next_argument(p), false) != STABLE)
            return error(
                i, v->span, NL_CHECK_SEMANTIC_ERROR, "P193-DEFINITION-RELATION",
                "read reloan needs input pointer and same-domain loan");
        step(i, NL_OWNER_READ_ROOT, v->span);
        return REF;
    }
    if (v->kind == NL_SYNTAX_ALLOCATED_ERASE_SLOT) {
        const NLSyntaxView *t = nl_syntax_node_view(v->data.call.type);
        const char *h = i->registry->types[i->definition.target - 1].name;
        if (v->data.call.argument_count != 1 || t == NULL ||
            t->kind != NL_SYNTAX_TYPE_NAME || !text(i, t->data.name, h) ||
            use(i, v->data.call.arguments, true) != EMPTY || !i->ended)
            return error(i, v->span, NL_CHECK_SEMANTIC_ERROR,
                         "P193-DEFINITION-TYPE",
                         "erase requires exact H empty conditional result");
        step(i, NL_OWNER_ERASE_SLOT, v->span);
        return RAW;
    }
    if (v->kind == NL_SYNTAX_EXPR_CALL) {
        const NLSyntaxNode *a = v->data.call.arguments;
        if (text(i, v->data.call.callee, "destroy")) {
            if (v->data.call.argument_count != 2 || i->ended ||
                use(i, a, false) != P ||
                use(i, nl_syntax_next_argument(a), false) != ENDING)
                return error(
                    i, v->span, NL_CHECK_SEMANTIC_ERROR,
                    "P193-DEFINITION-ORDER",
                    "EndRoot requires input pointer and exclusive loan");
            i->ended = true;
            step(i, NL_OWNER_END_ROOT, v->span);
            return EMPTY;
        }
        if (text(i, v->data.call.callee, "finalize_domain")) {
            if (v->data.call.argument_count != 1 || !i->ended || i->finalized ||
                i->depth != 0 || use(i, a, true) != D)
                return error(i, v->span, NL_CHECK_SEMANTIC_ERROR,
                             "P193-DEFINITION-ORDER",
                             "domain finalization requires completed EndRoot");
            i->finalized = true;
            step(i, NL_OWNER_FINALIZE_DOMAIN, v->span);
            return U;
        }
        if (text(i, v->data.call.callee, "deallocate")) {
            if (v->data.call.argument_count != 2 || !i->finalized ||
                i->released || use(i, a, true) != A ||
                use(i, nl_syntax_next_argument(a), true) != RAW)
                return error(
                    i, v->span, NL_CHECK_SEMANTIC_ERROR,
                    "P193-DEFINITION-RELATION",
                    "release requires original Allocation and raw chain");
            i->released = true;
            step(i, NL_OWNER_END_REGION, v->span);
            return U;
        }
    }
    return error(
        i, v->span, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
        "P193-DEFINITION-PROFILE",
        "operation needs an unrelated or unsupported entry obligation");
}

NLCheckStatus nl_owner_definition(const NLSemanticContext *c,
                                  NLFunctionBody *body, NLTypeId h,
                                  NLTypedOwnerDefinition *out,
                                  NLCheckDiagnostic *diagnostic)
{
    const bool producer = body->count == 4;
    Infer i = {.registry = c,
               .body = body,
               .definition = {.target = h,
                              .requirements = NL_OWNER_ALL_REQUIREMENTS,
                              .live_return = producer,
                              .head_link_required = producer},
               .count = body->count};
    const Role roles[] = {HEAD, P, A, D};
    for (size_t j = 0; j < body->count; ++j)
        i.symbols[j] = (Symbol){
            body->parameter_names[j], {0}, roles[j + (producer ? 0 : 1)], true};
    const NLSyntaxView *root =
        nl_syntax_node_view(nl_syntax_tree_root(body->syntax));
    const Role result = block(&i, root, 0);
    if (i.status == NL_CHECK_OK &&
        (producer ? (!i.returned || !i.detached || i.ended || i.finalized ||
                     i.released)
                  : (result != U || !i.ended || !i.finalized || !i.released)))
        (void)error(&i, root->span, NL_CHECK_SEMANTIC_ERROR,
                    "P193-DEFINITION-OBLIGATION",
                    "normal receiver exit lacks complete lifecycle closure");
    if (i.status != NL_CHECK_OK) {
        if (diagnostic != NULL)
            *diagnostic = i.diagnostic;
        return i.status;
    }
    i.definition.definition_checked = true;
    *out = i.definition;
    return NL_CHECK_OK;
}

bool nl_semantic_function_applicability(const NLSemanticContext *c,
                                        size_t function,
                                        NLTypedOwnerDefinition *out)
{
    if (c == NULL || out == NULL || function == 0 ||
        function > c->function_count || c->functions[function - 1].body == NULL)
        return false;
    const NLTypedOwnerDefinition d =
        c->functions[function - 1].body->owner_definition;
    if (!d.definition_checked)
        return false;
    *out = d;
    return true;
}

static NLCheckStatus relation_error(NLCheckDiagnostic *out,
                                    NLCheckStatus status, NLSourceSpan span,
                                    const char *code, const char *message)
{
    if (out != NULL)
        *out = (NLCheckDiagnostic){{NL_DIAG_ERROR,
                                    status == NL_CHECK_ANALYSIS_PRECISION_LIMIT
                                        ? "precision"
                                        : "semantic",
                                    code, message, NULL, NULL, 0},
                                   span};
    return status;
}

/* Compiler-internal predicate; consumes nothing, mints nothing. Tests may
 * mutate copies of source-derived entry snapshots to attack proof precision. */
NLCheckStatus nl_owner_relations(const NLSemanticContext *c,
                                 const NLValueId *values,
                                 const NLTypedOwnerDefinition *d,
                                 NLSourceSpan span,
                                 NLCheckDiagnostic *diagnostic)
{
    if (c == NULL || values == NULL || d == NULL || !d->definition_checked ||
        d->target == 0 || d->target > c->type_count ||
        d->requirements != NL_OWNER_ALL_REQUIREMENTS)
        return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < 3; ++i) {
        if (values[i] == 0 || values[i] > c->value_count)
            return NL_CHECK_INTERNAL_ERROR;
        if (c->values[values[i] - 1].dependencies != NL_DEPENDENCY_FREE ||
            c->values[values[i] - 1].value_dependency_count != 0)
            return relation_error(diagnostic, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                                  span, "P193-CALL-DEPENDENCY",
                                  "input dependency proof is incomplete");
    }
    const NLSemanticValueView p = c->values[values[0] - 1],
                              a = c->values[values[1] - 1],
                              domain = c->values[values[2] - 1];
    if (p.reference_count != 0 ||
        p.reference.provenance == NL_PROVENANCE_UNKNOWN) {
        return relation_error(diagnostic, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                              span, "P193-CALL-ROOT-PRECISION",
                              "unknown root origin cannot discharge entry");
    }
    const NLReferenceFacts f = p.reference;
    if (f.provenance != NL_PROVENANCE_VALID || f.place == 0 ||
        f.place > c->place_count || !nl_fixed_live(c, f.place) ||
        c->places[f.place - 1].incarnation != f.incarnation)
        return relation_error(diagnostic, NL_CHECK_SEMANTIC_ERROR, span,
                              "P193-CALL-ROOT",
                              "pointer origin is not current");
    const NLSemanticPlaceView root = c->places[f.place - 1];
    const NLSemanticTypeView h = c->types[d->target - 1].view;
    if (root.type != d->target || !root.independent_root ||
        root.parent_sum != 0 || root.parent_aggregate != 0 ||
        root.placement.region == 0 || root.placement.region > c->region_count ||
        !h.layout_known || !h.is_discardable || root.current_value == 0 ||
        !f.readable || f.occurrence_dependency != 0) {
        return relation_error(
            diagnostic, NL_CHECK_SEMANTIC_ERROR, span, "P193-CALL-ROOT",
            "entry requires the current complete allocated H root");
    }
    if (domain.domain == 0 || domain.domain > c->domain_count ||
        !c->domains[domain.domain - 1].live ||
        c->domains[domain.domain - 1].value != values[2] ||
        root.governing_domain != domain.domain) {
        return relation_error(
            diagnostic, NL_CHECK_SEMANTIC_ERROR, span, "P193-CALL-DOMAIN",
            "input LifetimeDomain does not govern the pointer's root");
    }
    if (a.allocation_region != root.placement.region) {
        return relation_error(
            diagnostic, NL_CHECK_SEMANTIC_ERROR, span, "P193-CALL-BACKING",
            "input Allocation does not own the root's BackingRegion");
    }
    const NLSemanticBackingView r = c->regions[root.placement.region - 1].view;
    if (!r.live || !r.ordinary_read || !r.ordinary_write ||
        root.placement.start != 0 || root.placement.length != h.size ||
        root.placement.length != r.size || h.alignment == 0 ||
        r.alignment < h.alignment || a.carrier != NL_CARRIER_LOOSE ||
        domain.carrier != NL_CARRIER_LOOSE) {
        return relation_error(
            diagnostic, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
            "P193-CALL-RECOVERY",
            "full original range/authority recovery unproved");
    }
    return NL_CHECK_OK;
}

/* Read-only consumer contract: the live-return certificate belongs to exactly
 * its owned caller entry/return worlds and synchronous checked producer body.
 * A cloned world's coincidental numeric IDs are not this evidence. */
bool nl_producer_valid(const NLCheckedFragment *f, NLCheckedNodeId id, bool raw)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    const NLSemanticContext *a = nl_checked_producer_entry(f, id),
                            *b = nl_checked_producer_return(f, id);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (v == NULL || a == NULL || b == NULL || a == b ||
        v->producer.entry_world != a || v->producer.return_world != b ||
        body == NULL || nl_checked_context(body) != nl_checked_context(f) ||
        !v->body_backed || !v->producer.entry_proved ||
        !v->producer.return_proved ||
        !v->producer.definition.definition_checked ||
        !v->producer.definition.live_return ||
        !v->producer.definition.head_link_required ||
        v->producer.definition.step_count != 0 ||
        v->producer.definition.requirements != NL_OWNER_ALL_REQUIREMENTS ||
        !v->producer.head.present || v->producer.head.index != 0 ||
        v->producer.head.access != NL_ACCESS_WRITE ||
        v->producer.head.nominal != v->producer.definition.target ||
        v->function == 0 || v->function > a->function_count ||
        v->argument_count != 4 ||
        !a->functions[v->function - 1].owner_producer ||
        body->body_owner != a->functions[v->function - 1].body ||
        v->result_count != 1 || v->results[0].value != v->producer.result ||
        v->results[0].type != a->functions[v->function - 1].result)
        return false;
    if (nl_sem_validate(a) != NL_CHECK_OK ||
        nl_sem_validate(b) != NL_CHECK_OK ||
        (raw && (nl_raw_validate(a) != NL_CHECK_OK ||
                 nl_raw_validate(b) != NL_CHECK_OK)))
        return false;
    const NLTypedOwnerDefinition def =
        a->functions[v->function - 1].body->owner_definition;
    if (def.target != v->producer.definition.target || !def.live_return ||
        !def.head_link_required || !def.definition_checked ||
        def.step_count != 0 || def.requirements != NL_OWNER_ALL_REQUIREMENTS)
        return false;
    for (size_t i = 0; i < 4; ++i)
        if (v->producer.inputs[i] == 0 ||
            v->producer.inputs[i] > a->value_count ||
            v->producer.inputs[i] > b->value_count ||
            v->producer.parameters[i] == 0 ||
            v->producer.parameters[i] > b->binding_count ||
            b->bindings[v->producer.parameters[i] - 1].view.value !=
                v->producer.inputs[i] ||
            b->bindings[v->producer.parameters[i] - 1].view.place ==
                v->producer.root)
            return false;
    if (nl_owner_relations(a, v->producer.inputs + 1, &def, v->span, NULL) !=
            NL_CHECK_OK ||
        v->producer.root == 0 || v->producer.root > a->place_count ||
        v->producer.root > b->place_count || v->producer.head.parent == 0 ||
        v->producer.head.parent > a->place_count ||
        v->producer.head.parent > b->place_count ||
        v->producer.head.child == 0 ||
        v->producer.head.child > a->place_count ||
        v->producer.head.child > b->place_count ||
        v->producer.range.region == 0 ||
        v->producer.range.region > b->region_count || v->producer.domain == 0 ||
        v->producer.domain > b->domain_count || v->producer.result == 0 ||
        v->producer.result > b->value_count)
        return false;
    const NLSemanticPlaceView t = a->places[v->producer.root - 1],
                              rt = b->places[v->producer.root - 1],
                              h = a->places[v->producer.head.parent - 1],
                              rh = b->places[v->producer.head.parent - 1],
                              l = a->places[v->producer.head.child - 1],
                              rl = b->places[v->producer.head.child - 1];
    const NLSemanticValueView p = a->values[v->producer.inputs[1] - 1],
                              href = a->values[v->producer.inputs[0] - 1],
                              package = b->values[v->producer.result - 1];
    if (p.reference.place != v->producer.root ||
        p.reference.incarnation != v->producer.incarnation || !t.live ||
        !rt.live || t.incarnation != rt.incarnation ||
        rt.incarnation != v->producer.incarnation ||
        t.governing_domain != v->producer.domain ||
        rt.governing_domain != v->producer.domain ||
        t.current_value != rt.current_value ||
        t.current_fact != rt.current_fact ||
        t.placement.start != v->producer.range.start ||
        t.placement.length != v->producer.range.length ||
        t.placement.region != v->producer.range.region ||
        rt.placement.region != t.placement.region ||
        rt.placement.start != t.placement.start ||
        rt.placement.length != t.placement.length ||
        !b->regions[v->producer.range.region - 1].view.live ||
        !b->domains[v->producer.domain - 1].live || !h.live || !rh.live ||
        h.incarnation != rh.incarnation ||
        h.incarnation != v->producer.head.parent_incarnation ||
        h.governing_domain != v->producer.head_domain ||
        rh.governing_domain != h.governing_domain ||
        h.placement.region == t.placement.region ||
        rh.placement.region != h.placement.region ||
        h.governing_domain == t.governing_domain || !l.live || !rl.live ||
        l.incarnation != rl.incarnation ||
        l.incarnation != v->producer.head.child_incarnation ||
        l.parent_aggregate != v->producer.head.parent ||
        rl.parent_aggregate != l.parent_aggregate ||
        l.parent_field_index != 0 || rl.parent_field_index != 0 ||
        l.current_value != v->producer.head_before ||
        l.current_fact != v->producer.head_before_fact ||
        rl.current_value != v->producer.head_after ||
        rl.current_fact != v->producer.head_after_fact ||
        l.current_fact == rl.current_fact || rl.payload_occurrence != 0 ||
        href.reference.place != v->producer.head.child ||
        href.reference.incarnation != l.incarnation ||
        href.reference_count != 0 ||
        href.reference.provenance != NL_PROVENANCE_VALID ||
        !href.reference.readable || !href.reference.writable ||
        href.dependencies != NL_DEPENDENCY_FREE || href.reference.scope == 0 ||
        href.reference.scope > a->scope_count ||
        !a->scopes[href.reference.scope - 1].active || l.current_value == 0 ||
        l.current_value > a->value_count || rl.current_value == 0 ||
        rl.current_value > b->value_count ||
        a->values[l.current_value - 1].variant != 2 ||
        b->values[rl.current_value - 1].variant != 1 ||
        package.carrier != NL_CARRIER_LOOSE ||
        package.type != v->results[0].type || package.field_count != 3 ||
        package.dependencies != NL_DEPENDENCY_FREE ||
        package.value_dependency_count != 0)
        return false;
    const NLSemanticBackingView
        ar = a->regions[v->producer.range.region - 1].view,
        br = b->regions[v->producer.range.region - 1].view;
    if (ar.size != br.size || ar.alignment != br.alignment ||
        ar.ordinary_read != br.ordinary_read ||
        ar.ordinary_write != br.ordinary_write)
        return false;
    const NLSemanticTypeView ht = a->types[href.type - 1].view;
    if (ht.kind != NL_TYPE_REF || ht.is_exclusive ||
        ht.access != NL_ACCESS_WRITE || ht.target != l.type ||
        v->producer.head.type != l.type ||
        v->producer.head.child_fact != l.current_fact ||
        v->producer.head.old_value != l.current_value ||
        v->producer.head.parent_fact != h.current_fact ||
        h.type != def.target || !h.independent_root ||
        h.placement.region == 0 || h.placement.region > a->region_count ||
        h.placement.region > b->region_count || h.governing_domain == 0 ||
        h.governing_domain > a->domain_count ||
        h.governing_domain > b->domain_count ||
        !a->domains[h.governing_domain - 1].live ||
        !b->domains[h.governing_domain - 1].live ||
        !a->regions[h.placement.region - 1].view.live ||
        !b->regions[h.placement.region - 1].view.live ||
        !a->regions[h.placement.region - 1].view.ordinary_read ||
        !a->regions[h.placement.region - 1].view.ordinary_write ||
        !b->regions[h.placement.region - 1].view.ordinary_read ||
        !b->regions[h.placement.region - 1].view.ordinary_write)
        return false;
    bool stable_origin = false;
    for (size_t i = 0; i < a->value_count; ++i) {
        const NLSemanticValueView stable = a->values[i];
        const NLSemanticTypeView st = a->types[stable.type - 1].view;
        const NLReferenceFacts sf = stable.reference;
        if (stable.carrier != NL_CARRIER_ENDED &&
            stable.dependencies == NL_DEPENDENCY_FREE &&
            stable.value_dependency_count == 0 && st.kind == NL_TYPE_REF &&
            st.target == 2 && !st.is_exclusive && stable.reference_count == 0 &&
            sf.scope == href.reference.scope &&
            sf.provenance == NL_PROVENANCE_VALID && sf.readable &&
            sf.place != 0 && sf.place <= a->place_count) {
            const NLSemanticPlaceView dp = a->places[sf.place - 1];
            if (dp.live && dp.incarnation == sf.incarnation &&
                dp.current_value != 0 && dp.type == 2 &&
                a->values[dp.current_value - 1].domain == h.governing_domain)
                stable_origin = true;
        }
    }
    if (!stable_origin)
        return false;
    const NLValueId payload = a->values[l.current_value - 1].sum_payload;
    if (payload == 0 || payload > a->value_count ||
        a->values[payload - 1].reference.place != v->producer.root ||
        a->values[payload - 1].reference.incarnation != v->producer.incarnation)
        return false;
    const NLSemanticValueView payload_value = a->values[payload - 1];
    if (payload_value.type != p.type || payload_value.reference_count != 0 ||
        payload_value.reference.provenance != NL_PROVENANCE_VALID ||
        payload_value.dependencies != NL_DEPENDENCY_FREE ||
        payload_value.value_dependency_count != 0 ||
        payload_value.reference.scope != p.reference.scope ||
        payload_value.reference.readable != p.reference.readable ||
        payload_value.reference.writable != p.reference.writable ||
        payload_value.reference.occurrence_dependency !=
            p.reference.occurrence_dependency)
        return false;
    for (size_t i = 0; i < 3; ++i) {
        const NLValueId field = package.fields[i];
        if (field == 0 || field > b->value_count)
            return false;
        const NLSemanticValueView m = b->values[field - 1];
        if (m.carrier != NL_CARRIER_AGGREGATE ||
            m.aggregate_owner != v->producer.result ||
            m.dependencies != NL_DEPENDENCY_FREE ||
            m.value_dependency_count != 0)
            return false;
        if (i == 0) {
            if (m.reference.place != v->producer.root ||
                m.reference.incarnation != v->producer.incarnation ||
                m.reference.provenance != NL_PROVENANCE_VALID ||
                m.reference_count != 0 ||
                m.reference.scope != p.reference.scope ||
                m.reference.occurrence_dependency !=
                    p.reference.occurrence_dependency ||
                m.reference.readable != p.reference.readable ||
                m.reference.writable != p.reference.writable)
                return false;
        } else {
            const NLSymbolId donor = v->producer.donor[i + 1],
                             parameter = v->producer.parameters[i + 1];
            if (field != v->producer.inputs[i + 1] || donor == 0 ||
                donor > a->binding_count || donor > b->binding_count ||
                donor == parameter ||
                a->bindings[donor - 1].view.availability != NL_CONSUMED ||
                b->bindings[donor - 1].view.availability != NL_CONSUMED ||
                b->bindings[parameter - 1].view.availability != NL_CONSUMED)
                return false;
            if (i == 1 && m.allocation_region != v->producer.range.region)
                return false;
            if (i == 2 && m.domain != v->producer.domain)
                return false;
        }
    }
    return true;
}

bool nl_checked_producer_valid(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    return nl_producer_valid(f, id, true);
}

static bool local_projection(const NLCheckedFragment *f, NLCheckedNodeId id,
                             const NLCheckedFragment *body,
                             NLProducerLocalReturnView *out)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    const NLCheckedNodeView *first =
        root ? nl_checked_node_view(body, root->first_item) : NULL;
    const NLCheckedNodeView *local =
        first ? nl_checked_node_view(body, first->next_item) : NULL;
    const NLCheckedNodeView *ret =
        local ? nl_checked_node_view(body, local->next_item) : NULL;
    if (!out || !call || !call->producer.return_proved || !root ||
        root->kind != NL_CHECKED_BLOCK || root->item_count != 3 || !local ||
        local->kind != NL_CHECKED_BINDING || !ret ||
        ret->kind != NL_CHECKED_RETURN)
        return false;
    *out = (NLProducerLocalReturnView){body,
                                       nl_checked_producer_entry(f, id),
                                       nl_checked_producer_return(f, id),
                                       first->next_item,
                                       local->initializer,
                                       ret->initializer,
                                       local->next_item,
                                       local->symbol,
                                       ret->returned};
    return true;
}
bool nl_checked_producer_local_return_view(const NLCheckedFragment *f,
                                           NLCheckedNodeId id,
                                           NLProducerLocalReturnView *out)
{
    return local_projection(f, id, nl_checked_call_body(f, id), out);
}
static void local_destroy(NLCheckedFragment *body)
{
    if (body->producer_local) {
        free(body->producer_local->nodes);
        free(body->producer_local);
    }
}
NLCheckStatus nl_producer_local_seal(NLCheckedFragment *body,
                                     const NLCheckedFragment *caller,
                                     NLCheckedNodeId call)
{
    NLProducerLocalReturnView issued;
    if (!local_projection(caller, call, body, &issued) || body->count != 17)
        return NL_CHECK_OK; /* CHECK-legal larger profiles remain unsupported.
                             */
    if (body->producer_local)
        return NL_CHECK_INTERNAL_ERROR;
    NLProducerLocalProof *p = calloc(1, sizeof(*p));
    if (!p)
        return NL_CHECK_OUT_OF_MEMORY;
    p->nodes = malloc(body->count * sizeof(*p->nodes));
    if (!p->nodes) {
        free(p);
        return NL_CHECK_OUT_OF_MEMORY;
    }
    p->caller = caller;
    p->body = body;
    p->context = body->context;
    p->call = call;
    p->root = body->root;
    p->count = body->count;
    p->origin = body->nodes;
    memcpy(p->nodes, body->nodes, body->count * sizeof(*p->nodes));
    p->call_node = *nl_checked_node_view(caller, call);
    p->source = body->source;
    p->plan = body->body_owner;
    p->target = body->function_target;
    const NLControlExitView *edge = nl_control_exit_view(body->exits, 0);
    p->exit_state = edge ? edge->state : NULL;
    p->issued = issued;
    body->producer_local = p;
    body->destroy_producer_local = local_destroy;
    return NL_CHECK_OK;
}
static bool local_span(NLSourceSpan a, NLSourceSpan b)
{
    return a.start_byte == b.start_byte && a.end_byte == b.end_byte;
}
static bool local_text(const NLSource *source, NLSourceSpan span,
                       const char *text)
{
    NLSourceView v;
    return text && nl_source_view(source, span, &v) &&
           v.length == strlen(text) && !memcmp(v.bytes, text, v.length);
}
/* Each mark is one actual issued operation, with its retained source origin.
 * The selected graph has exactly 17 nodes; a duplicate edge or orphan fails.
 * This is source correspondence only: R/O/A/D authority comes from the existing
 * independently validated producer entry/return worlds, never source tokens. */
static const NLCheckedNodeView *
local_mark(const NLCheckedFragment *body, NLCheckedNodeId id,
           NLCheckedKind kind, const NLSyntaxNode *syntax, bool seen[17])
{
    const NLSyntaxView *s = nl_syntax_node_view(syntax);
    const NLCheckedNodeView *n = nl_checked_node_view(body, id);
    if (!id || id > 17 || seen[id - 1] || !s || !n || n->kind != kind ||
        !local_span(n->span, s->span))
        return NULL;
    seen[id - 1] = true;
    return n;
}
bool nl_checked_producer_local_return_valid(
    const NLCheckedFragment *f, NLCheckedNodeId id,
    const NLProducerLocalReturnView *candidate)
{
    NLProducerLocalReturnView v;
    if (!nl_checked_producer_local_return_view(f, id, &v) ||
        !nl_checked_producer_valid(f, id))
        return false;
    if (candidate &&
        (candidate->body != v.body || candidate->entry != v.entry ||
         candidate->returned != v.returned ||
         candidate->declaration != v.declaration ||
         candidate->construction != v.construction ||
         candidate->consumer != v.consumer ||
         candidate->return_node != v.return_node ||
         candidate->local != v.local ||
         candidate->result.type != v.result.type ||
         candidate->result.value != v.result.value))
        return false;
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLCheckedFragment *body = v.body;
    const NLProducerLocalProof *seal = body->producer_local;
    if (!seal || seal->caller != f || seal->body != body ||
        seal->context != body->context || seal->call != id ||
        seal->root != body->root || seal->count != body->count ||
        body->count != 17 || seal->origin != body->nodes || !seal->nodes ||
        memcmp(seal->nodes, body->nodes, body->count * sizeof(*body->nodes)) ||
        seal->source != body->source || seal->plan != body->body_owner ||
        seal->target != body->function_target ||
        memcmp(&seal->call_node.producer, &call->producer,
               sizeof(call->producer)) ||
        seal->issued.body != v.body || seal->issued.entry != v.entry ||
        seal->issued.returned != v.returned || seal->issued.local != v.local ||
        seal->issued.declaration != v.declaration ||
        seal->issued.construction != v.construction ||
        seal->issued.consumer != v.consumer ||
        seal->issued.return_node != v.return_node ||
        seal->issued.result.type != v.result.type ||
        seal->issued.result.value != v.result.value)
        return false;
    const NLFunctionBody *plan = body->body_owner;
    const NLSemanticContext *b = v.returned;
    if (!plan || body->source != plan->source || !plan->syntax ||
        nl_syntax_tree_source(plan->syntax) != plan->source ||
        body->count != 17 || body->arm_count || body->body_count || !v.local ||
        v.local > b->binding_count)
        return false;
    const NLControlExitView *edge = nl_control_exit_view(body->exits, 0);
    const NLSemanticContext *exit =
        edge ? nl_control_state_context((NLControlState *)edge->state) : NULL;
    if (!edge || !exit || nl_control_exits_count(body->exits) != 1 ||
        edge->kind != NL_EXIT_RETURN || !body->function_target ||
        edge->target != body->function_target ||
        edge->state != seal->exit_state || edge->count != 1 ||
        edge->values[0].type != v.result.type ||
        edge->values[0].value != v.result.value ||
        call->function > exit->function_count ||
        exit->functions[call->function - 1].body != plan ||
        nl_sem_validate(exit) != NL_CHECK_OK)
        return false;
    NLSemanticSnapshot expected, actual;
    if (!nl_semantic_snapshot(b, &expected) ||
        !nl_semantic_snapshot(exit, &actual) ||
        memcmp(&expected, &actual, sizeof(actual)) ||
        memcmp(b->values, exit->values, b->value_count * sizeof(*b->values)) ||
        memcmp(b->places, exit->places, b->place_count * sizeof(*b->places)) ||
        memcmp(b->domains, exit->domains,
               b->domain_count * sizeof(*b->domains)) ||
        memcmp(b->scopes, exit->scopes, b->scope_count * sizeof(*b->scopes)))
        return false;
    for (size_t i = 0; i < b->binding_count; ++i)
        if (memcmp(&b->bindings[i].view, &exit->bindings[i].view,
                   sizeof(b->bindings[i].view)) ||
            b->bindings[i].hidden != exit->bindings[i].hidden ||
            strcmp(b->bindings[i].name, exit->bindings[i].name))
            return false;
    if (b->occurrence_count &&
        memcmp(b->occurrences, exit->occurrences,
               b->occurrence_count * sizeof(*b->occurrences)))
        return false;
    for (size_t i = 0; i < b->region_count; ++i) {
        const NLRawRegionEntry *x = &b->regions[i], *y = &exit->regions[i];
        if (memcmp(&x->view, &y->view, sizeof(x->view)) ||
            x->count != y->count ||
            (x->count && memcmp(x->intervals, y->intervals,
                                x->count * sizeof(*x->intervals))))
            return false;
    }
    for (size_t i = 0; i < body->count; ++i) {
        const NLCheckedNodeView *n = &body->nodes[i];
        bool binding_node = n->kind == NL_CHECKED_BINDING;
        bool field_node = n->kind == NL_CHECKED_AGGREGATE_FIELD;
        bool has_init =
            binding_node || field_node || n->kind == NL_CHECKED_RETURN;
        bool has_args = binding_node || n->kind == NL_CHECKED_AGGREGATE ||
                        n->kind == NL_CHECKED_REPLACE;
        bool ref_arg = n->kind == NL_CHECKED_IDENTIFIER &&
                       n->symbol == call->producer.parameters[0];
        if ((!has_init && n->initializer) ||
            (!has_args && (n->first_argument || n->argument_count)) ||
            (!field_node && !ref_arg && n->next_argument) ||
            (!binding_node && n->next_item) ||
            (n->kind != NL_CHECKED_BLOCK && (n->first_item || n->item_count)) ||
            n->tail)
            return false;
    }
    const NLSyntaxNode *tree = nl_syntax_tree_root(plan->syntax);
    const NLSyntaxView *block = nl_syntax_node_view(tree);
    if (!block || block->kind != NL_SYNTAX_BLOCK ||
        block->data.block.item_count != 3 || block->data.block.tail)
        return false;
    const NLSyntaxNode *s0 = block->data.block.items,
                       *s1 = nl_syntax_next_argument(s0),
                       *s2 = nl_syntax_next_argument(s1);
    const NLSyntaxView *a0 = nl_syntax_node_view(s0),
                       *a1 = nl_syntax_node_view(s1),
                       *a2 = nl_syntax_node_view(s2);
    if (!a0 || a0->kind != NL_SYNTAX_BINDING || !a1 ||
        a1->kind != NL_SYNTAX_BINDING || !a2 || a2->kind != NL_SYNTAX_RETURN ||
        nl_syntax_next_argument(s2))
        return false;
    bool seen[17] = {false};
    const NLCheckedNodeView *root =
        local_mark(body, body->root, NL_CHECKED_BLOCK, tree, seen);
    const NLCheckedNodeView *old =
        root ? local_mark(body, root->first_item, NL_CHECKED_BINDING, s0, seen)
             : NULL;
    const NLCheckedNodeView *decl =
        old ? local_mark(body, old->next_item, NL_CHECKED_BINDING, s1, seen)
            : NULL;
    const NLCheckedNodeView *ret =
        decl ? local_mark(body, decl->next_item, NL_CHECKED_RETURN, s2, seen)
             : NULL;
    if (!root || !old || !decl || !ret || root->item_count != 3 || root->tail ||
        !root->terminates || root->result_count ||
        root->returned.value != v.result.value ||
        root->returned.type != v.result.type || old->terminates ||
        decl->terminates || !ret->terminates || ret->next_item ||
        ret->result_count || old->argument_count != 1 ||
        decl->argument_count != 1 || old->type != 1 || decl->type != 1 ||
        old->result_count || decl->result_count || old->symbol == v.local ||
        decl->symbol != v.local || decl->initializer != v.construction ||
        ret->initializer != v.consumer ||
        v.result.value != call->producer.result || v.result.type != call->type)
        return false;
    const NLSyntaxNode *initializers[2] = {a0->data.binding.initializer,
                                           a1->data.binding.initializer};
    const NLCheckedNodeView *declarations[2] = {old, decl};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *d = declarations[i];
        const NLSyntaxView *ast = i ? a1 : a0;
        const NLCheckedNodeView *r =
            nl_checked_node_view(body, d->first_argument);
        if (!r || !r->symbol || r->symbol > b->binding_count ||
            r->symbol != d->symbol)
            return false;
        if (!r || r->kind != NL_CHECKED_RECEIVER || r->next_argument ||
            r->result_count || r->value_use != NL_VALUE_RECEIVED ||
            !local_span(r->span, ast->data.binding.name) ||
            !local_text(plan->source, ast->data.binding.name,
                        b->bindings[r->symbol - 1].name) ||
            d->first_argument > 17 || seen[d->first_argument - 1])
            return false;
        seen[d->first_argument - 1] = true;
    }
    const NLCheckedNodeView *aggregate = local_mark(
        body, v.construction, NL_CHECKED_AGGREGATE, initializers[1], seen);
    const NLCheckedNodeView *use =
        local_mark(body, v.consumer, NL_CHECKED_IDENTIFIER,
                   a2->data.statement.expression, seen);
    const NLSyntaxView *source_aggregate = nl_syntax_node_view(initializers[1]);
    const NLSyntaxView *source_use =
        nl_syntax_node_view(a2->data.statement.expression);
    const NLBindingEntry *binding = &b->bindings[v.local - 1];
    if (!aggregate || !use || source_aggregate->kind != NL_SYNTAX_AGGREGATE ||
        source_aggregate->data.aggregate.count != 3 ||
        source_use->kind != NL_SYNTAX_EXPR_NAME ||
        !local_text(plan->source, source_use->data.name, binding->name) ||
        aggregate->type != call->type || aggregate->result_count != 1 ||
        aggregate->results[0].value != v.result.value ||
        aggregate->argument_count != 3 || use->symbol != v.local ||
        use->value_use != NL_VALUE_CONSUMED || use->type != call->type ||
        use->result_count != 1 || use->results[0].value != v.result.value ||
        binding->view.value != v.result.value ||
        binding->view.type != call->type ||
        binding->view.availability != NL_CONSUMED || !binding->hidden ||
        !binding->view.place || binding->view.place > b->place_count ||
        binding->view.place == call->producer.root ||
        b->places[binding->view.place - 1].live ||
        b->places[binding->view.place - 1].current_value)
        return false;
    const NLSemanticValueView package = b->values[v.result.value - 1];
    bool fields[3] = {false};
    NLCheckedNodeId field = aggregate->first_argument;
    const NLSyntaxNode *source_field = source_aggregate->data.aggregate.fields;
    for (size_t i = 0; i < 3; ++i) {
        const NLCheckedNodeView *n = local_mark(
            body, field, NL_CHECKED_AGGREGATE_FIELD, source_field, seen);
        const NLSyntaxView *sf = nl_syntax_node_view(source_field);
        if (!n || n->field_index >= 3 || fields[n->field_index] || !sf ||
            sf->kind != NL_SYNTAX_FIELD ||
            !local_text(plan->source, sf->data.binding.name,
                        b->types[call->type - 1].field_names[n->field_index]))
            return false;
        size_t k = n->field_index;
        fields[k] = true;
        const NLCheckedNodeView *arg =
            local_mark(body, n->initializer, NL_CHECKED_IDENTIFIER,
                       sf->data.binding.initializer, seen);
        if (!arg || arg->symbol != call->producer.parameters[k + 1] ||
            arg->symbol == v.local || arg->result_count != 1 ||
            arg->results[0].value != package.fields[k] ||
            arg->type != n->type ||
            arg->value_use != (k ? NL_VALUE_CONSUMED : NL_VALUE_COPIED))
            return false;
        field = n->next_argument;
        source_field = nl_syntax_next_argument(source_field);
    }
    if (field || source_field)
        return false;
    const NLCheckedNodeView *replace = local_mark(
        body, old->initializer, NL_CHECKED_REPLACE, initializers[0], seen);
    const NLSyntaxView *rs = nl_syntax_node_view(initializers[0]);
    if (!replace || rs->kind != NL_SYNTAX_EXPR_CALL ||
        rs->data.call.argument_count != 2 ||
        !local_text(plan->source, rs->data.call.callee, "replace") ||
        replace->argument_count != 2 || replace->result_count != 1 ||
        replace->field.parent != call->producer.head.parent ||
        replace->field.child != call->producer.head.child ||
        replace->field.old_value != call->producer.head_before ||
        replace->field.new_value != call->producer.head_after ||
        replace->results[0].value != call->producer.head_before)
        return false;
    const NLSyntaxNode *ra = rs->data.call.arguments;
    const NLCheckedNodeView *ref = local_mark(body, replace->first_argument,
                                              NL_CHECKED_IDENTIFIER, ra, seen);
    const NLCheckedNodeView *none =
        ref ? local_mark(body, ref->next_argument, NL_CHECKED_SUM_CONSTRUCTOR,
                         nl_syntax_next_argument(ra), seen)
            : NULL;
    if (!ref || !none || ref->symbol != call->producer.parameters[0] ||
        ref->value_use != NL_VALUE_COPIED || none->next_argument ||
        none->variant != 1 || none->argument_count || none->result_count != 1 ||
        none->results[0].value != call->producer.head_after ||
        none->type != replace->type ||
        nl_checked_node_view(body, old->first_argument)->type !=
            replace->type ||
        nl_checked_node_view(body, decl->first_argument)->type != call->type)
        return false;
    for (size_t i = 0; i < 17; ++i)
        if (!seen[i])
            return false;
    return true;
}
