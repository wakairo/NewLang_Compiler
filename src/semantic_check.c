#include "semantic_internal.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
    const NLSource *source;
    size_t depth;
    size_t binding_floor, namespace_floor;
    size_t body_function;
    bool in_function_body;
    size_t arm_floor;
    bool has_arm_floor, in_match_arm;
    NLCheckStatus status;
    NLCheckDiagnostic diagnostic;
} Check;

static void fail(Check *check, NLCheckStatus status, NLSourceSpan span,
                 const char *code, const char *message)
{
    if (check->status != NL_CHECK_OK) {
        return;
    }
    const char *category = "semantic";
    if (status == NL_CHECK_SEMANTIC_UNSUPPORTED) {
        category = "unsupported";
    } else if (status == NL_CHECK_ANALYSIS_PRECISION_LIMIT) {
        category = "precision";
    } else if (status == NL_CHECK_OUT_OF_MEMORY ||
               status == NL_CHECK_RESOURCE_LIMIT) {
        category = "host";
    } else if (status == NL_CHECK_INTERNAL_ERROR) {
        category = "internal";
    }
    check->status = status;
    check->diagnostic = (NLCheckDiagnostic){
        {NL_DIAG_ERROR, category, code, message, NULL, NULL, 0}, span};
}

static bool host(Check *check, NLCheckStatus status, NLSourceSpan span)
{
    if (status == NL_CHECK_OK) {
        return true;
    }
    fail(check, status, span,
         status == NL_CHECK_OUT_OF_MEMORY    ? "P3-OUT-OF-MEMORY"
         : status == NL_CHECK_RESOURCE_LIMIT ? "P3-RESOURCE-LIMIT"
                                             : "P3-INTERNAL",
         status == NL_CHECK_OUT_OF_MEMORY
             ? "host allocation failed during semantic checking"
         : status == NL_CHECK_RESOURCE_LIMIT
             ? "P3 implementation resource budget exceeded"
             : "invalid semantic module state");
    return false;
}

static bool equal_name(Check *check, NLSourceSpan span, const char *name)
{
    NLSourceView view;
    if (!nl_source_view(check->source, span, &view)) {
        fail(check, NL_CHECK_INTERNAL_ERROR, span, "P3-INTERNAL",
             "invalid syntax name span");
        return false;
    }
    const size_t length =
        strlen(name); /* owned host registry string, not source */
    return length == view.length && memcmp(view.bytes, name, length) == 0;
}

static NLSymbolId resolve_binding(Check *check, NLSourceSpan span)
{
    for (size_t i = check->context->binding_count; i > check->namespace_floor;
         --i) {
        if (!check->context->bindings[i - 1].hidden &&
            equal_name(check, span, check->context->bindings[i - 1].name)) {
            return i;
        }
    }
    fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-UNKNOWN-BINDING",
         "unknown semantic binding");
    return 0;
}

static NLCheckedNodeView *view(Check *check, NLCheckedNodeId id)
{
    return &check->artifact->nodes[id - 1];
}

static NLCheckedNodeId add(Check *check, NLCheckedNodeView node)
{
    NLCheckedNodeId id = 0;
    (void)host(check, nl_checked_add(check->artifact, node, &id), node.span);
    return id;
}

static NLTypeId compound(Check *check, NLSemanticTypeKind kind, NLTypeId target,
                         NLAccessSyntax access, bool exclusive,
                         NLSourceSpan span)
{
    NLTypeId id = 0;
    (void)host(
        check,
        nl_sem_compound(check->context, kind, target, access, exclusive, &id),
        span);
    return id;
}

static bool enter(Check *check, NLSourceSpan span)
{
    if (check->depth >= NL_SEMANTIC_MAX_DEPTH) {
        fail(check, NL_CHECK_RESOURCE_LIMIT, span, "P3-DEPTH-LIMIT",
             "P3 implementation semantic traversal depth exceeded");
        return false;
    }
    ++check->depth;
    return true;
}

static NLTypeId check_type(Check *check, const NLSyntaxNode *syntax)
{
    const NLSyntaxView *const node = nl_syntax_node_view(syntax);
    if (!enter(check, node->span)) {
        return 0;
    }
    NLTypeId result = 0;
    if (node->kind == NL_SYNTAX_TYPE_NAME) {
        for (size_t i = 0; i < check->context->type_count; ++i) {
            const char *const name = check->context->types[i].name;
            if (name != NULL && equal_name(check, node->data.name, name)) {
                result = i + 1;
                break;
            }
        }
        if (result == 0) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, node->span, "P3-UNKNOWN-TYPE",
                 "unknown semantic type name");
        }
    } else if (node->kind == NL_SYNTAX_TYPE_PTR ||
               node->kind == NL_SYNTAX_TYPE_REF) {
        const bool is_ref = node->kind == NL_SYNTAX_TYPE_REF;
        const NLTypeId target =
            check_type(check, is_ref ? node->data.ref_type.target
                                     : node->data.ptr_type.target);
        if (target != 0) {
            result = compound(
                check, is_ref ? NL_TYPE_REF : NL_TYPE_PTR, target,
                is_ref ? node->data.ref_type.access : NL_ACCESS_READ,
                is_ref && node->data.ref_type.is_exclusive, node->span);
        }
    } else {
        fail(check, NL_CHECK_INTERNAL_ERROR, node->span, "P3-INTERNAL",
             "expected type syntax");
    }
    --check->depth;
    return result;
}

static bool scope_active(const NLSemanticContext *c, NLScopeId scope)
{
    size_t remaining = c->scope_count;
    while (scope != 0 && remaining-- != 0) {
        if (scope > c->scope_count || !c->scopes[scope - 1].active) {
            return false;
        }
        scope = c->scopes[scope - 1].parent;
    }
    return scope == 0;
}

static bool suspended(const NLSemanticContext *c, NLValueId value)
{
    const NLScopeId parent_scope = c->values[value - 1].reference.scope;
    for (size_t i = 0; i < c->scope_count; ++i) {
        const NLSemanticScopeView scope = c->scopes[i];
        if (scope.active &&
            (scope.parent_authority == value ||
             (scope.parent_authority == 0 && parent_scope != 0 &&
              scope.parent == parent_scope))) {
            return true;
        }
    }
    return false;
}

static bool reference_fact_live(Check *check, NLValueId value,
                                NLReferenceFacts fact, NLSourceSpan span)
{
    NLSemanticContext *const c = check->context;
    const NLSemanticValueView v = c->values[value - 1];
    const NLSemanticTypeView type = c->types[v.type - 1].view;
    if (fact.provenance == NL_PROVENANCE_UNKNOWN || fact.place == 0 ||
        fact.place > c->place_count) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
             "P3-UNKNOWN-PROVENANCE",
             "safe operation needs proven pointer/ref provenance");
        return false;
    }
    if (fact.provenance != NL_PROVENANCE_VALID) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-INVALID-PROVENANCE",
             "pointer/ref provenance is invalid");
        return false;
    }
    const NLSemanticPlaceView p = c->places[fact.place - 1];
    if (type.kind == NL_TYPE_REF && p.parent_sum != 0 &&
        (fact.occurrence_dependency == 0 ||
         fact.occurrence_dependency !=
             c->places[p.parent_sum - 1].payload_occurrence)) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P6-OCCURRENCE-DEPENDENCY",
             "conditional payload ref needs its current occurrence dependency");
        return false;
    }
    if (fact.occurrence_dependency != 0 &&
        (fact.occurrence_dependency > c->occurrence_count ||
         !c->occurrences[fact.occurrence_dependency - 1].live)) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P6-DEAD-OCCURRENCE",
             "payload occurrence has ended");
        return false;
    }
    if (!p.live || p.incarnation != fact.incarnation ||
        (p.placement.region != 0 &&
         !c->regions[p.placement.region - 1].view.live)) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-STALE-POINTER",
             "pointer/ref does not refer to a current live incarnation");
        return false;
    }
    if (p.type != type.target) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-REFERENT-TYPE",
             "referent type does not match pointer/ref target");
        return false;
    }
    if (type.kind == NL_TYPE_REF) {
        if (fact.scope == 0) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                 "P3-UNKNOWN-SCOPE", "ref scope evidence is unknown");
            return false;
        }
        if (!scope_active(c, fact.scope)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-DEAD-SCOPE",
                 "ref scope has ended");
            return false;
        }
        if (type.is_exclusive && suspended(c, value)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-SUSPENDED-AUTHORITY",
                 "exclusive parent has a live conflicting child reborrow");
            return false;
        }
        if (!fact.readable ||
            (type.access == NL_ACCESS_WRITE && !fact.writable)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-ACCESS",
                 "reference access evidence does not permit requested mode");
            return false;
        }
    }
    return true;
}

static bool reference_live(Check *check, NLValueId value, NLSourceSpan span)
{
    const NLSemanticValueView v = check->context->values[value - 1];
    for (size_t i = 0; i < nl_sem_ref_count(v); ++i)
        if (!reference_fact_live(check, value, nl_sem_ref_fact(v, i), span))
            return false;
    return true;
}

static bool concrete_ref(Check *check, NLValueId value, NLSourceSpan span)
{
    if (check->context->values[value - 1].reference_count != 0) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
             "P7-SINGULAR-REF-PRECISION",
             "operation requires a concrete referent; joined ref facts are "
             "retained");
        return false;
    }
    return true;
}

/* Explicit ref scopes are modeled separately from unsupported hidden value
 * dependencies. Unknown active referents cannot establish disjointness. */
static bool conflicts(Check *check, NLPlaceId place, bool exclusive,
                      bool ending, NLValueId except, NLSourceSpan span)
{
    NLSemanticContext *const c = check->context;
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView v = c->values[i];
        const NLSemanticTypeView t = c->types[v.type - 1].view;
        if (i + 1 == except || v.carrier == NL_CARRIER_ENDED ||
            t.kind != NL_TYPE_REF) {
            continue;
        }
        for (size_t alternative = 0; alternative < nl_sem_ref_count(v);
             ++alternative) {
            const NLReferenceFacts fact = nl_sem_ref_fact(v, alternative);
            if (fact.scope != 0 && !scope_active(c, fact.scope)) {
                continue;
            }
            if (t.is_exclusive && suspended(c, i + 1)) {
                continue;
            }
            if (fact.occurrence_dependency != 0 &&
                c->occurrences[fact.occurrence_dependency - 1].root == place &&
                c->occurrences[fact.occurrence_dependency - 1].live) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, span,
                     "P6-OCCURRENCE-CONFLICT",
                     "whole-sum transition would end a live payload ref "
                     "dependency");
                return true;
            }
            if (fact.place == 0 || fact.scope == 0) {
                if (exclusive || ending || t.is_exclusive) {
                    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                         "P3-UNKNOWN-ALIAS",
                         "unknown live ref facts cannot prove "
                         "acquisition/transition safety");
                    return true;
                }
            } else if (fact.place == place &&
                       (exclusive || ending || t.is_exclusive)) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-REF-CONFLICT",
                     "operation conflicts with a live reference capability");
                return true;
            }
        }
    }
    return false;
}

static NLSymbolId available(Check *check, NLSourceSpan name)
{
    const NLSymbolId symbol = resolve_binding(check, name);
    if (symbol != 0 && check->context->bindings[symbol - 1].view.availability !=
                           NL_AVAILABLE) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, name, "P3-USE-AFTER-CONSUME",
             "use of consumed semantic binding");
        return 0;
    }
    return symbol;
}

static NLCheckedNodeId binding_use(Check *check, NLSymbolId symbol,
                                   NLSourceSpan span, NLSourceSpan name)
{
    NLSemanticContext *const c = check->context;
    if (symbol == 0) {
        return 0;
    }
    if (c->bindings[symbol - 1].view.availability != NL_AVAILABLE) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-USE-AFTER-CONSUME",
             "use of consumed semantic binding");
        return 0;
    }
    const NLSemanticBindingView binding = c->bindings[symbol - 1].view;
    const NLSemanticTypeView type = c->types[binding.type - 1].view;
    if (type.kind == NL_TYPE_REF &&
        !reference_live(check, binding.value, span)) {
        return 0;
    }
    NLValueId value = binding.value;
    if (type.is_copy) {
        if (!host(check, nl_sem_copy_value(c, binding.value, &value), span)) {
            return 0;
        }
    } else {
        if (conflicts(check, binding.place, false, true, 0, span)) {
            return 0;
        }
        nl_sum_detach(c, binding.place);
        c->bindings[symbol - 1].view.availability = NL_CONSUMED;
        c->places[binding.place - 1].live = false;
        c->places[binding.place - 1].current_value = 0;
        c->places[binding.place - 1].current_fact = 0;
        c->places[binding.place - 1].governing_domain = 0;
        c->values[value - 1].carrier = NL_CARRIER_LOOSE;
        c->values[value - 1].owner_place = 0;
    }
    return add(check, (NLCheckedNodeView){.kind = NL_CHECKED_IDENTIFIER,
                                          .span = span,
                                          .name = name,
                                          .type = binding.type,
                                          .symbol = symbol,
                                          .value_use = type.is_copy
                                                           ? NL_VALUE_COPIED
                                                           : NL_VALUE_CONSUMED,
                                          .result_count = 1,
                                          .results = {{binding.type, value}}});
}

static NLCheckedNodeId identifier(Check *check, const NLSyntaxView *syntax)
{
    return binding_use(check, available(check, syntax->data.name), syntax->span,
                       syntax->data.name);
}

static bool ref_compatible(NLSemanticTypeView actual,
                           NLSemanticTypeView expected)
{
    return actual.kind == NL_TYPE_REF && expected.kind == NL_TYPE_REF &&
           actual.target == expected.target &&
           actual.is_exclusive == expected.is_exclusive &&
           (actual.access == expected.access ||
            (!actual.is_exclusive && actual.access == NL_ACCESS_WRITE &&
             expected.access == NL_ACCESS_READ));
}

static NLCheckedNodeId expression(Check *, const NLSyntaxNode *);

static NLCheckedNodeId binding_argument(Check *check, NLSymbolId symbol,
                                        NLTypeId expected, NLSourceSpan span,
                                        NLSourceSpan name)
{
    NLSemanticContext *const c = check->context;
    if (c->bindings[symbol - 1].view.availability != NL_AVAILABLE) {
        return binding_use(check, symbol, span, name);
    }
    if (expected != 0) {
        const NLSemanticBindingView binding = c->bindings[symbol - 1].view;
        const NLSemanticTypeView actual = c->types[binding.type - 1].view;
        const NLSemanticTypeView wanted = c->types[expected - 1].view;
        if (actual.kind == NL_TYPE_REF && actual.is_exclusive &&
            wanted.kind == NL_TYPE_REF && actual.target == wanted.target &&
            actual.access == NL_ACCESS_WRITE &&
            wanted.access == NL_ACCESS_READ) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, span,
                 "P3-EXCLUSIVE-MODE-UNSUPPORTED",
                 "exclusive mode-changing argument compatibility is not "
                 "established by the call-local reborrow rule alone");
            return 0;
        }
        /* Section 12.1 requires an exclusive selected parameter. Reject
         * ordinary adaptation before creating a child or consuming authority;
         * independently constructed ordinary children use the ordinary path. */
        if (actual.kind == NL_TYPE_REF && actual.is_exclusive &&
            wanted.kind == NL_TYPE_REF && !wanted.is_exclusive) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-TYPE-MISMATCH",
                 "exclusive actual cannot automatically fit an ordinary ref "
                 "parameter");
            return 0;
        }
        if (actual.kind == NL_TYPE_REF && actual.is_exclusive &&
            ref_compatible(actual, wanted)) {
            if (!reference_live(check, binding.value, span)) {
                return 0;
            }
            if (!concrete_ref(check, binding.value, span))
                return 0;
            if (conflicts(check, c->values[binding.value - 1].reference.place,
                          true, false, binding.value, span)) {
                return 0;
            }
            NLScopeId child_scope;
            if (!host(check,
                      nl_sem_new_scope(
                          c, c->values[binding.value - 1].reference.scope, true,
                          &child_scope),
                      span)) {
                return 0;
            }
            c->scopes[child_scope - 1].parent_authority = binding.value;
            NLSemanticValueView child = c->values[binding.value - 1];
            child.type = expected;
            child.reference.scope = child_scope;
            NLValueId value;
            if (!host(check, nl_sem_new_value(c, child, &value), span)) {
                return 0;
            }
            return add(check,
                       (NLCheckedNodeView){.kind = NL_CHECKED_IDENTIFIER,
                                           .span = span,
                                           .name = name,
                                           .type = expected,
                                           .symbol = symbol,
                                           .value_use = NL_VALUE_REBORROWED,
                                           .parameter_type = expected,
                                           .reborrow_scope = child_scope,
                                           .result_count = 1,
                                           .results = {{expected, value}}});
        }
    }
    return binding_use(check, symbol, span, name);
}

