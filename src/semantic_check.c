#include "semantic_internal.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
    const NLSource *source;
    size_t depth;
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
    for (size_t i = 0; i < check->context->binding_count; ++i) {
        if (equal_name(check, span, check->context->bindings[i].name)) {
            return i + 1;
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

static bool reference_live(Check *check, NLValueId value, NLSourceSpan span)
{
    NLSemanticContext *const c = check->context;
    const NLSemanticValueView v = c->values[value - 1];
    const NLSemanticTypeView type = c->types[v.type - 1].view;
    if (v.reference.provenance == NL_PROVENANCE_UNKNOWN ||
        v.reference.place == 0) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
             "P3-UNKNOWN-PROVENANCE",
             "safe operation needs proven pointer/ref provenance");
        return false;
    }
    if (v.reference.provenance != NL_PROVENANCE_VALID) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-INVALID-PROVENANCE",
             "pointer/ref provenance is invalid");
        return false;
    }
    const NLSemanticPlaceView p = c->places[v.reference.place - 1];
    if (!p.live || p.incarnation != v.reference.incarnation ||
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
        if (v.reference.scope == 0) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                 "P3-UNKNOWN-SCOPE", "ref scope evidence is unknown");
            return false;
        }
        if (!scope_active(c, v.reference.scope)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-DEAD-SCOPE",
                 "ref scope has ended");
            return false;
        }
        if (type.is_exclusive && suspended(c, value)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-SUSPENDED-AUTHORITY",
                 "exclusive parent has a live conflicting child reborrow");
            return false;
        }
        if (!v.reference.readable ||
            (type.access == NL_ACCESS_WRITE && !v.reference.writable)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-ACCESS",
                 "reference access evidence does not permit requested mode");
            return false;
        }
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
        if (v.reference.scope != 0 && !scope_active(c, v.reference.scope)) {
            continue;
        }
        if (t.is_exclusive && suspended(c, i + 1)) {
            continue;
        }
        if (v.reference.place == 0 || v.reference.scope == 0) {
            if (exclusive || ending || t.is_exclusive) {
                fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                     "P3-UNKNOWN-ALIAS",
                     "unknown live ref facts cannot prove "
                     "acquisition/transition safety");
                return true;
            }
        } else if (v.reference.place == place &&
                   (exclusive || ending || t.is_exclusive)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P3-REF-CONFLICT",
                 "operation conflicts with a live reference capability");
            return true;
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
    const NLSemanticValueView original = c->values[binding.value - 1];
    if (type.kind == NL_TYPE_REF &&
        !reference_live(check, binding.value, span)) {
        return 0;
    }
    NLValueId value = binding.value;
    if (type.is_copy) {
        if (!host(check, nl_sem_new_value(c, original, &value), span)) {
            return 0;
        }
    } else {
        if (conflicts(check, binding.place, false, true, 0, span)) {
            return 0;
        }
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
           (actual.access == expected.access ||
            (actual.access == NL_ACCESS_WRITE &&
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
        if (actual.kind == NL_TYPE_REF && actual.is_exclusive &&
            ref_compatible(actual, wanted)) {
            if (!reference_live(check, binding.value, span)) {
                return 0;
            }
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
    check->context->values[value - 1].carrier = NL_CARRIER_ENDED;
    check->context->values[value - 1].owner_place = 0;
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
    if ((kind != NL_TYPE_NOMINAL || type == 2) && kind != NL_TYPE_STORAGE &&
        kind != NL_TYPE_ALLOCATION && kind != NL_TYPE_BYTE &&
        kind != NL_TYPE_U8 && kind != NL_TYPE_USIZE && kind != NL_TYPE_ADDR) {
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
        const NLTypeId type =
            compound(check, NL_TYPE_PTR, t.target, NL_ACCESS_READ, false, span);
        if (type == 0) {
            return false;
        }
        NLSemanticValueView ptr = {.type = type, .reference = ref.reference};
        ptr.reference.scope = 0;
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
                              .readable = true,
                              .writable = true}},
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
            NLValueFactId fact;
            if (!host(check, nl_sem_fresh_fact(c, &fact), span)) {
                return false;
            }
            c->places[place - 1].current_value = second;
            c->places[place - 1].current_fact = fact;
            c->values[second - 1].carrier = NL_CARRIER_PLACE;
            c->values[second - 1].owner_place = place;
            c->values[old.current_value - 1].carrier =
                kind == NL_CHECKED_REPLACE ? NL_CARRIER_LOOSE
                                           : NL_CARRIER_ENDED;
            c->values[old.current_value - 1].owner_place = 0;
            refresh_bindings(check, place);
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
    if (syntax->data.call.argument_count != function.count) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->data.call.callee,
             "P3-ARITY", "argument count differs from resolved signature");
        return 0;
    }
    if (function.caller_effects) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->data.call.callee,
             "P3-EFFECT-SUMMARY-UNSUPPORTED",
             "caller-visible function effects are outside P3");
        return 0;
    }
    if (function.hidden_dependencies) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->data.call.callee,
             "P3-DEPENDENCIES-UNSUPPORTED",
             "function dependency summary is outside P3");
        return 0;
    }
    if (function.kind == NL_CHECKED_REGISTERED_CALL &&
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
                                       .function = function_id});
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
             c->types[expected - 1].view.kind == NL_TYPE_ALLOCATION)) {
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
    if (!primitive(check, id, arguments)) {
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

static NLCheckedNodeId expression(Check *check, const NLSyntaxNode *syntax)
{
    const NLSyntaxView *const node = nl_syntax_node_view(syntax);
    if (!enter(check, node->span)) {
        return 0;
    }
    NLCheckedNodeId result = 0;
    if (node->kind == NL_SYNTAX_EXPR_NAME) {
        result = identifier(check, node);
    } else if (node->kind == NL_SYNTAX_EXPR_CALL) {
        result = call(check, node);
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
    CHECK_LOAN
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
    if (!host(&check, nl_raw_validate(check.context), root->span)) {
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
    if (!host(&check, nl_raw_validate(c), operation->span)) {
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
