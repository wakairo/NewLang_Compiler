#include "semantic_internal.h"

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
    LIVE_TAIL,
    ROOT_RECORD
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
    NLTypeId record_type;
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
    return r == A || r == D || r == EMPTY || r == RAW || r == LIVE_TAIL ||
           r == ROOT_RECORD;
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
        if (item->kind == NL_SYNTAX_AGGREGATE_BINDING && i->record_type != 0) {
            const NLTypeEntry *record = &i->registry->types[i->record_type - 1];
            if (!text(i, item->data.aggregate.type_name, record->name) ||
                item->data.aggregate.count != 3 ||
                use(i, item->data.aggregate.initializer, true) != ROOT_RECORD)
                return error(i, item->span, NL_CHECK_SEMANTIC_ERROR,
                             "P276-DEFINITION-WHOLE",
                             "terminal must consume the complete record");
            const Role roles[] = {P, A, D};
            bool seen[3] = {false};
            for (const NLSyntaxNode *f = item->data.aggregate.fields; f != NULL;
                 f = nl_syntax_next_argument(f)) {
                const NLSourceSpan name = nl_syntax_node_view(f)->data.name;
                size_t index = 3;
                for (size_t j = 0; j < 3; ++j)
                    if (text(i, name, record->field_names[j]))
                        index = j;
                if (index == 3 || seen[index])
                    return error(i, name, NL_CHECK_SEMANTIC_ERROR,
                                 "P276-DEFINITION-WHOLE",
                                 "duplicate or unknown constituent");
                seen[index] = true;
                bind(i, name, roles[index], floor);
            }
        } else if (item->kind == NL_SYNTAX_BINDING) {
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

static NLCheckStatus owner_definition(const NLSemanticContext *c,
                                      NLFunctionBody *body, NLTypeId h,
                                      NLTypeId record,
                                      NLTypedOwnerDefinition *out,
                                      NLCheckDiagnostic *diagnostic)
{
    const bool producer = body->count == 4;
    Infer i = {.registry = c,
               .body = body,
               .record_type = record,
               .definition = {.target = h,
                              .requirements = NL_OWNER_ALL_REQUIREMENTS,
                              .live_return = producer,
                              .head_link_required = producer},
               .count = body->count};
    const Role roles[] = {HEAD, P, A, D};
    for (size_t j = 0; j < body->count; ++j)
        i.symbols[j] =
            (Symbol){body->parameter_names[j],
                     {0},
                     record != 0 ? ROOT_RECORD : roles[j + (producer ? 0 : 1)],
                     true};
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

NLCheckStatus nl_owner_definition(const NLSemanticContext *c,
                                  NLFunctionBody *body, NLTypeId h,
                                  NLTypedOwnerDefinition *out,
                                  NLCheckDiagnostic *diagnostic)
{
    return owner_definition(c, body, h, 0, out, diagnostic);
}

NLCheckStatus nl_root_record_definition(const NLSemanticContext *c,
                                        NLFunctionBody *body, NLTypeId record,
                                        NLTypedOwnerDefinition *out,
                                        NLCheckDiagnostic *diagnostic)
{
    if (body == NULL || body->count != 1 ||
        !nl_experimental_value_type(c, record) ||
        !nl_experimental_root_record_type(c, record))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    const NLTypeId pointer = c->types[record - 1].field_types[0];
    return owner_definition(c, body, c->types[pointer - 1].view.target, record,
                            out, diagnostic);
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
static NLCheckStatus
owner_relations(const NLSemanticContext *c, const NLValueId *values,
                const NLTypedOwnerDefinition *d, NLSourceSpan span,
                NLCheckDiagnostic *diagnostic, bool contained)
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
        r.alignment < h.alignment ||
        (!contained && (a.carrier != NL_CARRIER_LOOSE ||
                        domain.carrier != NL_CARRIER_LOOSE))) {
        return relation_error(
            diagnostic, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
            "P193-CALL-RECOVERY",
            "full original range/authority recovery unproved");
    }
    return NL_CHECK_OK;
}

NLCheckStatus nl_owner_relations(const NLSemanticContext *c,
                                 const NLValueId *values,
                                 const NLTypedOwnerDefinition *d,
                                 NLSourceSpan span, NLCheckDiagnostic *diag)
{
    return owner_relations(c, values, d, span, diag, false);
}

/* Read-only entry projection. The actual body still must perform complete
 * affine decomposition and run the original LOOSE-only primitive checker. */
NLCheckStatus nl_owner_record_relations(const NLSemanticContext *c,
                                        NLValueId id,
                                        const NLTypedOwnerDefinition *d)
{
    if (!c || !id || id > c->value_count)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    const NLSemanticValueView packet = c->values[id - 1];
    if (!nl_experimental_root_record_type(c, packet.type) ||
        packet.field_count != 3)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    for (size_t i = 0; i < 3; ++i) {
        if (!packet.fields[i] || packet.fields[i] > c->value_count)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        const NLSemanticValueView field = c->values[packet.fields[i] - 1];
        if (field.carrier != NL_CARRIER_AGGREGATE ||
            field.aggregate_owner != id ||
            field.type != c->types[packet.type - 1].field_types[i])
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    return owner_relations(c, packet.fields, d, (NLSourceSpan){0}, NULL, true);
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