static NLCheckedNodeId parameter_match(Check *check, NLCheckedNodeId id,
                                       NLTypeId expected)
{
    NLSemanticContext *const c = check->context;
    if (id == 0 || expected == 0) {
        return id;
    }
    NLCheckedNodeView *const checked = view(check, id);
    checked->parameter_type = expected;
    if (checked->result_count == 0 && expected == 1 && checked->type == 1) {
        return id;
    }
    if (checked->result_count != 1) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, view(check, id)->span,
             "P3-ARGUMENT-RESULTS",
             "argument needs one result; multi-result receiving is outside P3");
        return 0;
    }
    if (checked->type == expected) {
        return id;
    }
    const NLSemanticTypeView actual = c->types[checked->type - 1].view;
    const NLSemanticTypeView wanted = c->types[expected - 1].view;
    if (!actual.is_exclusive && !wanted.is_exclusive &&
        ref_compatible(actual, wanted)) {
        checked->contextually_weakened = true;
        return id;
    }
    fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, id)->span,
         "P3-TYPE-MISMATCH",
         "argument semantic type does not match selected parameter");
    return 0;
}

static NLCheckedNodeId argument(Check *check, const NLSyntaxNode *syntax,
                                NLTypeId expected)
{
    const NLSyntaxView *const node = nl_syntax_node_view(syntax);
    NLCheckedNodeId id;
    if (node->kind == NL_SYNTAX_EXPR_NAME) {
        const NLSymbolId symbol = available(check, node->data.name);
        if (symbol == 0) {
            return 0;
        }
        id = binding_argument(check, symbol, expected, node->span,
                              node->data.name);
    } else {
        id = expression(check, syntax);
    }
    return parameter_match(check, id, expected);
}

static NLTypeId write_parameter(Check *check, const NLSyntaxNode *syntax)
{
    const NLSyntaxView *const node = nl_syntax_node_view(syntax);
    if (node->kind != NL_SYNTAX_EXPR_NAME) {
        return 0;
    }
    const NLSymbolId symbol = available(check, node->data.name);
    if (symbol == 0) {
        return 0;
    }
    const NLSemanticTypeView type =
        check->context
            ->types[check->context->bindings[symbol - 1].view.type - 1]
            .view;
    if (type.kind != NL_TYPE_REF) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, node->span,
             "P3-WRITE-REF-REQUIRED", "destination needs ref<write,T>");
        return 0;
    }
    return compound(check, NL_TYPE_REF, type.target, NL_ACCESS_WRITE, false,
                    node->span);
}

static NLValueId one_result(Check *check, NLCheckedNodeId id)
{
    const NLCheckedNodeView node = *view(check, id);
    if (node.result_count != 1) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, node.span,
             "P3-SINGLE-RESULT-REQUIRED",
             "this operand requires one semantic value");
        return 0;
    }
    return node.results[0].value;
}

static NLValueId new_value(Check *check, NLSemanticValueView value,
                           NLSourceSpan span)
{
    NLValueId id = 0;
    (void)host(check, nl_sem_new_value(check->context, value, &id), span);
    return id;
}

static void end_temporary(Check *check, NLValueId value)
{
    nl_sem_end_value(check->context, value);
}

static NLDomainId domain_reference(Check *check, NLValueId value,
                                   bool exclusive, NLSourceSpan span)
{
    NLSemanticContext *const c = check->context;
    const NLSemanticValueView v = c->values[value - 1];
    const NLSemanticTypeView t = c->types[v.type - 1].view;
    if (t.kind != NL_TYPE_REF || t.target != 2 || t.is_exclusive != exclusive ||
        (t.access != NL_ACCESS_READ &&
         (exclusive || t.access != NL_ACCESS_WRITE))) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-DOMAIN-REF-REQUIRED",
             "operand needs compatible domain reference authority");
        return 0;
    }
    if (!reference_live(check, value, span)) {
        return 0;
    }
    if (!concrete_ref(check, value, span))
        return 0;
    const NLValueId package = c->places[v.reference.place - 1].current_value;
    const NLDomainId domain = c->values[package - 1].domain;
    if (domain == 0 || domain > c->domain_count ||
        !c->domains[domain - 1].live) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-DEAD-DOMAIN",
             "domain reference does not refer to live domain identity");
        return 0;
    }
    return domain;
}

static bool scalar_root_type(Check *check, NLTypeId type, NLSourceSpan span)
{
    const NLSemanticTypeKind kind = check->context->types[type - 1].view.kind;
    if (check->context->types[type - 1].view.field_count != 0 ||
        ((kind != NL_TYPE_NOMINAL || type == 2) && kind != NL_TYPE_STORAGE &&
         kind != NL_TYPE_ALLOCATION && kind != NL_TYPE_BYTE &&
         kind != NL_TYPE_U8 && kind != NL_TYPE_USIZE && kind != NL_TYPE_ADDR &&
         kind != NL_TYPE_SUM)) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, span,
             "P3-ROOT-PAYLOAD-UNSUPPORTED",
             "root transitions support flat payloads; nested ref/slot/domain "
             "payloads require additional checking");
        return false;
    }
    return true;
}

static bool write_ref(Check *check, NLValueId value, NLSourceSpan span)
{
    const NLSemanticTypeView t =
        check->context->types[check->context->values[value - 1].type - 1].view;
    if (t.kind != NL_TYPE_REF || t.access != NL_ACCESS_WRITE ||
        t.is_exclusive) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-WRITE-REF-REQUIRED",
             "destination needs ordinary ref<write,T> or its checked reborrow");
        return false;
    }
    if (!reference_live(check, value, span)) {
        return false;
    }
    if (!concrete_ref(check, value, span))
        return false;
    return !conflicts(check, check->context->values[value - 1].reference.place,
                      false, false, value, span);
}

/* Bindings designate places. Change(place) updates their current package;
 * historical package IDs must not be reused by a later identifier move. */
static void refresh_bindings(Check *check, NLPlaceId place)
{
    NLSemanticContext *const c = check->context;
    for (size_t i = 0; i < c->binding_count; ++i) {
        if (c->bindings[i].view.place == place &&
            c->bindings[i].view.availability == NL_AVAILABLE) {
            c->bindings[i].view.value = c->places[place - 1].current_value;
        }
    }
}

static bool primitive(Check *check, NLCheckedNodeId call,
                      const NLCheckedNodeId *args)
{
    NLSemanticContext *const c = check->context;
    NLCheckedNodeView operation = *view(check, call);
    const NLCheckedKind kind = operation.kind;
    const NLSourceSpan span = operation.span;
    operation.type = 1;
    if (kind == NL_CHECKED_DOMAIN_CREATE) {
        NLDomainId domain;
        NLValueId value;
        if (!host(check, nl_sem_new_domain(c, &domain, &value), span)) {
            return false;
        }
        operation.type = 2;
        operation.result_count = 1;
        operation.results[0] = (NLCheckedResult){2, value};
    } else if (kind == NL_CHECKED_REGISTERED_CALL) {
        const NLFunctionEntry function = c->functions[operation.function - 1];
        if (function.result != 1) {
            const NLValueId value = new_value(
                check, (NLSemanticValueView){.type = function.result}, span);
            if (value == 0) {
                return false;
            }
            operation.result_count = 1;
            operation.results[0] = (NLCheckedResult){function.result, value};
        }
        operation.type = function.result;
    } else if (kind == NL_CHECKED_DOMAIN_FINALIZE) {
        const NLValueId value = one_result(check, args[0]);
        if (value == 0) {
            return false;
        }
        const NLDomainId domain = c->values[value - 1].domain;
        if (domain == 0 || !c->domains[domain - 1].live) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                 "P3-DEAD-DOMAIN", "cannot finalize a dead domain");
            return false;
        }
        for (size_t i = 0; i < c->place_count; ++i) {
            if (c->places[i].live && c->places[i].governing_domain == domain) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                     "P3-GOVERNED-ROOTS-LIVE",
                     "cannot finalize LifetimeDomain while governed live roots "
                     "remain");
                return false;
            }
        }
        c->domains[domain - 1].live = false;
    } else if (kind == NL_CHECKED_PTR_FROM_REF) {
        const NLValueId value = one_result(check, args[0]);
        if (value == 0) {
            return false;
        }
        const NLSemanticValueView ref = c->values[value - 1];
        const NLSemanticTypeView t = c->types[ref.type - 1].view;
        if (t.kind != NL_TYPE_REF) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                 "P3-REF-REQUIRED", "ptr_from_ref requires a reference");
            return false;
        }
        if (t.is_exclusive) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED,
                 view(check, args[0])->span,
                 "P3-EXCLUSIVE-CONVERSION-UNSUPPORTED",
                 "exclusive ptr_from_ref input needs additional adjudicated "
                 "evidence");
            return false;
        }
        if (!reference_live(check, value, view(check, args[0])->span)) {
            return false;
        }
        if (!concrete_ref(check, value, view(check, args[0])->span))
            return false;
        const NLTypeId type =
            compound(check, NL_TYPE_PTR, t.target, NL_ACCESS_READ, false, span);
        if (type == 0) {
            return false;
        }
        NLSemanticValueView ptr = {.type = type, .reference = ref.reference};
        ptr.reference.scope = 0;
        ptr.reference.occurrence_dependency = 0;
        const NLValueId result = new_value(check, ptr, span);
        if (result == 0) {
            return false;
        }
        operation.type = type;
        operation.result_count = 1;
        operation.results[0] = (NLCheckedResult){type, result};
    } else if (kind == NL_CHECKED_INITIALIZE) {
        const NLValueId slot = one_result(check, args[0]);
        const NLValueId incoming = one_result(check, args[1]);
        const NLValueId stable = one_result(check, args[2]);
        if (slot == 0 || incoming == 0 || stable == 0) {
            return false;
        }
        const NLSemanticValueView slot_value = c->values[slot - 1];
        const NLSemanticTypeView slot_type = c->types[slot_value.type - 1].view;
        if (slot_type.kind != NL_TYPE_SLOT || slot_value.slot_place == 0) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                 "P3-SLOT-REQUIRED",
                 "initialize requires a typed empty slot responsibility");
            return false;
        }
        if (!scalar_root_type(check, slot_type.target,
                              view(check, args[0])->span)) {
            return false;
        }
        if (c->values[incoming - 1].type != slot_type.target) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[1])->span,
                 "P3-TYPE-MISMATCH",
                 "initialize value does not match slot target type");
            return false;
        }
        const NLDomainId domain =
            domain_reference(check, stable, false, view(check, args[2])->span);
        if (domain == 0) {
            return false;
        }
        const NLPlaceId place = slot_value.slot_place;
        if (c->places[place - 1].live ||
            c->places[place - 1].type != slot_type.target) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                 "P3-SLOT-NOT-EMPTY",
                 "slot destination is not definitely empty with matching type");
            return false;
        }
        bool readable = true, writable = true;
        if (slot_value.occupancy.region != 0) {
            const NLSemanticBackingView backing =
                c->regions[slot_value.occupancy.region - 1].view;
            if (!backing.ordinary_write) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                     "P4-INITIALIZE-BACKING-WRITE",
                     "typed initialize requires destination backing write "
                     "access");
                return false;
            }
            readable = backing.ordinary_read;
            writable = backing.ordinary_write;
        }
        if (!host(check, nl_sem_install(c, place, incoming, domain), span) ||
            !host(check, nl_raw_start_root(c, slot, place), span)) {
            return false;
        }
        const NLTypeId type = compound(check, NL_TYPE_PTR, slot_type.target,
                                       NL_ACCESS_READ, false, span);
        if (type == 0) {
            return false;
        }
        const NLValueId result = new_value(
            check,
            (NLSemanticValueView){
                .type = type,
                .reference = {.place = place,
                              .incarnation = c->places[place - 1].incarnation,
                              .provenance = NL_PROVENANCE_VALID,
                              .readable = readable,
                              .writable = writable}},
            span);
        if (result == 0) {
            return false;
        }
        operation.type = type;
        operation.result_count = 1;
        operation.results[0] = (NLCheckedResult){type, result};
    } else if (kind == NL_CHECKED_TAKE || kind == NL_CHECKED_DESTROY) {
        const NLValueId ptr = one_result(check, args[0]);
        const NLValueId ending = one_result(check, args[1]);
        if (ptr == 0 || ending == 0) {
            return false;
        }
        const NLSemanticValueView token = c->values[ptr - 1];
        if (c->types[token.type - 1].view.kind != NL_TYPE_PTR) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                 "P3-PTR-REQUIRED", "take/destroy requires ptr<T>");
            return false;
        }
        if (!reference_live(check, ptr, view(check, args[0])->span)) {
            return false;
        }
        const NLPlaceId place = token.reference.place;
        const NLSemanticPlaceView old = c->places[place - 1];
        if (kind == NL_CHECKED_TAKE &&
            (!token.reference.readable ||
             (old.placement.region != 0 &&
              !c->regions[old.placement.region - 1].view.ordinary_read))) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                 "P4-TAKE-BACKING-READ",
                 "take requires source ordinary read access to materialize "
                 "the current value");
            return false;
        }
        if (!old.independent_root) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                 "P3-NOT-LIFETIME-ROOT",
                 "target is not an independently lifetime-ending root");
            return false;
        }
        if (!scalar_root_type(check, old.type, view(check, args[0])->span)) {
            return false;
        }
        const NLDomainId domain =
            domain_reference(check, ending, true, view(check, args[1])->span);
        if (domain == 0) {
            return false;
        }
        if (domain != old.governing_domain) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[1])->span,
                 "P3-DOMAIN-MISMATCH",
                 "lifetime-ending authority refers to wrong LifetimeDomain");
            return false;
        }
        if (conflicts(check, place, false, true, 0,
                      view(check, args[0])->span)) {
            return false;
        }
        if (kind == NL_CHECKED_DESTROY &&
            !c->types[old.type - 1].view.is_discardable) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                 "P3-DISCARDABLE-REQUIRED", "destroy requires Discardable(T)");
            return false;
        }
        nl_sum_detach(c, place);
        if (kind == NL_CHECKED_DESTROY)
            nl_sem_end_value(c, old.current_value);
        c->places[place - 1].live = false;
        c->places[place - 1].placement = (NLBackingRange){0};
        c->places[place - 1].current_value = 0;
        c->places[place - 1].current_fact = 0;
        c->places[place - 1].governing_domain = 0;
        c->values[old.current_value - 1].carrier =
            kind == NL_CHECKED_TAKE ? NL_CARRIER_LOOSE : NL_CARRIER_ENDED;
        c->values[old.current_value - 1].owner_place = 0;
        const NLTypeId slot_type = compound(check, NL_TYPE_SLOT, old.type,
                                            NL_ACCESS_READ, false, span);
        if (slot_type == 0) {
            return false;
        }
        const NLValueId slot = new_value(
            check,
            (NLSemanticValueView){.type = slot_type, .slot_place = place},
            span);
        if (slot == 0 ||
            !host(check, nl_raw_end_root(c, old.placement, slot), span)) {
            return false;
        }
        operation.type = kind == NL_CHECKED_TAKE ? old.type : slot_type;
        operation.result_count = kind == NL_CHECKED_TAKE ? 2 : 1;
        if (kind == NL_CHECKED_TAKE) {
            operation.results[0] =
                (NLCheckedResult){old.type, old.current_value};
            operation.results[1] = (NLCheckedResult){slot_type, slot};
        } else {
            operation.results[0] = (NLCheckedResult){slot_type, slot};
        }
    } else if (kind == NL_CHECKED_REPLACE || kind == NL_CHECKED_STORE ||
               kind == NL_CHECKED_SWAP) {
        const NLValueId first = one_result(check, args[0]);
        const NLValueId second = one_result(check, args[1]);
        if (first == 0 || second == 0 ||
            !write_ref(check, first, view(check, args[0])->span)) {
            return false;
        }
        const NLPlaceId place = c->values[first - 1].reference.place;
        const NLSemanticPlaceView old = c->places[place - 1];
        if (!scalar_root_type(check, old.type, view(check, args[0])->span)) {
            return false;
        }
        if (kind == NL_CHECKED_SWAP) {
            if (c->types[old.type - 1].view.kind == NL_TYPE_SUM ||
                old.parent_sum != 0) {
                fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, span,
                     "P6-SWAP-UNSUPPORTED",
                     "sum/conditional payload swap is outside this slice");
                return false;
            }
            if (!write_ref(check, second, view(check, args[1])->span)) {
                return false;
            }
            const NLPlaceId other = c->values[second - 1].reference.place;
            if (c->places[other - 1].type != old.type) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[1])->span,
                     "P3-TYPE-MISMATCH", "swap referent types differ");
                return false;
            }
            if (other != place) {
                const NLValueId other_value =
                    c->places[other - 1].current_value;
                NLValueFactId first_fact, second_fact;
                if (!host(check, nl_sem_fresh_fact(c, &first_fact), span) ||
                    !host(check, nl_sem_fresh_fact(c, &second_fact), span)) {
                    return false;
                }
                c->places[place - 1].current_value = other_value;
                c->places[place - 1].current_fact = first_fact;
                c->places[other - 1].current_value = old.current_value;
                c->places[other - 1].current_fact = second_fact;
                c->values[other_value - 1].owner_place = place;
                c->values[old.current_value - 1].owner_place = other;
                refresh_bindings(check, place);
                refresh_bindings(check, other);
            }
        } else {
            if (c->values[second - 1].type != old.type) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[1])->span,
                     "P3-TYPE-MISMATCH",
                     "new value type differs from destination");
                return false;
            }
            if (kind == NL_CHECKED_STORE &&
                !c->types[old.type - 1].view.is_discardable) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[0])->span,
                     "P3-DISCARDABLE-REQUIRED",
                     "store requires Discardable(T)");
                return false;
            }
            if (c->types[old.type - 1].view.kind == NL_TYPE_SUM &&
                conflicts(check, place, false, false, first, span))
                return false;
            NLValueFactId fact;
            if (!host(check, nl_sem_fresh_fact(c, &fact), span)) {
                return false;
            }
            nl_sum_detach(c, place);
            c->places[place - 1].current_value = second;
            c->places[place - 1].current_fact = fact;
            c->values[second - 1].carrier = NL_CARRIER_PLACE;
            c->values[second - 1].owner_place = place;
            c->values[old.current_value - 1].carrier =
                kind == NL_CHECKED_REPLACE ? NL_CARRIER_LOOSE
                                           : NL_CARRIER_ENDED;
            c->values[old.current_value - 1].owner_place = 0;
            c->values[old.current_value - 1].sum_owner = 0;
            if (kind == NL_CHECKED_STORE)
                nl_sem_end_value(c, old.current_value);
            if (!host(check, nl_sum_attach(c, place), span) ||
                !host(check, nl_sum_payload_changed(c, place), span))
                return false;
            refresh_bindings(check, place);
            if (old.parent_sum != 0)
                refresh_bindings(check, old.parent_sum);
            if (kind == NL_CHECKED_REPLACE) {
                operation.type = old.type;
                operation.result_count = 1;
                operation.results[0] =
                    (NLCheckedResult){old.type, old.current_value};
            }
        }
    }
    *view(check, call) = operation;
    return true;
}

