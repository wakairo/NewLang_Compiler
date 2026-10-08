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
    RAW
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
    bool ended, finalized, released, returned;
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
    return r == A || r == D || r == EMPTY || r == RAW;
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
            if (affine(r))
                (void)error(i, item->span, NL_CHECK_SEMANTIC_ERROR,
                            "P193-DEFINITION-OBLIGATION",
                            "non-Discardable conditional result is discarded");
            if (item->kind == NL_SYNTAX_RETURN) {
                if (i->depth != 0 || r != U)
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
    Infer i = {
        .registry = c,
        .body = body,
        .definition = {.target = h, .requirements = NL_OWNER_ALL_REQUIREMENTS},
        .count = 3};
    const Role roles[] = {P, A, D};
    for (size_t j = 0; j < 3; ++j)
        i.symbols[j] = (Symbol){body->parameter_names[j], {0}, roles[j], true};
    const NLSyntaxView *root =
        nl_syntax_node_view(nl_syntax_tree_root(body->syntax));
    const Role result = block(&i, root, 0);
    if (i.status == NL_CHECK_OK &&
        (result != U || !i.ended || !i.finalized || !i.released))
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