static NLCheckedNodeId source_block(Check *, const NLSyntaxView *);
static bool run_body(Check *, NLCheckedNodeId, const NLFunctionEntry *,
                     const NLCheckedNodeId *);

static NLCheckedNodeId call(Check *check, const NLSyntaxView *syntax)
{
    NLSemanticContext *const c = check->context;
    size_t function_id = 0;
    for (size_t i = 0; i < c->function_count; ++i) {
        if (equal_name(check, syntax->data.call.callee, c->functions[i].name)) {
            function_id = i + 1;
            break;
        }
    }
    if (function_id == 0) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->data.call.callee,
             "P3-UNKNOWN-CALLEE", "unknown semantic callee");
        return 0;
    }
    const NLFunctionEntry function = c->functions[function_id - 1];
    if (check->in_function_body && function.body != NULL) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->data.call.callee,
             function_id == check->body_function ? "P8-RECURSION-UNSUPPORTED"
                                                 : "P8-NESTED-BODY-CALL",
             "body-backed call chains/recursion are outside P8");
        return 0;
    }
    if (check->in_function_body &&
        function.kind != NL_CHECKED_REGISTERED_CALL &&
        function.kind != NL_CHECKED_PTR_FROM_REF &&
        function.kind != NL_CHECKED_REPLACE &&
        function.kind != NL_CHECKED_STORE && function.kind != NL_CHECKED_SWAP) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
             "P8-BODY-OPERATION",
             "operation is outside the bounded function body profile");
        return 0;
    }
    if (syntax->data.call.argument_count != function.count) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->data.call.callee,
             "P3-ARITY", "argument count differs from resolved signature");
        return 0;
    }
    if (function.body == NULL && function.caller_effects) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->data.call.callee,
             "P3-EFFECT-SUMMARY-UNSUPPORTED",
             "caller-visible function effects are outside P3");
        return 0;
    }
    if (function.body == NULL && function.hidden_dependencies) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->data.call.callee,
             "P3-DEPENDENCIES-UNSUPPORTED",
             "function dependency summary is outside P3");
        return 0;
    }
    if (function.body == NULL && function.kind == NL_CHECKED_REGISTERED_CALL &&
        ((c->types[function.result - 1].view.kind != NL_TYPE_NOMINAL &&
          function.result != 1) ||
         function.result == 2)) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->data.call.callee,
             "P3-RESULT-SUMMARY-UNSUPPORTED",
             "returned core authority/capability facts need a richer function "
             "summary");
        return 0;
    }
    const NLCheckedNodeId id =
        add(check, (NLCheckedNodeView){.kind = function.kind,
                                       .span = syntax->span,
                                       .name = syntax->data.call.callee,
                                       .function = function_id,
                                       .body_backed = function.body != NULL});
    if (id == 0) {
        return 0;
    }
    NLCheckedNodeId arguments[NL_SEMANTIC_MAX_PARAMETERS] = {0};
    const NLSyntaxNode *argument_syntax = syntax->data.call.arguments;
    NLCheckedNodeId tail = 0;
    NLTypeId destination_parameter = 0;
    for (size_t i = 0; i < function.count; ++i) {
        NLTypeId expected = function.kind == NL_CHECKED_REGISTERED_CALL
                                ? function.parameters[i]
                                : 0;
        if ((function.kind == NL_CHECKED_TAKE ||
             function.kind == NL_CHECKED_DESTROY) &&
            i == 1) {
            expected = compound(check, NL_TYPE_REF, 2, NL_ACCESS_READ, true,
                                nl_syntax_node_view(argument_syntax)->span);
        }
        if (function.kind == NL_CHECKED_DOMAIN_FINALIZE) {
            expected = 2;
        }
        if (function.kind == NL_CHECKED_INITIALIZE && i == 2) {
            expected = compound(check, NL_TYPE_REF, 2, NL_ACCESS_READ, false,
                                nl_syntax_node_view(argument_syntax)->span);
        }
        if ((function.kind == NL_CHECKED_REPLACE ||
             function.kind == NL_CHECKED_STORE ||
             function.kind == NL_CHECKED_SWAP) &&
            i == 0) {
            destination_parameter = write_parameter(check, argument_syntax);
            expected = destination_parameter;
        }
        if (function.kind == NL_CHECKED_SWAP && i == 1) {
            expected = destination_parameter;
        }
        if (check->status != NL_CHECK_OK) {
            return 0;
        }
        if (function.kind == NL_CHECKED_REGISTERED_CALL &&
            (expected == 2 ||
             c->types[expected - 1].view.kind == NL_TYPE_SLOT ||
             c->types[expected - 1].view.kind == NL_TYPE_STORAGE ||
             c->types[expected - 1].view.kind == NL_TYPE_ALLOCATION ||
             nl_sum_authority(c, expected))) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED,
                 nl_syntax_node_view(argument_syntax)->span,
                 "P3-AUTHORITY-SUMMARY-UNSUPPORTED",
                 "affine core authority transfer needs a richer user-function "
                 "summary");
            return 0;
        }
        arguments[i] = argument(check, argument_syntax, expected);
        if (arguments[i] == 0) {
            return 0;
        }
        if (function.kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedNodeView argument_view = *view(check, arguments[i]);
            const NLValueId value = argument_view.result_count == 1
                                        ? argument_view.results[0].value
                                        : 0;
            if (value != 0 &&
                c->types[c->values[value - 1].type - 1].view.kind ==
                    NL_TYPE_REF) {
                if (!reference_live(check, value,
                                    view(check, arguments[i])->span))
                    return 0;
                if (function.body == NULL &&
                    c->values[value - 1].reference_count != 0 &&
                    (c->types[expected - 1].view.access != NL_ACCESS_READ ||
                     function.caller_effects || function.hidden_dependencies)) {
                    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                         view(check, arguments[i])->span, "P7-CALL-PRECISION",
                         "joined ref argument requires read-only/no-effect "
                         "call");
                    return 0;
                }
            }
        }
        if (function.body == NULL &&
            function.kind == NL_CHECKED_REGISTERED_CALL && expected != 0 &&
            c->types[expected - 1].view.kind == NL_TYPE_REF &&
            c->types[c->types[expected - 1].view.target - 1].view.kind ==
                NL_TYPE_SUM &&
            c->types[expected - 1].view.access == NL_ACCESS_WRITE) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                 view(check, arguments[i])->span, "P6-CALL-PRESERVATION",
                 "whole-sum write call needs an occurrence-preservation "
                 "summary");
            return 0;
        }
        if (function.kind == NL_CHECKED_REGISTERED_CALL &&
            view(check, arguments[i])->result_count > 1) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED,
                 view(check, arguments[i])->span, "P3-ARGUMENT-RESULTS",
                 "multi-result argument receiving is outside P3");
            return 0;
        }
        if (tail == 0) {
            view(check, id)->first_argument = arguments[i];
        } else {
            view(check, tail)->next_argument = arguments[i];
        }
        tail = arguments[i];
        ++view(check, id)->argument_count;
        argument_syntax = nl_syntax_next_argument(argument_syntax);
    }
    if (!(function.body != NULL ? run_body(check, id, &function, arguments)
                                : primitive(check, id, arguments))) {
        return 0;
    }
    /* Finish each call-local child only at the call's return, after all later
     * argument evaluation and the primitive. No child-derived result escapes
     * the supported user-call contract. Forwarded package results stay alive.
     */
    for (size_t i = 0; i < function.count; ++i) {
        const NLCheckedNodeView argument_view = *view(check, arguments[i]);
        if (argument_view.reborrow_scope != 0) {
            c->scopes[argument_view.reborrow_scope - 1].active = false;
        }
        for (size_t r = 0; r < argument_view.result_count; ++r) {
            const NLValueId value = argument_view.results[r].value;
            bool forwarded = c->values[value - 1].carrier == NL_CARRIER_PLACE;
            for (size_t s = 0; s < view(check, id)->result_count; ++s) {
                forwarded =
                    forwarded || view(check, id)->results[s].value == value;
            }
            if (!forwarded) {
                end_temporary(check, value);
            }
        }
    }
    return id;
}

static NLCheckedNodeId source_binding(Check *, const NLSyntaxView *);
static NLCheckedNodeId source_statement(Check *, const NLSyntaxView *);
static NLCheckedNodeId source_block(Check *, const NLSyntaxView *);
static NLCheckedNodeId aggregate(Check *, const NLSyntaxView *);
static NLCheckedNodeId sum_constructor(Check *, const NLSyntaxView *);
static NLCheckedNodeId sum_match(Check *, const NLSyntaxView *);

static NLCheckedNodeId expression(Check *check, const NLSyntaxNode *syntax)
{
    const NLSyntaxView *const node = nl_syntax_node_view(syntax);
    if (!enter(check, node->span)) {
        return 0;
    }
    NLCheckedNodeId result = 0;
    if (check->in_function_body && node->kind != NL_SYNTAX_EXPR_NAME &&
        node->kind != NL_SYNTAX_EXPR_CALL && node->kind != NL_SYNTAX_BLOCK) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, node->span,
             "P8-BODY-PROFILE",
             "expression needs a richer relative body analysis");
        --check->depth;
        return 0;
    }
    if (node->kind == NL_SYNTAX_EXPR_NAME) {
        result = identifier(check, node);
    } else if (node->kind == NL_SYNTAX_EXPR_CALL) {
        result = call(check, node);
    } else if (node->kind == NL_SYNTAX_BLOCK) {
        result = source_block(check, node);
    } else if (node->kind == NL_SYNTAX_SUM_CONSTRUCTOR) {
        result = sum_constructor(check, node);
    } else if (node->kind == NL_SYNTAX_MATCH) {
        result = sum_match(check, node);
    } else if (node->kind == NL_SYNTAX_AGGREGATE) {
        result = aggregate(check, node);
    } else {
        fail(check, NL_CHECK_INTERNAL_ERROR, node->span, "P3-INTERNAL",
             "expected expression syntax");
    }
    --check->depth;
    return result;
}

static NLCheckedNodeId binding(Check *check, const NLSyntaxView *syntax)
{
    /* Resolve names from spans, not tokens or reparsed text. This sole name
     * copy becomes registry ownership; lexemes elsewhere remain borrowed. */
    NLSourceView name;
    if (!nl_source_view(check->source, syntax->data.binding.name, &name)) {
        fail(check, NL_CHECK_INTERNAL_ERROR, syntax->span, "P3-INTERNAL",
             "invalid binding span");
        return 0;
    }
    char *const owned_name = malloc(name.length + 1);
    if (owned_name == NULL) {
        (void)host(check, NL_CHECK_OUT_OF_MEMORY, syntax->data.binding.name);
        return 0;
    }
    memcpy(owned_name, name.bytes, name.length);
    owned_name[name.length] = 0;
    if (nl_semantic_find_binding(check->context, owned_name) != 0) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->data.binding.name,
             "P3-DUPLICATE-BINDING",
             "binding is single-assignment in this semantic scope");
        free(owned_name);
        return 0;
    }
    const NLCheckedNodeId initializer =
        expression(check, syntax->data.binding.initializer);
    if (initializer == 0) {
        free(owned_name);
        return 0;
    }
    const NLCheckedNodeView initialized = *view(check, initializer);
    if (initialized.result_count != 1) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->data.binding.name,
             "P3-DIRECT-RECEIVING-UNSUPPORTED",
             "single-name binding needs one value; direct multi-result "
             "receiving is outside P2/P3");
        free(owned_name);
        return 0;
    }
    NLSymbolId symbol;
    const NLCheckStatus status = nl_sem_bind(
        check->context, owned_name, initialized.results[0].value, &symbol);
    free(owned_name);
    if (!host(check, status, syntax->data.binding.name)) {
        return 0;
    }
    return add(check, (NLCheckedNodeView){.kind = NL_CHECKED_BINDING,
                                          .span = syntax->span,
                                          .name = syntax->data.binding.name,
                                          .type = initialized.type,
                                          .symbol = symbol,
                                          .initializer = initializer});
}

static char *source_name_copy(Check *check, NLSourceSpan span)
{
    NLSourceView bytes;
    if (!nl_source_view(check->source, span, &bytes)) {
        (void)host(check, NL_CHECK_INTERNAL_ERROR, span);
        return NULL;
    }
    char *name = malloc(bytes.length + 1);
    if (name == NULL) {
        (void)host(check, NL_CHECK_OUT_OF_MEMORY, span);
        return NULL;
    }
    memcpy(name, bytes.bytes, bytes.length);
    name[bytes.length] = 0;
    return name;
}

static bool fresh_name(Check *check, const char *name, NLSourceSpan span)
{
    for (size_t i = check->binding_floor; i < check->context->binding_count;
         ++i) {
        if (!check->context->bindings[i].hidden &&
            strcmp(name, check->context->bindings[i].name) == 0) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P5-DUPLICATE-BINDING",
                 "binding name already exists in current lexical scope");
            return false;
        }
    }
    return true;
}

static NLTypeId aggregate_type(Check *check, NLSourceSpan name)
{
    for (size_t i = 0; i < check->context->type_count; ++i) {
        const NLTypeEntry *entry = &check->context->types[i];
        if (entry->name != NULL && equal_name(check, name, entry->name)) {
            if (entry->view.field_count == 0) {
                fail(
                    check, NL_CHECK_SEMANTIC_UNSUPPORTED, name,
                    "P5-AGGREGATE-SHAPE-UNSUPPORTED",
                    "type has no registered aggregate shape in selected slice");
                return 0;
            }
            return i + 1;
        }
    }
    fail(check, NL_CHECK_SEMANTIC_ERROR, name, "P5-UNKNOWN-AGGREGATE",
         "unknown registered aggregate type");
    return 0;
}

static bool aggregate_fields(Check *check, NLTypeId type,
                             const NLSyntaxNode *fields, size_t count,
                             bool construction,
                             size_t indices[NL_SEMANTIC_MAX_FIELDS])
{
    const NLTypeEntry *entry = &check->context->types[type - 1];
    bool seen[NL_SEMANTIC_MAX_FIELDS] = {false};
    size_t n = 0;
    for (const NLSyntaxNode *f = fields; f != NULL;
         f = nl_syntax_next_argument(f)) {
        const NLSyntaxView *v = nl_syntax_node_view(f);
        const NLSourceSpan name =
            construction ? v->data.binding.name : v->data.name;
        size_t index = entry->view.field_count;
        for (size_t j = 0; j < entry->view.field_count; ++j) {
            if (equal_name(check, name, entry->field_names[j])) {
                index = j;
                break;
            }
        }
        if (index == entry->view.field_count || seen[index]) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, name, "P5-AGGREGATE-FIELD",
                 "unknown or duplicate aggregate field");
            return false;
        }
        seen[index] = true;
        indices[n++] = index;
    }
    if (count != entry->view.field_count || n != count) {
        fail(check, NL_CHECK_SEMANTIC_ERROR,
             fields == NULL ? (NLSourceSpan){0, 0}
                            : nl_syntax_node_view(fields)->span,
             "P5-AGGREGATE-FIELD-COUNT",
             "all aggregate fields must occur exactly once");
        return false;
    }
    return true;
}

static NLCheckedNodeId aggregate(Check *check, const NLSyntaxView *syntax)
{
    const NLTypeId type =
        aggregate_type(check, syntax->data.aggregate.type_name);
    if (type == 0)
        return 0;
    size_t indices[NL_SEMANTIC_MAX_FIELDS];
    if (!aggregate_fields(check, type, syntax->data.aggregate.fields,
                          syntax->data.aggregate.count, true, indices))
        return 0;
    NLSemanticContext *c = check->context;
    NLSemanticValueView package = {.type = type,
                                   .field_count = syntax->data.aggregate.count};
    const NLCheckedNodeId id =
        add(check, (NLCheckedNodeView){.kind = NL_CHECKED_AGGREGATE,
                                       .span = syntax->span,
                                       .name = syntax->data.aggregate.type_name,
                                       .type = type});
    if (id == 0)
        return 0;
    NLCheckedNodeId previous = 0;
    size_t n = 0;
    for (const NLSyntaxNode *f = syntax->data.aggregate.fields; f != NULL;
         f = nl_syntax_next_argument(f), ++n) {
        const NLSyntaxView *field = nl_syntax_node_view(f);
        const NLCheckedNodeId init =
            expression(check, field->data.binding.initializer);
        if (init == 0)
            return 0;
        const NLValueId value = one_result(check, init);
        if (value == 0)
            return 0;
        const size_t index = indices[n];
        if (c->values[value - 1].type !=
            c->types[type - 1].field_types[index]) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, init)->span,
                 "P5-FIELD-TYPE",
                 "aggregate initializer type differs from registered field");
            return 0;
        }
        package.fields[index] = value;
        const NLCheckedNodeId field_id =
            add(check, (NLCheckedNodeView){.kind = NL_CHECKED_AGGREGATE_FIELD,
                                           .span = field->span,
                                           .name = field->data.binding.name,
                                           .initializer = init,
                                           .field_index = index,
                                           .type = c->values[value - 1].type});
        if (field_id == 0)
            return 0;
        if (previous == 0)
            view(check, id)->first_argument = field_id;
        else
            view(check, previous)->next_argument = field_id;
        previous = field_id;
        ++view(check, id)->argument_count;
    }
    const NLValueId value = new_value(check, package, syntax->span);
    if (value == 0)
        return 0;
    for (size_t f = 0; f < package.field_count; ++f) {
        c->values[package.fields[f] - 1].carrier = NL_CARRIER_AGGREGATE;
        c->values[package.fields[f] - 1].aggregate_owner = value;
        c->values[package.fields[f] - 1].owner_place = 0;
    }
    view(check, id)->result_count = 1;
    view(check, id)->results[0] = (NLCheckedResult){type, value};
    return id;
}

static NLCheckedNodeId source_binding(Check *check, const NLSyntaxView *syntax)
{
    const bool multi = syntax->kind == NL_SYNTAX_MULTI_BINDING;
    const bool destructure = syntax->kind == NL_SYNTAX_AGGREGATE_BINDING;
    const size_t count = multi         ? syntax->data.multi_binding.count
                         : destructure ? syntax->data.aggregate.count
                                       : 1;
    if (count > NL_SEMANTIC_MAX_FIELDS) {
        fail(check, NL_CHECK_RESOURCE_LIMIT, syntax->span, "P5-RECEIVER-LIMIT",
             "P5 receiver budget exceeded");
        return 0;
    }
    const NLSyntaxNode *receivers = multi ? syntax->data.multi_binding.receivers
                                    : destructure
                                        ? syntax->data.aggregate.fields
                                        : NULL;
    const NLSyntaxNode *initializer =
        multi         ? syntax->data.multi_binding.initializer
        : destructure ? syntax->data.aggregate.initializer
                      : syntax->data.binding.initializer;
    NLTypeId type = 0;
    size_t indices[NL_SEMANTIC_MAX_FIELDS] = {0};
    if (destructure) {
        type = aggregate_type(check, syntax->data.aggregate.type_name);
        if (type == 0 ||
            !aggregate_fields(check, type, receivers, count, false, indices))
            return 0;
    }
    char *names[NL_SEMANTIC_MAX_FIELDS] = {NULL};
    NLSourceSpan spans[NL_SEMANTIC_MAX_FIELDS];
    NLCheckedNodeId result = 0;
    const NLSyntaxNode *r = receivers;
    for (size_t i = 0; i < count; ++i) {
        spans[i] = receivers == NULL ? syntax->data.binding.name
                                     : nl_syntax_node_view(r)->data.name;
        names[i] = source_name_copy(check, spans[i]);
        if (names[i] == NULL || !fresh_name(check, names[i], spans[i]))
            goto cleanup;
        for (size_t j = 0; j < i; ++j) {
            if (strcmp(names[i], names[j]) == 0) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, spans[i],
                     "P5-DUPLICATE-RECEIVER",
                     "result receivers must be distinct");
                goto cleanup;
            }
        }
        if (r != NULL)
            r = nl_syntax_next_argument(r);
    }
    const NLCheckedNodeId init = expression(check, initializer);
    if (init == 0)
        goto cleanup;
    NLCheckedNodeView rhs = *view(check, init);
    NLValueId values[NL_SEMANTIC_MAX_FIELDS] = {0};
    if (destructure) {
        if (rhs.result_count != 1 || rhs.results[0].type != type) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, init)->span,
                 "P5-DESTRUCTURE-TYPE",
                 "destructuring RHS must be the selected aggregate");
            goto cleanup;
        }
        const NLValueId aggregate_value = rhs.results[0].value;
        const NLSemanticValueView original =
            check->context->values[aggregate_value - 1];
        if (original.field_count != count) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
                 "P5-AGGREGATE-VALUE-UNSUPPORTED",
                 "aggregate member packages are unavailable");
            goto cleanup;
        }
        /* Complete whole-value destruction, not a partial-move binding state.
         */
        check->context->values[aggregate_value - 1].carrier = NL_CARRIER_ENDED;
        for (size_t i = 0; i < count; ++i) {
            values[i] = original.fields[indices[i]];
            check->context->values[values[i] - 1].carrier = NL_CARRIER_LOOSE;
            check->context->values[values[i] - 1].aggregate_owner = 0;
        }
    } else if (!multi && rhs.result_count == 0 && rhs.type == 1) {
        values[0] =
            new_value(check, (NLSemanticValueView){.type = 1}, syntax->span);
        if (values[0] == 0)
            goto cleanup;
    } else {
        if (rhs.result_count != count) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
                 "P5-RESULT-ARITY",
                 "receiver count must exactly match independent checked "
                 "results");
            goto cleanup;
        }
        for (size_t i = 0; i < count; ++i)
            values[i] = rhs.results[i].value;
    }
    result = add(check, (NLCheckedNodeView){
                            .kind = multi         ? NL_CHECKED_MULTI_BINDING
                                    : destructure ? NL_CHECKED_AGGREGATE_BINDING
                                                  : NL_CHECKED_BINDING,
                            .span = syntax->span,
                            .initializer = init,
                            .type = 1});
    if (result == 0)
        goto cleanup;
    NLCheckedNodeId previous = 0;
    for (size_t i = 0; i < count; ++i) {
        NLSymbolId symbol = 0;
        if (!host(check,
                  nl_sem_bind_in_scope(check->context, names[i], values[i],
                                       check->binding_floor, &symbol),
                  spans[i])) {
            result = 0;
            goto cleanup;
        }
        const NLCheckedNodeId receiver =
            add(check, (NLCheckedNodeView){
                           .kind = NL_CHECKED_RECEIVER,
                           .span = spans[i],
                           .name = spans[i],
                           .value_use = NL_VALUE_RECEIVED,
                           .symbol = symbol,
                           .type = check->context->values[values[i] - 1].type,
                           .field_index = indices[i]});
        if (receiver == 0) {
            result = 0;
            goto cleanup;
        }
        if (previous == 0)
            view(check, result)->first_argument = receiver;
        else
            view(check, previous)->next_argument = receiver;
        previous = receiver;
        ++view(check, result)->argument_count;
        if (!multi && !destructure) {
            view(check, result)->symbol = symbol;
            view(check, result)->name = spans[i];
        }
    }
cleanup:
    for (size_t i = 0; i < count; ++i)
        free(names[i]);
    return result;
}

static bool discard_results(Check *check, NLCheckedNodeId id)
{
    const NLCheckedNodeView node = *view(check, id);
    for (size_t r = 0; r < node.result_count; ++r) {
        if (!check->context->types[node.results[r].type - 1]
                 .view.is_discardable) {
            fail(
                check, NL_CHECK_SEMANTIC_ERROR, node.span,
                "P5-DISCARDABLE-REQUIRED",
                "expression statement cannot discard a non-Discardable result");
            return false;
        }
    }
    for (size_t r = 0; r < node.result_count; ++r)
        end_temporary(check, node.results[r].value);
    return true;
}

static NLCheckedNodeId source_statement(Check *check,
                                        const NLSyntaxView *syntax)
{
    const NLCheckedNodeId init =
        expression(check, syntax->data.statement.expression);
    if (init == 0 || !discard_results(check, init))
        return 0;
    return add(check, (NLCheckedNodeView){.kind = NL_CHECKED_STATEMENT,
                                          .span = syntax->span,
                                          .initializer = init,
                                          .type = 1});
}

static NLCheckedNodeId source_block(Check *check, const NLSyntaxView *syntax)
{
    NLSemanticContext *c = check->context;
    const size_t previous_floor = check->binding_floor;
    const size_t floor =
        check->has_arm_floor ? check->arm_floor : c->binding_count;
    check->has_arm_floor = false;
    check->binding_floor = floor;
    const NLCheckedNodeId id = add(
        check, (NLCheckedNodeView){
                   .kind = NL_CHECKED_BLOCK, .span = syntax->span, .type = 1});
    if (id == 0)
        goto cleanup;
    NLCheckedNodeId previous = 0;
    for (const NLSyntaxNode *item = syntax->data.block.items; item != NULL;
         item = nl_syntax_next_argument(item)) {
        const NLSyntaxView *v = nl_syntax_node_view(item);
        if (check->in_function_body && v->kind != NL_SYNTAX_BINDING &&
            v->kind != NL_SYNTAX_STATEMENT) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, v->span,
                 "P8-BODY-PROFILE",
                 "body receiving pattern needs richer relative analysis");
            goto cleanup;
        }
        const NLCheckedNodeId child = v->kind == NL_SYNTAX_STATEMENT
                                          ? source_statement(check, v)
                                          : source_binding(check, v);
        if (child == 0)
            goto cleanup;
        if (previous == 0)
            view(check, id)->first_item = child;
        else
            view(check, previous)->next_item = child;
        previous = child;
        ++view(check, id)->item_count;
    }
    if (syntax->data.block.tail != NULL) {
        const NLCheckedNodeId tail = expression(check, syntax->data.block.tail);
        if (tail == 0)
            goto cleanup;
        const NLCheckedNodeView result = *view(check, tail);
        view(check, id)->tail = tail;
        view(check, id)->type = result.type;
        view(check, id)->result_count = result.result_count;
        memcpy(view(check, id)->results, result.results,
               sizeof(result.results));
    }
    for (size_t i = floor; i < c->binding_count; ++i) {
        NLBindingEntry *entry = &c->bindings[i];
        if (entry->hidden)
            continue;
        if (entry->view.availability == NL_AVAILABLE) {
            if (!c->types[entry->view.type - 1].view.is_discardable) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
                     "P5-SCOPE-OBLIGATION",
                     "non-Discardable local remains available at block exit");
                goto cleanup;
            }
            if (conflicts(check, entry->view.place, false, true, 0,
                          syntax->span))
                goto cleanup;
            nl_sum_detach(c, entry->view.place);
            end_temporary(check, entry->view.value);
            c->places[entry->view.place - 1].live = false;
            c->places[entry->view.place - 1].current_value = 0;
            c->places[entry->view.place - 1].current_fact = 0;
            c->places[entry->view.place - 1].governing_domain = 0;
            entry->view.availability = NL_CONSUMED;
        }
        entry->hidden = true;
    }
cleanup:
    check->binding_floor = previous_floor;
    return check->status == NL_CHECK_OK ? id : 0;
}

static bool body_result(Check *check, const NLFunctionEntry *function,
                        NLCheckedNodeId body, NLSourceSpan span)
{
    const NLCheckedNodeView result = *view(check, body);
    if (result.type != function->result || result.result_count > 1 ||
        (result.result_count == 0 && function->result != 1)) {
        fail(
            check, NL_CHECK_SEMANTIC_ERROR, span, "P8-BODY-RESULT",
            "normal body result must exactly match the declared single result");
        return false;
    }
    return true;
}

static bool run_body(Check *caller, NLCheckedNodeId call_id,
                     const NLFunctionEntry *function,
                     const NLCheckedNodeId *arguments)
{
    NLSemanticContext *c = caller->context;
    const size_t binding_floor = c->binding_count, scope_floor = c->scope_count,
                 place_floor = c->place_count;
    const NLSourceSpan call_span = view(caller, call_id)->span;
    Check body = {.context = c,
                  .source = function->body->source,
                  .binding_floor = binding_floor,
                  .namespace_floor = binding_floor,
                  .arm_floor = binding_floor,
                  .has_arm_floor = true,
                  .in_function_body = true,
                  .body_function = view(caller, call_id)->function};
    body.artifact = malloc(sizeof(*body.artifact));
    if (body.artifact == NULL) {
        (void)host(caller, NL_CHECK_OUT_OF_MEMORY, call_span);
        return false;
    }
    *body.artifact = (NLCheckedFragment){.source = body.source,
                                         .context = caller->artifact->context};
    if (!host(&body, nl_body_retain(function->body), (NLSourceSpan){0}))
        goto failure;
    body.artifact->body_owner = function->body;
    body.artifact->release_body = nl_body_release;
    for (size_t i = 0; i < function->count; ++i) {
        const NLCheckedNodeView argument = *view(caller, arguments[i]);
        NLValueId value =
            argument.result_count == 1 ? argument.results[0].value : 0;
        if (value == 0) {
            value = new_value(&body, (NLSemanticValueView){.type = 1},
                              (NLSourceSpan){0});
            if (value == 0)
                goto failure;
        }
        /* Contextual ordinary write->read compatibility produces a fresh
         * parameter capability, retaining all facts and the caller package. */
        if (c->values[value - 1].type != function->parameters[i]) {
            NLSemanticValueView weakened = c->values[value - 1];
            weakened.type = function->parameters[i];
            value = new_value(&body, weakened, (NLSourceSpan){0});
            if (value == 0)
                goto failure;
        }
        NLSymbolId symbol;
        if (!host(&body,
                  nl_sem_bind_in_scope(c, function->body->parameter_names[i],
                                       value, binding_floor, &symbol),
                  (NLSourceSpan){0}))
            goto failure;
    }
    body.artifact->root = source_block(
        &body,
        nl_syntax_node_view(nl_syntax_tree_root(function->body->syntax)));
    if (body.artifact->root == 0 ||
        !body_result(
            &body, function, body.artifact->root,
            nl_syntax_node_view(nl_syntax_tree_root(function->body->syntax))
                ->span))
        goto failure;
    const NLCheckStatus exit_status =
        nl_sem_function_exit(c, scope_floor, place_floor);
    if (exit_status != NL_CHECK_OK) {
        fail(&body, exit_status, (NLSourceSpan){0}, "P8-EXIT-DEPENDENCY",
             "surviving state depends on an ending function-local scope/place");
        goto failure;
    }
    for (size_t i = scope_floor; i < c->scope_count; ++i)
        c->scopes[i].active = false;
    /* Save owned checked body evidence; no source expansion into caller nodes.
     */
    if (caller->artifact->body_count == 64) {
        (void)host(&body, NL_CHECK_RESOURCE_LIMIT, (NLSourceSpan){0});
        goto failure;
    }
    const size_t count = caller->artifact->body_count;
    NLCheckedFragment **bodies =
        realloc(caller->artifact->bodies, (count + 1) * sizeof(*bodies));
    if (bodies == NULL) {
        (void)host(&body, NL_CHECK_OUT_OF_MEMORY, (NLSourceSpan){0});
        goto failure;
    }
    caller->artifact->bodies = bodies;
    NLCheckedNodeId *calls =
        realloc(caller->artifact->body_calls, (count + 1) * sizeof(*calls));
    if (calls == NULL) {
        (void)host(&body, NL_CHECK_OUT_OF_MEMORY, (NLSourceSpan){0});
        goto failure;
    }
    caller->artifact->body_calls = calls;
    const NLCheckedNodeView result = *view(&body, body.artifact->root);
    view(caller, call_id)->type = result.type;
    view(caller, call_id)->result_count = result.result_count;
    memcpy(view(caller, call_id)->results, result.results,
           sizeof(result.results));
    caller->artifact->bodies[count] = body.artifact;
    caller->artifact->body_calls[count] = call_id;
    ++caller->artifact->body_count;
    return true;
failure:
    /* Report a body diagnostic at the CALL site. The detailed definition
     * diagnostic has already been checked against its own source at
     * registration. */
    fail(caller, body.status, call_span, body.diagnostic.diagnostic.code,
         body.diagnostic.diagnostic.message);
    nl_checked_destroy(body.artifact);
    return false;
}

/* P6 is a finite branch checker, not a general CFG/relational solver. Arm
 * contexts own hypothetical cases; their IDs never enter the committed state.
 */
static size_t sum_variant(Check *check, NLTypeId type, NLSourceSpan name)
{
    const NLTypeEntry *e = &check->context->types[type - 1];
    for (size_t i = 0; i < e->view.variant_count; ++i)
        if (equal_name(check, name, e->variant_names[i]))
            return i + 1;
    fail(check, NL_CHECK_SEMANTIC_ERROR, name, "P6-UNKNOWN-VARIANT",
         "variant is not in the exact nominal sum type");
    return 0;
}

static NLCheckedNodeId sum_constructor(Check *check, const NLSyntaxView *s)
{
    NLSemanticContext *c = check->context;
    NLTypeId type = 0;
    for (size_t i = 0; i < c->type_count; ++i)
        if (c->types[i].name != NULL &&
            equal_name(check, s->data.constructor.qualifier,
                       c->types[i].name)) {
            type = i + 1;
            break;
        }
    if (type == 0 || c->types[type - 1].view.kind != NL_TYPE_SUM) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, s->data.constructor.qualifier,
             "P6-SUM-QUALIFIER",
             "constructor requires a registered nominal sum qualifier");
        return 0;
    }
    size_t variant = sum_variant(check, type, s->data.constructor.variant);
    if (variant == 0)
        return 0;
    NLTypeId payload_type = c->types[type - 1].variant_types[variant - 1];
    if ((payload_type == 0 && s->data.constructor.parentheses) ||
        (payload_type != 0 && (!s->data.constructor.parentheses ||
                               s->data.constructor.argument_count != 1))) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, s->data.constructor.variant,
             "P6-CONSTRUCTOR-SHAPE",
             "constructor payload shape/arity does not match variant");
        return 0;
    }
    NLCheckedNodeId init = 0;
    NLValueId payload = 0;
    if (payload_type != 0) {
        init = expression(check, s->data.constructor.arguments);
        if (init == 0 || (payload = one_result(check, init)) == 0)
            return 0;
        if (c->values[payload - 1].type != payload_type) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, init)->span,
                 "P6-PAYLOAD-TYPE",
                 "constructor payload type differs from registered variant");
            return 0;
        }
    }
    NLValueId value =
        new_value(check,
                  (NLSemanticValueView){
                      .type = type, .variant = variant, .sum_payload = payload},
                  s->span);
    if (value == 0)
        return 0;
    if (payload != 0) {
        c->values[payload - 1].carrier = NL_CARRIER_SUM;
        c->values[payload - 1].sum_owner = value;
    }
    return add(check,
               (NLCheckedNodeView){.kind = NL_CHECKED_SUM_CONSTRUCTOR,
                                   .span = s->span,
                                   .qualifier = s->data.constructor.qualifier,
                                   .name = s->data.constructor.variant,
                                   .variant = variant,
                                   .type = type,
                                   .initializer = init,
                                   .result_count = 1,
                                   .results = {{type, value}}});
}

static bool flat_copy(const NLSemanticContext *c, NLTypeId type)
{
    const NLSemanticTypeView t = c->types[type - 1].view;
    return t.is_copy && t.field_count == 0 &&
           (t.kind == NL_TYPE_NOMINAL || t.kind == NL_TYPE_UNIT ||
            t.kind == NL_TYPE_BYTE || t.kind == NL_TYPE_U8 ||
            t.kind == NL_TYPE_USIZE || t.kind == NL_TYPE_ADDR);
}

static void match_precision(Check *check, NLSourceSpan span)
{
    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span, "P6-JOIN-PRECISION",
         "branch post-state/result needs a richer relational join than P6 "
         "supports");
}

static bool same_place_frame(NLSemanticPlaceView a, NLSemanticPlaceView b)
{
    return a.type == b.type && a.live == b.live &&
           a.incarnation == b.incarnation &&
           a.independent_root == b.independent_root &&
           a.parent_sum == b.parent_sum &&
           a.governing_domain == b.governing_domain &&
           a.placement.region == b.placement.region &&
           a.placement.start == b.placement.start &&
           a.placement.length == b.placement.length;
}

static bool arm_frame(Check *check, const NLSemanticContext *before,
                      const NLSemanticContext *after, NLPlaceId root,
                      size_t persistent_places, size_t persistent_scopes,
                      bool changed[NL_SEMANTIC_MAX_ENTRIES], NLSourceSpan span)
{
    /* Existing scopes, domains, raw backing facts cannot silently diverge.
     * New local scopes/packages remain arm-owned. */
    if (before->domain_count != after->domain_count ||
        before->region_count != after->region_count)
        goto precision;
    for (size_t i = 0; i < before->domain_count; ++i)
        if (before->domains[i].live != after->domains[i].live)
            goto precision;
    for (size_t i = 0; i < persistent_scopes; ++i)
        if (before->scopes[i].active != after->scopes[i].active)
            goto precision;
    for (size_t i = 0; i < before->region_count; ++i) {
        const NLRawRegionEntry *a = &before->regions[i],
                               *b = &after->regions[i];
        if (a->view.live != b->view.live || a->count != b->count)
            goto precision;
        for (size_t j = 0; j < a->count; ++j)
            if (a->intervals[j].start != b->intervals[j].start ||
                a->intervals[j].length != b->intervals[j].length ||
                a->intervals[j].state.validity !=
                    b->intervals[j].state.validity ||
                a->intervals[j].state.value_known !=
                    b->intervals[j].state.value_known ||
                a->intervals[j].state.value != b->intervals[j].state.value)
                goto precision;
    }
    for (size_t i = 0; i < persistent_places; ++i) {
        const NLSemanticPlaceView a = before->places[i], b = after->places[i];
        if (i + 1 == root || (root != 0 && a.parent_sum == root))
            continue;
        if (!same_place_frame(a, b)) {
            /* Ordinary outer affine consumption is joined separately below. */
            bool consumed = false;
            for (size_t j = 0; j < before->binding_count; ++j)
                if (before->bindings[j].view.place == i + 1 &&
                    before->bindings[j].view.availability == NL_AVAILABLE &&
                    after->bindings[j].view.availability == NL_CONSUMED &&
                    !nl_sum_authority(before, a.type) && a.parent_sum == 0 &&
                    before->types[a.type - 1].view.kind != NL_TYPE_SUM &&
                    a.placement.region == 0)
                    consumed = true;
            if (!consumed)
                goto precision;
        } else if (a.current_fact != b.current_fact ||
                   a.current_value != b.current_value) {
            if (!flat_copy(before, a.type))
                goto precision;
            changed[i] = true;
        }
    }
    return true;
precision:
    match_precision(check, span);
    return false;
}

static bool save_arm(Check *check, NLCheckedNodeId match,
                     NLCheckedFragment *arm, NLSourceSpan span)
{
    if (check->artifact->arm_count == 64) {
        (void)host(check, NL_CHECK_RESOURCE_LIMIT, span);
        return false;
    }
    NLCheckedArm *arms =
        realloc(check->artifact->arms,
                (check->artifact->arm_count + 1) * sizeof(*arms));
    if (arms == NULL) {
        (void)host(check, NL_CHECK_OUT_OF_MEMORY, span);
        return false;
    }
    check->artifact->arms = arms;
    arms[check->artifact->arm_count++] = (NLCheckedArm){match, arm};
    return true;
}

/* P7 keeps complete facts as a finite may-set. Sorting/deduplication is only
 * canonicalization, never a choice of one incoming referent. */
static int ref_order(NLReferenceFacts a, NLReferenceFacts b)
{
    const size_t left[] = {
        a.place,    a.incarnation,          a.scope, a.provenance, a.readable,
        a.writable, a.occurrence_dependency};
    const size_t right[] = {
        b.place,    b.incarnation,          b.scope, b.provenance, b.readable,
        b.writable, b.occurrence_dependency};
    for (size_t i = 0; i < sizeof(left) / sizeof(left[0]); ++i)
        if (left[i] != right[i])
            return left[i] < right[i] ? -1 : 1;
    return 0;
}

static bool ref_alternative(Check *check, NLSemanticValueView *join,
                            NLReferenceFacts fact, NLSourceSpan span)
{
    size_t i = 0;
    while (i < join->reference_count &&
           ref_order(join->references[i], fact) < 0)
        ++i;
    if (i < join->reference_count && ref_order(join->references[i], fact) == 0)
        return true;
    if (join->reference_count == NL_SEMANTIC_MAX_REF_ALTERNATIVES) {
        (void)host(check, NL_CHECK_RESOURCE_LIMIT, span);
        return false;
    }
    for (size_t j = join->reference_count; j > i; --j)
        join->references[j] = join->references[j - 1];
    join->references[i] = fact;
    ++join->reference_count;
    return true;
}

static bool join_ref_type(const NLSemanticContext *c, NLTypeId type)
{
    const NLSemanticTypeView t = c->types[type - 1].view;
    if (t.kind != NL_TYPE_REF || t.is_exclusive)
        return false;
    const NLSemanticTypeView target = c->types[t.target - 1].view;
    return target.field_count == 0 && t.target != 2 &&
           (target.kind == NL_TYPE_NOMINAL || target.kind == NL_TYPE_BYTE ||
            target.kind == NL_TYPE_U8 || target.kind == NL_TYPE_USIZE ||
            target.kind == NL_TYPE_ADDR);
}

static bool ref_join_precision(Check *check, NLSourceSpan span)
{
    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
         "P7-REF-JOIN-PRECISION",
         "ref join cannot prove public stable origin/scope or unchanged "
         "incoming state");
    return false;
}

static bool same_scope(NLSemanticScopeView a, NLSemanticScopeView b)
{
    return a.active == b.active && a.parent == b.parent &&
           a.parent_authority == b.parent_authority;
}

static bool rebase_ref_result(Check *public_check, Check *branch,
                              const NLSemanticContext *guard, NLValueId output,
                              NLPlaceId root, NLScopeId child_scope,
                              NLReferenceFacts parent,
                              NLSemanticValueView *join, NLSourceSpan span)
{
    const NLSemanticContext *c = public_check->context, *b = branch->context;
    const NLSemanticValueView result = b->values[output - 1];
    if (result.type > c->type_count || !join_ref_type(c, result.type))
        return ref_join_precision(branch, span);
    /* This bounded result join has no memory-state phi. Guard refinement is
     * arm-owned; operation effects on public-prefix state must be absent. */
    for (size_t i = 0; i < c->place_count; ++i) {
        const NLSemanticPlaceView before = guard->places[i],
                                  after = b->places[i];
        if (!same_place_frame(before, after) ||
            before.current_fact != after.current_fact ||
            before.payload_occurrence != after.payload_occurrence ||
            before.current_value != after.current_value)
            return ref_join_precision(branch, span);
    }
    for (size_t i = 0; i < c->scope_count; ++i)
        if (!same_scope(c->scopes[i], b->scopes[i]))
            return ref_join_precision(branch, span);
    for (size_t i = 0; i < c->binding_count; ++i)
        if (guard->bindings[i].view.availability !=
            b->bindings[i].view.availability)
            return ref_join_precision(branch, span);
    if (!reference_live(branch, output, span))
        return false;
    for (size_t k = 0; k < nl_sem_ref_count(result); ++k) {
        const NLReferenceFacts f = nl_sem_ref_fact(result, k);
        bool imported = false;
        /* Existing facts require an actual public package origin, unchanged
         * public prefix scope/place incarnation and occurrence identity. IDs
         * that merely happen to have the same number cannot pass this proof. */
        for (size_t i = 0; i < c->value_count && !imported; ++i) {
            const NLSemanticValueView old = c->values[i];
            if (old.carrier == NL_CARRIER_ENDED || old.type != result.type)
                continue;
            for (size_t j = 0; j < nl_sem_ref_count(old); ++j) {
                if (ref_order(f, nl_sem_ref_fact(old, j)) != 0)
                    continue;
                if (f.place == 0 || f.place > c->place_count || f.scope == 0 ||
                    f.scope > c->scope_count || !scope_active(c, f.scope) ||
                    !same_place_frame(c->places[f.place - 1],
                                      b->places[f.place - 1]))
                    continue;
                if (f.occurrence_dependency != 0 &&
                    (f.occurrence_dependency > c->occurrence_count ||
                     !c->occurrences[f.occurrence_dependency - 1].live ||
                     c->places[f.place - 1].parent_sum == 0 ||
                     c->places[c->places[f.place - 1].parent_sum - 1]
                             .payload_occurrence != f.occurrence_dependency))
                    continue;
                if (!ref_alternative(branch, join, f, span))
                    return false;
                imported = true;
                break;
            }
        }
        if (imported)
            continue;
        /* Borrowed pattern derivation is proven structurally against its
         * guard, then rebuilt using PUBLIC root/occurrence/place/parent scope.
         * The arm's short lexical child scope never escapes. */
        if (root == 0 || child_scope == 0 || f.scope != child_scope)
            return ref_join_precision(branch, span);
        const NLOccurrenceId guarded =
            guard->places[root - 1].payload_occurrence;
        if (guarded == 0 || f.occurrence_dependency != guarded ||
            f.place != guard->occurrences[guarded - 1].payload_place ||
            f.incarnation != guard->places[f.place - 1].incarnation ||
            f.provenance != NL_PROVENANCE_VALID ||
            f.readable != parent.readable || f.writable != parent.writable ||
            !scope_active(c, parent.scope))
            return ref_join_precision(branch, span);
        const NLSemanticPlaceView public_root = c->places[root - 1];
        const size_t actual_variant =
            c->values[public_root.current_value - 1].variant;
        if (actual_variant != guard->occurrences[guarded - 1].variant) {
            /* Exact unchanged current variant proves this payload edge
             * unreachable. No hypothetical public occurrence is minted. */
            continue;
        }
        const NLOccurrenceId public_occurrence = public_root.payload_occurrence;
        if (public_occurrence == 0 ||
            !c->occurrences[public_occurrence - 1].live)
            return ref_join_precision(branch, span);
        const NLPlaceId public_child =
            c->occurrences[public_occurrence - 1].payload_place;
        NLReferenceFacts rebased = f;
        rebased.place = public_child;
        rebased.incarnation = c->places[public_child - 1].incarnation;
        rebased.scope = parent.scope;
        rebased.occurrence_dependency = public_occurrence;
        if (!ref_alternative(branch, join, rebased, span))
            return false;
    }
    return true;
}

static NLCheckedNodeId sum_match(Check *check, const NLSyntaxView *s)
{
    if (check->in_match_arm) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span, "P6-NESTED-MATCH",
             "nested match joins are outside this bounded slice");
        return 0;
    }
    NLSemanticContext *c = check->context;
    NLCheckedNodeId scrutinee_node = expression(check, s->data.match.scrutinee);
    if (scrutinee_node == 0)
        return 0;
    NLValueId scrutinee = one_result(check, scrutinee_node);
    if (scrutinee == 0)
        return 0;
    const NLSemanticValueView input = c->values[scrutinee - 1];
    const NLSemanticTypeView input_type = c->types[input.type - 1].view;
    bool borrowed = input_type.kind == NL_TYPE_REF && !input_type.is_exclusive;
    NLTypeId type = borrowed ? input_type.target : input.type;
    if (c->types[type - 1].view.kind != NL_TYPE_SUM) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, s->span, "P6-MATCH-SUM",
             "match requires a sum value or ordinary sum ref; no implicit "
             "borrow/deref");
        return 0;
    }
    if (borrowed && !reference_live(check, scrutinee, s->span))
        return 0;
    if (borrowed && !concrete_ref(check, scrutinee, s->span))
        return 0;
    NLPlaceId root = borrowed ? input.reference.place : 0;
    if (borrowed) {
        /* A live external occurrence ref implies a relational variant fact.
         * P6 cannot erase that fact while hypothesizing another arm. Reject
         * conservatively instead of checking an inconsistent guarded state. */
        for (size_t i = 0; i < c->value_count; ++i) {
            const NLSemanticValueView v = c->values[i];
            if (v.carrier == NL_CARRIER_ENDED ||
                c->types[v.type - 1].view.kind != NL_TYPE_REF)
                continue;
            for (size_t k = 0; k < nl_sem_ref_count(v); ++k) {
                const NLReferenceFacts f = nl_sem_ref_fact(v, k);
                if (f.occurrence_dependency != 0 && scope_active(c, f.scope) &&
                    c->occurrences[f.occurrence_dependency - 1].root == root) {
                    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, s->span,
                         "P6-EXTERNAL-OCCURRENCE",
                         "pre-existing payload ref needs a relational "
                         "variant-aware branch guard");
                    return 0;
                }
            }
        }
    }
    const NLSemanticTypeView sum_type = c->types[type - 1].view;
    bool seen[NL_SEMANTIC_MAX_VARIANTS] = {false};
    size_t variants[NL_SEMANTIC_MAX_VARIANTS] = {0}, arm_count = 0;
    for (const NLSyntaxNode *arm = s->data.match.arms; arm != NULL;
         arm = nl_syntax_next_argument(arm)) {
        const NLSyntaxView *a = nl_syntax_node_view(arm);
        size_t v = sum_variant(check, type, a->data.arm.variant);
        if (v == 0)
            return 0;
        if (seen[v - 1]) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, a->data.arm.variant,
                 "P6-EXHAUSTIVENESS", "duplicate variant arm");
            return 0;
        }
        seen[v - 1] = true;
        variants[arm_count++] = v;
        NLTypeId payload_type = c->types[type - 1].variant_types[v - 1];
        if ((payload_type != 0) != a->data.arm.payload) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, a->data.arm.variant,
                 "P6-PATTERN-SHAPE",
                 "pattern payload shape does not match variant");
            return 0;
        }
        if (!borrowed && a->data.arm.wildcard &&
            !c->types[payload_type - 1].view.is_discardable) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, a->data.arm.binding,
                 "P6-PAYLOAD-DISCARD",
                 "consuming wildcard requires Discardable(payload type)");
            return 0;
        }
    }
    if (arm_count != sum_type.variant_count) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, s->span, "P6-EXHAUSTIVENESS",
             "every variant needs exactly one arm even when current variant is "
             "known");
        return 0;
    }
    if (borrowed) {
        for (size_t i = 0; i < sum_type.variant_count; ++i) {
            const NLTypeId payload = c->types[type - 1].variant_types[i];
            if (payload != 0 &&
                compound(check, NL_TYPE_REF, payload, input_type.access, false,
                         s->span) == 0)
                return 0;
        }
    }
    NLSemanticValueView joined_ref = {0};
    NLCheckedNodeId id =
        add(check, (NLCheckedNodeView){.kind = NL_CHECKED_MATCH,
                                       .span = s->span,
                                       .initializer = scrutinee_node,
                                       .borrowed_match = borrowed,
                                       .item_count = arm_count});
    if (id == 0)
        return 0;
    bool changed[NL_SEMANTIC_MAX_ENTRIES] = {false};
    NLAvailability availability[NL_SEMANTIC_MAX_ENTRIES] = {0};
    NLTypeId result_type = 0;
    size_t result_count = 0, index = 0;
    bool identity_result = !borrowed;
    bool whole_write = false, payload_write = false;
    size_t incoming_variant = 0;
    for (const NLSyntaxNode *arm = s->data.match.arms; arm != NULL;
         arm = nl_syntax_next_argument(arm), ++index) {
        const NLSyntaxView *a = nl_syntax_node_view(arm);
        const size_t variant = variants[index];
        const NLTypeId payload_type =
            c->types[type - 1].variant_types[variant - 1];
        Check branch = {.source = check->source,
                        .depth = check->depth,
                        .binding_floor = c->binding_count,
                        .arm_floor = c->binding_count,
                        .has_arm_floor = true,
                        .in_match_arm = true};
        NLSemanticContext *guard = NULL;
        if (!host(&branch, nl_sem_clone(c, &branch.context), a->span))
            goto arm_failure;
        branch.artifact = malloc(sizeof(*branch.artifact));
        if (branch.artifact == NULL) {
            (void)host(&branch, NL_CHECK_OUT_OF_MEMORY, a->span);
            goto arm_failure;
        }
        *branch.artifact =
            (NLCheckedFragment){.source = check->source,
                                .context = branch.context,
                                .destroy_context = nl_semantic_destroy};
        NLSemanticContext *b = branch.context;
        NLValueId current_sum =
            borrowed ? b->places[root - 1].current_value : scrutinee;
        NLValueId payload = b->values[current_sum - 1].sum_payload;
        /* A static arm guard refines only this owned hypothetical context.
         * Unknown authority packages cannot be used by source raw operations or
         * registered authority calls; no hypothetical claim is imported. */
        if (b->values[current_sum - 1].variant != variant) {
            if (borrowed)
                nl_sum_detach(b, root);
            nl_sem_end_value(b, current_sum);
            payload =
                payload_type == 0
                    ? 0
                    : new_value(&branch,
                                (NLSemanticValueView){.type = payload_type},
                                a->span);
            if (branch.status != NL_CHECK_OK)
                goto arm_failure;
            NLValueId hypothetical = new_value(
                &branch,
                (NLSemanticValueView){
                    .type = type, .variant = variant, .sum_payload = payload},
                a->span);
            if (hypothetical == 0)
                goto arm_failure;
            current_sum = hypothetical;
            if (payload != 0) {
                b->values[payload - 1].carrier = NL_CARRIER_SUM;
                b->values[payload - 1].sum_owner = current_sum;
            }
            if (borrowed) {
                b->places[root - 1].current_value = current_sum;
                b->values[current_sum - 1].carrier = NL_CARRIER_PLACE;
                b->values[current_sum - 1].owner_place = root;
                if (!host(&branch, nl_sum_attach(b, root), a->span))
                    goto arm_failure;
                refresh_bindings(&branch, root);
            }
        }
        NLSymbolId pattern_symbol = 0;
        NLScopeId payload_scope = 0;
        if (payload_type != 0 && !a->data.arm.wildcard) {
            NLValueId binding_value = payload;
            if (borrowed) {
                const NLOccurrenceId occurrence =
                    b->places[root - 1].payload_occurrence;
                const NLPlaceId child =
                    b->occurrences[occurrence - 1].payload_place;
                if (!host(&branch,
                          nl_sem_new_scope(b, input.reference.scope, true,
                                           &payload_scope),
                          a->span))
                    goto arm_failure;
                const NLTypeId ref_type =
                    compound(&branch, NL_TYPE_REF, payload_type,
                             input_type.access, false, a->span);
                if (ref_type == 0)
                    goto arm_failure;
                binding_value = new_value(
                    &branch,
                    (NLSemanticValueView){
                        .type = ref_type,
                        .reference = {.place = child,
                                      .incarnation =
                                          b->places[child - 1].incarnation,
                                      .scope = payload_scope,
                                      .provenance = NL_PROVENANCE_VALID,
                                      .readable = input.reference.readable,
                                      .writable = input.reference.writable,
                                      .occurrence_dependency = occurrence}},
                    a->span);
                if (binding_value == 0)
                    goto arm_failure;
            } else {
                b->values[payload - 1].carrier = NL_CARRIER_LOOSE;
                b->values[payload - 1].sum_owner = 0;
            }
            NLSourceView name;
            (void)nl_source_view(check->source, a->data.arm.binding, &name);
            char *copy = malloc(name.length + 1);
            if (copy == NULL) {
                (void)host(&branch, NL_CHECK_OUT_OF_MEMORY, a->span);
                goto arm_failure;
            }
            memcpy(copy, name.bytes, name.length);
            copy[name.length] = 0;
            NLCheckStatus status = nl_sem_bind_in_scope(
                b, copy, binding_value, branch.binding_floor, &pattern_symbol);
            free(copy);
            if (!host(&branch, status, a->span))
                goto arm_failure;
        } else if (!borrowed && payload != 0)
            nl_sem_end_value(b, payload);
        if (!borrowed) {
            b->values[current_sum - 1].carrier = NL_CARRIER_ENDED;
            b->values[current_sum - 1].sum_payload = 0;
        }
        if (!host(&branch, nl_sem_clone(b, &guard), a->span))
            goto arm_failure;
        NLCheckedNodeId body = expression(&branch, a->data.arm.body);
        if (body == 0)
            goto arm_failure;
        const NLCheckedNodeView body_view = *view(&branch, body);
        if (body_view.result_count > 1) {
            match_precision(&branch, a->span);
            goto arm_failure;
        }
        NLTypeId branch_type =
            body_view.result_count == 0 ? 1 : body_view.results[0].type;
        if (index == 0) {
            result_type = branch_type;
            result_count = body_view.result_count;
            for (size_t j = 0; j < c->binding_count; ++j)
                availability[j] = b->bindings[j].view.availability;
        } else {
            if (branch_type != result_type ||
                body_view.result_count != result_count) {
                fail(&branch, NL_CHECK_SEMANTIC_ERROR, a->span,
                     "P6-RESULT-JOIN",
                     "normal match arms need exactly matching result types");
                goto arm_failure;
            }
            for (size_t j = 0; j < c->binding_count; ++j)
                if (!c->types[c->bindings[j].view.type - 1].view.is_copy &&
                    availability[j] != b->bindings[j].view.availability) {
                    fail(&branch, NL_CHECK_SEMANTIC_ERROR, a->span,
                         "P6-AVAILABILITY-JOIN",
                         "outer non-Copy availability differs between normal "
                         "arms");
                    goto arm_failure;
                }
        }
        bool identity = false;
        if (!borrowed && result_count == 1 && branch_type == type) {
            const NLSemanticValueView output =
                b->values[body_view.results[0].value - 1];
            identity = output.variant == variant &&
                       (payload_type == 0 || output.sum_payload == payload);
            if (!identity && output.variant == variant && payload_type != 0 &&
                flat_copy(b, payload_type)) {
                const NLCheckedNodeView *tail =
                    nl_checked_node_view(branch.artifact, body_view.tail);
                if (tail != NULL && tail->kind == NL_CHECKED_SUM_CONSTRUCTOR) {
                    const NLCheckedNodeView *arg = nl_checked_node_view(
                        branch.artifact, tail->initializer);
                    identity = arg != NULL &&
                               arg->kind == NL_CHECKED_IDENTIFIER &&
                               arg->symbol == pattern_symbol;
                }
            }
        }
        identity_result = identity_result && identity;
        if (result_count != 0 &&
            b->types[branch_type - 1].view.kind == NL_TYPE_REF) {
            if (!rebase_ref_result(
                    check, &branch, guard, body_view.results[0].value, root,
                    payload_scope, input.reference, &joined_ref, a->span))
                goto arm_failure;
        }
        if (!arm_frame(&branch, guard, b, root, c->place_count, c->scope_count,
                       changed, a->span))
            goto arm_failure;
        if (borrowed) {
            const NLSemanticPlaceView g = guard->places[root - 1],
                                      post = b->places[root - 1];
            if (!same_place_frame(g, post)) {
                match_precision(&branch, a->span);
                goto arm_failure;
            }
            bool whole =
                post.payload_occurrence != g.payload_occurrence ||
                (payload_type == 0 && post.current_fact != g.current_fact);
            if (whole) {
                size_t next_variant = b->values[post.current_value - 1].variant;
                if (!sum_type.is_copy ||
                    (index != 0 &&
                     (!whole_write || incoming_variant != next_variant))) {
                    match_precision(&branch, a->span);
                    goto arm_failure;
                }
                if (index == 0)
                    incoming_variant = next_variant;
            } else if (whole_write) {
                match_precision(&branch, a->span);
                goto arm_failure;
            }
            if (index != 0 && whole != whole_write) {
                match_precision(&branch, a->span);
                goto arm_failure;
            }
            whole_write = whole;
            if (!whole && payload_type != 0) {
                NLPlaceId child =
                    guard->occurrences[g.payload_occurrence - 1].payload_place;
                if (!same_place_frame(guard->places[child - 1],
                                      b->places[child - 1])) {
                    match_precision(&branch, a->span);
                    goto arm_failure;
                }
                if (guard->places[child - 1].current_fact !=
                    b->places[child - 1].current_fact) {
                    if (!flat_copy(b, payload_type)) {
                        match_precision(&branch, a->span);
                        goto arm_failure;
                    }
                    payload_write = true;
                }
            }
        }
        if (payload_scope != 0)
            b->scopes[payload_scope - 1].active = false;
        branch.artifact->root = add(
            &branch, (NLCheckedNodeView){.kind = NL_CHECKED_MATCH_ARM,
                                         .span = a->span,
                                         .name = a->data.arm.variant,
                                         .variant = variant,
                                         .symbol = pattern_symbol,
                                         .initializer = body,
                                         .type = branch_type,
                                         .result_count = body_view.result_count,
                                         .results = {body_view.results[0]}});
        if (branch.artifact->root == 0 ||
            !save_arm(check, id, branch.artifact, a->span))
            goto arm_failure;
        nl_semantic_destroy(guard);
        continue;
    arm_failure:
        nl_semantic_destroy(guard);
        if (branch.status != NL_CHECK_OK) {
            check->status = branch.status;
            check->diagnostic = branch.diagnostic;
        }
        if (branch.artifact != NULL)
            nl_checked_destroy(branch.artifact);
        else
            nl_semantic_destroy(branch.context);
        return 0;
    }
    /* A compound type created only inside an arm is not a public identity,
     * even if a second clone happens to allocate the same numeric TypeId. */
    if (result_type > c->type_count) {
        match_precision(check, s->span);
        return 0;
    }
    const bool ref_result =
        result_count != 0 && c->types[result_type - 1].view.kind == NL_TYPE_REF;
    if (ref_result && joined_ref.reference_count == 0) {
        (void)ref_join_precision(check, s->span);
        return 0;
    }
    if (result_count != 0 && !flat_copy(c, result_type) && !identity_result &&
        !ref_result) {
        match_precision(check, s->span);
        return 0;
    }
    if (!borrowed && nl_sum_authority(c, type) && !identity_result) {
        match_precision(check, s->span);
        return 0;
    }
    /* Construct a common state from proven effects. No branch context commits.
     */
    for (size_t j = 0; j < c->binding_count; ++j) {
        const NLSemanticBindingView v = c->bindings[j].view;
        if (v.availability == NL_AVAILABLE && availability[j] == NL_CONSUMED) {
            if (nl_sum_authority(c, v.type)) {
                match_precision(check, s->span);
                return 0;
            }
            NLCheckedNodeId used = binding_use(check, j + 1, s->span, s->span);
            if (used == 0)
                return 0;
            nl_sem_end_value(c, view(check, used)->results[0].value);
        }
    }
    for (size_t j = 0; j < c->place_count; ++j) {
        if (!changed[j])
            continue;
        const NLSemanticPlaceView old = c->places[j];
        NLValueId value =
            new_value(check, (NLSemanticValueView){.type = old.type}, s->span);
        if (value == 0 ||
            !host(check, nl_sem_fresh_fact(c, &c->places[j].current_fact),
                  s->span))
            return 0;
        nl_sem_end_value(c, old.current_value);
        c->places[j].current_value = value;
        c->values[value - 1].carrier = NL_CARRIER_PLACE;
        c->values[value - 1].owner_place = j + 1;
        refresh_bindings(check, j + 1);
    }
    if (borrowed && whole_write) {
        NLTypeId ptype = c->types[type - 1].variant_types[incoming_variant - 1];
        if (ptype != 0 && !flat_copy(c, ptype)) {
            match_precision(check, s->span);
            return 0;
        }
        NLValueId p =
            ptype == 0 ? 0
                       : new_value(check, (NLSemanticValueView){.type = ptype},
                                   s->span);
        if (check->status != NL_CHECK_OK)
            return 0;
        NLValueId next = new_value(
            check,
            (NLSemanticValueView){
                .type = type, .variant = incoming_variant, .sum_payload = p},
            s->span);
        if (next == 0 ||
            conflicts(check, root, false, false, scrutinee, s->span))
            return 0;
        NLValueId old = c->places[root - 1].current_value;
        nl_sum_detach(c, root);
        nl_sem_end_value(c, old);
        c->places[root - 1].current_value = next;
        c->values[next - 1].carrier = NL_CARRIER_PLACE;
        c->values[next - 1].owner_place = root;
        if (!host(check,
                  nl_sem_fresh_fact(c, &c->places[root - 1].current_fact),
                  s->span) ||
            !host(check, nl_sum_attach(c, root), s->span))
            return 0;
        refresh_bindings(check, root);
    } else if (borrowed && payload_write &&
               c->places[root - 1].payload_occurrence != 0) {
        NLPlaceId child =
            c->occurrences[c->places[root - 1].payload_occurrence - 1]
                .payload_place;
        NLValueId old = c->places[child - 1].current_value;
        NLValueId next = new_value(
            check, (NLSemanticValueView){.type = c->values[old - 1].type},
            s->span);
        if (next == 0)
            return 0;
        nl_sem_end_value(c, old);
        c->places[child - 1].current_value = next;
        c->values[next - 1].carrier = NL_CARRIER_PLACE;
        c->values[next - 1].owner_place = child;
        if (!host(check,
                  nl_sem_fresh_fact(c, &c->places[child - 1].current_fact),
                  s->span) ||
            !host(check, nl_sum_payload_changed(c, child), s->span))
            return 0;
        refresh_bindings(check, root);
    }
    NLValueId output = 0;
    if (identity_result) {
        output = new_value(check, input, s->span);
        if (output == 0)
            return 0;
        c->values[scrutinee - 1].carrier = NL_CARRIER_ENDED;
        if (input.sum_payload != 0) {
            c->values[input.sum_payload - 1].carrier = NL_CARRIER_SUM;
            c->values[input.sum_payload - 1].sum_owner = output;
        }
    } else {
        nl_sem_end_value(c, scrutinee);
        if (result_count != 0) {
            joined_ref.type = result_type;
            output = new_value(check,
                               ref_result
                                   ? joined_ref
                                   : (NLSemanticValueView){.type = result_type},
                               s->span);
        }
        if (check->status != NL_CHECK_OK)
            return 0;
    }
    view(check, id)->type = result_type;
    view(check, id)->result_count = result_count;
    if (result_count != 0)
        view(check, id)->results[0] = (NLCheckedResult){result_type, output};
    return id;
}

static NLCheckedNodeId loan(Check *check, const NLSyntaxView *syntax)
{
    NLSemanticContext *const c = check->context;
    const NLSourceSpan source_span =
        nl_syntax_node_view(syntax->data.loan.source)->span;
    const NLSymbolId source = available(check, source_span);
    if (source == 0) {
        return 0;
    }
    const NLSemanticBindingView binding_view = c->bindings[source - 1].view;
    const NLSemanticValueView source_value = c->values[binding_view.value - 1];
    const NLSemanticTypeView source_type = c->types[source_value.type - 1].view;
    NLPlaceId place = binding_view.place;
    NLDomainId domain = 0;
    NLSymbolId stability = 0;
    NLScopeId parent = 0;
    NLTypeId target = binding_view.type;
    bool weakened = false;
    if (source_type.kind == NL_TYPE_PTR) {
        if (syntax->data.loan.is_exclusive &&
            syntax->data.loan.access == NL_ACCESS_WRITE) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, source_span,
                 "P3-PTR-EXCLUSIVE-WRITE-UNSUPPORTED",
                 "pointer exclusive-write acquisition needs additional "
                 "adjudicated evidence");
            return 0;
        }
        if (syntax->data.loan.stability == NULL) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, source_span,
                 "P3-STABILITY-REQUIRED",
                 "ptr loan requires stability evidence for governing domain");
            return 0;
        }
        if (!reference_live(check, binding_view.value, source_span)) {
            return 0;
        }
        if (c->places[source_value.reference.place - 1].parent_sum != 0) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, source_span,
                 "P6-CONDITIONAL-LOAN-UNSUPPORTED",
                 "live payload ptr-to-ref acquisition needs an "
                 "occurrence-aware body plan");
            return 0;
        }
        const NLSourceSpan stable_span =
            nl_syntax_node_view(syntax->data.loan.stability)->span;
        stability = available(check, stable_span);
        if (stability == 0) {
            return 0;
        }
        const NLSemanticBindingView stable_binding =
            c->bindings[stability - 1].view;
        if (c->types[stable_binding.type - 1].view.is_exclusive) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, stable_span,
                 "P3-EXCLUSIVE-STABILITY-UNSUPPORTED",
                 "exclusive loan stability requires a body-extent reborrow "
                 "plan outside the P3 header subset");
            return 0;
        }
        domain =
            domain_reference(check, stable_binding.value, false, stable_span);
        if (domain == 0) {
            return 0;
        }
        place = source_value.reference.place;
        target = source_type.target;
        if (c->places[place - 1].governing_domain != domain) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, stable_span,
                 "P3-DOMAIN-MISMATCH",
                 "ptr loan stability refers to wrong governing domain");
            return 0;
        }
        if (!source_value.reference.readable ||
            (syntax->data.loan.access == NL_ACCESS_WRITE &&
             !source_value.reference.writable)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, source_span, "P3-ACCESS",
                 "ptr access evidence does not permit requested mode");
            return 0;
        }
        parent = c->values[stable_binding.value - 1].reference.scope;
        weakened =
            c->types[stable_binding.type - 1].view.access == NL_ACCESS_WRITE;
    } else {
        if (syntax->data.loan.stability != NULL) {
            fail(check, NL_CHECK_SEMANTIC_ERROR,
                 nl_syntax_node_view(syntax->data.loan.stability)->span,
                 "P3-LOCAL-USING-FORBIDDEN",
                 "local-source canonical loan does not accept using");
            return 0;
        }
        if (source_type.kind == NL_TYPE_REF ||
            source_type.kind == NL_TYPE_SLOT) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, source_span,
                 "P3-LOCAL-AUTHORITY-LOAN-UNSUPPORTED",
                 "nested capability/occupancy local loans are outside P3");
            return 0;
        }
        if (source_value.domain != 0) {
            domain = source_value.domain;
        }
    }
    if (conflicts(check, place, syntax->data.loan.is_exclusive, false, 0,
                  source_span)) {
        return 0;
    }
    const NLTypeId ref_type =
        compound(check, NL_TYPE_REF, target, syntax->data.loan.access,
                 syntax->data.loan.is_exclusive, syntax->span);
    if (ref_type == 0) {
        return 0;
    }
    NLScopeId scope;
    if (!host(check, nl_sem_new_scope(c, parent, false, &scope),
              syntax->span)) {
        return 0;
    }
    return add(check,
               (NLCheckedNodeView){
                   .kind = NL_CHECKED_LOAN_HEADER,
                   .span = syntax->span,
                   .name = syntax->data.loan.binding,
                   .type = ref_type,
                   .loan = {.source = source,
                            .stability = stability,
                            .binding = syntax->data.loan.binding,
                            .body_open = syntax->data.loan.body_open,
                            .body_interior = syntax->data.loan.body_interior,
                            .body_close = syntax->data.loan.body_close,
                            .access = syntax->data.loan.access,
                            .is_exclusive = syntax->data.loan.is_exclusive,
                            .stability_weakened = weakened,
                            .place = place,
                            .incarnation = c->places[place - 1].incarnation,
                            .domain = domain,
                            .scope = scope,
                            .dependency_scope = parent,
                            .prevent_lifetime_end = true,
                            .prevent_conflicting_access =
                                syntax->data.loan.is_exclusive,
                            .body_nonescape_proved = false}});
}

typedef enum {
    CHECK_TYPE,
    CHECK_EXPRESSION,
    CHECK_BINDING,
    CHECK_LOAN,
    CHECK_SOURCE
} Entry;

static NLCheckStatus check_fragment(NLSemanticContext *context,
                                    const NLSyntaxTree *tree,
                                    NLCheckedFragment **out,
                                    NLCheckDiagnostic *diagnostic, Entry entry)
{
    if (context == NULL || tree == NULL || out == NULL || *out != NULL) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    const NLSyntaxView *const root =
        nl_syntax_node_view(nl_syntax_tree_root(tree));
    if (root == NULL ||
        (entry == CHECK_TYPE && root->kind != NL_SYNTAX_TYPE_NAME &&
         root->kind != NL_SYNTAX_TYPE_PTR &&
         root->kind != NL_SYNTAX_TYPE_REF) ||
        (entry == CHECK_EXPRESSION && root->kind != NL_SYNTAX_EXPR_NAME &&
         root->kind != NL_SYNTAX_EXPR_CALL) ||
        (entry == CHECK_BINDING && root->kind != NL_SYNTAX_BINDING) ||
        (entry == CHECK_SOURCE && root->kind != NL_SYNTAX_EXPR_NAME &&
         root->kind != NL_SYNTAX_EXPR_CALL && root->kind != NL_SYNTAX_BINDING &&
         root->kind != NL_SYNTAX_MULTI_BINDING &&
         root->kind != NL_SYNTAX_AGGREGATE_BINDING &&
         root->kind != NL_SYNTAX_AGGREGATE &&
         root->kind != NL_SYNTAX_SUM_CONSTRUCTOR &&
         root->kind != NL_SYNTAX_MATCH && root->kind != NL_SYNTAX_BLOCK &&
         root->kind != NL_SYNTAX_STATEMENT) ||
        (entry == CHECK_LOAN && root->kind != NL_SYNTAX_LOAN)) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    Check check = {.source = nl_syntax_tree_source(tree)};
    if (!host(&check, nl_sem_clone(context, &check.context), root->span)) {
        goto failure;
    }
    check.artifact = malloc(sizeof(*check.artifact));
    if (check.artifact == NULL) {
        (void)host(&check, NL_CHECK_OUT_OF_MEMORY, root->span);
        goto failure;
    }
    *check.artifact =
        (NLCheckedFragment){.source = check.source, .context = context};
    for (size_t i = 0; i < check.context->value_count; ++i) {
        if (check.context->values[i].carrier != NL_CARRIER_ENDED &&
            check.context->values[i].dependencies != NL_DEPENDENCY_FREE) {
            fail(&check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, root->span,
                 "P3-DEPENDENCIES-UNSUPPORTED",
                 "hidden semantic dependency checking is outside P3 supported "
                 "slice");
            goto failure;
        }
    }
    if (entry == CHECK_TYPE) {
        const NLTypeId type = check_type(&check, nl_syntax_tree_root(tree));
        if (type != 0) {
            check.artifact->root =
                add(&check, (NLCheckedNodeView){.kind = NL_CHECKED_TYPE,
                                                .span = root->span,
                                                .type = type});
        }
    } else if (entry == CHECK_EXPRESSION) {
        check.artifact->root = expression(&check, nl_syntax_tree_root(tree));
    } else if (entry == CHECK_BINDING) {
        check.artifact->root = binding(&check, root);
    } else if (entry == CHECK_SOURCE) {
        if (root->kind == NL_SYNTAX_BINDING ||
            root->kind == NL_SYNTAX_MULTI_BINDING ||
            root->kind == NL_SYNTAX_AGGREGATE_BINDING)
            check.artifact->root = source_binding(&check, root);
        else if (root->kind == NL_SYNTAX_STATEMENT)
            check.artifact->root = source_statement(&check, root);
        else
            check.artifact->root =
                expression(&check, nl_syntax_tree_root(tree));
    } else {
        check.artifact->root = loan(&check, root);
    }
    if (check.status != NL_CHECK_OK) {
        goto failure;
    }
    if (check.artifact->root == 0) {
        fail(&check, NL_CHECK_INTERNAL_ERROR, root->span, "P3-INTERNAL",
             "semantic checker did not produce an artifact");
        goto failure;
    }
    if (!host(&check, nl_sum_validate(check.context), root->span) ||
        !host(&check, nl_raw_validate(check.context), root->span)) {
        goto failure;
    }
    nl_sem_commit(context, check.context);
    *out = check.artifact;
    return NL_CHECK_OK;
failure:
    nl_semantic_destroy(check.context);
    nl_checked_destroy(check.artifact);
    if (diagnostic != NULL) {
        *diagnostic = check.diagnostic;
    }
    return check.status;
}

NLCheckStatus nl_semantic_check_type(NLSemanticContext *c,
                                     const NLSyntaxTree *t,
                                     NLCheckedFragment **out,
                                     NLCheckDiagnostic *d)
{
    return check_fragment(c, t, out, d, CHECK_TYPE);
}
NLCheckStatus nl_semantic_check_expression(NLSemanticContext *c,
                                           const NLSyntaxTree *t,
                                           NLCheckedFragment **out,
                                           NLCheckDiagnostic *d)
{
    return check_fragment(c, t, out, d, CHECK_EXPRESSION);
}
NLCheckStatus nl_semantic_check_binding(NLSemanticContext *c,
                                        const NLSyntaxTree *t,
                                        NLCheckedFragment **out,
                                        NLCheckDiagnostic *d)
{
    return check_fragment(c, t, out, d, CHECK_BINDING);
}
NLCheckStatus nl_semantic_check_loan_header(NLSemanticContext *c,
                                            const NLSyntaxTree *t,
                                            NLCheckedFragment **out,
                                            NLCheckDiagnostic *d)
{
    return check_fragment(c, t, out, d, CHECK_LOAN);
}

NLCheckStatus nl_semantic_check_source_fragment(NLSemanticContext *c,
                                                const NLSyntaxTree *t,
                                                NLCheckedFragment **out,
                                                NLCheckDiagnostic *d)
{
    return check_fragment(c, t, out, d, CHECK_SOURCE);
}

bool nl_check_diagnostic_render(FILE *stream, const NLSource *source,
                                const NLCheckDiagnostic *diagnostic)
{
    if (diagnostic == NULL || !nl_source_span_valid(source, diagnostic->span)) {
        return false;
    }
    NLSourceView bytes;
    if (!nl_source_view(source, (NLSourceSpan){0, diagnostic->span.end_byte},
                        &bytes)) {
        return false;
    }
    NLSourceRange range = {nl_source_name(source), 1, 1, 1, 1};
    size_t line = 1, column = 1;
    for (size_t i = 0; i <= bytes.length; ++i) {
        if (i == diagnostic->span.start_byte) {
            range.start_line = line;
            range.start_column = column;
        }
        if (i == diagnostic->span.end_byte) {
            range.end_line = line;
            range.end_column = column;
            break;
        }
        if (bytes.bytes[i] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
    }
    NLDiagnostic rendered = diagnostic->diagnostic;
    rendered.range = &range;
    return nl_diagnostic_render(stream, &rendered);
}

static size_t raw_operand_count(NLRawOperationKind kind)
{
    if (kind == NL_RAW_ALLOCATE || kind == NL_RAW_BYTE_TO_U8 ||
        kind == NL_RAW_U8_TO_BYTE) {
        return 0;
    }
    return kind == NL_RAW_DEALLOCATE || kind == NL_RAW_MERGE ||
                   kind == NL_RAW_COPY_BYTES
               ? 2
               : 1;
}

static NLCheckedNodeId raw_argument(Check *check, NLRawOperand operand,
                                    NLTypeId expected, NLValueId prior)
{
    NLSemanticContext *const c = check->context;
    NLCheckedNodeId id;
    if (operand.binding != 0) {
        id = binding_argument(check, operand.binding, expected, operand.span,
                              operand.span);
    } else {
        const NLSemanticValueView original = c->values[operand.loose - 1];
        const NLSemanticTypeView type = c->types[original.type - 1].view;
        if (original.carrier != NL_CARRIER_LOOSE ||
            (!type.is_copy && operand.loose == prior)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, operand.span,
                 "P4-LOOSE-RESPONSIBILITY",
                 "operand is not a unique loose value responsibility");
            return 0;
        }
        if (type.kind == NL_TYPE_REF &&
            !reference_live(check, operand.loose, operand.span)) {
            return 0;
        }
        if (type.kind == NL_TYPE_REF && type.is_exclusive &&
            expected != original.type) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, operand.span,
                 "P4-LOOSE-EXCLUSIVE-UNSUPPORTED",
                 "exclusive contextual reborrow requires a binding");
            return 0;
        }
        NLValueId value = operand.loose;
        if (type.is_copy &&
            !host(check, nl_sem_new_value(c, original, &value), operand.span)) {
            return 0;
        }
        id = add(check, (NLCheckedNodeView){
                            .kind = NL_CHECKED_IDENTIFIER,
                            .span = operand.span,
                            .type = original.type,
                            .value_use = type.is_copy ? NL_VALUE_COPIED
                                                      : NL_VALUE_CONSUMED,
                            .result_count = 1,
                            .results = {{original.type, value}}});
    }
    return parameter_match(check, id, expected);
}

NLCheckStatus nl_semantic_check_raw_operation(NLSemanticContext *context,
                                              const NLRawOperation *operation,
                                              NLCheckedFragment **out,
                                              NLCheckDiagnostic *diagnostic)
{
    static const NLCheckedKind kinds[] = {NL_CHECKED_ALLOCATE,
                                          NL_CHECKED_DEALLOCATE,
                                          NL_CHECKED_SPLIT,
                                          NL_CHECKED_MERGE,
                                          NL_CHECKED_INTO_SLOT,
                                          NL_CHECKED_ERASE_SLOT,
                                          NL_CHECKED_STORAGE_LEN,
                                          NL_CHECKED_STORAGE_ADDR,
                                          NL_CHECKED_STORAGE_READ_BYTE,
                                          NL_CHECKED_STORAGE_WRITE_BYTE,
                                          NL_CHECKED_COPY_RAW_BYTES,
                                          NL_CHECKED_BYTE_TO_U8,
                                          NL_CHECKED_U8_TO_BYTE};
    if (context == NULL || operation == NULL || out == NULL || *out != NULL ||
        (unsigned)operation->kind >= sizeof(kinds) / sizeof(kinds[0]) ||
        (operation->source != NULL &&
         !nl_source_span_valid(operation->source, operation->span))) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    const size_t count = raw_operand_count(operation->kind);
    for (size_t i = 0; i < count; ++i) {
        const NLRawOperand operand = operation->operands[i];
        if ((operand.binding == 0) == (operand.loose == 0) ||
            operand.binding > context->binding_count ||
            operand.loose > context->value_count ||
            (operation->source != NULL &&
             !nl_source_span_valid(operation->source, operand.span))) {
            return NL_CHECK_INTERNAL_ERROR;
        }
    }
    Check check = {.source = operation->source};
    if (!host(&check, nl_sem_clone(context, &check.context), operation->span)) {
        goto failure;
    }
    check.artifact = malloc(sizeof(*check.artifact));
    if (check.artifact == NULL) {
        (void)host(&check, NL_CHECK_OUT_OF_MEMORY, operation->span);
        goto failure;
    }
    *check.artifact =
        (NLCheckedFragment){.source = check.source, .context = context};
    NLSemanticContext *const c = check.context;
    for (size_t i = 0; i < c->value_count; ++i) {
        if (c->values[i].carrier != NL_CARRIER_ENDED &&
            c->values[i].dependencies != NL_DEPENDENCY_FREE) {
            fail(
                &check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, operation->span,
                "P3-DEPENDENCIES-UNSUPPORTED",
                "hidden semantic dependencies are outside the supported slice");
            goto failure;
        }
    }
    check.artifact->root =
        add(&check, (NLCheckedNodeView){.kind = kinds[operation->kind],
                                        .span = operation->span});
    if (check.artifact->root == 0) {
        goto failure;
    }
    NLCheckedNodeId args[2] = {0};
    NLValueId values[2] = {0};
    const NLTypeId storage = nl_semantic_core_type(c, NL_TYPE_STORAGE);
    for (size_t i = 0; i < count; ++i) {
        NLTypeId expected = storage;
        if (operation->kind == NL_RAW_DEALLOCATE && i == 0) {
            expected = nl_semantic_core_type(c, NL_TYPE_ALLOCATION);
        } else if (operation->kind == NL_RAW_ERASE_SLOT) {
            expected = 0;
        } else if (operation->kind >= NL_RAW_STORAGE_LEN &&
                   operation->kind <= NL_RAW_COPY_BYTES) {
            expected = compound(&check, NL_TYPE_REF, storage, NL_ACCESS_READ,
                                false, operation->operands[i].span);
        }
        if (check.status != NL_CHECK_OK) {
            goto failure;
        }
        args[i] =
            raw_argument(&check, operation->operands[i], expected, values[0]);
        if (args[i] == 0) {
            goto failure;
        }
        values[i] = one_result(&check, args[i]);
        if (values[i] == 0) {
            goto failure;
        }
        const NLSemanticTypeView actual =
            c->types[c->values[values[i] - 1].type - 1].view;
        if (actual.kind == NL_TYPE_REF &&
            (!reference_live(&check, values[i], operation->operands[i].span) ||
             !concrete_ref(&check, values[i], operation->operands[i].span) ||
             conflicts(&check, c->values[values[i] - 1].reference.place, false,
                       false, values[i], operation->operands[i].span))) {
            goto failure;
        }
        if (i == 0) {
            view(&check, check.artifact->root)->first_argument = args[i];
        } else {
            view(&check, args[i - 1])->next_argument = args[i];
        }
        ++view(&check, check.artifact->root)->argument_count;
    }
    check.status =
        nl_raw_apply(c, operation, values, view(&check, check.artifact->root),
                     &check.diagnostic);
    if (check.status != NL_CHECK_OK) {
        goto failure;
    }
    for (size_t i = 0; i < count; ++i) {
        if (view(&check, args[i])->reborrow_scope != 0) {
            c->scopes[view(&check, args[i])->reborrow_scope - 1].active = false;
        }
        end_temporary(&check, values[i]);
    }
    if (!host(&check, nl_sum_validate(c), operation->span) ||
        !host(&check, nl_raw_validate(c), operation->span)) {
        goto failure;
    }
    nl_sem_commit(context, c);
    *out = check.artifact;
    return NL_CHECK_OK;
failure:
    nl_semantic_destroy(check.context);
    nl_checked_destroy(check.artifact);
    if (diagnostic != NULL) {
        *diagnostic = check.diagnostic;
    }
    return check.status;
}

NLCheckStatus nl_semantic_join_references(NLSemanticContext *context,
                                          const NLValueId *values, size_t count,
                                          NLValueId *out)
{
    if (context == NULL || values == NULL || count == 0 || out == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (count > NL_SEMANTIC_MAX_ENTRIES)
        return NL_CHECK_RESOURCE_LIMIT;
    for (size_t i = 0; i < count; ++i)
        if (values[i] == 0 || values[i] > context->value_count)
            return NL_CHECK_INTERNAL_ERROR;
    Check check = {0};
    check.status = nl_sem_clone(context, &check.context);
    if (check.status != NL_CHECK_OK)
        return check.status;
    NLSemanticValueView join = {.type = context->values[values[0] - 1].type};
    if (!join_ref_type(context, join.type)) {
        check.status = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
        goto done;
    }
    for (size_t i = 0; i < count; ++i) {
        const NLSemanticValueView v = check.context->values[values[i] - 1];
        if (v.carrier == NL_CARRIER_ENDED || v.type != join.type) {
            check.status = NL_CHECK_SEMANTIC_ERROR;
            goto done;
        }
        if (!reference_live(&check, values[i], (NLSourceSpan){0}))
            goto done;
        for (size_t j = 0; j < nl_sem_ref_count(v); ++j)
            if (!ref_alternative(&check, &join, nl_sem_ref_fact(v, j),
                                 (NLSourceSpan){0}))
                goto done;
    }
    NLValueId result;
    check.status = nl_sem_new_value(check.context, join, &result);
    if (check.status == NL_CHECK_OK)
        check.status = nl_sum_validate(check.context);
    if (check.status == NL_CHECK_OK)
        check.status = nl_raw_validate(check.context);
    if (check.status == NL_CHECK_OK) {
        nl_sem_commit(context, check.context);
        check.context = NULL;
        *out = result;
    }
done:
    nl_semantic_destroy(check.context);
    return check.status;
}

static bool body_plain_type(const NLSemanticContext *c, NLTypeId type)
{
    const NLSemanticTypeView t = c->types[type - 1].view;
    return t.field_count == 0 && type != 2 &&
           (t.kind == NL_TYPE_NOMINAL || t.kind == NL_TYPE_UNIT ||
            t.kind == NL_TYPE_BYTE || t.kind == NL_TYPE_U8 ||
            t.kind == NL_TYPE_USIZE || t.kind == NL_TYPE_ADDR);
}
static bool body_signature_type(const NLSemanticContext *c, NLTypeId type,
                                bool parameter)
{
    if (body_plain_type(c, type))
        return true;
    const NLSemanticTypeView t = c->types[type - 1].view;
    return ((t.kind == NL_TYPE_PTR) ||
            (parameter && t.kind == NL_TYPE_REF && !t.is_exclusive)) &&
           body_plain_type(c, t.target);
}

/* Definition checking must not depend on any caller's runtime/fixture state. */
static void clear_definition_state(NLSemanticContext *c)
{
    for (size_t i = 0; i < c->binding_count; ++i)
        free(c->bindings[i].name);
    free(c->bindings);
    free(c->values);
    free(c->places);
    free(c->domains);
    free(c->scopes);
    free(c->occurrences);
    nl_raw_dispose(c);
    NLTypeEntry *types = c->types;
    NLFunctionEntry *functions = c->functions;
    const size_t type_count = c->type_count, function_count = c->function_count;
    *c = (NLSemanticContext){.types = types,
                             .type_count = type_count,
                             .functions = functions,
                             .function_count = function_count};
}

static bool check_definition(Check *registration, size_t function_id)
{
    Check definition = {.source = registration->source,
                        .in_function_body = true,
                        .body_function = function_id,
                        .has_arm_floor = true};
    const NLSourceSpan span =
        nl_syntax_node_view(
            nl_syntax_tree_root(
                registration->context->functions[function_id - 1].body->syntax))
            ->span;
    if (!host(&definition,
              nl_sem_clone(registration->context, &definition.context), span))
        goto failure;
    NLSemanticContext *c = definition.context;
    clear_definition_state(c);
    const NLFunctionEntry function = c->functions[function_id - 1];
    NLValueId values[NL_SEMANTIC_MAX_PARAMETERS] = {0};
    /* These formal sites supply type/ownership checking only. No proof of
     * formal disjointness is retained; EVERY call rechecks actual relations. */
    for (size_t i = 0; i < function.count; ++i) {
        const NLTypeId type = function.parameters[i];
        const NLSemanticTypeView t = c->types[type - 1].view;
        NLSemanticValueView value = {.type = type};
        if (t.kind == NL_TYPE_REF || t.kind == NL_TYPE_PTR) {
            NLValueId referent = new_value(
                &definition, (NLSemanticValueView){.type = t.target}, span);
            NLPlaceId place;
            if (referent == 0 ||
                !host(&definition,
                      nl_sem_new_place(c, t.target, 0, true, referent, &place),
                      span))
                goto failure;
            NLScopeId scope = 0;
            if (t.kind == NL_TYPE_REF &&
                !host(&definition, nl_sem_new_scope(c, 0, true, &scope), span))
                goto failure;
            value.reference = (NLReferenceFacts){
                place, c->places[place - 1].incarnation,
                scope, NL_PROVENANCE_VALID,
                true,  t.kind == NL_TYPE_PTR || t.access == NL_ACCESS_WRITE,
                0};
        }
        values[i] = new_value(&definition, value, span);
        if (values[i] == 0)
            goto failure;
    }
    const size_t scope_floor = c->scope_count, place_floor = c->place_count;
    for (size_t i = 0; i < function.count; ++i) {
        NLSymbolId symbol;
        if (!host(&definition,
                  nl_sem_bind_in_scope(c, function.body->parameter_names[i],
                                       values[i], 0, &symbol),
                  span))
            goto failure;
    }
    definition.artifact = malloc(sizeof(*definition.artifact));
    if (definition.artifact == NULL) {
        (void)host(&definition, NL_CHECK_OUT_OF_MEMORY, span);
        goto failure;
    }
    *definition.artifact =
        (NLCheckedFragment){.source = definition.source, .context = c};
    definition.artifact->root = source_block(
        &definition,
        nl_syntax_node_view(nl_syntax_tree_root(function.body->syntax)));
    if (definition.artifact->root == 0 ||
        !body_result(&definition, &function, definition.artifact->root, span))
        goto failure;
    const NLCheckStatus exit_status =
        nl_sem_function_exit(c, scope_floor, place_floor);
    if (exit_status != NL_CHECK_OK) {
        fail(&definition, exit_status, span, "P8-EXIT-DEPENDENCY",
             "surviving state depends on an ending function-local scope/place");
        goto failure;
    }
    if (!host(&definition, nl_sum_validate(c), span) ||
        !host(&definition, nl_raw_validate(c), span))
        goto failure;
    nl_checked_destroy(definition.artifact);
    nl_semantic_destroy(c);
    return true;
failure:
    registration->status = definition.status;
    registration->diagnostic = definition.diagnostic;
    nl_checked_destroy(definition.artifact);
    nl_semantic_destroy(definition.context);
    return false;
}

NLCheckStatus nl_semantic_register_function_body(
    NLSemanticContext *context, const char *name,
    const NLFunctionParameter *parameters, size_t count, NLTypeId result,
    const NLSyntaxTree *syntax, NLCheckDiagnostic *diagnostic)
{
    if (context == NULL || name == NULL || name[0] == 0 || syntax == NULL ||
        (count != 0 && parameters == NULL) || result == 0 ||
        result > context->type_count)
        return NL_CHECK_INTERNAL_ERROR;
    if (count > NL_SEMANTIC_MAX_PARAMETERS)
        return NL_CHECK_RESOURCE_LIMIT;
    const NLSyntaxView *root = nl_syntax_node_view(nl_syntax_tree_root(syntax));
    Check check = {.source = nl_syntax_tree_source(syntax)};
    NLTypeId types[NL_SEMANTIC_MAX_PARAMETERS] = {0};
    if (root->kind != NL_SYNTAX_BLOCK) {
        fail(&check, NL_CHECK_SEMANTIC_UNSUPPORTED, root->span, "P8-BODY-BLOCK",
             "function body must be the existing exact lexical block");
        goto failure;
    }
    if (!body_signature_type(context, result, false))
        goto signature_limit;
    for (size_t i = 0; i < count; ++i) {
        if (parameters[i].name == NULL || parameters[i].name[0] == 0 ||
            parameters[i].type == 0 || parameters[i].type > context->type_count)
            return NL_CHECK_INTERNAL_ERROR;
        if (!body_signature_type(context, parameters[i].type, true))
            goto signature_limit;
        for (size_t j = 0; j < i; ++j)
            if (strcmp(parameters[i].name, parameters[j].name) == 0) {
                fail(&check, NL_CHECK_SEMANTIC_ERROR, root->span,
                     "P8-DUPLICATE-PARAMETER",
                     "function parameter names must be unique");
                goto failure;
            }
        types[i] = parameters[i].type;
    }
    for (size_t i = 0; i < context->function_count; ++i)
        if (strcmp(name, context->functions[i].name) == 0) {
            fail(&check, NL_CHECK_SEMANTIC_ERROR, root->span,
                 "P8-DUPLICATE-FUNCTION",
                 "function name is already registered");
            goto failure;
        }
    if (!host(&check, nl_sem_clone(context, &check.context), root->span))
        goto failure;
    if (!host(&check,
              nl_semantic_register_function(check.context, name, types, count,
                                            result, false, false),
              root->span))
        goto failure;
    const size_t function_id = check.context->function_count;
    if (!host(&check,
              nl_body_create(syntax, parameters, count,
                             &check.context->functions[function_id - 1].body),
              root->span))
        goto failure;
    if (!check_definition(&check, function_id))
        goto failure;
    if (!host(&check, nl_sum_validate(check.context), root->span) ||
        !host(&check, nl_raw_validate(check.context), root->span))
        goto failure;
    nl_sem_commit(context, check.context);
    return NL_CHECK_OK;
signature_limit:
    fail(&check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, root->span,
         "P8-SIGNATURE-PRECISION",
         "body signature needs unsupported authority/ref-result/structural "
         "analysis");
failure:
    nl_semantic_destroy(check.context);
    if (diagnostic != NULL)
        *diagnostic = check.diagnostic;
    return check.status;
}
