#include "newlang/parser.h"
#include "semantic_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct LoopControl {
    struct LoopControl *parent; /* borrowed lexical target stack */
    NLControlTarget *target;
    NLControlState *entry;
    NLLoopHeader *header;
    NLTypeId types[NL_CONTROL_MAX_SLOTS];
    size_t count, bindings, places, scopes;
} LoopControl;

typedef struct Check {
    const struct Check *caller; /* borrowed synchronous active call chain */
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
    const NLSource *source;
    size_t depth;
    size_t binding_floor, namespace_floor;
    size_t body_function, body_steps;
    size_t branch_steps;
    size_t *branch_budget; /* synchronous finite checking/replay work */
    size_t *body_budget;   /* borrowed counter for this synchronous body walk */
    bool in_function_body, definition, terminated, in_source_loan;
    NLTypeId function_result;
    size_t function_binding_floor, function_scope_floor, function_place_floor;
    size_t function_value_prefix;
    NLValueFactId function_fact_prefix;
    NLCheckedResult
        returned;      /* compatibility summary, never exit classification */
    LoopControl *loop; /* synchronous borrowed nearest-loop context */
    NLControlTarget *function_target; /* borrowed active function identity */
    size_t arm_floor;
    bool has_arm_floor, in_match_arm;
    bool allocation_some_path; /* nearest trial outcome, not ancestor success */
    bool closure_probe; /* isolated #244 certificate integration, never CLI */
    size_t allocation_depth; /* one-link gate two; certified profile five */
    size_t allocation_sites;
    size_t *allocation_budget; /* borrowed counter, active body walk only */
    bool allocated_slice; /* owned Some world, never a runtime success claim */
    /* Borrowed synchronous proof from the exact independently checked call. */
    const NLCheckedFragment *custody_origin;
    NLCheckedNodeId custody_origin_match;
    NLValueId custody_packet, custody_old_none;
    NLPlaceId custody_sink;
    bool custody_recipient_body;
    NLCheckedNodeId custody_pending;
    NLSymbolId custody_binding;
    bool custody_continuation, custody_extracted, custody_final_none;
    NLValueId custody_recovered, custody_saved;
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

static bool closure_status(Check *check, NLCheckStatus status,
                           NLSourceSpan span)
{
    if (status == NL_CHECK_ANALYSIS_PRECISION_LIMIT) {
        fail(check, status, span, "CAPTURED-CLOSURE-PRECISION",
             "complete original-owner fork/cleanup proof is unavailable");
        return false;
    }
    return host(check, status, span);
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

static bool lexical_name(Check *check, const void *bytes, size_t length,
                         NLSourceSpan span)
{
    const NLOrdinaryNameClass kind = nl_ordinary_name_class(bytes, length);
    if (kind == NL_NAME_ADMISSIBLE)
        return true;
    fail(check, NL_CHECK_SEMANTIC_ERROR, span, nl_ordinary_name_code(kind),
         nl_ordinary_name_message(kind));
    return false;
}

static bool lexical_source_name(Check *check, NLSourceSpan span)
{
    NLSourceView bytes;
    if (!nl_source_view(check->source, span, &bytes)) {
        (void)host(check, NL_CHECK_INTERNAL_ERROR, span);
        return false;
    }
    return lexical_name(check, bytes.bytes, bytes.length, span);
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
        if (result == 0 && equal_name(check, node->data.name, "LiveTail")) {
            NLTypeId h = 0;
            for (size_t i = 0; i < check->context->type_count; ++i)
                if (nl_recursive_local_type(check->context, i + 1))
                    h = i + 1;
            const NLCheckStatus status =
                nl_live_tail_registry(check->context, h, &result);
            if (status == NL_CHECK_SEMANTIC_ERROR)
                fail(check, status, node->span, "P208-LIVETAIL-NAME",
                     "compiler-known LiveTail conflicts with a declared name");
            else if (status == NL_CHECK_SEMANTIC_UNSUPPORTED)
                fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, node->span,
                     "P208-TYPE-PROFILE",
                     "LiveTail requires one completed H source profile");
            else
                (void)host(check, status, node->span);
        }
        if (result == 0 && check->status == NL_CHECK_OK) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, node->span, "P3-UNKNOWN-TYPE",
                 "unknown semantic type name");
        } else if (result != 0 &&
                   check->context->types[result - 1].incomplete) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, node->span,
                 "REC-INCOMPLETE-TYPE",
                 "incomplete header has no ordinary type use");
            result = 0;
        }
    } else if (node->kind == NL_SYNTAX_OPTION_BACKING) {
        NLTypeId h = 0;
        for (size_t i = 0; i < check->context->type_count; ++i)
            if (nl_recursive_local_type(check->context, i + 1))
                h = i + 1;
        NLCheckStatus status =
            nl_allocated_registry(check->context, h, &result);
        if (status == NL_CHECK_SEMANTIC_UNSUPPORTED)
            fail(check, status, node->span, "ALLOCATED-TYPE-PROFILE",
                 "requires one completed recursive H");
        else
            (void)host(check, status, node->span);
    } else if (node->kind == NL_SYNTAX_OPTION_LIVE_TAIL) {
        NLTypeId h = 0;
        for (size_t i = 0; i < check->context->type_count; ++i)
            if (nl_recursive_local_type(check->context, i + 1))
                h = i + 1;
        NLCheckStatus status = nl_custody_registry(check->context, h, &result);
        if (status == NL_CHECK_SEMANTIC_UNSUPPORTED)
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, node->span,
                 "CUSTODY-TYPE-PROFILE",
                 "custody requires one completed H source profile");
        else
            (void)host(check, status, node->span);
    } else if (node->kind == NL_SYNTAX_OPTION_PTR) {
        const NLSyntaxView *target =
            nl_syntax_node_view(node->data.ptr_type.target);
        NLTypeId header = 0;
        for (size_t i = 0; i < check->context->type_count; ++i)
            if (check->context->types[i].name != NULL &&
                equal_name(check, target->data.name,
                           check->context->types[i].name))
                header = i + 1;
        if (header == 0) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, target->span,
                 "P3-UNKNOWN-TYPE", "unknown recursive target name");
        } else if (!check->context->types[header - 1].recursive_header) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, target->span,
                 "REC-SELF-TARGET",
                 "exact Option target requires the bounded nominal header");
        } else {
            NLTypeId ptr = compound(check, NL_TYPE_PTR, header, NL_ACCESS_READ,
                                    false, node->span);
            if (ptr != 0)
                (void)host(check,
                           nl_recursive_option(check->context, ptr, &result),
                           node->span);
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
    if (!nl_fixed_live(c, fact.place) || p.incarnation != fact.incarnation ||
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
            } else if (nl_fixed_overlap(c, fact.place, place) &&
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
        if (nl_fixed_type(c, binding.type) &&
            conflicts(check, binding.place, false, false, 0, span))
            return 0;
        if (!host(check, nl_sem_copy_value(c, binding.value, &value), span)) {
            return 0;
        }
    } else {
        const NLCheckStatus ending =
            nl_fixed_end_dependencies(c, binding.place, SIZE_MAX);
        if (ending != NL_CHECK_OK) {
            fail(check, ending, span, "FIELD-END-DEPENDENCY",
                 "surviving value depends on the ending root");
            return 0;
        }
        if (conflicts(check, binding.place, false, true, 0, span)) {
            return 0;
        }
        nl_sum_detach(c, binding.place);
        nl_fixed_detach(c, binding.place);
        c->bindings[symbol - 1].view.availability = NL_CONSUMED;
        c->places[binding.place - 1].live = false;
        c->places[binding.place - 1].current_value = 0;
        c->places[binding.place - 1].current_fact = 0;
        c->places[binding.place - 1].governing_domain = 0;
        c->values[value - 1].carrier = NL_CARRIER_LOOSE;
        c->values[value - 1].owner_place = 0;
    }
    return add(
        check,
        (NLCheckedNodeView){
            .kind = NL_CHECKED_IDENTIFIER,
            .span = span,
            .name = name,
            .type = binding.type,
            .symbol = symbol,
            .value_use = type.is_copy ? NL_VALUE_COPIED : NL_VALUE_CONSUMED,
            .result_count = 1,
            .results = {{binding.type, value}},
            .has_reference_result = type.kind == NL_TYPE_PTR &&
                                    c->values[value - 1].reference_count == 0,
            .reference_result = c->values[value - 1].reference});
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
    if (node->kind == NL_SYNTAX_EXPR_NAME &&
        !equal_name(check, node->data.name, "unit")) {
        const NLSymbolId symbol = available(check, node->data.name);
        if (symbol == 0) {
            return 0;
        }
        id = binding_argument(check, symbol, expected, node->span,
                              node->data.name);
    } else {
        id = expression(check, syntax);
    }
    if (check->terminated)
        return id;
    return parameter_match(check, id, expected);
}

static bool ref_field_selection(Check *, const NLSyntaxView *, NLCheckedField *,
                                NLValueId *);
static NLTypeId write_parameter(Check *check, const NLSyntaxNode *syntax)
{
    const NLSyntaxView *const node = nl_syntax_node_view(syntax);
    if (check->allocated_slice && node->kind == NL_SYNTAX_FIELD_DESIGNATOR) {
        NLCheckedField field;
        NLValueId parent;
        if (!ref_field_selection(check, node, &field, &parent))
            return 0;
        return compound(check, NL_TYPE_REF, field.type, NL_ACCESS_WRITE, false,
                        node->span);
    }
    if (node->kind != NL_SYNTAX_EXPR_NAME ||
        equal_name(check, node->data.name, "unit")) {
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
    if (check->allocated_slice && nl_recursive_local_type(check->context, type))
        return true;
    if (check->context->types[type - 1].view.field_count != 0 ||
        ((kind != NL_TYPE_NOMINAL || type == 2) && kind != NL_TYPE_STORAGE &&
         kind != NL_TYPE_ALLOCATION && kind != NL_TYPE_BOOL &&
         kind != NL_TYPE_BYTE && kind != NL_TYPE_U8 && kind != NL_TYPE_USIZE &&
         kind != NL_TYPE_ADDR && kind != NL_TYPE_SUM)) {
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
        operation.lifetime_domain = domain;
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
        if (ref.dependencies != NL_DEPENDENCY_FREE) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                 "P3-DEPENDENCIES-UNSUPPORTED",
                 "ptr conversion does not preserve this value dependency "
                 "evidence");
            return false;
        }
        const NLTypeId type =
            compound(check, NL_TYPE_PTR, t.target, NL_ACCESS_READ, false, span);
        if (type == 0) {
            return false;
        }
        NLSemanticValueView ptr = {.type = type,
                                   .dependencies = ref.dependencies,
                                   .reference = ref.reference};
        ptr.reference.scope = 0;
        ptr.reference.occurrence_dependency = 0;
        const NLValueId result = new_value(check, ptr, span);
        if (result == 0) {
            return false;
        }
        operation.type = type;
        operation.result_count = 1;
        operation.results[0] = (NLCheckedResult){type, result};
        operation.has_reference_result = true;
        operation.reference_result = ptr.reference;
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
        operation.has_reference_result = true;
        operation.reference_result = c->values[result - 1].reference;
        operation.lifetime_place = place;
        operation.lifetime_incarnation = c->places[place - 1].incarnation;
        operation.lifetime_domain = domain;
        operation.lifetime_range = c->places[place - 1].placement;
        operation.backing = operation.lifetime_range.region;
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
        operation.lifetime_place = place;
        operation.lifetime_incarnation = old.incarnation;
        operation.lifetime_domain = old.governing_domain;
        operation.lifetime_range = old.placement;
        operation.backing = old.placement.region;
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
        const NLCheckStatus end_status =
            nl_fixed_end_dependencies(c, place, SIZE_MAX);
        if (end_status != NL_CHECK_OK) {
            fail(check, end_status, span, "FIELD-END-DEPENDENCY",
                 "surviving value depends on ending root");
            return false;
        }
        nl_sum_detach(c, place);
        nl_fixed_detach(c, place);
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
        if (first != 0 && kind != NL_CHECKED_SWAP &&
            c->types[c->values[first - 1].type - 1].view.kind == NL_TYPE_REF &&
            c->values[first - 1].reference_count == 0) {
            const NLPlaceId target = c->values[first - 1].reference.place;
            if (target != 0 && target <= c->place_count &&
                c->places[target - 1].parent_aggregate != 0 &&
                view(check, args[1])->type != c->places[target - 1].type) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, args[1])->span,
                     "P3-TYPE-MISMATCH",
                     "new value type differs from fixed field type");
                return false;
            }
        }
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
            if (old.parent_aggregate != 0) {
                fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, span,
                     "FIELD-SWAP-PROFILE",
                     "fixed-field swap is outside the bounded mutation slice");
                return false;
            }
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
            if (c->places[other - 1].parent_aggregate != 0) {
                fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, span,
                     "FIELD-SWAP-PROFILE",
                     "fixed-field swap is outside the bounded mutation slice");
                return false;
            }
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
            const NLCheckStatus dep = nl_fixed_change_dependencies(
                c, place, kind == NL_CHECKED_STORE ? old.current_value : 0);
            if (dep != NL_CHECK_OK) {
                fail(check, dep, span, "FIELD-VALUE-DEPENDENCY",
                     "Change conflicts with a surviving current-value "
                     "dependency");
                return false;
            }
            if (old.parent_aggregate != 0) {
                const NLPlaceId root = old.parent_aggregate;
                const NLSemanticPlaceView before = c->places[root - 1];
                NLCheckedField evidence = {
                    .present = true,
                    .dependency_compatible = true,
                    .nominal = before.type,
                    .type = old.type,
                    .index = old.parent_field_index,
                    .parent = root,
                    .child = place,
                    .parent_incarnation = before.incarnation,
                    .child_incarnation = old.incarnation,
                    .parent_fact = before.current_fact,
                    .child_fact = old.current_fact,
                    .access = NL_ACCESS_WRITE,
                    .old_value = old.current_value,
                    .new_value = second,
                    .payload_occurrence = old.payload_occurrence};
                for (size_t i = 0; i < c->binding_count; ++i)
                    if (!c->bindings[i].hidden &&
                        c->bindings[i].view.place == root)
                        evidence.base = i + 1;
                if (view(check, args[0])->field.present &&
                    view(check, args[0])->field.child == place)
                    evidence.base = view(check, args[0])->field.base;
                const bool record =
                    check->allocated_slice &&
                    c->types[before.type - 1].recursive_header &&
                    c->types[before.type - 1].view.field_count == 4;
                if (record && kind != NL_CHECKED_REPLACE) {
                    fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, span,
                         "THREE-LINK-CHANGE-PROFILE",
                         "three-link source selects replace only");
                    return false;
                }
                if (record && !closure_status(check,
                                              nl_captured_change_begin(
                                                  check->artifact, call, c),
                                              span))
                    return false;
                if (!host(check,
                          nl_fixed_change(c, place, second,
                                          kind == NL_CHECKED_STORE),
                          span))
                    return false;
                evidence.parent_post_fact = c->places[root - 1].current_fact;
                evidence.child_post_fact = c->places[place - 1].current_fact;
                evidence.post_payload_occurrence =
                    c->places[place - 1].payload_occurrence;
                operation.field = evidence;
                refresh_bindings(check, root);
                if (kind == NL_CHECKED_REPLACE) {
                    operation.type = old.type;
                    operation.result_count = 1;
                    operation.results[0] =
                        (NLCheckedResult){old.type, old.current_value};
                }
                *view(check, call) = operation;
                if (record &&
                    !host(check, nl_captured_change_end(check->artifact, c),
                          span))
                    return false;
                return true;
            }
            NLValueFactId fact;
            if (!host(check, nl_sem_fresh_fact(c, &fact), span)) {
                return false;
            }
            if (check->custody_continuation && place == check->custody_sink) {
                const NLSymbolId owner = view(check, args[0])->symbol;
                if (owner == 0 || owner > c->binding_count) {
                    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                         "CUSTODY-EXTRACTION-SOURCE",
                         "recovery requires the exact source loan operand");
                    return false;
                }
                const NLValueId source_ref = c->bindings[owner - 1].view.value;
                for (size_t i = 0; i < c->value_count; ++i) {
                    const NLSemanticValueView v = c->values[i];
                    if (v.carrier != NL_CARRIER_ENDED &&
                        c->types[v.type - 1].view.kind == NL_TYPE_REF &&
                        i + 1 != first && i + 1 != source_ref) {
                        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                             "CUSTODY-EXTRACTION-ALIAS",
                             "extra live alias requires a stronger recovery "
                             "proof");
                        return false;
                    }
                }
                operation.custody_extraction.present = true;
                operation.custody_extraction.sink = place;
                operation.custody_extraction.old_sum = old.current_value;
                operation.custody_extraction.new_sum = second;
                operation.custody_extraction.packet =
                    c->values[old.current_value - 1].sum_payload;
                operation.custody_extraction.old_occurrence =
                    old.payload_occurrence;
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
    if (check->custody_recipient_body && kind == NL_CHECKED_REPLACE) {
        const NLValueId old = operation.results[0].value;
        if (check->custody_old_none != 0 || operation.result_count != 1 ||
            old == 0 || c->values[old - 1].variant != 1 ||
            c->values[old - 1].sum_payload != 0) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                 "CUSTODY-OLD-NONE-PROOF",
                 "recipient old sum is not source-proven exact None");
            return false;
        }
        check->custody_old_none = old;
    }
    if (check->custody_continuation && kind == NL_CHECKED_REPLACE) {
        const NLValueId ref = one_result(check, args[0]);
        if (c->values[ref - 1].reference.place == check->custody_sink) {
            if (check->custody_recovered != 0 ||
                c->values[c->places[check->custody_sink - 1].current_value - 1]
                        .variant != 1) {
                fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                     "CUSTODY-EXTRACTION-PROOF",
                     "one source extraction returning old Some/None required");
                return false;
            }
            check->custody_recovered = operation.results[0].value;
        }
    }
    *view(check, call) = operation;
    return true;
}

static NLCheckedNodeId source_block(Check *, const NLSyntaxView *);
static bool run_body(Check *, NLCheckedNodeId, const NLFunctionEntry *,
                     const NLCheckedNodeId *);

static NLCheckedNodeId allocated_raw(Check *, const NLSyntaxView *,
                                     NLRawOperationKind);
static NLCheckedNodeId allocated_ref(Check *, const NLSyntaxView *);
static NLCheckedNodeId link_read(Check *, const NLSyntaxView *);

/* §18.1a admission is read-only. Arguments have undergone ordinary Copy/move
 * in the private transaction, but no callee bindings/effects exist yet. */
static bool owner_entry(Check *check, NLCheckedNodeId id,
                        const NLFunctionEntry *function,
                        const NLCheckedNodeId *arguments)
{
    NLSemanticContext *c = check->context;
    const NLSourceSpan span = view(check, id)->span;
    const NLTypedOwnerDefinition d = function->body->owner_definition;
    if (!d.definition_checked || d.requirements != NL_OWNER_ALL_REQUIREMENTS ||
        d.target != c->types[function->parameters[0] - 1].view.target ||
        (d.step_count != 4 && d.step_count != 5) || !check->allocated_slice ||
        check->allocation_depth != 2 || check->body_function == 0 ||
        strcmp(c->functions[check->body_function - 1].name, "main") != 0) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
             "P193-CALL-PROFILE", "requires the closed two-H main call world");
        return false;
    }
    for (size_t i = 0; i < d.step_count; ++i)
        if (d.steps[i] != (NLTypedOwnerStep)(i + (d.step_count == 4 ? 1 : 0))) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                 "P193-CALL-DEFINITION",
                 "conditional definition evidence incomplete");
            return false;
        }
    NLValueId values[3];
    for (size_t i = 0; i < 3; ++i) {
        values[i] = one_result(check, arguments[i]);
        if (values[i] == 0)
            return false;
        const NLCheckedNodeView a = *view(check, arguments[i]);
        if (a.kind != NL_CHECKED_IDENTIFIER || a.symbol == 0 ||
            a.value_use != (i == 0 ? NL_VALUE_COPIED : NL_VALUE_CONSUMED)) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                 "P193-CALL-TRANSFER", "requires original available bindings");
            return false;
        }
        if (c->values[values[i] - 1].dependencies != NL_DEPENDENCY_FREE ||
            c->values[values[i] - 1].value_dependency_count != 0) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                 "P193-CALL-DEPENDENCY",
                 "input dependency proof is incomplete");
            return false;
        }
    }
    if (!reference_live(check, values[0], span))
        return false;
    check->status = nl_owner_relations(c, values, &d, span, &check->diagnostic);
    if (check->status != NL_CHECK_OK)
        return false;
    const NLReferenceFacts f = c->values[values[0] - 1].reference;
    const NLSemanticPlaceView root = c->places[f.place - 1];
    const NLSemanticValueView domain = c->values[values[2] - 1];
    /* These existing checks prove unique occupancy/Allocation and preservation
     * against all surviving Value/Occurrence blockers, not only selected p. */
    if (!host(check, nl_raw_validate(c), span))
        return false;
    if (conflicts(check, f.place, false, true, 0, span))
        return false;
    NLCheckStatus blockers = nl_fixed_end_dependencies(c, f.place, SIZE_MAX);
    if (blockers != NL_CHECK_OK) {
        fail(check, blockers, span, "P193-CALL-BLOCKER",
             "surviving dependency blocks receiver EndRoot");
        return false;
    }
    for (size_t i = 0; i < c->place_count; ++i)
        if (c->places[i].live && c->places[i].independent_root &&
            c->places[i].governing_domain == domain.domain &&
            i + 1 != f.place) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P193-CALL-DOMAIN-ROOTS",
                 "received domain also governs an unrelated live root");
            return false;
        }
    if (check->artifact->owner_entry != NULL) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
             "P193-CALL-PRECISION",
             "one receiver call per bounded path artifact");
        return false;
    }
    if (!host(check, nl_sem_clone(c, &check->artifact->owner_entry), span))
        return false;
    check->artifact->owner_entry_call = id;
    check->artifact->destroy_owner_entry = nl_semantic_destroy;
    view(check, id)->owner_call.definition = d;
    view(check, id)->owner_call.entry_proved = true;
    view(check, id)->owner_call.root = f.place;
    view(check, id)->owner_call.incarnation = f.incarnation;
    view(check, id)->owner_call.range = root.placement;
    view(check, id)->owner_call.domain = domain.domain;
    for (size_t i = 0; i < 3; ++i) {
        view(check, id)->owner_call.inputs[i] = values[i];
        view(check, id)->owner_call.donor[i] =
            view(check, arguments[i])->symbol;
    }
    return true;
}

/* §18.1b proof in the current caller world, before any callee execution.
 * This complements, never substitutes for, independent symbolic definition. */
static bool producer_entry(Check *check, NLCheckedNodeId id,
                           const NLFunctionEntry *function,
                           const NLCheckedNodeId *arguments)
{
    NLSemanticContext *c = check->context;
    const NLSourceSpan span = view(check, id)->span;
    const NLTypedOwnerDefinition d = function->body->owner_definition;
    if (!d.definition_checked || !d.live_return || !d.head_link_required ||
        d.requirements != NL_OWNER_ALL_REQUIREMENTS || d.step_count != 0) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
             "P208-CALL-DEFINITION",
             "incomplete independent producer requirements");
        return false;
    }
    NLValueId values[4];
    for (size_t i = 0; i < 4; ++i) {
        values[i] = one_result(check, arguments[i]);
        if (values[i] == 0)
            return false;
        const NLCheckedNodeView a = *view(check, arguments[i]);
        if (i != 0 &&
            (a.kind != NL_CHECKED_IDENTIFIER || a.symbol == 0 ||
             a.value_use != (i == 1 ? NL_VALUE_COPIED : NL_VALUE_CONSUMED))) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                 "P208-CALL-TRANSFER",
                 "tail requires original available value carriers");
            return false;
        }
    }
    check->status =
        nl_owner_relations(c, values + 1, &d, span, &check->diagnostic);
    if (check->status != NL_CHECK_OK)
        return false;
    const NLReferenceFacts tail = c->values[values[1] - 1].reference;
    if (!reference_live(check, values[1], span) ||
        !host(check, nl_raw_validate(c), span) ||
        conflicts(check, tail.place, false, true, 0, span))
        return false;
    NLCheckStatus blockers = nl_fixed_end_dependencies(c, tail.place, SIZE_MAX);
    if (blockers != NL_CHECK_OK) {
        fail(check, blockers, span, "P208-CALL-BLOCKER",
             "surviving tail dependency prevents future recovery");
        return false;
    }
    const NLCheckedNodeView a = *view(check, arguments[0]);
    const NLSemanticValueView head = c->values[values[0] - 1];
    const NLReferenceFacts h = head.reference;
    if (a.kind != NL_CHECKED_FIELD_REF || !a.field.present ||
        a.field.index != 0 || head.reference_count != 0 ||
        head.dependencies != NL_DEPENDENCY_FREE ||
        head.value_dependency_count != 0 ||
        !reference_live(check, values[0], span) ||
        !write_ref(check, values[0], span)) {
        if (check->status == NL_CHECK_OK)
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
                 "P208-CALL-HEAD-PROJECTION",
                 "current source-projected write link ref required");
        return false;
    }
    if (h.place == 0 || h.place > c->place_count)
        return false;
    const NLSemanticPlaceView field = c->places[h.place - 1];
    if (field.parent_aggregate == 0 || field.parent_field_index != 0 ||
        field.parent_aggregate != a.field.parent || h.place != a.field.child ||
        field.incarnation != a.field.child_incarnation ||
        field.current_fact != a.field.child_fact) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P208-CALL-HEAD",
             "head projection is not current");
        return false;
    }
    const NLSemanticPlaceView hp = c->places[field.parent_aggregate - 1],
                              tp = c->places[tail.place - 1];
    const NLDomainId domain = c->values[values[3] - 1].domain;
    bool scoped = false;
    /* An enclosing source loan may belong to an ancestor artifact across a
     * finite match fork. Prove its current Domain capability in this world,
     * never by searching just one branch's syntactic node list. */
    for (size_t n = 0; n < c->value_count; ++n) {
        const NLSemanticValueView stable = c->values[n];
        const NLSemanticTypeView st = c->types[stable.type - 1].view;
        const NLReferenceFacts sf = stable.reference;
        if (stable.carrier != NL_CARRIER_ENDED &&
            stable.dependencies == NL_DEPENDENCY_FREE &&
            stable.value_dependency_count == 0 && st.kind == NL_TYPE_REF &&
            st.target == 2 && !st.is_exclusive && stable.reference_count == 0 &&
            sf.scope == h.scope && sf.provenance == NL_PROVENANCE_VALID &&
            sf.readable && sf.place != 0 && sf.place <= c->place_count) {
            const NLSemanticPlaceView dp = c->places[sf.place - 1];
            if (dp.live && dp.incarnation == sf.incarnation && dp.type == 2 &&
                dp.current_value != 0 &&
                c->values[dp.current_value - 1].domain == hp.governing_domain)
                scoped = true;
        }
    }
    if (!scoped || !scope_active(c, h.scope) ||
        !nl_fixed_live(c, field.parent_aggregate) || hp.type != d.target ||
        !hp.independent_root || hp.incarnation != a.field.parent_incarnation ||
        hp.placement.region == 0 ||
        hp.placement.region == tp.placement.region ||
        hp.governing_domain == 0 || hp.governing_domain == domain ||
        !c->domains[hp.governing_domain - 1].live ||
        !c->regions[hp.placement.region - 1].view.live ||
        !c->regions[hp.placement.region - 1].view.ordinary_write ||
        !c->regions[hp.placement.region - 1].view.ordinary_read ||
        field.current_value == 0) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P208-CALL-HEAD",
             "distinct live head backing/domain scoped write relation missing");
        return false;
    }
    const NLSemanticValueView option = c->values[field.current_value - 1];
    if (option.variant != 2 || option.sum_payload == 0) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P208-CALL-HEAD-VALUE",
             "head link must currently be Some of tail");
        return false;
    }
    const NLSemanticValueView payload = c->values[option.sum_payload - 1];
    const NLReferenceFacts pf = payload.reference;
    if (payload.reference_count != 0 ||
        payload.dependencies != NL_DEPENDENCY_FREE ||
        payload.value_dependency_count != 0 ||
        payload.type != c->values[values[1] - 1].type ||
        pf.provenance != NL_PROVENANCE_VALID || pf.place != tail.place ||
        pf.incarnation != tail.incarnation || pf.scope != tail.scope ||
        pf.occurrence_dependency != tail.occurrence_dependency ||
        pf.readable != tail.readable || pf.writable != tail.writable) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P208-CALL-HEAD-VALUE",
             "head Some payload has different source provenance");
        return false;
    }
    for (size_t i = 0; i < c->place_count; ++i)
        if (c->places[i].live && c->places[i].independent_root &&
            c->places[i].governing_domain == domain && i + 1 != tail.place) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P208-CALL-DOMAIN-ROOTS",
                 "tail domain governs unrelated live root");
            return false;
        }
    if (check->artifact->producer_entry != NULL) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
             "P208-CALL-PRECISION", "one producer per bounded path");
        return false;
    }
    if (!host(check, nl_sem_clone(c, &check->artifact->producer_entry), span))
        return false;
    check->artifact->producer_call = id;
    check->artifact->destroy_producer_world = nl_semantic_destroy;
    view(check, id)->producer.definition = d;
    view(check, id)->producer.entry_proved = true;
    view(check, id)->producer.entry_world = check->artifact->producer_entry;
    view(check, id)->producer.root = tail.place;
    view(check, id)->producer.incarnation = tail.incarnation;
    view(check, id)->producer.range = tp.placement;
    view(check, id)->producer.domain = domain;
    view(check, id)->producer.head_domain = hp.governing_domain;
    view(check, id)->producer.head = a.field;
    view(check, id)->producer.head_before = field.current_value;
    view(check, id)->producer.head_before_fact = field.current_fact;
    for (size_t i = 0; i < 4; ++i) {
        view(check, id)->producer.inputs[i] = values[i];
        view(check, id)->producer.donor[i] = view(check, arguments[i])->symbol;
    }
    return true;
}

/* #217 admission is read-only: no argument evaluation, owner transfer or
 * transfer certificate here. Actual body/continuation checks follow only
 * after these source/world obligations pass. */
static bool custody_preflight(Check *check, const NLSyntaxView *syntax,
                              const NLFunctionEntry *function)
{
    NLSemanticContext *c = check->context;
    const NLCustodyDefinition d = function->body->custody_definition;
    const NLSyntaxNode *args = syntax->data.call.arguments;
    const NLSyntaxView *sink_syntax = nl_syntax_node_view(args),
                       *packet_syntax =
                           nl_syntax_node_view(nl_syntax_next_argument(args));
    if (!d.definition_checked ||
        d.requirements != NL_CUSTODY_ALL_REQUIREMENTS ||
        !check->allocated_slice || !check->in_source_loan ||
        sink_syntax->kind != NL_SYNTAX_EXPR_NAME ||
        packet_syntax->kind != NL_SYNTAX_EXPR_NAME) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
             "CUSTODY-ENTRY-PRECISION",
             "recipient preflight requires qualified original packet and "
             "direct scoped caller-local sink arguments");
        return false;
    }
    const NLSymbolId sink_symbol = available(check, sink_syntax->span);
    if (sink_symbol == 0)
        return false;
    const NLSymbolId packet_symbol = available(check, packet_syntax->span);
    if (packet_symbol == 0)
        return false;
    const NLSemanticBindingView sink_binding =
        c->bindings[sink_symbol - 1].view;
    const NLSemanticTypeView t = c->types[sink_binding.type - 1].view;
    if (t.kind != NL_TYPE_REF || t.is_exclusive ||
        t.access != NL_ACCESS_WRITE || t.target != d.option) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, sink_syntax->span,
             "CUSTODY-SINK-MODE",
             "recipient requires an ordinary write ref to its custody sum");
        return false;
    }
    if (!concrete_ref(check, sink_binding.value, sink_syntax->span) ||
        !reference_live(check, sink_binding.value, sink_syntax->span))
        return false;
    const NLSemanticValueView ref = c->values[sink_binding.value - 1];
    const NLReferenceFacts f = ref.reference;
    const NLSemanticPlaceView root = c->places[f.place - 1];
    const NLSemanticValueView current = c->values[root.current_value - 1];
    if (!root.implicit_local || !root.independent_root ||
        root.parent_sum != 0 || root.parent_aggregate != 0 ||
        root.placement.region != 0 || root.governing_domain != 0 ||
        root.type != d.option || root.payload_occurrence != 0 ||
        current.type != d.option || current.variant != 1 ||
        current.sum_payload != 0 || current.carrier != NL_CARRIER_PLACE ||
        current.owner_place != f.place ||
        current.dependencies != NL_DEPENDENCY_FREE ||
        current.value_dependency_count != 0) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, sink_syntax->span,
             "CUSTODY-CURRENT-NONE-PRECISION",
             "caller-local current exact None and empty occurrence required");
        return false;
    }
    bool scoped_local = false;
    for (NLCheckedNodeId n = 1; n <= check->artifact->count; ++n) {
        const NLCheckedNodeView *v = view(check, n);
        if (v->kind != NL_CHECKED_LOAN_HEADER ||
            v->loan.ref_symbol != sink_symbol || v->loan.scope != f.scope ||
            v->loan.place != f.place ||
            v->loan.incarnation != root.incarnation ||
            v->loan.access != NL_ACCESS_WRITE || !v->loan.implicit_local ||
            !v->loan.prevent_lifetime_end || v->loan.from_ptr ||
            v->loan.source == 0 || v->loan.source > c->binding_count)
            continue;
        const NLSemanticBindingView owner =
            c->bindings[v->loan.source - 1].view;
        scoped_local = !c->bindings[v->loan.source - 1].hidden &&
                       owner.availability == NL_AVAILABLE &&
                       owner.place == f.place &&
                       owner.value == root.current_value;
    }
    if (!scoped_local || c->scopes[f.scope - 1].parent != 0 ||
        c->scopes[f.scope - 1].parent_authority != 0) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, sink_syntax->span,
             "CUSTODY-SCOPE-PRECISION",
             "sink requires its actual source-owned caller-local loan");
        return false;
    }
    for (size_t i = 0; i < c->scope_count; ++i)
        if (c->scopes[i].active && i + 1 != f.scope) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                 "CUSTODY-ALIAS-PRECISION",
                 "additional active scope is outside custody entry proof");
            return false;
        }
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView v = c->values[i];
        if (v.carrier == NL_CARRIER_ENDED)
            continue;
        if (v.dependencies != NL_DEPENDENCY_FREE ||
            v.value_dependency_count != 0 ||
            (c->types[v.type - 1].view.kind == NL_TYPE_REF &&
             i + 1 != sink_binding.value)) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                 "CUSTODY-ALIAS-PRECISION",
                 "surviving alias or unresolved dependency requires proof");
            return false;
        }
    }
    const NLValueId packet = c->bindings[packet_symbol - 1].view.value;
    if (!host(check, nl_raw_validate(c), syntax->span))
        return false;
    if (c->values[packet - 1].type != d.packet ||
        !nl_packet_available_inherited(check->artifact, packet)) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, packet_syntax->span,
             "CUSTODY-PACKET-ORIGIN-PRECISION",
             "available intact original packet requires qualified parent "
             "producer and current child-world correspondence");
        return false;
    }
    return true;
}

static NLCheckedNodeId call(Check *check, const NLSyntaxView *syntax)
{
    NLSemanticContext *const c = check->context;
    if (equal_name(check, syntax->data.call.callee, "deallocate"))
        return allocated_raw(check, syntax, NL_RAW_DEALLOCATE);
    if (check->allocated_slice &&
        equal_name(check, syntax->data.call.callee, "read"))
        return link_read(check, syntax);
    if (equal_name(check, syntax->data.call.callee, "lifetime_domain")) {
        if (!check->allocated_slice || syntax->data.call.argument_count != 0) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
                 "ALLOCATED-SOURCE-PROFILE",
                 "domain creation requires the allocated H source world");
            return 0;
        }
        NLCheckedNodeId id =
            add(check, (NLCheckedNodeView){.kind = NL_CHECKED_DOMAIN_CREATE,
                                           .span = syntax->span});
        return id != 0 && primitive(check, id, NULL) ? id : 0;
    }
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
    if (function.body != NULL) {
        for (const Check *active = check; active != NULL;
             active = active->caller) {
            if (active->body_function == function_id) {
                fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                     "P11-RECURSIVE-ANALYSIS-PRECISION",
                     "resolved recursive body requires SCC summary analysis");
                return 0;
            }
        }
    }
    if (check->in_function_body &&
        function.kind != NL_CHECKED_REGISTERED_CALL &&
        function.kind != NL_CHECKED_PTR_FROM_REF &&
        function.kind != NL_CHECKED_REPLACE &&
        function.kind != NL_CHECKED_STORE && function.kind != NL_CHECKED_SWAP &&
        !(check->allocated_slice &&
          (function.kind == NL_CHECKED_INITIALIZE ||
           function.kind == NL_CHECKED_DESTROY ||
           function.kind == NL_CHECKED_DOMAIN_FINALIZE))) {
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
    if (function.custody_recipient) {
        /* Conditional definition is not caller authority. Prove source entry
         * before argument evaluation/consume; body replay and the finite
         * continuation separately prove transfer and final responsibility. */
        if (!custody_preflight(check, syntax, &function))
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
        (c->types[function.result - 1].view.field_count != 0 ||
         (c->types[function.result - 1].view.kind != NL_TYPE_NOMINAL &&
          c->types[function.result - 1].view.kind != NL_TYPE_BOOL &&
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
    if (function.custody_recipient) {
        check->artifact->destroy_custody_world = nl_semantic_destroy;
        check->artifact->custody_call_id = id;
        if (check->artifact->custody_entry != NULL) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                 "CUSTODY-CALL-PRECISION",
                 "one original custody transfer per policy world required");
            return 0;
        }
        if (!host(check, nl_sem_clone(c, &check->artifact->custody_entry),
                  syntax->span))
            return 0;
        const NLSyntaxView *a =
                               nl_syntax_node_view(syntax->data.call.arguments),
                           *b = nl_syntax_node_view(nl_syntax_next_argument(
                               syntax->data.call.arguments));
        const NLSymbolId sink = available(check, a->span),
                         donor = available(check, b->span);
        if (!sink || !donor)
            return 0;
        const NLReferenceFacts ref =
            c->values[c->bindings[sink - 1].view.value - 1].reference;
        view(check, id)->custody_call.entry_proved = true;
        view(check, id)->custody_call.origin = check->artifact->packet_parent;
        view(check, id)->custody_call.origin_match =
            check->artifact->packet_match;
        view(check, id)->custody_call.entry_world =
            check->artifact->custody_entry;
        view(check, id)->custody_call.world = check->artifact->context;
        view(check, id)->custody_call.sink = ref.place;
        view(check, id)->custody_call.sink_incarnation =
            c->places[ref.place - 1].incarnation;
        view(check, id)->custody_call.packet =
            c->bindings[donor - 1].view.value;
        view(check, id)->custody_call.donor = donor;
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
            !function.owner_receiver && !function.owner_producer &&
            !function.custody_recipient &&
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
        if (arguments[i] == 0)
            return 0;
        if (check->terminated) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                 "P9-OPERAND-TERMINATION",
                 "termination with pending call operands is outside P9");
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
    if (function.owner_producer &&
        !producer_entry(check, id, &function, arguments))
        return 0;
    if (function.owner_receiver &&
        !owner_entry(check, id, &function, arguments))
        return 0;
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
            const NLValueCarrier carrier = c->values[value - 1].carrier;
            bool forwarded = carrier == NL_CARRIER_PLACE ||
                             carrier == NL_CARRIER_SUM ||
                             carrier == NL_CARRIER_AGGREGATE;
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
static bool custody_type(const NLSemanticContext *c, NLTypeId type)
{
    const NLTypeId packet = c->types[type - 1].option_target;
    return packet != 0 && c->types[packet - 1].live_tail_target != 0;
}
static bool avs_type(const NLSemanticContext *c, NLTypeId type)
{
    if (c->types[type - 1].recursive_header && !c->types[type - 1].incomplete)
        return true; /* exactly completed declaration profile, not general
                        aggregate */
    const NLSemanticTypeView t = c->types[type - 1].view;
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    return t.kind == NL_TYPE_NOMINAL && t.field_count == 2 && t.is_copy &&
           t.is_discardable && c->types[type - 1].field_types[0] == u8 &&
           c->types[type - 1].field_types[1] == u8;
}

static NLCheckedNodeId aggregate(Check *, const NLSyntaxView *);
static NLCheckedNodeId sum_constructor(Check *, const NLSyntaxView *);
static NLCheckedNodeId sum_match(Check *, const NLSyntaxView *);
static NLCheckedNodeId source_if(Check *, const NLSyntaxView *);
static NLCheckedNodeId source_loop(Check *, const NLSyntaxView *);
static NLCheckedNodeId source_local_loan(Check *, const NLSyntaxView *);
static NLCheckedNodeId loan(Check *, const NLSyntaxView *);
static NLCheckedNodeId allocated_domain_loan(Check *, const NLSyntaxView *);
static NLCheckedNodeId source_control(Check *, const NLSyntaxView *);

static NLSymbolId field_binding(Check *check, NLSourceSpan name)
{
    for (size_t i = check->context->binding_count; i > check->namespace_floor;
         --i)
        if (!check->context->bindings[i - 1].hidden &&
            equal_name(check, name, check->context->bindings[i - 1].name))
            return i;
    return 0;
}

static bool fixed_selection(Check *check, const NLSyntaxView *s,
                            NLAccessSyntax access, NLCheckedField *out)
{
    NLSemanticContext *c = check->context;
    const NLSymbolId symbol =
        field_binding(check, s->data.field_designator.base);
    if (symbol == 0 || !nl_fixed_type(c, c->bindings[symbol - 1].view.type)) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span, "FIELD-PROFILE",
             "field base requires a direct Pair or completed recursive local");
        return false;
    }
    const NLSemanticBindingView b = c->bindings[symbol - 1].view;
    const NLSemanticPlaceView p = c->places[b.place - 1];
    if (b.availability != NL_AVAILABLE || !p.live || !p.implicit_local ||
        !p.independent_root || p.parent_aggregate != 0 || p.parent_sum != 0) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, s->span, "FIELD-STALE-BASE",
             "field base is not a current live direct lexical local");
        return false;
    }
    size_t index = p.fixed_field_count;
    for (size_t i = 0; i < p.fixed_field_count; ++i)
        if (equal_name(check, s->data.field_designator.field,
                       c->types[b.type - 1].field_names[i]))
            index = i;
    if (index == p.fixed_field_count) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, s->span, "FIELD-UNKNOWN-FIELD",
             "field is not declared by the selected nominal");
        return false;
    }
    if (nl_recursive_local_type(c, b.type) &&
        index >= c->types[b.type - 1].view.field_count - 1) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span,
             "NODE-LINK-FIELD-PROFILE",
             "bounded recursive field surface selects only its declared link");
        return false;
    }
    const NLPlaceId child = p.fixed_fields[index];
    if (!nl_fixed_live(c, child)) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, s->span, "FIELD-STALE-CHILD",
             "fixed field incarnation has ended");
        return false;
    }
    const NLSemanticPlaceView f = c->places[child - 1];
    *out = (NLCheckedField){.present = true,
                            .dependency_compatible = true,
                            .base = symbol,
                            .nominal = b.type,
                            .type = f.type,
                            .index = index,
                            .parent = b.place,
                            .child = child,
                            .parent_incarnation = p.incarnation,
                            .child_incarnation = f.incarnation,
                            .parent_fact = p.current_fact,
                            .child_fact = f.current_fact,
                            .access = access,
                            .old_value = f.current_value,
                            .payload_occurrence = f.payload_occurrence};
    return true;
}

/* The static projection key is (completed nominal, declaration index), never
 * a C offset. The current fixed-child place is its incarnation-specific site.
 * A ref base is selected before its field label and remains a separate route
 * from a lexical value read. No ptr, implicit dereference or mode promotion. */
static bool ref_field_selection(Check *check, const NLSyntaxView *s,
                                NLCheckedField *out, NLValueId *parent)
{
    NLSemanticContext *c = check->context;
    const NLSymbolId symbol = available(check, s->data.field_designator.base);
    if (symbol == 0)
        return false;
    const NLSemanticBindingView b = c->bindings[symbol - 1].view;
    const NLSemanticTypeView t = c->types[b.type - 1].view;
    if (!check->allocated_slice || t.kind != NL_TYPE_REF || t.is_exclusive ||
        !nl_recursive_local_type(c, t.target)) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span,
             "HEAP-LINK-REF-PROFILE",
             "projection requires an ordinary scoped completed H root ref");
        return false;
    }
    if (!concrete_ref(check, b.value, s->span) ||
        !reference_live(check, b.value, s->span))
        return false;
    const NLSemanticValueView v = c->values[b.value - 1];
    if (!host(check, nl_fixed_dependencies(c), s->span))
        return false;
    const NLSemanticPlaceView p = c->places[v.reference.place - 1];
    if (!p.independent_root || p.parent_aggregate != 0 || p.parent_sum != 0 ||
        p.placement.region == 0 || p.governing_domain == 0 ||
        !c->regions[p.placement.region - 1].view.live ||
        !c->domains[p.governing_domain - 1].live ||
        p.fixed_field_count != c->types[t.target - 1].view.field_count) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span,
             "HEAP-LINK-ROOT-PROFILE",
             "projection requires the current allocated H root");
        return false;
    }
    size_t index = p.fixed_field_count;
    for (size_t i = 0; i + 1 < p.fixed_field_count; ++i)
        if (equal_name(check, s->data.field_designator.field,
                       c->types[t.target - 1].field_names[i]))
            index = i;
    if (index == p.fixed_field_count) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span,
             "HEAP-LINK-FIELD-PROFILE",
             "projection selects only H's committed recursive link");
        return false;
    }
    const NLPlaceId child = p.fixed_fields[index];
    if (!nl_fixed_live(c, child)) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, s->span, "FIELD-STALE-CHILD",
             "projected fixed child incarnation has ended");
        return false;
    }
    const NLSemanticPlaceView f = c->places[child - 1];
    *parent = b.value;
    *out = (NLCheckedField){.present = true,
                            .dependency_compatible = true,
                            .base = symbol,
                            .nominal = t.target,
                            .type = f.type,
                            .index = index,
                            .parent = v.reference.place,
                            .child = child,
                            .parent_incarnation = p.incarnation,
                            .child_incarnation = f.incarnation,
                            .parent_fact = p.current_fact,
                            .child_fact = f.current_fact,
                            .access = t.access,
                            .old_value = f.current_value,
                            .payload_occurrence = f.payload_occurrence};
    return true;
}

static NLCheckedNodeId field_ref(Check *check, const NLSyntaxView *s)
{
    NLSemanticContext *c = check->context;
    NLCheckedField field;
    NLValueId parent;
    if (!ref_field_selection(check, s, &field, &parent) ||
        conflicts(check, field.child, false, false, parent, s->span))
        return 0;
    /* Copy exactly the parent's dependency evidence. Fixed projection changes
     * the referent site/type, not scope, permission or authority. Ordinary refs
     * do not implicitly depend on the mutable current ValueFact. */
    NLSemanticValueView projected = c->values[parent - 1];
    projected.type =
        compound(check, NL_TYPE_REF, field.type, field.access, false, s->span);
    if (projected.type == 0)
        return 0;
    projected.reference.place = field.child;
    projected.reference.incarnation = field.child_incarnation;
    const NLCheckedNodeId arg =
        binding_argument(check, field.base, 0, s->data.field_designator.base,
                         s->data.field_designator.base);
    if (arg == 0)
        return 0;
    const NLValueId value = new_value(check, projected, s->span);
    if (value == 0)
        return 0;
    end_temporary(check, one_result(check, arg));
    const NLSemanticPlaceView root = c->places[field.parent - 1];
    return add(check, (NLCheckedNodeView){
                          .kind = NL_CHECKED_FIELD_REF,
                          .span = s->span,
                          .type = projected.type,
                          .symbol = field.base,
                          .first_argument = arg,
                          .argument_count = 1,
                          .field = field,
                          .has_reference_result = true,
                          .reference_result = projected.reference,
                          .lifetime_place = field.parent,
                          .lifetime_incarnation = field.parent_incarnation,
                          .lifetime_domain = root.governing_domain,
                          .lifetime_range = root.placement,
                          .result_count = 1,
                          .results = {{projected.type, value}}});
}

static NLCheckedNodeId link_read(Check *check, const NLSyntaxView *s)
{
    const NLSyntaxView *operand = nl_syntax_node_view(s->data.call.arguments);
    if (s->data.call.argument_count != 1 || operand == NULL ||
        operand->kind != NL_SYNTAX_FIELD_DESIGNATOR) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span,
             "HEAP-LINK-READ-PROFILE", "read requires the bounded ref@link");
        return 0;
    }
    const NLCheckedNodeId projected = field_ref(check, operand);
    if (projected == 0)
        return 0;
    const NLCheckedNodeView ref = *view(check, projected);
    const NLTypeId expected = compound(check, NL_TYPE_REF, ref.field.type,
                                       NL_ACCESS_READ, false, s->span);
    if (expected == 0 || parameter_match(check, projected, expected) == 0)
        return 0;
    NLSemanticContext *c = check->context;
    NLValueId copy = 0;
    if (!host(check, nl_sem_copy_value(c, ref.field.old_value, &copy), s->span))
        return 0;
    end_temporary(check, ref.results[0].value);
    return add(check, (NLCheckedNodeView){.kind = NL_CHECKED_LINK_READ,
                                          .span = s->span,
                                          .type = ref.field.type,
                                          .first_argument = projected,
                                          .argument_count = 1,
                                          .field = ref.field,
                                          .value_use = NL_VALUE_COPIED,
                                          .result_count = 1,
                                          .results = {{ref.field.type, copy}}});
}

static NLCheckedNodeId field_read(Check *check, const NLSyntaxView *s)
{
    NLSemanticContext *c = check->context;
    const NLSymbolId base = field_binding(check, s->data.field_designator.base);
    if (check->allocated_slice && base != 0 &&
        c->types[c->bindings[base - 1].view.type - 1].view.kind == NL_TYPE_REF)
        return field_ref(check, s);
    NLCheckedField evidence;
    if (!fixed_selection(check, s, NL_ACCESS_READ, &evidence))
        return 0;
    if (conflicts(check, evidence.child, false, false, 0, s->span))
        return 0;
    NLValueId copy;
    if (!host(check, nl_sem_copy_value(c, evidence.old_value, &copy), s->span))
        return 0;
    const NLSemanticValueView v = c->values[copy - 1];
    return add(check,
               (NLCheckedNodeView){
                   .kind = NL_CHECKED_FIELD_READ,
                   .span = s->span,
                   .type = evidence.type,
                   .symbol = evidence.base,
                   .value_use = NL_VALUE_COPIED,
                   .field = evidence,
                   .result_count = 1,
                   .results = {{evidence.type, copy}},
                   .has_scalar_result = v.scalar_known,
                   .scalar_result = {v.type, v.scalar_known, v.scalar_value}});
}

/* The source payload is decimal mathematical-integer evidence, not a C
 * conversion. Keep the accumulator bounded by 255: arbitrarily long payloads
 * cannot overflow the host, truncate, or wrap. No signed/general numeric IR. */
static NLCheckedNodeId u8_literal(Check *check, const NLSyntaxView *syntax)
{
    NLSourceView digits;
    if (!nl_source_view(check->source, syntax->data.u8_digits, &digits) ||
        digits.length == 0) {
        fail(check, NL_CHECK_INTERNAL_ERROR, syntax->span, "P3-INTERNAL",
             "invalid u8 literal payload span");
        return 0;
    }
    size_t value = 0;
    for (size_t i = 0; i < digits.length; ++i) {
        const unsigned char digit = digits.bytes[i];
        if (digit < '0' || digit > '9') {
            fail(check, NL_CHECK_INTERNAL_ERROR, syntax->span, "P3-INTERNAL",
                 "expected decimal u8 payload");
            return 0;
        }
        const size_t n = digit - '0';
        if (value > (255 - n) / 10) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
                 "V1-U8-LITERAL-RANGE", "u8 literal is outside 0..255");
            return 0;
        }
        value = value * 10 + n;
    }
    const NLTypeId type = nl_semantic_core_type(check->context, NL_TYPE_U8);
    const NLValueId package = new_value(
        check,
        (NLSemanticValueView){
            .type = type, .scalar_known = true, .scalar_value = value},
        syntax->span);
    if (package == 0)
        return 0;
    return add(check,
               (NLCheckedNodeView){.kind = NL_CHECKED_U8_LITERAL,
                                   .span = syntax->span,
                                   .type = type,
                                   .result_count = 1,
                                   .results = {{type, package}},
                                   .has_scalar_result = true,
                                   .scalar_result = {type, true, value}});
}

static NLCheckedNodeId expression(Check *check, const NLSyntaxNode *syntax)
{
    /* Seed before any branch copies Check, so distinct syntactic trials in
     * sibling worlds share the finite source-profile counter. */
    if (check->allocation_budget == NULL)
        check->allocation_budget = &check->allocation_sites;
    const NLSyntaxView *const node = nl_syntax_node_view(syntax);
    if (!enter(check, node->span)) {
        return 0;
    }
    NLCheckedNodeId result = 0;
    if (check->in_function_body && node->kind != NL_SYNTAX_EXPR_NAME &&
        node->kind != NL_SYNTAX_EXPR_CALL && node->kind != NL_SYNTAX_BLOCK &&
        node->kind != NL_SYNTAX_SUM_CONSTRUCTOR &&
        node->kind != NL_SYNTAX_FIELD_DESIGNATOR &&
        node->kind != NL_SYNTAX_MATCH && node->kind != NL_SYNTAX_IF &&
        node->kind != NL_SYNTAX_LOOP && node->kind != NL_SYNTAX_U8_LITERAL &&
        node->kind != NL_SYNTAX_AGGREGATE &&
        node->kind != NL_SYNTAX_LOCAL_READ_LOAN &&
        node->kind != NL_SYNTAX_LOCAL_WRITE_LOAN &&
        node->kind != NL_SYNTAX_ALLOCATED_INTO_SLOT &&
        node->kind != NL_SYNTAX_ALLOCATED_ERASE_SLOT &&
        node->kind != NL_SYNTAX_ALLOCATED_REF &&
        node->kind != NL_SYNTAX_ALLOCATED_TRY) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, node->span,
             "P8-BODY-PROFILE",
             "expression needs a richer relative body analysis");
        --check->depth;
        return 0;
    }
    if (node->kind == NL_SYNTAX_MATCH || node->kind == NL_SYNTAX_IF ||
        node->kind == NL_SYNTAX_LOOP) {
        for (size_t i = 0; i < check->context->value_count; ++i)
            if (check->context->values[i].carrier != NL_CARRIER_ENDED &&
                check->context->values[i].dependencies ==
                    NL_EXACT_VALUE_DEPENDENCIES) {
                fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, node->span,
                     "FIELD-DEPENDENCY-CONTROL-PRECISION",
                     "exact ValueFact evidence is not admitted by this control "
                     "join");
                --check->depth;
                return 0;
            }
    }
    if (node->kind == NL_SYNTAX_EXPR_NAME) {
        result = equal_name(check, node->data.name, "unit")
                     ? add(check, (NLCheckedNodeView){.kind = NL_CHECKED_UNIT,
                                                      .span = node->span,
                                                      .type = 1})
                     : identifier(check, node);
    } else if (node->kind == NL_SYNTAX_U8_LITERAL) {
        result = u8_literal(check, node);
    } else if (node->kind == NL_SYNTAX_ALLOCATED_INTO_SLOT ||
               node->kind == NL_SYNTAX_ALLOCATED_ERASE_SLOT) {
        result = allocated_raw(check, node,
                               node->kind == NL_SYNTAX_ALLOCATED_INTO_SLOT
                                   ? NL_RAW_INTO_SLOT
                                   : NL_RAW_ERASE_SLOT);
    } else if (node->kind == NL_SYNTAX_ALLOCATED_REF) {
        result = allocated_ref(check, node);
    } else if (node->kind == NL_SYNTAX_ALLOCATED_TRY) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, node->span,
             "ALLOCATED-OUTCOME-PRECISION",
             "fallible allocation requires direct consuming outcome match in "
             "this slice");
    } else if (node->kind == NL_SYNTAX_EXPR_CALL) {
        result = call(check, node);
    } else if (node->kind == NL_SYNTAX_BLOCK) {
        result = source_block(check, node);
    } else if (node->kind == NL_SYNTAX_SUM_CONSTRUCTOR) {
        result = sum_constructor(check, node);
    } else if (node->kind == NL_SYNTAX_FIELD_DESIGNATOR) {
        result = field_read(check, node);
    } else if (node->kind == NL_SYNTAX_MATCH) {
        result = sum_match(check, node);
    } else if (node->kind == NL_SYNTAX_IF) {
        result = source_if(check, node);
    } else if (node->kind == NL_SYNTAX_LOOP) {
        result = source_loop(check, node);
    } else if (node->kind == NL_SYNTAX_LOCAL_READ_LOAN ||
               node->kind == NL_SYNTAX_LOCAL_WRITE_LOAN) {
        result = source_local_loan(check, node);
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
    if (!lexical_source_name(check, syntax->data.binding.name))
        return 0;
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
    if (!lexical_name(check, name, strlen(name), span))
        return false;
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
            if (entry->incomplete) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, name,
                     "REC-INCOMPLETE-TYPE",
                     "aggregate construction needs completion");
                return 0;
            }
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
    const bool live_tail =
        check->context->types[type - 1].live_tail_target != 0;
    const bool producer =
        check->body_function != 0 &&
        check->context->functions[check->body_function - 1].owner_producer;
    if (live_tail && (!producer || !check->allocated_slice)) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
             "P208-CONSTRUCTOR-CONTEXT",
             "LiveTail construction requires a proved producer call");
        return 0;
    }
    if (check->in_function_body && !avs_type(check->context, type) &&
        !live_tail &&
        !(check->allocated_slice &&
          nl_experimental_root_record_type(check->context, type))) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
             "P8-BODY-PROFILE",
             "function-body aggregate admission is bounded to two u8 fields");
        return 0;
    }
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
    if (live_tail) {
        const NLTypedOwnerDefinition d =
            c->functions[check->body_function - 1].body->owner_definition;
        check->status = nl_owner_relations(c, package.fields, &d, syntax->span,
                                           &check->diagnostic);
        if (check->status != NL_CHECK_OK)
            return 0;
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
        if (check->in_function_body && !avs_type(check->context, type) &&
            !(check->allocated_slice &&
              (check->context->types[type - 1].one_backing_target != 0 ||
               check->context->types[type - 1].live_tail_target != 0 ||
               nl_experimental_root_record_type(check->context, type)))) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
                 "P8-BODY-PROFILE",
                 "function-body destructuring is bounded to two u8 fields");
            return 0;
        }
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
    if (check->terminated) {
        result = init;
        goto cleanup;
    }
    NLCheckedNodeView rhs = *view(check, init);
    NLValueId values[NL_SEMANTIC_MAX_FIELDS] = {0};
    if (destructure) {
        if (rhs.result_count != 1 || rhs.results[0].type != type) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, init)->span,
                 "P5-DESTRUCTURE-TYPE",
                 "destructuring RHS must be the selected aggregate");
            goto cleanup;
        }
        if (check->context->types[type - 1].live_tail_target != 0) {
            const NLCheckedNodeView *join = nl_checked_node_view(
                check->artifact, check->artifact->packet_match);
            if (join != NULL && join->packet_fork.retained &&
                !nl_packet_receiving_after_join(check->artifact, check->context,
                                                rhs.results[0].value)) {
                fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                     "P222-RECEIVING-ORIGIN",
                     "post-join receiving requires the unchanged original "
                     "packet, root, backing and domain");
                goto cleanup;
            }
            const NLCheckedNodeId source = check->artifact->producer_call;
            const bool local =
                source != 0 && view(check, source)->producer.return_proved &&
                view(check, source)->producer.result == rhs.results[0].value;
            const bool extracted =
                check->custody_continuation && check->custody_extracted &&
                rhs.results[0].value == check->custody_packet &&
                rhs.results[0].value == check->custody_saved;
            if (!local && !extracted &&
                !nl_packet_inherited(check->artifact, rhs.results[0].value)) {
                fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                     "P208-DESTRUCTURE-ORIGIN",
                     "whole LiveTail receiving requires checked producer "
                     "return or qualified parent origin");
                goto cleanup;
            }
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
    if (destructure && check->artifact->packet_parent != NULL &&
        check->context->types[type - 1].live_tail_target != 0) {
        view(check, result)->packet_origin.ancestor =
            check->artifact->packet_parent;
        view(check, result)->packet_origin.match =
            check->artifact->packet_match;
        view(check, result)->packet_origin.world = check->artifact->context;
        view(check, result)->packet_origin.entry_world =
            check->artifact->packet_entry;
        view(check, result)->packet_origin.packet = rhs.results[0].value;
    }
    if (destructure && check->artifact->packet_retained_post != NULL &&
        check->context->types[type - 1].live_tail_target != 0) {
        const NLCheckedNodeView *join = nl_checked_node_view(
            check->artifact, check->artifact->packet_match);
        if (join != NULL && join->packet_fork.retained) {
            view(check, result)->packet_origin.ancestor = check->artifact;
            view(check, result)->packet_origin.match =
                check->artifact->packet_match;
            view(check, result)->packet_origin.world = check->artifact->context;
            view(check, result)->packet_origin.entry_world =
                check->artifact->packet_retained_post;
            view(check, result)->packet_origin.packet = rhs.results[0].value;
        }
    }
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
        if (!multi && !destructure &&
            (check->context->values[values[i] - 1].type ==
                 nl_semantic_core_type(check->context, NL_TYPE_U8) ||
             nl_fixed_type(check->context,
                           check->context->values[values[i] - 1].type) ||
             (check->allocated_slice &&
              (check->context->values[values[i] - 1].type == 2 ||
               custody_type(check->context,
                            check->context->values[values[i] - 1].type)))))
            check->context
                ->places[check->context->bindings[symbol - 1].view.place - 1]
                .implicit_local = true;
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
    if (init == 0)
        return 0;
    if (check->terminated)
        return init;
    if (!discard_results(check, init))
        return 0;
    return add(check, (NLCheckedNodeView){.kind = NL_CHECKED_STATEMENT,
                                          .span = syntax->span,
                                          .initializer = init,
                                          .type = 1});
}

static bool end_bindings(Check *check, size_t floor, NLSourceSpan span)
{
    NLSemanticContext *c = check->context;
    for (size_t i = floor; i < c->binding_count; ++i) {
        NLBindingEntry *entry = &c->bindings[i];
        if (entry->hidden)
            continue;
        if (entry->view.availability == NL_AVAILABLE) {
            if (!c->types[entry->view.type - 1].view.is_discardable) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, span,
                     "P5-SCOPE-OBLIGATION",
                     "non-Discardable local remains available at block exit");
                return false;
            }
            const NLCheckStatus ending =
                nl_fixed_end_dependencies(c, entry->view.place, floor);
            if (ending != NL_CHECK_OK) {
                fail(check, ending, span, "FIELD-END-DEPENDENCY",
                     "surviving value depends on ending local");
                return false;
            }
            if (conflicts(check, entry->view.place, false, true, 0, span))
                return false;
            nl_sum_detach(c, entry->view.place);
            nl_fixed_detach(c, entry->view.place);
            end_temporary(check, entry->view.value);
            c->places[entry->view.place - 1].live = false;
            c->places[entry->view.place - 1].current_value = 0;
            c->places[entry->view.place - 1].current_fact = 0;
            c->places[entry->view.place - 1].governing_domain = 0;
            entry->view.availability = NL_CONSUMED;
        }
        entry->hidden = true;
    }
    return true;
}

/* The one common return boundary; unit has no package, not a synthetic result.
 */
static NLCheckedNodeId source_return(Check *check, const NLSyntaxView *syntax)
{
    if (!check->in_function_body || check->in_source_loan) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
             "P9-RETURN-CONTEXT",
             "return requires a registered ordinary function body");
        return 0;
    }
    NLCheckedNodeId expression_id =
        expression(check, syntax->data.statement.expression);
    if (expression_id == 0 || check->terminated)
        return expression_id;
    const NLCheckedNodeView value = *view(check, expression_id);
    if (value.type != 0 &&
        check->context->types[value.type - 1].view.kind == NL_TYPE_REF) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
             "P9-REF-RETURN",
             "ordinary ref cannot escape as a function result");
        return 0;
    }
    if (value.type != check->function_result || value.result_count > 1 ||
        (value.result_count == 0 && value.type != 1)) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span, "P9-RETURN-TYPE",
             "return expression must exactly match the function result");
        return 0;
    }
    if (!end_bindings(check, check->function_binding_floor, syntax->span))
        return 0;
    NLCheckStatus status =
        nl_sem_function_exit(check->context, check->function_scope_floor,
                             check->function_place_floor);
    if (status != NL_CHECK_OK) {
        fail(check, status, syntax->span, "P9-RETURN-DEPENDENCY",
             "return state depends on an ending function-local scope/place");
        return 0;
    }
    check->returned = value.result_count == 0 ? (NLCheckedResult){.type = 1}
                                              : value.results[0];
    NLControlState *post = NULL;
    if (!host(check,
              nl_control_state_capture(check->loop == NULL ? NULL
                                                           : check->loop->entry,
                                       check->context, &post),
              syntax->span))
        return 0;
    const NLCheckStatus recorded = nl_control_exit_append(
        &check->artifact->exits, NL_EXIT_RETURN, check->function_target, post,
        &check->returned, 1);
    nl_control_state_destroy(post);
    if (!host(check, recorded, syntax->span))
        return 0;
    check->terminated = true;
    return add(check, (NLCheckedNodeView){.kind = NL_CHECKED_RETURN,
                                          .span = syntax->span,
                                          .initializer = expression_id,
                                          .terminates = true,
                                          .returned = check->returned});
}

static bool custody_continue_block(Check *, const NLSyntaxView *,
                                   const NLSyntaxNode *, size_t,
                                   NLCheckedNodeId);
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
            v->kind != NL_SYNTAX_AGGREGATE_BINDING &&
            v->kind != NL_SYNTAX_STATEMENT && v->kind != NL_SYNTAX_RETURN &&
            v->kind != NL_SYNTAX_CONTINUE && v->kind != NL_SYNTAX_BREAK) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, v->span,
                 "P8-BODY-PROFILE",
                 "body receiving pattern needs richer relative analysis");
            goto cleanup;
        }
        const NLCheckedNodeId child =
            v->kind == NL_SYNTAX_CONTINUE || v->kind == NL_SYNTAX_BREAK
                ? source_control(check, v)
            : v->kind == NL_SYNTAX_RETURN    ? source_return(check, v)
            : v->kind == NL_SYNTAX_STATEMENT ? source_statement(check, v)
                                             : source_binding(check, v);
        if (child == 0)
            goto cleanup;
        if (previous == 0)
            view(check, id)->first_item = child;
        else
            view(check, previous)->next_item = child;
        previous = child;
        ++view(check, id)->item_count;
        if (check->custody_pending != 0) {
            if (!custody_continue_block(
                    check, syntax, nl_syntax_next_argument(item), floor, id))
                goto cleanup;
            goto cleanup;
        }
        if (check->terminated)
            break;
    }
    if (!check->terminated && syntax->data.block.tail != NULL) {
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
    if (!check->terminated && !end_bindings(check, floor, syntax->span))
        goto cleanup;
    if (check->terminated) {
        view(check, id)->type = 0;
        view(check, id)->result_count = 0;
        view(check, id)->terminates = true;
        view(check, id)->returned = check->returned;
    }
cleanup:
    check->binding_floor = previous_floor;
    return check->status == NL_CHECK_OK ? id : 0;
}

/* A finite alternative can combine divergence with real Return edges. An
 * absent compatibility cache is not an empty exit set or a fake unit return.
 * This bounded summary requires flat Copy output and an exact caller frame. */
static bool zero_normal_returns(Check *check, NLTypeId declared,
                                NLCheckedNodeId body, NLSourceSpan span);

static bool body_result(Check *check, NLTypeId declared, NLCheckedNodeId body,
                        NLSourceSpan span)
{
    const NLCheckedNodeView result = *view(check, body);
    for (size_t i = 0; i < check->context->type_count; ++i)
        if (check->context->types[i].recursive_header &&
            check->context->types[i].view.field_count == 4 &&
            (check->allocation_budget == NULL ||
             *check->allocation_budget != 5)) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, span,
                 "FIVE-ROOT-SOURCE-PROFILE",
                 "three-link main requires exactly five nested allocation "
                 "sites");
            return false;
        }
    if (!host(check,
              nl_control_function_boundary(
                  check->artifact->exits, check->function_target, declared,
                  check->function_scope_floor, check->function_place_floor),
              span))
        return false;
    if (check->terminated) {
        if (nl_control_exits_count(check->artifact->exits) == 0)
            return true;
        if (check->returned.type == declared)
            return true;
        return zero_normal_returns(check, declared, body, span);
    }
    if (result.type != declared || result.result_count > 1 ||
        (result.result_count == 0 && declared != 1)) {
        fail(
            check, NL_CHECK_SEMANTIC_ERROR, span, "P8-BODY-RESULT",
            "normal body result must exactly match the declared single result");
        return false;
    }
    return true;
}

/* Both registered plans and trusted host-known body fixtures use the same
 * source control/result/exit path. Call-site publication remains transactional.
 */
static bool join_if_function_exits(Check *);

static bool function_block(Check *check, const NLSyntaxView *syntax)
{
    if (!host(check,
              nl_control_target_create(NL_TARGET_FUNCTION,
                                       &check->artifact->function_target),
              syntax->span))
        return false;
    check->function_target = check->artifact->function_target;
    check->function_value_prefix = check->context->value_count;
    check->function_fact_prefix = check->context->last_value_fact;
    check->artifact->root = source_block(check, syntax);
    if (check->artifact->root == 0 ||
        !body_result(check, check->function_result, check->artifact->root,
                     syntax->span))
        return false;
    /* Divergence has no function-exit post-state to validate or clean up. */
    if (check->terminated &&
        nl_control_exits_count(check->artifact->exits) == 0)
        return true;
    const NLCheckStatus status =
        nl_sem_function_exit(check->context, check->function_scope_floor,
                             check->function_place_floor);
    if (status != NL_CHECK_OK) {
        fail(check, status, syntax->span, "P8-EXIT-DEPENDENCY",
             "surviving state depends on an ending function-local scope/place");
        return false;
    }
    if (!join_if_function_exits(check))
        return false;
    for (size_t i = check->function_scope_floor;
         i < check->context->scope_count; ++i)
        check->context->scopes[i].active = false;
    return true;
}

NLCheckStatus nl_sem_check_function_block(NLSemanticContext *context,
                                          const NLSyntaxTree *tree,
                                          NLFunctionBoundary boundary,
                                          NLCheckedFragment **out,
                                          NLCheckDiagnostic *diagnostic)
{
    if (context == NULL || tree == NULL || out == NULL || *out != NULL ||
        boundary.result == 0 || boundary.result > context->type_count ||
        boundary.bindings > context->binding_count ||
        boundary.scopes > context->scope_count ||
        boundary.places > context->place_count)
        return NL_CHECK_INTERNAL_ERROR;
    const NLSyntaxView *syntax = nl_syntax_node_view(nl_syntax_tree_root(tree));
    if (syntax == NULL || syntax->kind != NL_SYNTAX_BLOCK)
        return NL_CHECK_INTERNAL_ERROR;
    Check check = {.source = nl_syntax_tree_source(tree),
                   .in_function_body = true,
                   .function_result = boundary.result,
                   .binding_floor = boundary.bindings,
                   .namespace_floor = boundary.bindings,
                   .arm_floor = boundary.bindings,
                   .has_arm_floor = true,
                   .function_binding_floor = boundary.bindings,
                   .function_scope_floor = boundary.scopes,
                   .function_place_floor = boundary.places};
    if (!host(&check, nl_sem_clone(context, &check.context), syntax->span))
        goto failure;
    check.artifact = malloc(sizeof(*check.artifact));
    if (check.artifact == NULL) {
        (void)host(&check, NL_CHECK_OUT_OF_MEMORY, syntax->span);
        goto failure;
    }
    *check.artifact =
        (NLCheckedFragment){.source = check.source, .context = context};
    if (!function_block(&check, syntax) ||
        !host(&check, nl_sem_validate(check.context), syntax->span) ||
        !host(&check, nl_raw_validate(check.context), syntax->span))
        goto failure;
    nl_sem_commit(context, check.context);
    *out = check.artifact;
    return NL_CHECK_OK;
failure:
    nl_semantic_destroy(check.context);
    nl_checked_destroy(check.artifact);
    if (diagnostic != NULL)
        *diagnostic = check.diagnostic;
    return check.status;
}

static bool run_body(Check *caller, NLCheckedNodeId call_id,
                     const NLFunctionEntry *function,
                     const NLCheckedNodeId *arguments)
{
    if (caller->branch_budget == NULL)
        caller->branch_budget = &caller->branch_steps;
    if (caller->body_budget == NULL)
        caller->body_budget = &caller->body_steps;
    if (*caller->body_budget >= NL_SEMANTIC_MAX_ENTRIES) {
        fail(caller, NL_CHECK_RESOURCE_LIMIT, view(caller, call_id)->span,
             "P11-BODY-WORK-LIMIT", "bounded body-call work budget exceeded");
        return false;
    }
    ++*caller->body_budget;
    NLSemanticContext *c = caller->context;
    const size_t binding_floor = c->binding_count, scope_floor = c->scope_count,
                 place_floor = c->place_count;
    const NLSourceSpan call_span = view(caller, call_id)->span;
    Check body = {.caller = caller,
                  .body_budget = caller->body_budget,
                  .branch_budget = caller->branch_budget,
                  .depth = caller->depth,
                  .definition = caller->definition,
                  .closure_probe = caller->closure_probe,
                  .context = c,
                  .source = function->body->source,
                  .binding_floor = binding_floor,
                  .namespace_floor = binding_floor,
                  .arm_floor = binding_floor,
                  .has_arm_floor = true,
                  .in_function_body = true,
                  .function_result = function->result,
                  .function_binding_floor = binding_floor,
                  .function_scope_floor = scope_floor,
                  .function_place_floor = place_floor,
                  .body_function = view(caller, call_id)->function};
    body.custody_recipient_body = function->custody_recipient;
    if (function->custody_recipient) {
        body.custody_origin = view(caller, call_id)->custody_call.origin;
        body.custody_origin_match =
            view(caller, call_id)->custody_call.origin_match;
        body.custody_packet = view(caller, call_id)->custody_call.packet;
        body.custody_sink = view(caller, call_id)->custody_call.sink;
    }
    body.allocated_slice = function->custody_recipient ||
                           (function->owner_receiver &&
                            view(caller, call_id)->owner_call.entry_proved) ||
                           (function->owner_producer &&
                            view(caller, call_id)->producer.entry_proved);
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
        if (function->owner_producer) {
            view(caller, call_id)->producer.parameters[i] = symbol;
            if (i == 3)
                c->places[c->bindings[symbol - 1].view.place - 1]
                    .implicit_local = true;
        }
        if (function->custody_recipient)
            view(caller, call_id)->custody_call.parameters[i] = symbol;
        if (function->owner_receiver) {
            view(caller, call_id)->owner_call.parameters[i] = symbol;
            if (i == 2)
                c->places[c->bindings[symbol - 1].view.place - 1]
                    .implicit_local = true;
        }
    }
    if (!function_block(&body, nl_syntax_node_view(nl_syntax_tree_root(
                                   function->body->syntax))))
        goto failure;
    if (function->owner_producer) {
        const NLCheckedNodeView proof = *view(caller, call_id);
        const NLCheckedResult result =
            body.terminated ? body.returned
                            : view(&body, body.artifact->root)->results[0];
        const NLSemanticValueView package = result.value == 0
                                                ? (NLSemanticValueView){0}
                                                : c->values[result.value - 1];
        const NLSemanticPlaceView root = c->places[proof.producer.root - 1],
                                  head =
                                      c->places[proof.producer.head.parent - 1],
                                  link =
                                      c->places[proof.producer.head.child - 1];
        bool preserved =
            result.type == function->result && package.field_count == 3 &&
            root.live && root.incarnation == proof.producer.incarnation &&
            root.placement.region == proof.producer.range.region &&
            root.placement.start == proof.producer.range.start &&
            root.placement.length == proof.producer.range.length &&
            root.governing_domain == proof.producer.domain &&
            c->regions[proof.producer.range.region - 1].view.live &&
            c->domains[proof.producer.domain - 1].live && head.live &&
            head.incarnation == proof.producer.head.parent_incarnation &&
            head.governing_domain == proof.producer.head_domain && link.live &&
            link.incarnation == proof.producer.head.child_incarnation &&
            link.current_fact != proof.producer.head_before_fact &&
            link.current_value != 0 &&
            c->values[link.current_value - 1].variant == 1 &&
            link.payload_occurrence == 0 &&
            c->values[proof.producer.head_before - 1].carrier ==
                NL_CARRIER_ENDED &&
            package.dependencies == NL_DEPENDENCY_FREE &&
            package.value_dependency_count == 0;
        for (size_t i = 0; i < 3 && preserved; ++i) {
            const NLSemanticValueView member = c->values[package.fields[i] - 1];
            preserved = member.carrier == NL_CARRIER_AGGREGATE &&
                        member.aggregate_owner == result.value &&
                        member.dependencies == NL_DEPENDENCY_FREE &&
                        member.value_dependency_count == 0;
            if (i == 0)
                preserved =
                    preserved && member.reference_count == 0 &&
                    member.reference.provenance == NL_PROVENANCE_VALID &&
                    member.reference.place == proof.producer.root &&
                    member.reference.incarnation == proof.producer.incarnation;
            else
                preserved = preserved &&
                            package.fields[i] == proof.producer.inputs[i + 1] &&
                            c->bindings[proof.producer.donor[i + 1] - 1]
                                    .view.availability == NL_CONSUMED &&
                            c->bindings[proof.producer.parameters[i + 1] - 1]
                                    .view.availability == NL_CONSUMED;
        }
        if (!preserved) {
            fail(&body, NL_CHECK_ANALYSIS_PRECISION_LIMIT, (NLSourceSpan){0},
                 "P208-CALL-RETURN",
                 "exact original live tail/head/result relation unproved");
            goto failure;
        }
        if (!host(&body, nl_sem_clone(c, &caller->artifact->producer_return),
                  call_span))
            goto failure;
        view(caller, call_id)->producer.result = result.value;
        view(caller, call_id)->producer.head_after = link.current_value;
        view(caller, call_id)->producer.head_after_fact = link.current_fact;
        view(caller, call_id)->producer.return_proved = true;
        view(caller, call_id)->producer.return_world =
            caller->artifact->producer_return;
    }
    if (function->owner_receiver) {
        const NLCheckedNodeView proof = *view(caller, call_id);
        const NLSemanticPlaceView root = c->places[proof.owner_call.root - 1];
        bool closed =
            !root.live && root.current_value == 0 && root.current_fact == 0 &&
            root.placement.region == 0 && root.governing_domain == 0 &&
            root.incarnation == proof.owner_call.incarnation &&
            !c->regions[proof.owner_call.range.region - 1].view.live &&
            !c->domains[proof.owner_call.domain - 1].live;
        for (size_t i = 1; i < 3; ++i)
            closed =
                closed &&
                c->values[proof.owner_call.inputs[i] - 1].carrier ==
                    NL_CARRIER_ENDED &&
                c->bindings[proof.owner_call.donor[i] - 1].view.availability ==
                    NL_CONSUMED &&
                c->bindings[proof.owner_call.parameters[i] - 1]
                        .view.availability == NL_CONSUMED;
        if (!closed) {
            fail(&body, NL_CHECK_SEMANTIC_ERROR, (NLSourceSpan){0},
                 "P193-CALL-POSTSTATE",
                 "receiver did not establish exact closed post-state");
            goto failure;
        }
        view(caller, call_id)->owner_call.post_proved = true;
    }
    if (function->custody_recipient) {
        const NLCheckedNodeView proof = *view(caller, call_id);
        const NLSemanticPlaceView sink = c->places[proof.custody_call.sink - 1];
        const NLSemanticValueView some = c->values[sink.current_value - 1];
        const NLCheckedNodeView *producer = nl_checked_node_view(
            proof.custody_call.origin,
            nl_checked_node_view(proof.custody_call.origin,
                                 proof.custody_call.origin_match)
                ->packet_fork.producer);
        bool valid =
            sink.live &&
            sink.incarnation == proof.custody_call.sink_incarnation &&
            some.variant == 2 &&
            some.sum_payload == proof.custody_call.packet &&
            sink.payload_occurrence != 0 && body.custody_old_none != 0 &&
            c->values[body.custody_old_none - 1].carrier == NL_CARRIER_ENDED &&
            c->bindings[proof.custody_call.donor - 1].view.availability ==
                NL_CONSUMED &&
            c->bindings[proof.custody_call.parameters[1] - 1]
                    .view.availability == NL_CONSUMED &&
            c->places[producer->producer.root - 1].live &&
            c->places[producer->producer.root - 1].incarnation ==
                producer->producer.incarnation &&
            c->places[producer->producer.root - 1].governing_domain ==
                producer->producer.domain &&
            c->regions[producer->producer.range.region - 1].view.live &&
            c->domains[producer->producer.domain - 1].live;
        if (!valid) {
            fail(&body, NL_CHECK_ANALYSIS_PRECISION_LIMIT, (NLSourceSpan){0},
                 "CUSTODY-CALL-POST",
                 "original owner/None/occurrence post-state not proved");
            goto failure;
        }
        if (!host(&body, nl_sem_clone(c, &caller->artifact->custody_post),
                  call_span))
            goto failure;
        view(caller, call_id)->custody_call.post_proved = true;
        view(caller, call_id)->custody_call.post_world =
            caller->artifact->custody_post;
        view(caller, call_id)->custody_call.old_none = body.custody_old_none;
        view(caller, call_id)->custody_call.new_some = sink.current_value;
        view(caller, call_id)->custody_call.occurrence =
            sink.payload_occurrence;
    }
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
    const bool no_exit =
        body.terminated && nl_control_exits_count(body.artifact->exits) == 0;
    if (no_exit) {
        caller->terminated = true;
        caller->returned = (NLCheckedResult){0};
        view(caller, call_id)->terminates = true;
    }
    view(caller, call_id)->type = no_exit ? 0 : function->result;
    view(caller, call_id)->result_count =
        body.terminated ? (body.returned.value != 0 ? 1 : 0)
                        : result.result_count;
    if (body.terminated)
        view(caller, call_id)->results[0] = body.returned;
    else
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
    if (s->data.constructor.type != NULL) {
        type = check_type(check, s->data.constructor.type);
        if (type == 0)
            return 0;
        const NLTypeId ptr = c->types[type - 1].option_target;
        const bool custody =
            ptr != 0 && c->types[ptr - 1].live_tail_target != 0;
        if (custody &&
            (!check->allocated_slice ||
             (!equal_name(check, s->data.constructor.variant, "None") &&
              !check->custody_recipient_body))) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, s->span,
                 "CUSTODY-CONSTRUCTION-PRECISION",
                 "custody construction requires world-qualified original "
                 "packet transfer evidence");
            return 0;
        }
        if (ptr == 0 ||
            (!custody &&
             c->types[c->types[ptr - 1].view.target - 1].incomplete)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, s->span, "REC-INCOMPLETE-TYPE",
                 "Option value construction needs header completion");
            return 0;
        }
    }
    for (size_t i = 0; type == 0 && i < c->type_count; ++i)
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
        if (check->terminated) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, s->span,
                 "P9-OPERAND-TERMINATION",
                 "termination with a pending constructor is outside P9");
            return 0;
        }
        if (init == 0 || (payload = one_result(check, init)) == 0)
            return 0;
        if (c->values[payload - 1].type != payload_type) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, view(check, init)->span,
                 "P6-PAYLOAD-TYPE",
                 "constructor payload type differs from registered variant");
            return 0;
        }
    }
    if (check->custody_recipient_body && custody_type(c, type) &&
        (payload != check->custody_packet || variant != 2)) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, s->span,
             "CUSTODY-PACKET-TRANSFER",
             "Some must contain the original checked packet");
        return 0;
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
           (t.kind == NL_TYPE_NOMINAL || t.kind == NL_TYPE_BOOL ||
            t.kind == NL_TYPE_UNIT || t.kind == NL_TYPE_BYTE ||
            t.kind == NL_TYPE_U8 || t.kind == NL_TYPE_USIZE ||
            t.kind == NL_TYPE_ADDR);
}

static void match_precision(Check *check, NLSourceSpan span)
{
    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span, "P6-JOIN-PRECISION",
         "branch post-state/result needs a richer relational join than P6 "
         "supports");
}

static bool same_place_frame(NLSemanticPlaceView a, NLSemanticPlaceView b)
{
    return nl_fixed_frame_same(a, b) && a.type == b.type && a.live == b.live &&
           a.incarnation == b.incarnation &&
           a.independent_root == b.independent_root &&
           a.parent_sum == b.parent_sum &&
           a.governing_domain == b.governing_domain &&
           a.implicit_local == b.implicit_local &&
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
           (target.kind == NL_TYPE_NOMINAL || target.kind == NL_TYPE_BOOL ||
            target.kind == NL_TYPE_BYTE || target.kind == NL_TYPE_U8 ||
            target.kind == NL_TYPE_USIZE || target.kind == NL_TYPE_ADDR);
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

/* Finite IF joins reuse P6 frame/Copy widening and P7 complete ref facts.
 * Every branch is an owned hypothetical context. Public state is constructed
 * only from stable prefix identities or a proved sole continuation. */
static void if_precision(Check *check, NLSourceSpan span)
{
    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span, "P13-JOIN-PRECISION",
         "conditional package identity or correlated state is not "
         "representable");
}

static bool widen_place(Check *check, size_t index, NLSourceSpan span)
{
    NLSemanticContext *c = check->context;
    const NLSemanticPlaceView old = c->places[index];
    NLValueId next =
        new_value(check, (NLSemanticValueView){.type = old.type}, span);
    if (next == 0 ||
        !host(check, nl_sem_fresh_fact(c, &c->places[index].current_fact),
              span))
        return false;
    nl_sem_end_value(c, old.current_value);
    c->places[index].current_value = next;
    c->values[next - 1].carrier = NL_CARRIER_PLACE;
    c->values[next - 1].owner_place = index + 1;
    refresh_bindings(check, index + 1);
    return true;
}

static NLCheckedNodeId source_if(Check *check, const NLSyntaxView *syntax)
{
    if (check->branch_budget == NULL)
        check->branch_budget = &check->branch_steps;
    if (*check->branch_budget >= NL_SEMANTIC_MAX_ENTRIES) {
        fail(check, NL_CHECK_RESOURCE_LIMIT, syntax->span,
             "P13-BRANCH-WORK-LIMIT",
             "finite branch checking/replay work budget exceeded");
        return 0;
    }
    ++*check->branch_budget;
    NLCheckedNodeId condition =
        expression(check, syntax->data.conditional.condition);
    if (condition == 0)
        return 0;
    const NLCheckedNodeView cond = *view(check, condition);
    if (check->terminated || cond.result_count != 1 ||
        cond.type != nl_semantic_core_type(check->context, NL_TYPE_BOOL)) {
        fail(check, NL_CHECK_SEMANTIC_ERROR,
             nl_syntax_node_view(syntax->data.conditional.condition)->span,
             "P13-IF-CONDITION",
             "if condition requires one exact core bool result");
        return 0;
    }
    end_temporary(check, cond.results[0].value);
    NLCheckedNodeId id =
        add(check, (NLCheckedNodeView){.kind = NL_CHECKED_IF,
                                       .span = syntax->span,
                                       .initializer = condition,
                                       .item_count = 2});
    if (id == 0)
        return 0;
    NLSemanticContext *c = check->context;
    const NLSyntaxNode *arms[] = {syntax->data.conditional.then_block,
                                  syntax->data.conditional.else_block};
    NLCheckedFragment *evidence[2] = {NULL, NULL};
    NLCheckedNodeView outcomes[2] = {{0}, {0}};
    size_t normal = 0, sole = 0;
    for (size_t i = 0; i < 2; ++i) {
        Check branch = *check;
        branch.context = NULL;
        branch.artifact = NULL;
        if (!host(&branch, nl_sem_clone(c, &branch.context), syntax->span))
            goto failure;
        branch.artifact = malloc(sizeof(*branch.artifact));
        if (branch.artifact == NULL) {
            (void)host(&branch, NL_CHECK_OUT_OF_MEMORY, syntax->span);
            goto failure;
        }
        *branch.artifact =
            (NLCheckedFragment){.source = check->source,
                                .context = branch.context,
                                .destroy_context = nl_semantic_destroy,
                                .branch_value_prefix = c->value_count,
                                .branch_fact_prefix = c->last_value_fact};
        NLCheckedNodeId body = expression(&branch, arms[i]);
        if (body == 0)
            goto failure;
        outcomes[i] = *view(&branch, body);
        if (!branch.terminated) {
            ++normal;
            sole = i;
        }
        branch.artifact->root =
            add(&branch,
                (NLCheckedNodeView){
                    .kind = NL_CHECKED_IF_ARM,
                    .span = nl_syntax_node_view(arms[i])->span,
                    .initializer = body,
                    .type = outcomes[i].type,
                    .result_count = outcomes[i].result_count,
                    .results = {outcomes[i].results[0], outcomes[i].results[1]},
                    .terminates = branch.terminated,
                    .returned = branch.returned});
        if (branch.artifact->root == 0 ||
            !save_arm(check, id, branch.artifact, syntax->span))
            goto failure;
        evidence[i] = branch.artifact;
        continue;
    failure:
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
    for (size_t i = 0; i < 2; ++i)
        if (!host(check,
                  nl_control_exits_union(&check->artifact->exits,
                                         evidence[i]->exits),
                  syntax->span))
            return 0;
    for (size_t i = 0; i < 2; ++i)
        if (!host(check,
                  nl_control_exits_union(&check->artifact->loop_returns,
                                         evidence[i]->loop_returns),
                  syntax->span))
            return 0;
    view(check, id)->normal_arms = normal;
    if (normal == 0 && !(nl_control_exits_only_return(evidence[0]->exits,
                                                      check->function_target) &&
                         nl_control_exits_only_return(
                             evidence[1]->exits, check->function_target))) {
        check->terminated = true;
        check->returned = (NLCheckedResult){0};
        view(check, id)->terminates = true;
        return id;
    }
    if (normal == 1) {
        /* Reapply the ONLY normal edge against candidate IDs. This is not a
         * known-condition choice: both arms and all return exits were checked.
         * Early-return evidence is also joined at the enclosing function exit.
         */
        /* Replay establishes only the public normal state. Its exit proof was
         * already retained from both forked arms; do not duplicate that set. */
        NLControlExits *proof = check->artifact->exits;
        NLControlExits *loop_proof = check->artifact->loop_returns;
        check->artifact->exits = NULL;
        check->artifact->loop_returns = NULL;
        NLCheckedNodeId body = expression(check, arms[sole]);
        nl_control_exits_destroy(check->artifact->exits);
        check->artifact->exits = proof;
        nl_control_exits_destroy(check->artifact->loop_returns);
        check->artifact->loop_returns = loop_proof;
        if (body == 0)
            return 0;
        const NLCheckedNodeView result = *view(check, body);
        view(check, id)->tail = body;
        view(check, id)->type = result.type;
        view(check, id)->result_count = result.result_count;
        memcpy(view(check, id)->results, result.results,
               sizeof(result.results));
        return id;
    }
    const bool returning = normal == 0;
    const NLSemanticContext *post[2] = {evidence[0]->context,
                                        evidence[1]->context};
    NLCheckedResult results[2];
    size_t counts[2];
    NLTypeId types[2];
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *arm =
            nl_checked_node_view(evidence[i], evidence[i]->root);
        results[i] = returning ? arm->returned : outcomes[i].results[0];
        counts[i] = returning ? (results[i].value != 0 ? 1 : 0)
                              : outcomes[i].result_count;
        types[i] = returning ? results[i].type : outcomes[i].type;
    }
    for (size_t i = 0; i < 2; ++i)
        if (types[i] == 1 && counts[i] <= 1) {
            counts[i] = 0;
            results[i].value = 0;
        }
    if (types[0] > c->type_count || types[1] > c->type_count) {
        if_precision(check, syntax->span);
        return 0;
    }
    if (counts[0] > 1 || counts[1] > 1 || counts[0] != counts[1] ||
        types[0] != types[1] || (counts[0] == 0 && types[0] != 1)) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span, "P13-IF-RESULT",
             "normal if arms require the same single result type");
        return 0;
    }
    for (size_t j = 0; j < c->binding_count; ++j)
        if (!c->types[c->bindings[j].view.type - 1].view.is_copy &&
            post[0]->bindings[j].view.availability !=
                post[1]->bindings[j].view.availability) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
                 "P6-AVAILABILITY-JOIN",
                 "outer non-Copy availability differs between normal arms");
            return 0;
        }
    bool changed[NL_SEMANTIC_MAX_ENTRIES] = {false};
    NLSemanticValueView joined_ref = {.type = types[0]};
    const bool reference = c->types[types[0] - 1].view.kind == NL_TYPE_REF;
    const bool copying = flat_copy(c, types[0]);
    NLValueId identity = 0;
    if (counts[0] != 0 && !copying && !reference) {
        if (results[0].value == 0 || results[0].value != results[1].value ||
            results[0].value > c->value_count ||
            post[0]->values[results[0].value - 1].dependencies !=
                NL_DEPENDENCY_FREE ||
            post[1]->values[results[1].value - 1].dependencies !=
                NL_DEPENDENCY_FREE) {
            if_precision(check, syntax->span);
            return 0;
        }
        identity = results[0].value;
        /* Same ID must denote the unchanged original package, not a mutation
         * or an authority whose conditional effects are unrepresented. */
        if (nl_sum_authority(c, types[0]) ||
            c->types[types[0] - 1].view.kind == NL_TYPE_SUM ||
            c->types[types[0] - 1].view.field_count != 0) {
            if_precision(check, syntax->span);
            return 0;
        }
    }
    for (size_t i = 0; i < 2; ++i) {
        Check branch = *check;
        branch.context = (NLSemanticContext *)post[i];
        branch.artifact = evidence[i];
        if (counts[i] != 0 && copying &&
            post[i]->values[results[i].value - 1].dependencies !=
                NL_DEPENDENCY_FREE) {
            if_precision(check, syntax->span);
            return 0;
        }
        if (reference && !rebase_ref_result(check, &branch, c, results[i].value,
                                            0, 0, (NLReferenceFacts){0},
                                            &joined_ref, syntax->span)) {
            check->status = branch.status;
            check->diagnostic = branch.diagnostic;
            return 0;
        }
        if (!arm_frame(check, c, post[i], 0,
                       returning ? check->function_place_floor : c->place_count,
                       c->scope_count, changed, syntax->span))
            return 0;
    }
    for (size_t j = 0; j < c->binding_count; ++j) {
        const NLSemanticBindingView before = c->bindings[j].view;
        if (!c->types[before.type - 1].view.is_copy &&
            before.availability == NL_AVAILABLE &&
            post[0]->bindings[j].view.availability == NL_CONSUMED) {
            if (nl_sum_authority(c, before.type)) {
                if_precision(check, syntax->span);
                return 0;
            }
            NLCheckedNodeId consumed =
                binding_use(check, j + 1, syntax->span, syntax->span);
            if (consumed == 0)
                return 0;
            if (view(check, consumed)->results[0].value != identity)
                nl_sem_end_value(c, view(check, consumed)->results[0].value);
        }
    }
    for (size_t j = 0; j < c->place_count; ++j)
        if (changed[j] && !widen_place(check, j, syntax->span))
            return 0;
    NLValueId output = identity;
    if (counts[0] != 0 && output == 0) {
        output = new_value(check,
                           reference ? joined_ref
                                     : (NLSemanticValueView){.type = types[0]},
                           syntax->span);
        if (output == 0)
            return 0;
    }
    if (returning) {
        if (!end_bindings(check, check->function_binding_floor, syntax->span) ||
            !host(check,
                  nl_sem_function_exit(c, check->function_scope_floor,
                                       check->function_place_floor),
                  syntax->span))
            return 0;
        check->terminated = true;
        check->returned = (NLCheckedResult){types[0], output};
        view(check, id)->terminates = true;
        view(check, id)->returned = check->returned;
    } else {
        view(check, id)->type = types[0];
        view(check, id)->result_count = counts[0];
        if (counts[0] != 0)
            view(check, id)->results[0] = (NLCheckedResult){types[0], output};
    }
    return id;
}

/* Return edges bypass the local normal join, but still contribute to the
 * enclosing function's result and caller-visible effects. Inspect IF evidence
 * only: P9 match has its own proven concrete-variant interpretation. No branch
 * locals or hypothetical identities are imported. */
static bool if_exit_evidence(Check *check, const NLCheckedFragment *artifact,
                             NLCheckedResult final, size_t prefix,
                             NLValueFactId fact_prefix, bool *found,
                             bool changed[NL_SEMANTIC_MAX_ENTRIES])
{
    NLSemanticContext *c = check->context;
    for (size_t i = 0; i < artifact->arm_count; ++i) {
        const NLCheckedNodeView *parent =
            nl_checked_node_view(artifact, artifact->arms[i].match);
        if (parent->kind != NL_CHECKED_IF)
            continue;
        const NLCheckedFragment *arm = artifact->arms[i].artifact;
        const NLCheckedNodeView *result = nl_checked_node_view(arm, arm->root);
        const size_t arm_prefix = prefix < arm->branch_value_prefix
                                      ? prefix
                                      : arm->branch_value_prefix;
        const NLValueFactId arm_facts = fact_prefix < arm->branch_fact_prefix
                                            ? fact_prefix
                                            : arm->branch_fact_prefix;
        if (!result->terminates ||
            !nl_control_exits_only_return(arm->exits, check->function_target)) {
            if (!if_exit_evidence(check, arm, final, arm_prefix, arm_facts,
                                  found, changed))
                return false;
            continue;
        }
        *found = true;
        const NLSemanticContext *b = arm->context;
        if (final.type != result->returned.type) {
            if_precision(check, parent->span);
            return false;
        }
        if (!flat_copy(c, final.type) &&
            (final.value == 0 || final.value != result->returned.value ||
             final.value > arm_prefix)) {
            if_precision(check, parent->span);
            return false;
        }
        if (result->returned.value != 0 &&
            b->values[result->returned.value - 1].dependencies !=
                NL_DEPENDENCY_FREE) {
            if_precision(check, parent->span);
            return false;
        }
        /* Only caller prefix sites/scopes survive this function boundary. */
        if (c->region_count != b->region_count ||
            c->domain_count != b->domain_count) {
            if_precision(check, parent->span);
            return false;
        }
        for (size_t j = 0; j < c->domain_count; ++j)
            if (c->domains[j].live != b->domains[j].live) {
                if_precision(check, parent->span);
                return false;
            }
        for (size_t j = 0; j < check->function_scope_floor; ++j)
            if (!same_scope(c->scopes[j], b->scopes[j])) {
                if_precision(check, parent->span);
                return false;
            }
        for (size_t j = 0; j < check->function_place_floor; ++j) {
            const NLSemanticPlaceView a = c->places[j], other = b->places[j];
            if (!same_place_frame(a, other)) {
                if_precision(check, parent->span);
                return false;
            }
            /* Equal numbers allocated AFTER a fork are not equal facts. A
             * common value/current-fact must originate in the shared prefix. */
            if (a.current_value != other.current_value ||
                a.current_fact != other.current_fact ||
                a.current_value > arm_prefix || a.current_fact > arm_facts) {
                if (!flat_copy(c, a.type)) {
                    if_precision(check, parent->span);
                    return false;
                }
                changed[j] = true;
            }
        }
    }
    return true;
}

static bool join_if_function_exits(Check *check)
{
    NLCheckedNodeView root = *view(check, check->artifact->root);
    NLCheckedResult final = check->terminated ? check->returned
                            : root.result_count == 0
                                ? (NLCheckedResult){.type = root.type}
                                : root.results[0];
    bool found = false, changed[NL_SEMANTIC_MAX_ENTRIES] = {false};
    if (!if_exit_evidence(check, check->artifact, final,
                          check->context->value_count,
                          check->context->last_value_fact, &found, changed))
        return false;
    for (size_t i = 0;
         i < nl_control_exits_count(check->artifact->loop_returns); ++i) {
        const NLControlExitView *edge =
            nl_control_exit_view(check->artifact->loop_returns, i);
        const NLSemanticContext *b =
            nl_control_state_context((NLControlState *)edge->state);
        if (edge->values[0].type != final.type ||
            !flat_copy(check->context, final.type)) {
            if_precision(check, root.span);
            return false;
        }
        for (size_t j = 0; j < check->function_place_floor; ++j) {
            const NLSemanticPlaceView a = check->context->places[j],
                                      other = b->places[j];
            if (!same_place_frame(a, other) ||
                a.current_value != other.current_value ||
                a.current_fact != other.current_fact ||
                a.current_value > check->function_value_prefix ||
                a.current_fact > check->function_fact_prefix) {
                if_precision(check, root.span);
                return false;
            }
        }
        found = true;
    }
    if (!found)
        return true;
    for (size_t i = 0; i < check->function_place_floor; ++i)
        if (changed[i] && !widen_place(check, i, root.span))
            return false;
    if (flat_copy(check->context, final.type) && final.value != 0) {
        nl_sem_end_value(check->context, final.value);
        final.value = new_value(
            check, (NLSemanticValueView){.type = final.type}, root.span);
        if (final.value == 0)
            return false;
    }
    if (check->terminated) {
        check->returned = final;
        view(check, check->artifact->root)->returned = final;
    } else if (root.result_count != 0)
        view(check, check->artifact->root)->results[0] = final;
    return true;
}

static bool zero_normal_returns(Check *check, NLTypeId declared,
                                NLCheckedNodeId body, NLSourceSpan span)
{
    NLSemanticContext *c = check->context;
    if (!flat_copy(c, declared) || c->region_count != 0)
        goto precision;
    for (size_t k = 0; k < nl_control_exits_count(check->artifact->exits);
         ++k) {
        const NLControlExitView *e =
            nl_control_exit_view(check->artifact->exits, k);
        const NLSemanticContext *b =
            nl_control_state_context((NLControlState *)e->state);
        for (size_t j = 0; j < check->function_place_floor; ++j)
            if (!same_place_frame(c->places[j], b->places[j]) ||
                c->places[j].current_value != b->places[j].current_value ||
                c->places[j].current_fact != b->places[j].current_fact ||
                c->places[j].current_value > check->function_value_prefix ||
                c->places[j].current_fact > check->function_fact_prefix)
                goto precision;
        for (size_t j = 0; j < check->function_scope_floor; ++j)
            if (!same_scope(c->scopes[j], b->scopes[j]))
                goto precision;
        if (c->domain_count != b->domain_count)
            goto precision;
        for (size_t j = 0; j < c->domain_count; ++j)
            if (c->domains[j].live != b->domains[j].live)
                goto precision;
        for (size_t j = check->function_binding_floor; j < c->binding_count;
             ++j)
            if (!c->bindings[j].hidden &&
                c->bindings[j].view.availability == NL_AVAILABLE &&
                !c->types[c->bindings[j].view.type - 1].view.is_copy &&
                (j >= b->binding_count ||
                 b->bindings[j].view.availability != NL_CONSUMED ||
                 c->bindings[j].view.value != b->bindings[j].view.value ||
                 b->values[c->bindings[j].view.value - 1].carrier !=
                     NL_CARRIER_ENDED))
                goto precision;
    }
    for (size_t j = check->function_binding_floor; j < c->binding_count; ++j)
        if (!c->bindings[j].hidden &&
            c->bindings[j].view.availability == NL_AVAILABLE &&
            !c->types[c->bindings[j].view.type - 1].view.is_copy) {
            NLCheckedNodeId use = binding_use(check, j + 1, span, span);
            if (use == 0)
                return false;
            end_temporary(check, view(check, use)->results[0].value);
        }
    if (!end_bindings(check, check->function_binding_floor, span))
        return false;
    for (size_t j = 0; j < c->value_count; ++j) {
        if (c->types[c->values[j].type - 1].view.is_copy ||
            c->values[j].carrier != NL_CARRIER_LOOSE)
            continue;
        if (j >= check->function_value_prefix)
            goto precision;
        bool ended = true;
        for (size_t k = 0; k < nl_control_exits_count(check->artifact->exits);
             ++k) {
            const NLControlExitView *e =
                nl_control_exit_view(check->artifact->exits, k);
            const NLSemanticContext *b =
                nl_control_state_context((NLControlState *)e->state);
            if (b->values[j].carrier != NL_CARRIER_ENDED)
                ended = false;
        }
        if (ended)
            nl_sem_end_value(c, j + 1);
    }
    check->returned = (NLCheckedResult){
        declared,
        declared == 1
            ? 0
            : new_value(check, (NLSemanticValueView){.type = declared}, span)};
    view(check, body)->returned = check->returned;
    return check->status == NL_CHECK_OK;
precision:
    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span,
         "P14-ZERO-NORMAL-RETURN-PRECISION",
         "zero-normal function Return alternatives need a richer result/frame "
         "join");
    return false;
}

/* P14: a single abstract transfer, not a concrete first iteration. Copy
 * carried slots are top; the captured outer frame is an exact invariant.
 * Closure checks every retained Continue snapshot against that same H. */
static void loop_precision(Check *check, NLSourceSpan span)
{
    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, span, "P14-LOOP-PRECISION",
         "cyclic identity/dependency or memory correlation is not "
         "representable");
}
static NLCheckedResult loop_operand(Check *check, NLCheckedNodeId id,
                                    NLSourceSpan span, bool materialize_unit)
{
    const NLCheckedNodeView v = *view(check, id);
    if (v.type == 1 && v.result_count == 0) {
        NLValueId value =
            materialize_unit
                ? new_value(check, (NLSemanticValueView){.type = 1}, span)
                : 0;
        return (NLCheckedResult){1, value};
    }
    if (v.result_count != 1) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, span, "P14-ONE-VALUE",
             "loop receiving requires exactly one normal value");
        return (NLCheckedResult){0};
    }
    return v.results[0];
}
static NLCheckedNodeId source_control(Check *check, const NLSyntaxView *syntax)
{
    const bool continuing = syntax->kind == NL_SYNTAX_CONTINUE;
    LoopControl *loop = check->loop;
    if (loop == NULL) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
             "P14-CONTROL-CONTEXT",
             "continue/break requires an active lexical loop target");
        return 0;
    }
    size_t count = continuing ? syntax->data.call.argument_count : 1;
    if (continuing && count != loop->count) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span, "P14-CONTINUE-ARITY",
             "continue arguments must match the loop parameter count");
        return 0;
    }
    NLCheckedResult results[NL_CONTROL_MAX_SLOTS] = {{0}};
    const NLSyntaxNode *operand = continuing
                                      ? syntax->data.call.arguments
                                      : syntax->data.statement.expression;
    NLCheckedNodeId first = 0;
    for (size_t i = 0; i < count; ++i) {
        NLCheckedNodeId child = expression(check, operand);
        if (child == 0 || check->terminated)
            return child;
        if (first == 0)
            first = child;
        results[i] = loop_operand(
            check, child, nl_syntax_node_view(operand)->span, continuing);
        if (check->status != NL_CHECK_OK)
            return 0;
        if (continuing && results[i].type != loop->types[i]) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
                 "P14-CONTINUE-TYPE",
                 "continue value must have the exact parameter static type");
            return 0;
        }
        if (continuing)
            operand = nl_syntax_next_argument(operand);
    }
    const NLSemanticContext *entry = nl_control_state_context(loop->entry);
    if (continuing)
        for (size_t i = 0; i < loop->bindings; ++i)
            if (!entry->types[entry->bindings[i].view.type - 1].view.is_copy &&
                entry->bindings[i].view.availability !=
                    check->context->bindings[i].view.availability) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
                     "P14-CONTINUE-AVAILABILITY",
                     "captured outer non-Copy availability must equal loop "
                     "entry");
                return 0;
            }
    if (!end_bindings(check, loop->bindings, syntax->span))
        return 0;
    NLControlState *post = NULL;
    NLCheckStatus status =
        nl_control_state_project(loop->entry, check->context, &post);
    if (status != NL_CHECK_OK) {
        fail(check, status, syntax->span, "P14-ITERATION-ESCAPE",
             "iteration-local responsibility/dependency cannot survive this "
             "edge");
        return 0;
    }
    status = nl_control_exit_append(
        &check->artifact->exits, continuing ? NL_EXIT_CONTINUE : NL_EXIT_BREAK,
        loop->target, post, results, count);
    nl_control_state_destroy(post);
    if (!host(check, status, syntax->span))
        return 0;
    check->terminated = true;
    check->returned = (NLCheckedResult){0};
    return add(check,
               (NLCheckedNodeView){.kind = continuing ? NL_CHECKED_CONTINUE
                                                      : NL_CHECKED_BREAK,
                                   .span = syntax->span,
                                   .initializer = first,
                                   .terminates = true});
}

static bool loop_return_frame(Check *check, const LoopControl *loop,
                              const NLControlExitView *edge, NLSourceSpan span)
{
    const NLSemanticContext *a = nl_control_state_context(loop->entry);
    const NLSemanticContext *b =
        nl_control_state_context((NLControlState *)edge->state);
    if (!nl_control_target_same(edge->target, check->function_target) ||
        !flat_copy(a, edge->values[0].type))
        goto precision;
    for (size_t i = 0; i < check->function_place_floor; ++i)
        if (!same_place_frame(a->places[i], b->places[i]) ||
            a->places[i].current_value != b->places[i].current_value ||
            a->places[i].current_fact != b->places[i].current_fact)
            goto precision;
    for (size_t i = 0; i < check->function_scope_floor; ++i)
        if (!same_scope(a->scopes[i], b->scopes[i]))
            goto precision;
    if (a->region_count != 0 || a->domain_count != b->domain_count)
        goto precision;
    for (size_t i = 0; i < a->domain_count; ++i)
        if (a->domains[i].live != b->domains[i].live)
            goto precision;
    return true;
precision:
    loop_precision(check, span);
    return false;
}

static NLCheckedNodeId source_loop(Check *check, const NLSyntaxView *syntax)
{
    const size_t count = syntax->data.loop.count;
    if (count > NL_CONTROL_MAX_SLOTS) {
        (void)host(check, NL_CHECK_RESOURCE_LIMIT, syntax->span);
        return 0;
    }
    LoopControl loop = {.parent = check->loop, .count = count};
    char *names[NL_CONTROL_MAX_SLOTS] = {NULL};
    NLValueId slots[NL_CONTROL_MAX_SLOTS] = {0};
    NLCheckedFragment *body_artifact = NULL;
    NLSemanticContext *body_context = NULL;
    NLCheckedNodeId id = 0;
    size_t i = 0;
    for (const NLSyntaxNode *p = syntax->data.loop.parameters; p != NULL;
         p = nl_syntax_next_argument(p), ++i) {
        const NLSyntaxView *param = nl_syntax_node_view(p);
        if (!lexical_source_name(check, param->data.binding.name))
            goto cleanup;
        names[i] = source_name_copy(check, param->data.binding.name);
        if (names[i] == NULL)
            goto cleanup;
        for (size_t j = 0; j < i; ++j)
            if (strcmp(names[i], names[j]) == 0) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, param->span,
                     "P14-DUPLICATE-PARAMETER",
                     "loop parameters must have distinct names");
                goto cleanup;
            }
    }
    /* No parameter is introduced into the outer initializer namespace. */
    i = 0;
    for (const NLSyntaxNode *p = syntax->data.loop.parameters; p != NULL;
         p = nl_syntax_next_argument(p), ++i) {
        const NLSyntaxView *param = nl_syntax_node_view(p);
        NLCheckedNodeId init =
            expression(check, param->data.binding.initializer);
        if (init == 0)
            goto cleanup;
        if (check->terminated) {
            id = init;
            goto cleanup;
        }
        NLCheckedResult value = loop_operand(check, init, param->span, true);
        if (check->status != NL_CHECK_OK)
            goto cleanup;
        loop.types[i] = value.type;
        slots[i] = value.value;
    }
    NLSemanticContext *c = check->context;
    loop.bindings = c->binding_count;
    loop.places = c->place_count;
    loop.scopes = c->scope_count;
    if (!host(check, nl_control_target_create(NL_TARGET_LOOP, &loop.target),
              syntax->span) ||
        !host(check, nl_control_state_create(c, &loop.entry), syntax->span))
        goto cleanup;
    NLCheckStatus status = nl_loop_header_create_bounded(
        loop.entry, loop.target, slots, count, true, false, &loop.header);
    /* Rich finite entries need no cyclic abstraction if no Continue exists.
     * Preserve their entry facts; never claim cyclic closure for them. */
    if (status != NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
        !host(check, status, syntax->span))
        goto cleanup;
    if (!host(check, nl_sem_clone(c, &body_context), syntax->span))
        goto cleanup;
    body_artifact = malloc(sizeof(*body_artifact));
    if (body_artifact == NULL) {
        (void)host(check, NL_CHECK_OUT_OF_MEMORY, syntax->span);
        goto cleanup;
    }
    *body_artifact =
        (NLCheckedFragment){.source = check->source,
                            .context = body_context,
                            .destroy_context = nl_semantic_destroy};
    body_context = NULL; /* artifact owns it from here */
    Check body = *check;
    body.context = (NLSemanticContext *)body_artifact->context;
    body.artifact = body_artifact;
    body.loop = &loop;
    body.binding_floor = body.arm_floor = loop.bindings;
    body.has_arm_floor = true;
    for (i = 0; i < count; ++i) {
        NLValueId value = slots[i];
        if (flat_copy(body.context, loop.types[i]))
            value =
                new_value(&body, (NLSemanticValueView){.type = loop.types[i]},
                          syntax->span);
        NLSymbolId symbol = 0;
        if (value == 0 ||
            !host(&body,
                  nl_sem_bind_in_scope(body.context, names[i], value,
                                       loop.bindings, &symbol),
                  syntax->span))
            goto body_failure;
    }
    body.artifact->root = expression(&body, syntax->data.loop.body);
    if (body.artifact->root == 0)
        goto body_failure;
    if (!body.terminated) {
        fail(&body, NL_CHECK_SEMANTIC_ERROR, syntax->span,
             "P14-BODY-FALLTHROUGH",
             "loop body may not complete normally; explicit control exit "
             "required");
        goto body_failure;
    }
    bool recurrent = false;
    for (i = 0; i < nl_control_exits_count(body.artifact->exits); ++i)
        if (nl_control_exit_view(body.artifact->exits, i)->kind ==
            NL_EXIT_CONTINUE)
            recurrent = true;
    status =
        recurrent
            ? (loop.header == NULL
                   ? NL_CHECK_ANALYSIS_PRECISION_LIMIT
                   : nl_loop_header_closure(loop.header, body.artifact->exits))
            : NL_CHECK_OK;
    if (status != NL_CHECK_OK) {
        if (status == NL_CHECK_ANALYSIS_PRECISION_LIMIT)
            loop_precision(&body, syntax->span);
        else
            fail(&body, status, syntax->span, "P14-CONTINUE-INVARIANT",
                 "a continue successor fails the header's exact invariant");
        goto body_failure;
    }
    size_t breaks = 0, continues = 0, returns = 0;
    const NLControlExitView *first_break = NULL;
    for (i = 0; i < nl_control_exits_count(body.artifact->exits); ++i) {
        const NLControlExitView *edge =
            nl_control_exit_view(body.artifact->exits, i);
        if (edge->kind == NL_EXIT_CONTINUE) {
            ++continues;
        } else if (edge->kind == NL_EXIT_BREAK &&
                   nl_control_target_same(edge->target, loop.target)) {
            ++breaks;
            if (first_break == NULL)
                first_break = edge;
        } else if (edge->kind == NL_EXIT_RETURN) {
            if (!loop_return_frame(check, &loop, edge, syntax->span))
                goto cleanup;
            ++returns;
            if (!host(check,
                      nl_control_exit_append(
                          &check->artifact->exits, NL_EXIT_RETURN,
                          check->function_target, edge->state, edge->values, 1),
                      syntax->span) ||
                !host(check,
                      nl_control_exit_append(
                          &check->artifact->loop_returns, NL_EXIT_RETURN,
                          check->function_target, edge->state, edge->values, 1),
                      syntax->span))
                goto cleanup;
        } else {
            (void)host(check, NL_CHECK_INTERNAL_ERROR, syntax->span);
            goto cleanup;
        }
    }
    if (loop.parent != NULL && continues != 0) {
        loop_precision(check, syntax->span); /* no nested convergence claim */
        goto cleanup;
    }
    id = add(check, (NLCheckedNodeView){.kind = NL_CHECKED_LOOP,
                                        .span = syntax->span,
                                        .header_inductive = true,
                                        .continue_edges = continues,
                                        .break_edges = breaks,
                                        .return_edges = returns});
    if (id == 0)
        goto cleanup;
    if (breaks == 0) {
        check->terminated = true;
        check->returned = (NLCheckedResult){0};
        if (returns != 0) {
            /* Publish only the common function-exit ownership effect. There
             * is no normal loop state: Continue remains solely in its H. */
            for (size_t j = check->function_binding_floor; j < loop.bindings;
                 ++j) {
                if (c->bindings[j].hidden ||
                    c->bindings[j].view.availability != NL_AVAILABLE ||
                    c->types[c->bindings[j].view.type - 1].view.is_copy)
                    continue;
                for (size_t k = 0;
                     k < nl_control_exits_count(body.artifact->exits); ++k) {
                    const NLControlExitView *e =
                        nl_control_exit_view(body.artifact->exits, k);
                    if (e->kind != NL_EXIT_RETURN)
                        continue;
                    const NLSemanticContext *b =
                        nl_control_state_context((NLControlState *)e->state);
                    if (b->bindings[j].view.availability != NL_CONSUMED ||
                        b->bindings[j].view.value !=
                            c->bindings[j].view.value) {
                        loop_precision(check, syntax->span);
                        goto cleanup;
                    }
                }
                NLCheckedNodeId used =
                    binding_use(check, j + 1, syntax->span, syntax->span);
                if (used == 0)
                    goto cleanup;
                end_temporary(check, view(check, used)->results[0].value);
            }
            if (!end_bindings(check, check->function_binding_floor,
                              syntax->span))
                goto cleanup;
            NLTypeId type = check->function_result;
            check->returned = (NLCheckedResult){
                type, type == 1 ? 0
                                : new_value(check,
                                            (NLSemanticValueView){.type = type},
                                            syntax->span)};
            if (type != 1 && check->returned.value == 0)
                goto cleanup;
        }
        view(check, id)->terminates = true;
        view(check, id)->returned = check->returned;
    } else {
        const NLTypeId type = first_break->values[0].type;
        const NLSemanticContext *first =
            nl_control_state_context((NLControlState *)first_break->state);
        NLValueId value = first_break->values[0].value;
        bool changed[NL_SEMANTIC_MAX_ENTRIES] = {false};
        NLSemanticValueView ref_join = {.type = type};
        if (type > c->type_count) {
            loop_precision(check, syntax->span);
            goto cleanup;
        }
        const bool ref = join_ref_type(c, type);
        for (i = 0; i < nl_control_exits_count(body.artifact->exits); ++i) {
            const NLControlExitView *edge =
                nl_control_exit_view(body.artifact->exits, i);
            if (edge->kind != NL_EXIT_BREAK)
                continue;
            const NLSemanticContext *post =
                nl_control_state_context((NLControlState *)edge->state);
            if (edge->values[0].type != type) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
                     "P14-BREAK-TYPE",
                     "all normal break results must have the same static type");
                goto cleanup;
            }
            for (size_t j = 0; j < loop.bindings; ++j)
                if (!c->types[c->bindings[j].view.type - 1].view.is_copy &&
                    first->bindings[j].view.availability !=
                        post->bindings[j].view.availability) {
                    fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
                         "P14-BREAK-AVAILABILITY",
                         "break exits disagree on captured non-Copy "
                         "availability");
                    goto cleanup;
                }
            if (!arm_frame(check, c, post, 0, loop.places, loop.scopes, changed,
                           syntax->span))
                goto cleanup;
            if (ref) {
                Check branch = *check;
                branch.context = (NLSemanticContext *)post;
                if (edge->values[0].value == 0 ||
                    !rebase_ref_result(check, &branch, c, edge->values[0].value,
                                       0, 0, (NLReferenceFacts){0}, &ref_join,
                                       syntax->span)) {
                    if (branch.status != NL_CHECK_OK) {
                        check->status = branch.status;
                        check->diagnostic = branch.diagnostic;
                    }
                    goto cleanup;
                }
            } else if (!flat_copy(c, type)) {
                /* No affine phi made from static type or coincident private
                 * IDs. */
                if (value == 0 || value != edge->values[0].value ||
                    value > c->value_count ||
                    c->values[value - 1].type != type ||
                    c->values[value - 1].dependencies != NL_DEPENDENCY_FREE ||
                    post->values[value - 1].dependencies !=
                        NL_DEPENDENCY_FREE ||
                    post->values[value - 1].carrier != NL_CARRIER_LOOSE) {
                    loop_precision(check, syntax->span);
                    goto cleanup;
                }
            } else if (edge->values[0].value != 0 &&
                       post->values[edge->values[0].value - 1].dependencies !=
                           NL_DEPENDENCY_FREE) {
                loop_precision(check, syntax->span);
                goto cleanup;
            }
        }
        for (size_t j = 0; j < loop.bindings; ++j)
            if (!c->types[c->bindings[j].view.type - 1].view.is_copy &&
                c->bindings[j].view.availability == NL_AVAILABLE &&
                first->bindings[j].view.availability == NL_CONSUMED &&
                binding_use(check, j + 1, syntax->span, syntax->span) == 0)
                goto cleanup;
        for (size_t j = 0; j < loop.places; ++j)
            if (changed[j] && !widen_place(check, j, syntax->span))
                goto cleanup;
        /* End consumed public-prefix affine packages only when every normal
         * Break certificate proves that ending. Never duplicate a body owner.
         */
        for (size_t j = 0; j < c->value_count; ++j) {
            if (c->types[c->values[j].type - 1].view.is_copy ||
                c->values[j].carrier != NL_CARRIER_LOOSE || j + 1 == value)
                continue;
            bool ended = true;
            for (size_t k = 0; k < nl_control_exits_count(body.artifact->exits);
                 ++k) {
                const NLControlExitView *e =
                    nl_control_exit_view(body.artifact->exits, k);
                if (e->kind != NL_EXIT_BREAK)
                    continue;
                const NLSemanticContext *b =
                    nl_control_state_context((NLControlState *)e->state);
                if (j >= b->value_count ||
                    b->values[j].carrier != NL_CARRIER_ENDED)
                    ended = false;
            }
            if (ended)
                nl_sem_end_value(c, j + 1);
        }
        if (ref) {
            if (ref_join.reference_count == 0) {
                loop_precision(check, syntax->span);
                goto cleanup;
            }
            ref_join.reference = ref_join.references[0];
            value = new_value(check, ref_join, syntax->span);
        } else if (flat_copy(c, type)) {
            value = type == 1
                        ? 0
                        : new_value(check, (NLSemanticValueView){.type = type},
                                    syntax->span);
        }
        if (check->status != NL_CHECK_OK)
            goto cleanup;
        view(check, id)->type = type;
        view(check, id)->result_count = value == 0 ? 0 : 1;
        view(check, id)->results[0] = (NLCheckedResult){type, value};
    }
    if (!save_arm(check, id, body_artifact, syntax->span))
        goto cleanup;
    body_artifact = NULL;
    goto cleanup;
body_failure:
    check->status = body.status;
    check->diagnostic = body.diagnostic;
cleanup:
    for (i = 0; i < count; ++i)
        free(names[i]);
    nl_checked_destroy(body_artifact);
    nl_semantic_destroy(body_context);
    nl_loop_header_destroy(loop.header);
    nl_control_state_destroy(loop.entry);
    nl_control_target_destroy(loop.target);
    return check->status == NL_CHECK_OK ? id : 0;
}

/* P9's bounded finite function plan: zero or one normal arm. Guarded
 * snapshots prove every arm/exit; only actual plan evaluation mutates the
 * candidate. Hypothetical IDs/post-state are never imported. */
static bool function_arm(Check *check, const NLSyntaxView *arm, NLValueId input,
                         size_t variant, bool hypothetical, NLSymbolId *binder)
{
    if (binder != NULL)
        *binder = 0;
    NLSemanticContext *c = check->context;
    const NLTypeId type = c->values[input - 1].type;
    const NLTypeId payload_type = c->types[type - 1].variant_types[variant - 1];
    NLValueId sum = input, payload = c->values[sum - 1].sum_payload;
    if (c->values[sum - 1].variant != variant) {
        if (!hypothetical) {
            (void)host(check, NL_CHECK_INTERNAL_ERROR, arm->span);
            return false;
        }
        nl_sem_end_value(c, sum);
        payload =
            payload_type == 0
                ? 0
                : new_value(check, (NLSemanticValueView){.type = payload_type},
                            arm->span);
        if (check->status != NL_CHECK_OK)
            return false;
        sum = new_value(check,
                        (NLSemanticValueView){.type = type,
                                              .variant = variant,
                                              .sum_payload = payload},
                        arm->span);
        if (sum == 0)
            return false;
        if (payload != 0) {
            c->values[payload - 1].carrier = NL_CARRIER_SUM;
            c->values[payload - 1].sum_owner = sum;
        }
    }
    if (payload != 0) {
        if (arm->data.arm.wildcard)
            nl_sem_end_value(c, payload);
        else {
            c->values[payload - 1].carrier = NL_CARRIER_LOOSE;
            c->values[payload - 1].sum_owner = 0;
            char *name = source_name_copy(check, arm->data.arm.binding);
            if (name == NULL)
                return false;
            NLSymbolId symbol;
            NLCheckStatus status = nl_sem_bind_in_scope(
                c, name, payload, check->binding_floor, &symbol);
            free(name);
            if (!host(check, status, arm->span))
                return false;
            if (binder != NULL)
                *binder = symbol;
        }
    }
    c->values[sum - 1].carrier = NL_CARRIER_ENDED;
    c->values[sum - 1].sum_payload = 0;
    return true;
}

/* The Node link witness needs only a unit result and an unchanged public
 * frame. Prove EVERY normal arm against the incoming prefix, then discard
 * the temporary scrutinee locally. No arm-owned IDs or state are imported.
 * Exact semantic comparison is intentionally conservative (no widening). */
static bool node_unit_value_same(NLSemanticValueView a, NLSemanticValueView b)
{
    if (a.type != b.type || a.carrier != b.carrier ||
        a.owner_place != b.owner_place ||
        a.aggregate_owner != b.aggregate_owner ||
        a.dependencies != b.dependencies ||
        a.value_dependency_count != b.value_dependency_count ||
        memcmp(a.value_dependencies, b.value_dependencies,
               sizeof(a.value_dependencies)) != 0 ||
        a.domain != b.domain || ref_order(a.reference, b.reference) != 0 ||
        a.reference_count != b.reference_count ||
        a.slot_place != b.slot_place ||
        a.allocation_region != b.allocation_region ||
        a.occupancy.region != b.occupancy.region ||
        a.occupancy.start != b.occupancy.start ||
        a.occupancy.length != b.occupancy.length ||
        a.field_count != b.field_count ||
        memcmp(a.fields, b.fields, sizeof(a.fields)) != 0 ||
        a.variant != b.variant || a.sum_payload != b.sum_payload ||
        a.sum_owner != b.sum_owner || a.scalar_known != b.scalar_known ||
        a.scalar_value != b.scalar_value)
        return false;
    for (size_t i = 0; i < a.reference_count; ++i)
        if (ref_order(a.references[i], b.references[i]) != 0)
            return false;
    return true;
}

static bool node_unit_arm_frame(const NLSemanticContext *before,
                                const NLSemanticContext *after, NLValueId input,
                                const NLCheckedNodeView *result)
{
    if (result->terminates || result->type != 1 || result->result_count > 1 ||
        before->domain_count != after->domain_count ||
        before->region_count != after->region_count)
        return false;
    for (size_t i = 0; i < before->domain_count; ++i)
        if (before->domains[i].live != after->domains[i].live ||
            before->domains[i].value != after->domains[i].value)
            return false;
    for (size_t i = 0; i < before->region_count; ++i) {
        const NLRawRegionEntry *a = &before->regions[i],
                               *b = &after->regions[i];
        if (a->view.live != b->view.live || a->view.size != b->view.size ||
            a->view.alignment != b->view.alignment ||
            a->view.ordinary_read != b->view.ordinary_read ||
            a->view.ordinary_write != b->view.ordinary_write ||
            a->view.address_known != b->view.address_known ||
            a->view.address != b->view.address || a->count != b->count)
            return false;
        for (size_t j = 0; j < a->count; ++j)
            if (a->intervals[j].start != b->intervals[j].start ||
                a->intervals[j].length != b->intervals[j].length ||
                a->intervals[j].state.validity !=
                    b->intervals[j].state.validity ||
                a->intervals[j].state.value_known !=
                    b->intervals[j].state.value_known ||
                a->intervals[j].state.value != b->intervals[j].state.value)
                return false;
    }
    if (result->result_count == 1 &&
        after->values[result->results[0].value - 1].dependencies !=
            NL_DEPENDENCY_FREE)
        return false;
    for (size_t i = 0; i < before->binding_count; ++i) {
        const NLSemanticBindingView a = before->bindings[i].view,
                                    b = after->bindings[i].view;
        if (a.availability != b.availability || a.type != b.type ||
            a.value != b.value || a.place != b.place ||
            before->bindings[i].hidden != after->bindings[i].hidden)
            return false;
    }
    for (size_t i = 0; i < before->place_count; ++i) {
        const NLSemanticPlaceView a = before->places[i], b = after->places[i];
        if (!same_place_frame(a, b) || a.current_value != b.current_value ||
            a.current_fact != b.current_fact ||
            a.payload_occurrence != b.payload_occurrence)
            return false;
    }
    for (size_t i = 0; i < before->scope_count; ++i)
        if (!same_scope(before->scopes[i], after->scopes[i]))
            return false;
    for (size_t i = 0; i < before->occurrence_count; ++i) {
        const NLSemanticOccurrenceView a = before->occurrences[i],
                                       b = after->occurrences[i];
        if (a.live != b.live || a.root != b.root ||
            a.payload_place != b.payload_place || a.variant != b.variant)
            return false;
    }
    for (size_t i = 0; i < before->value_count; ++i) {
        if (i + 1 == input ||
            (input != 0 && i + 1 == before->values[input - 1].sum_payload))
            continue; /* owned temporary consumed by the pattern */
        if (!node_unit_value_same(before->values[i], after->values[i]))
            return false;
    }
    return true;
}

/* Identity prefix is qualified only by two owned snapshots captured at the
 * actual clone boundary. Fresh post-fork facts/IDs never enter this mapping. */
bool nl_packet_same_entry(const NLSemanticContext *a,
                          const NLSemanticContext *b)
{
    const NLCheckedNodeView unit = {.type = 1};
    if (a == NULL || b == NULL || a == b || a->type_count != b->type_count ||
        a->function_count != b->function_count ||
        a->value_count != b->value_count || a->place_count != b->place_count ||
        a->binding_count != b->binding_count ||
        a->scope_count != b->scope_count ||
        a->occurrence_count != b->occurrence_count ||
        a->last_value_fact != b->last_value_fact ||
        a->last_incarnation != b->last_incarnation ||
        nl_sem_validate(a) != NL_CHECK_OK || nl_sem_validate(b) != NL_CHECK_OK)
        return false;
    for (size_t i = 0; i < a->type_count; ++i) {
        const NLTypeEntry *x = &a->types[i], *y = &b->types[i];
        const NLSemanticTypeView u = x->view, v = y->view;
        if (u.kind != v.kind || u.is_copy != v.is_copy ||
            u.is_discardable != v.is_discardable || u.target != v.target ||
            u.access != v.access || u.is_exclusive != v.is_exclusive ||
            u.layout_known != v.layout_known ||
            u.variant_count != v.variant_count ||
            u.field_count != v.field_count || u.size != v.size ||
            u.alignment != v.alignment || x->incomplete != y->incomplete ||
            x->recursive_header != y->recursive_header ||
            x->allocated_target != y->allocated_target ||
            x->one_backing_target != y->one_backing_target ||
            x->live_tail_target != y->live_tail_target ||
            x->option_target != y->option_target ||
            memcmp(x->field_types, y->field_types, sizeof(x->field_types)) !=
                0 ||
            memcmp(x->variant_types, y->variant_types,
                   sizeof(x->variant_types)) != 0)
            return false;
    }
    for (size_t i = 0; i < a->function_count; ++i)
        if (a->functions[i].body != b->functions[i].body ||
            a->functions[i].owner_receiver != b->functions[i].owner_receiver ||
            a->functions[i].owner_producer != b->functions[i].owner_producer ||
            a->functions[i].custody_recipient !=
                b->functions[i].custody_recipient)
            return false;
    for (size_t i = 0; i < a->binding_count; ++i)
        if (strcmp(a->bindings[i].name, b->bindings[i].name) != 0)
            return false;
    return node_unit_arm_frame(a, b, 0, &unit);
}

/* Exact ancestor-prefix equivalence; suffix claims must all be closed.
 * Post-fork numeric IDs are never compared across worlds or imported. */
bool nl_allocated_post_matches(const NLSemanticContext *expected,
                               const NLSemanticContext *arm,
                               const NLCheckedNodeView *result)
{
    if (result->result_count == 1 &&
        (result->results[0].value == 0 ||
         result->results[0].value > arm->value_count ||
         result->results[0].type != 1 ||
         arm->values[result->results[0].value - 1].type != 1))
        return false;
    if (arm->region_count < expected->region_count ||
        arm->domain_count < expected->domain_count ||
        arm->place_count < expected->place_count ||
        arm->value_count < expected->value_count ||
        arm->binding_count < expected->binding_count ||
        arm->scope_count < expected->scope_count ||
        arm->occurrence_count < expected->occurrence_count ||
        nl_sem_validate(arm) != NL_CHECK_OK)
        return false;
    for (size_t j = expected->region_count; j < arm->region_count; ++j)
        if (arm->regions[j].view.live)
            return false;
    for (size_t j = expected->domain_count; j < arm->domain_count; ++j)
        if (arm->domains[j].live)
            return false;
    for (size_t j = expected->place_count; j < arm->place_count; ++j)
        if (arm->places[j].live)
            return false;
    for (size_t j = expected->value_count; j < arm->value_count; ++j)
        if (arm->values[j].carrier != NL_CARRIER_ENDED &&
            !arm->types[arm->values[j].type - 1].view.is_discardable)
            return false;
    NLSemanticContext prefix = *arm;
    prefix.region_count = expected->region_count;
    prefix.domain_count = expected->domain_count;
    return node_unit_arm_frame(expected, &prefix, 0, result);
}

static NLCheckedNodeId allocated_match(Check *check, const NLSyntaxView *syntax)
{
    const NLSyntaxView *trial =
        nl_syntax_node_view(syntax->data.match.scrutinee);
    if (check->allocation_budget == NULL)
        check->allocation_budget = &check->allocation_sites;
    NLTypeId h = check_type(check, trial->data.call.type), option = 0;
    if (h == 0)
        return 0;
    const bool certified = check->closure_probe ||
                           (nl_recursive_local_type(check->context, h) &&
                            check->context->types[h - 1].view.field_count == 4);
    const size_t limit = certified ? 5 : 2;
    if (check->loop != NULL || *check->allocation_budget >= limit ||
        (*check->allocation_budget != 0 &&
         ((!check->allocation_some_path) ||
          (certified ? check->allocation_depth == 0
                     : check->allocation_depth != 1) ||
          !check->allocated_slice)) ||
        check->allocation_depth >= limit ||
        (check->allocation_depth != 0 && !check->allocated_slice) ||
        check->artifact->captured_post != NULL ||
        check->artifact->captured_closure != NULL) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, trial->span,
             "ALLOCATED-CARDINALITY-PROFILE",
             "allocation requires one outer trial and at most one nested "
             "Some-arm trial, outside loops");
        return 0;
    }
    ++*check->allocation_budget;

    NLCheckStatus registry = nl_allocated_registry(check->context, h, &option);
    if (registry == NL_CHECK_SEMANTIC_UNSUPPORTED) {
        fail(check, registry, trial->span, "ALLOCATED-TARGET-PROFILE",
             "both trials require the same completed recursive nominal H");
        return 0;
    }
    if (!host(check, registry, trial->span))
        return 0;
    NLSemanticContext *c = check->context;
    const NLSyntaxView *arms[2] = {0};
    for (const NLSyntaxNode *n = syntax->data.match.arms; n != NULL;
         n = nl_syntax_next_argument(n)) {
        const NLSyntaxView *a = nl_syntax_node_view(n);
        size_t variant = sum_variant(check, option, a->data.arm.variant);
        if (variant == 0)
            return 0;
        if (arms[variant - 1] != NULL ||
            (variant == 2) != a->data.arm.payload ||
            (variant == 2 && a->data.arm.wildcard)) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, a->span, "ALLOCATED-PATTERN",
                 "allocation outcome requires unique None and consuming Some "
                 "binding");
            return 0;
        }
        arms[variant - 1] = a;
    }
    if (arms[0] == NULL || arms[1] == NULL) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
             "ALLOCATED-EXHAUSTIVENESS",
             "both allocation outcomes must be checked");
        return 0;
    }
    NLCheckedNodeId init =
        add(check, (NLCheckedNodeView){
                       .kind = NL_CHECKED_TRY_ALLOCATE_ONE,
                       .span = trial->span,
                       .type = option,
                       .allocation_trial = true,
                       .allocation_target = h,
                       .allocation_size = c->types[h - 1].view.size,
                       .allocation_alignment = c->types[h - 1].view.alignment});
    if (init == 0)
        return 0;
    NLCheckedNodeId id = add(
        check, (NLCheckedNodeView){.kind = NL_CHECKED_MATCH,
                                   .span = syntax->span,
                                   .initializer = init,
                                   .item_count = 2,
                                   .match_binding_prefix = c->binding_count});
    if (id == 0)
        return 0;
    NLSemanticContext *closed_prefix = NULL;
    NLCheckedNodeView certificate = {0};
    NLCapturedClosure *closure = NULL;
    if (certified) {
        if (!closure_status(check, nl_captured_closure_create(c, &closure),
                            trial->span))
            return 0;
        check->artifact->captured_closure = closure;
        check->artifact->destroy_captured_closure = nl_captured_closure_destroy;
        check->artifact->captured_match = id;
        closure->parent_world = check->artifact->context;
        if (closure->view.count != 0) {
            closed_prefix = (NLSemanticContext *)closure->view.closed_post;
            certificate.captured_frame_closed = true;
        }
    } else if (check->allocated_slice) {
        NLCheckStatus s =
            nl_allocated_closed_prefix(c, &closed_prefix, &certificate);
        if (s != NL_CHECK_OK) {
            if (s == NL_CHECK_ANALYSIS_PRECISION_LIMIT)
                fail(check, s, trial->span, "ALLOCATED-CAPTURE-PRECISION",
                     "nested trial requires one dependency-free live captured "
                     "head with exact allocation/domain owners");
            else
                (void)host(check, s, trial->span);
            return 0;
        }
    }
    for (size_t i = 0; i < 2; ++i) {
        Check branch = *check;
        branch.context = NULL;
        branch.artifact = NULL;
        branch.allocated_slice = check->allocated_slice || i == 1;
        branch.allocation_depth = check->allocation_depth + 1;
        branch.allocation_some_path = i == 1;
        branch.in_match_arm = true;
        branch.binding_floor = branch.arm_floor = c->binding_count;
        branch.has_arm_floor = true;
        if (!host(&branch, nl_sem_clone(c, &branch.context), arms[i]->span))
            goto failure;
        branch.artifact = malloc(sizeof(*branch.artifact));
        if (branch.artifact == NULL) {
            (void)host(&branch, NL_CHECK_OUT_OF_MEMORY, arms[i]->span);
            goto failure;
        }
        *branch.artifact = (NLCheckedFragment){0};
        branch.artifact->source = check->source;
        branch.artifact->context = branch.context;
        branch.artifact->destroy_context = nl_semantic_destroy;
        if (closure != NULL &&
            !host(&branch,
                  nl_sem_clone(branch.context, &closure->branches[i].entry),
                  arms[i]->span))
            goto failure;
        if (closure != NULL)
            closure->branches[i].entry_origin = closure->branches[i].entry;
        NLCheckedNodeView grant = {.span = trial->span};
        branch.status = nl_allocated_grant(branch.context, option, i == 1,
                                           &grant, &branch.diagnostic);
        if (branch.status != NL_CHECK_OK)
            goto failure;
        NLCheckedNodeId event = add(&branch, grant);
        NLSymbolId binder = 0;
        if (event == 0 ||
            !function_arm(&branch, arms[i], grant.results[0].value, i + 1,
                          false, &binder))
            goto failure;
        NLCheckedNodeId body = expression(&branch, arms[i]->data.arm.body);
        if (body == 0)
            goto failure;
        NLCheckedNodeView result = *view(&branch, body);
        const NLSemanticContext *expected =
            closed_prefix != NULL ? closed_prefix : c;
        if (branch.terminated ||
            !nl_allocated_post_matches(expected, branch.context, &result)) {
            fail(&branch, NL_CHECK_ANALYSIS_PRECISION_LIMIT, arms[i]->span,
                 closed_prefix != NULL ? "ALLOCATED-CAPTURED-JOIN-PRECISION"
                                       : "ALLOCATED-CLOSED-WORLD-PRECISION",
                 "each allocation world must close its own responsibilities "
                 "and prove the exact captured-prefix post-state");
            goto failure;
        }
        branch.artifact->root =
            add(&branch, (NLCheckedNodeView){.kind = NL_CHECKED_MATCH_ARM,
                                             .span = arms[i]->span,
                                             .variant = i + 1,
                                             .symbol = binder,
                                             .initializer = event,
                                             .tail = body,
                                             .type = 1,
                                             .normal_frame_unchanged =
                                                 closed_prefix == NULL});
        if (branch.artifact->root == 0)
            goto failure;
        if (closure != NULL) {
            closure->branches[i].arm = branch.artifact;
            closure->branches[i].world = branch.context;
        }
        if (!save_arm(check, id, branch.artifact, arms[i]->span))
            goto failure;
        continue;
    failure:
        if (branch.status != NL_CHECK_OK) {
            check->status = branch.status;
            check->diagnostic = branch.diagnostic;
        }
        if (branch.artifact != NULL)
            nl_checked_destroy(branch.artifact);
        else
            nl_semantic_destroy(branch.context);
        if (closure == NULL)
            nl_semantic_destroy(closed_prefix);
        return 0;
    }
    view(check, id)->type = 1;
    view(check, id)->normal_arms = 2;
    view(check, id)->normal_frame_unchanged = closed_prefix == NULL;
    view(check, id)->captured_frame_closed = closed_prefix != NULL;
    if (closure != NULL &&
        !closure_status(check, nl_captured_closure_finish(check->artifact, id),
                        syntax->span))
        return 0;
    if (closed_prefix != NULL) {
        /* Both arms proved this ancestor-derived target. No arm is selected. */
        NLSemanticContext *continuation = NULL;
        if (!host(check, nl_sem_clone(closed_prefix, &continuation),
                  syntax->span)) {
            if (closure == NULL)
                nl_semantic_destroy(closed_prefix);
            return 0;
        }
        nl_sem_commit(c, continuation);
        check->artifact->captured_post = closed_prefix;
        check->artifact->captured_match = id;
        if (closure == NULL)
            check->artifact->destroy_captured_post = nl_semantic_destroy;
        NLCheckedNodeView *v = view(check, id);
        v->captured_frame_closed = certificate.captured_frame_closed;
        v->captured_backing = certificate.captured_backing;
        v->captured_root = certificate.captured_root;
        v->captured_incarnation = certificate.captured_incarnation;
        v->captured_domain = certificate.captured_domain;
        v->captured_allocation = certificate.captured_allocation;
        v->captured_domain_binding = certificate.captured_domain_binding;
    }
    view(check, id)->type = 1;
    view(check, id)->normal_arms = 2;
    view(check, id)->normal_frame_unchanged =
        !certificate.captured_frame_closed;
    return id;
}

static NLCheckedNodeId function_match(Check *check, const NLSyntaxView *syntax)
{
    if (nl_syntax_node_view(syntax->data.match.scrutinee)->kind ==
        NL_SYNTAX_ALLOCATED_TRY)
        return allocated_match(check, syntax);
    if (check->in_match_arm && !check->allocated_slice) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
             "P6-NESTED-MATCH",
             "nested match joins are outside this bounded slice");
        return 0;
    }
    NLCheckedNodeId init = expression(check, syntax->data.match.scrutinee);
    if (init == 0 || check->terminated)
        return init;
    NLValueId input = one_result(check, init);
    if (input == 0)
        return 0;
    NLSemanticContext *c = check->context;
    const NLSemanticValueView value = c->values[input - 1];
    const NLTypeId type = value.type;
    if (c->types[type - 1].view.kind != NL_TYPE_SUM || value.variant == 0) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
             "P9-MATCH-PRECISION",
             "function match requires a concrete registered sum value");
        return 0;
    }
    bool seen[NL_SEMANTIC_MAX_VARIANTS] = {false};
    size_t variants[NL_SEMANTIC_MAX_VARIANTS] = {0}, count = 0;
    const NLSyntaxNode *arms[NL_SEMANTIC_MAX_VARIANTS] = {0};
    for (const NLSyntaxNode *arm = syntax->data.match.arms; arm != NULL;
         arm = nl_syntax_next_argument(arm)) {
        const NLSyntaxView *a = nl_syntax_node_view(arm);
        if (a->data.arm.payload && !a->data.arm.wildcard &&
            !lexical_source_name(check, a->data.arm.binding))
            return 0;
        size_t variant = sum_variant(check, type, a->data.arm.variant);
        if (variant == 0)
            return 0;
        if (seen[variant - 1]) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, a->span, "P6-EXHAUSTIVENESS",
                 "duplicate variant arm");
            return 0;
        }
        seen[variant - 1] = true;
        NLTypeId payload = c->types[type - 1].variant_types[variant - 1];
        if ((payload != 0) != a->data.arm.payload) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, a->span, "P6-PATTERN-SHAPE",
                 "pattern payload shape does not match variant");
            return 0;
        }
        if (payload != 0 && a->data.arm.wildcard &&
            !c->types[payload - 1].view.is_discardable) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, a->span, "P6-PAYLOAD-DISCARD",
                 "wildcard requires Discardable payload");
            return 0;
        }
        arms[count] = arm;
        variants[count++] = variant;
    }
    const bool custody_recovery = check->custody_continuation &&
                                  custody_type(c, type) &&
                                  input == check->custody_recovered;
    const bool second_none =
        check->custody_continuation && check->custody_extracted &&
        !check->in_source_loan && custody_type(c, type) && count == 1 &&
        variants[0] == 1 && value.variant == 1 && value.sum_payload == 0 &&
        view(check, init)->symbol == check->custody_binding &&
        !c->places[check->custody_sink - 1].live &&
        value.carrier == NL_CARRIER_LOOSE;
    const bool first_none =
        check->custody_recipient_body && custody_type(c, type) && count == 1 &&
        variants[0] == 1 && value.variant == 1 && value.sum_payload == 0 &&
        input == check->custody_old_none && value.carrier == NL_CARRIER_LOOSE;
    if (second_none) {
        for (size_t scope = 0; scope < c->scope_count; ++scope)
            if (c->scopes[scope].active) {
                fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                     "CUSTODY-NONE-SCOPE",
                     "final exact-None consumption requires expired loans");
                return 0;
            }
    }
    if (count != c->types[type - 1].view.variant_count && !first_none &&
        !second_none) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span, "P6-EXHAUSTIVENESS",
             "every variant requires exactly one arm");
        return 0;
    }
    NLCheckedNodeId id =
        add(check, (NLCheckedNodeView){.kind = NL_CHECKED_MATCH,
                                       .span = syntax->span,
                                       .initializer = init,
                                       .match_binding_prefix = c->binding_count,
                                       .item_count = count});
    if (id == 0)
        return 0;
    if (first_none)
        view(check, id)->custody_none_site = 1;
    if (second_none)
        view(check, id)->custody_none_site = 2;
    if (custody_recovery || second_none) {
        if (custody_recovery &&
            (count != 2 || value.variant == 0 ||
             (value.variant == 2 &&
              value.sum_payload != check->custody_packet))) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                 "CUSTODY-RECOVERED-ORIGIN",
                 "recovered Some must carry the original package in this "
                 "qualified world");
            return 0;
        }
        size_t selected = 0;
        while (selected < count && variants[selected] != value.variant)
            ++selected;
        if (selected == count) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                 "CUSTODY-RECOVERY-VARIANT",
                 "source-proven recovery variant has no arm");
            return 0;
        }
        NLSymbolId binder = 0;
        if (!function_arm(check, nl_syntax_node_view(arms[selected]), input,
                          value.variant, false, &binder))
            return 0;
        if (custody_recovery) {
            check->custody_extracted = true;
            check->custody_saved = value.sum_payload;
        }
        NLCheckedNodeId child = expression(
            check, nl_syntax_node_view(arms[selected])->data.arm.body);
        if (child == 0)
            return 0;
        if (view(check, child)->type != 1 || check->terminated) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                 "CUSTODY-RECOVERY-BODY",
                 "bounded recovery requires normal unit arm completion");
            return 0;
        }
        view(check, id)->initializer = init;
        view(check, id)->first_item = child;
        view(check, id)->normal_arms = 1;
        view(check, id)->custody_selected_variant = value.variant;
        view(check, id)->type = 1;
        if (second_none)
            check->custody_final_none = true;
        return id;
    }

    size_t normal = 0, normal_index = 0, actual_index = 0;
    const NLTypeId ptr = c->types[type - 1].option_target;
    bool node_unit_join =
        ptr != 0 && nl_recursive_local_type(c, c->types[ptr - 1].view.target);
    bool captured_packet = false;
    if (check->artifact->producer_call != 0) {
        const NLValueId packet =
            view(check, check->artifact->producer_call)->producer.result;
        for (size_t i = 0; i < c->binding_count; ++i)
            captured_packet |=
                c->bindings[i].view.availability == NL_AVAILABLE &&
                c->bindings[i].view.value == packet;
    }
    const bool packet_join = node_unit_join && count == 2 && captured_packet;
    unsigned packet_modes = 3; /* intersection of two parent-derived targets */
    NLSymbolId custody_symbol = 0;
    if (packet_join)
        for (size_t b = 0; b < c->binding_count; ++b)
            if (!c->bindings[b].hidden &&
                c->bindings[b].view.availability == NL_AVAILABLE &&
                custody_type(c, c->bindings[b].view.type)) {
                if (custody_symbol != 0) {
                    fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
                         "CUSTODY-MULTIPLE-LOCAL",
                         "one original custody local required");
                    return 0;
                }
                custody_symbol = b + 1;
            }

    if (packet_join) {
        const NLCheckStatus prepared =
            nl_packet_fork_prepare(check->artifact, id, c);
        if (prepared == NL_CHECK_ANALYSIS_PRECISION_LIMIT) {
            fail(check, prepared, syntax->span, "P219-FORK-ENTRY-PRECISION",
                 "exact available original packet and unloaned parent-world "
                 "prefix required");
            return 0;
        }
        if (!host(check, prepared, syntax->span))
            return 0;
        if (!host(check,
                  nl_packet_closed(check->artifact, id, c,
                                   &check->artifact->packet_post),
                  syntax->span))
            return 0;
        if (!host(check,
                  nl_packet_retained(check->artifact, id, c,
                                     &check->artifact->packet_retained_post),
                  syntax->span))
            return 0;
        /* Scrutinee temporary is consumed in either pattern, no arm ID. */
        nl_sem_end_value(check->artifact->packet_post, input);
        check->artifact->packet_post->values[input - 1].sum_payload = 0;
        view(check, id)->packet_fork.post_world = check->artifact->packet_post;
    }
    for (size_t i = 0; i < count; ++i) {
        if (variants[i] == value.variant)
            actual_index = i;
        const NLSyntaxView *a = nl_syntax_node_view(arms[i]);
        Check branch = *check;
        branch.context = NULL;
        branch.artifact = NULL;
        branch.in_match_arm = true;
        branch.binding_floor = branch.arm_floor = c->binding_count;
        branch.has_arm_floor = true;
        if (!host(&branch, nl_sem_clone(c, &branch.context), a->span))
            goto failure;
        branch.artifact = malloc(sizeof(*branch.artifact));
        if (branch.artifact == NULL) {
            (void)host(&branch, NL_CHECK_OUT_OF_MEMORY, a->span);
            goto failure;
        }
        *branch.artifact =
            (NLCheckedFragment){.source = check->source,
                                .context = branch.context,
                                .destroy_context = nl_semantic_destroy};
        if (packet_join) {
            branch.artifact->packet_parent = check->artifact;
            branch.artifact->packet_match = id;
            branch.artifact->destroy_packet_world = nl_semantic_destroy;
            if (!host(&branch, nl_sem_clone(c, &branch.artifact->packet_entry),
                      a->span))
                goto failure;
        }
        NLSymbolId binder = 0;
        if (!function_arm(&branch, a, input, variants[i], true, &binder))
            goto failure;
        NLCheckedNodeId body = expression(&branch, a->data.arm.body);
        if (body == 0)
            goto failure;
        if (!branch.terminated) {
            ++normal;
            normal_index = i;
        }
        const NLCheckedNodeView result = *view(&branch, body);
        if (packet_join)
            branch.context->values[input - 1].sum_payload = 0;
        bool retained = false;
        if (packet_join) {
            retained = nl_packet_arm_retained(check->artifact, id,
                                              branch.artifact, &result);
            const bool closed = nl_packet_arm_closed(check->artifact, id,
                                                     branch.artifact, &result);
            packet_modes &= (closed ? 1U : 0U) | (retained ? 2U : 0U);
            if (packet_modes == 0 && custody_symbol == 0) {
                fail(&branch, NL_CHECK_ANALYSIS_PRECISION_LIMIT, a->span,
                     "P219-POSTSTATE-PRECISION",
                     "both arms must prove the same original-owner retained "
                     "or closed parent postcondition");
                goto failure;
            }
        }
        node_unit_join =
            node_unit_join && !branch.terminated &&
            nl_control_exits_count(branch.artifact->exits) == 0 &&
            nl_control_exits_count(branch.artifact->loop_returns) == 0 &&
            node_unit_arm_frame(c, branch.context, input, &result);
        branch.artifact->root = add(
            &branch, (NLCheckedNodeView){.kind = NL_CHECKED_MATCH_ARM,
                                         .span = a->span,
                                         .variant = variants[i],
                                         .symbol = binder,
                                         .initializer = body,
                                         .terminates = branch.terminated,
                                         .type = result.type,
                                         .result_count = result.result_count,
                                         .results = {result.results[0]},
                                         .returned = branch.returned});
        if (retained && branch.artifact->root != 0) {
            view(&branch, branch.artifact->root)->packet_origin.ancestor =
                check->artifact;
            view(&branch, branch.artifact->root)->packet_origin.match = id;
            view(&branch, branch.artifact->root)->packet_origin.world =
                branch.artifact->context;
            view(&branch, branch.artifact->root)->packet_origin.entry_world =
                branch.artifact->packet_entry;
            view(&branch, branch.artifact->root)->packet_origin.packet =
                view(check, id)->packet_fork.packet;
        }
        if (!host(check,
                  nl_control_exits_union(&check->artifact->exits,
                                         branch.artifact->exits),
                  a->span))
            goto failure;
        if (!host(check,
                  nl_control_exits_union(&check->artifact->loop_returns,
                                         branch.artifact->loop_returns),
                  a->span))
            goto failure;
        if (branch.artifact->root == 0 ||
            !save_arm(check, id, branch.artifact, a->span))
            goto failure;
        continue;
    failure:
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
    if (check->loop != NULL && normal == 0) {
        check->terminated = true;
        check->returned = (NLCheckedResult){0};
        view(check, id)->terminates = true;
        return id;
    }
    if (normal > 1) {
        if (packet_join && custody_symbol != 0) {
            check->custody_pending = id;
            check->artifact->custody_join = id;
            check->artifact->custody_binding = custody_symbol;
            view(check, id)->normal_arms = 2;
            view(check, id)->type = 1;
            return id;
        }
        if (packet_join) {
            /* Reconstruct ONLY the parent prefix, never select an arm. */
            NLSemanticContext *post = NULL;
            const bool retained = packet_modes == 2;
            if (!host(check,
                      retained
                          ? nl_sem_clone(check->artifact->packet_retained_post,
                                         &post)
                          : nl_packet_closed(check->artifact, id, c, &post),
                      syntax->span))
                return 0;
            nl_sem_end_value(post, input);
            post->values[input - 1].sum_payload = 0;
            nl_sem_commit(c, post);
            view(check, id)->normal_arms = 2;
            view(check, id)->packet_fork.closed = !retained;
            view(check, id)->packet_fork.retained = retained;
            if (retained)
                view(check, id)->packet_fork.post_world =
                    check->artifact->packet_retained_post;
            view(check, id)->type = 1;
            return id;
        }
        if (node_unit_join) {
            end_temporary(check, input);
            view(check, id)->normal_arms = normal;
            view(check, id)->normal_frame_unchanged = true;
            view(check, id)->type = 1;
            return id;
        }
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
             "P9-CONTINUATION-PRECISION",
             "multiple normal function-match states require a richer join");
        return 0;
    }
    /* Registration checks the sole normal continuation, or all return exits.
     * A real call applies only its proven concrete variant against actual IDs.
     */
    size_t chosen_index = (check->definition || check->loop != NULL)
                              ? (normal == 0 ? 0 : normal_index)
                              : actual_index;
    const NLSyntaxView *chosen_arm = nl_syntax_node_view(arms[chosen_index]);
    Check chosen = *check;
    chosen.in_match_arm = true;
    chosen.binding_floor = chosen.arm_floor = c->binding_count;
    chosen.has_arm_floor = true;
    if (!function_arm(&chosen, chosen_arm, input, variants[chosen_index],
                      check->definition || check->loop != NULL, NULL))
        goto chosen_failure;
    NLCheckedNodeId body = expression(&chosen, chosen_arm->data.arm.body);
    if (body == 0)
        goto chosen_failure;
    check->terminated = chosen.terminated;
    check->returned = chosen.returned;
    view(check, id)->normal_arms = normal;
    view(check, id)->terminates = chosen.terminated;
    view(check, id)->returned = chosen.returned;
    view(check, id)->tail = body;
    if (!chosen.terminated) {
        const NLCheckedNodeView result = *view(check, body);
        view(check, id)->type = result.type;
        view(check, id)->result_count = result.result_count;
        memcpy(view(check, id)->results, result.results,
               sizeof(result.results));
    }
    return id;
chosen_failure:
    check->status = chosen.status;
    check->diagnostic = chosen.diagnostic;
    return 0;
}

/* A finite source continuation, not an Unknown Option/current-memory join.
 * Each policy world keeps its own actual owner and runs the same lexical
 * suffix.
 */
static bool custody_adopted_frame(Check *check, const NLCheckedFragment *arm,
                                  NLSymbolId symbol)
{
    const NLCheckedFragment *parent = check->artifact;
    const NLSemanticContext *c = arm->context,
                            *before = parent->packet_retained_post;
    const NLCheckedNodeView *call =
        nl_checked_node_view(arm, arm->custody_call_id);
    const NLCheckedNodeView *m =
        nl_checked_node_view(parent, parent->packet_match);
    if (call == NULL || !call->custody_call.entry_proved ||
        !call->custody_call.post_proved ||
        call->custody_call.origin != parent ||
        call->custody_call.origin_match != parent->packet_match ||
        call->custody_call.packet != m->packet_fork.packet ||
        arm->custody_entry == NULL || arm->custody_post == NULL)
        return false;
    if (!host(check, nl_sem_validate(c), call->span) ||
        !host(check, nl_raw_validate(c), call->span))
        return false;
    const NLPlaceId sink = c->bindings[symbol - 1].view.place;
    const NLValueId packet = m->packet_fork.packet,
                    some = c->places[sink - 1].current_value;
    const NLOccurrenceId occurrence = c->places[sink - 1].payload_occurrence;
    if (sink != call->custody_call.sink ||
        c->places[sink - 1].incarnation !=
            call->custody_call.sink_incarnation ||
        some != call->custody_call.new_some ||
        occurrence != call->custody_call.occurrence || occurrence == 0 ||
        occurrence <= before->occurrence_count ||
        c->values[some - 1].variant != 2 ||
        c->values[some - 1].sum_payload != packet ||
        c->bindings[symbol - 1].view.availability != NL_AVAILABLE ||
        c->bindings[m->packet_fork.binding - 1].view.availability !=
            NL_CONSUMED)
        return false;
    for (size_t i = 0; i < c->scope_count; ++i)
        if (c->scopes[i].active)
            return false;
    const NLPlaceId child = c->occurrences[occurrence - 1].payload_place;
    if (child <= before->place_count || !c->occurrences[occurrence - 1].live ||
        c->occurrences[occurrence - 1].root != sink ||
        !c->places[child - 1].live ||
        c->places[child - 1].current_value != packet ||
        c->values[packet - 1].owner_place != child ||
        c->values[packet - 1].sum_owner != some)
        return false;
    NLSemanticValueView expected_packet = before->values[packet - 1];
    expected_packet.carrier = NL_CARRIER_PLACE;
    expected_packet.owner_place = child;
    expected_packet.sum_owner = some;
    if (!node_unit_value_same(expected_packet, c->values[packet - 1]) ||
        c->values[some - 1].dependencies != NL_DEPENDENCY_FREE ||
        c->values[some - 1].value_dependency_count != 0)
        return false;
    NLSemanticContext *normalized = NULL;
    if (!host(check, nl_sem_clone(c, &normalized), call->span))
        return false;
    /* Compare all OTHER parent facts exactly. Only the demonstrated one
     * ownership edge (packet place -> current C payload) is projected here;
     * this comparison world is never published as the adopted continuation. */
    normalized->bindings[symbol - 1].view = before->bindings[symbol - 1].view;
    normalized->bindings[m->packet_fork.binding - 1].view =
        before->bindings[m->packet_fork.binding - 1].view;
    const NLPlaceId old_packet =
        before->bindings[m->packet_fork.binding - 1].view.place;
    normalized->places[sink - 1] = before->places[sink - 1];
    normalized->places[old_packet - 1] = before->places[old_packet - 1];
    const NLValueId old_none = before->bindings[symbol - 1].view.value;
    normalized->values[old_none - 1] = before->values[old_none - 1];
    normalized->values[packet - 1] = before->values[packet - 1];
    normalized->occurrences[occurrence - 1].live = false;
    normalized->places[child - 1].live = false;
    normalized->places[child - 1].current_value = 0;
    normalized->places[child - 1].current_fact = 0;
    normalized->places[child - 1].governing_domain = 0;
    normalized->values[some - 1].carrier = NL_CARRIER_ENDED;
    normalized->values[some - 1].sum_payload = 0;
    const NLCheckedNodeView unit = {.type = 1};
    const bool valid = nl_allocated_post_matches(before, normalized, &unit);
    nl_semantic_destroy(normalized);
    return valid;
}

static bool custody_final_target(Check *check, size_t floor, NLSourceSpan span)
{
    NLCheckedFragment *f = check->artifact;
    f->custody_floor = floor;
    NLSemanticContext *post = NULL;
    if (!host(check, nl_sem_clone(f->packet_post, &post), span))
        return false;
    f->custody_final_post = post;
    const NLCheckedNodeView *m = nl_checked_node_view(f, f->custody_join),
                            *p = nl_checked_node_view(f,
                                                      m->packet_fork.producer);
    const NLPlaceId head = p->producer.head.parent;
    const NLDomainId domain = p->producer.head_domain;
    const NLBackingRegionId region = post->places[head - 1].placement.region;
    if (!region || !post->regions[region - 1].view.live ||
        !post->domains[domain - 1].live)
        return false;
    nl_sum_detach(post, head);
    nl_fixed_detach(post, head);
    nl_sem_end_value(post, post->places[head - 1].current_value);
    post->places[head - 1].live = false;
    post->places[head - 1].current_value = 0;
    post->places[head - 1].current_fact = 0;
    post->places[head - 1].governing_domain = 0;
    post->places[head - 1].placement = (NLBackingRange){0};
    post->domains[domain - 1].live = false;
    post->regions[region - 1].view.live = false;
    post->raw_interval_count -= post->regions[region - 1].count;
    free(post->regions[region - 1].intervals);
    post->regions[region - 1].intervals = NULL;
    post->regions[region - 1].count = 0;
    for (size_t i = 0; i < post->binding_count; ++i) {
        NLSemanticBindingView *b = &post->bindings[i].view;
        const NLSemanticValueView v = post->values[b->value - 1];
        if ((b->type == 2 && v.domain == domain) ||
            (post->types[b->type - 1].view.kind == NL_TYPE_ALLOCATION &&
             v.allocation_region == region) ||
            i + 1 == f->custody_binding) {
            b->availability = NL_CONSUMED;
            nl_sum_detach(post, b->place);
            nl_fixed_detach(post, b->place);
            nl_sem_end_value(post, b->value);
            post->places[b->place - 1].live = false;
            post->places[b->place - 1].current_value = 0;
            post->places[b->place - 1].current_fact = 0;
            post->places[b->place - 1].governing_domain = 0;
        }
    }
    Check target = *check;
    target.context = post;
    if (!end_bindings(&target, floor, span)) {
        check->status = target.status;
        check->diagnostic = target.diagnostic;
        return false;
    }
    return host(check, nl_sem_validate(post), span) &&
           host(check, nl_raw_validate(post), span);
}

static bool custody_continue_block(Check *check, const NLSyntaxView *block,
                                   const NLSyntaxNode *first, size_t floor,
                                   NLCheckedNodeId block_id)
{
    const NLCheckedNodeId match = check->custody_pending;
    NLCheckedFragment *f = check->artifact;
    const NLSymbolId binding = f->custody_binding;
    const NLPlaceId sink = check->context->bindings[binding - 1].view.place;
    const NLCheckedNodeView *m = nl_checked_node_view(f, match);
    bool seen_adoption = false, seen_refusal = false;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_match_arm(f, match, i);
        const NLCheckedNodeView *r = nl_checked_node_view(arm, arm->root);
        if (arm->custody_call_id) {
            if (seen_adoption || !custody_adopted_frame(check, arm, binding))
                goto precision;
            seen_adoption = true;
        } else {
            if (seen_refusal || !nl_packet_arm_closed(f, match, arm, r))
                goto precision;
            seen_refusal = true;
        }
    }
    if (!seen_adoption || !seen_refusal ||
        !custody_final_target(check, floor, block->span))
        goto precision;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_match_arm(f, match, i);
        Check continuation = *check;
        continuation.custody_pending = 0;
        continuation.custody_continuation = true;
        continuation.custody_binding = binding;
        continuation.custody_sink = sink;
        continuation.custody_packet = m->packet_fork.packet;
        continuation.custody_origin = f;
        continuation.custody_origin_match = match;
        continuation.context = NULL;
        continuation.artifact = NULL;
        continuation.has_arm_floor = false;
        if (!host(check, nl_sem_clone(arm->context, &continuation.context),
                  block->span))
            return false;
        continuation.artifact = malloc(sizeof(*continuation.artifact));
        if (continuation.artifact == NULL) {
            nl_semantic_destroy(continuation.context);
            return host(check, NL_CHECK_OUT_OF_MEMORY, block->span);
        }
        *continuation.artifact = (NLCheckedFragment){0};
        f->custody_continuations[i] = continuation.artifact;
        continuation.artifact->source = check->source;
        continuation.artifact->context = continuation.context;
        continuation.artifact->custody_policy_world = arm->context;
        continuation.artifact->custody_continuation_world =
            continuation.context;
        continuation.artifact->destroy_context = nl_semantic_destroy;
        continuation.artifact->destroy_custody_world = nl_semantic_destroy;
        if (!host(check,
                  nl_sem_clone(arm->context,
                               &continuation.artifact->custody_entry),
                  block->span))
            return false;
        continuation.artifact->packet_parent = f;
        continuation.artifact->packet_match = match;
        continuation.artifact->destroy_packet_world = nl_semantic_destroy;
        if (!host(check,
                  nl_sem_clone(f->packet_entry,
                               &continuation.artifact->packet_entry),
                  block->span))
            return false;
        const NLSyntaxView suffix = {
            .kind = NL_SYNTAX_BLOCK,
            .span = block->span,
            .data.block = {.items = first, .tail = block->data.block.tail}};
        continuation.has_arm_floor = true;
        continuation.arm_floor = floor;
        continuation.artifact->root = source_block(&continuation, &suffix);
        if (!continuation.artifact->root || !continuation.custody_extracted ||
            !continuation.custody_final_none || continuation.terminated) {
            check->status = continuation.status;
            check->diagnostic = continuation.diagnostic;
            goto precision;
        }
        const NLSemanticContext *c = continuation.context;
        const NLSemanticBindingView b = c->bindings[binding - 1].view;
        if (b.availability != NL_CONSUMED || !c->bindings[binding - 1].hidden ||
            c->places[sink - 1].live || c->values[b.value - 1].variant != 1 ||
            c->values[b.value - 1].sum_payload != 0 ||
            c->values[b.value - 1].carrier != NL_CARRIER_ENDED)
            goto precision;
        NLSemanticContext *normalized = NULL;
        if (!host(check, nl_sem_clone(c, &normalized), block->span))
            return false;
        /* Historical ended None IDs differ; no live authority is reidentified.
         */
        normalized->bindings[binding - 1].view.value =
            f->custody_final_post->bindings[binding - 1].view.value;
        const NLCheckedNodeView *result = nl_checked_node_view(
            continuation.artifact, continuation.artifact->root);
        const bool same = nl_allocated_post_matches(f->custody_final_post,
                                                    normalized, result);
        nl_semantic_destroy(normalized);
        if (!same)
            goto precision;
    }
    NLSemanticContext *commit = NULL;
    if (!host(check, nl_sem_clone(f->custody_final_post, &commit), block->span))
        return false;
    nl_sem_commit(check->context, commit);
    check->custody_pending = 0;
    view(check, block_id)->type = 1;
    return true;
precision:
    if (check->status == NL_CHECK_OK)
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, block->span,
             "CUSTODY-CONDITIONAL-CONTINUATION",
             "both correlated source continuations and original final closure "
             "must be proved");
    return false;
}

const NLCheckedFragment *
nl_checked_custody_continuation(const NLCheckedFragment *f,
                                NLCheckedNodeId match, size_t i)
{
    return f != NULL && f->custody_join == match && i < 2
               ? f->custody_continuations[i]
               : NULL;
}

static bool custody_call_certificate(const NLCheckedFragment *parent,
                                     const NLCheckedFragment *arm,
                                     NLSymbolId custody)
{
    const NLCheckedNodeView *call =
        nl_checked_node_view(arm, arm->custody_call_id);
    const NLSemanticContext *entry = arm->custody_entry,
                            *post = arm->custody_post;
    if (call == NULL || entry == NULL || post == NULL || entry == post ||
        call->kind != NL_CHECKED_REGISTERED_CALL || call->argument_count != 2 ||
        !call->body_backed || !call->custody_call.entry_proved ||
        !call->custody_call.post_proved ||
        call->custody_call.origin != parent ||
        call->custody_call.origin_match != parent->custody_join ||
        call->custody_call.entry_world != entry ||
        call->custody_call.post_world != post ||
        call->custody_call.world != arm->context || call->function == 0 ||
        call->function > entry->function_count)
        return false;
    const NLFunctionEntry fn = entry->functions[call->function - 1];
    const NLCheckedFragment *body =
        nl_checked_call_body(arm, arm->custody_call_id);
    if (!fn.custody_recipient || fn.body == NULL || body == NULL ||
        body->body_owner != fn.body ||
        !fn.body->custody_definition.definition_checked ||
        fn.body->custody_definition.requirements != NL_CUSTODY_ALL_REQUIREMENTS)
        return false;
    NLCheckedFragment lookup = *arm;
    lookup.context = entry;
    if (!nl_packet_available_inherited(&lookup, call->custody_call.packet))
        return false;
    if (custody == 0 || custody > entry->binding_count ||
        custody > post->binding_count)
        return false;
    const NLPlaceId sink = entry->bindings[custody - 1].view.place;
    if (sink == 0 || sink > entry->place_count || sink > post->place_count)
        return false;
    const NLCheckedNodeView *arg0 =
                                nl_checked_node_view(arm, call->first_argument),
                            *arg1 = arg0 == NULL
                                        ? NULL
                                        : nl_checked_node_view(
                                              arm, arg0->next_argument);
    if (arg0 == NULL || arg1 == NULL || arg0->kind != NL_CHECKED_IDENTIFIER ||
        arg1->kind != NL_CHECKED_IDENTIFIER || arg0->result_count != 1 ||
        arg1->result_count != 1 || arg1->next_argument != 0 ||
        arg0->value_use != NL_VALUE_COPIED ||
        arg1->value_use != NL_VALUE_CONSUMED || arg0->symbol == 0 ||
        arg0->symbol > entry->binding_count || arg1->symbol == 0 ||
        arg1->symbol > entry->binding_count ||
        arg1->symbol > post->binding_count ||
        arg1->symbol != call->custody_call.donor ||
        arg1->results[0].value != call->custody_call.packet ||
        entry->bindings[arg1->symbol - 1].view.availability != NL_AVAILABLE ||
        post->bindings[arg1->symbol - 1].view.availability != NL_CONSUMED ||
        sink != call->custody_call.sink ||
        entry->places[sink - 1].incarnation !=
            call->custody_call.sink_incarnation)
        return false;
    const NLValueId ref = entry->bindings[arg0->symbol - 1].view.value;
    if (ref == 0 || ref > entry->value_count ||
        call->custody_call.old_none == 0 ||
        call->custody_call.old_none > post->value_count)
        return false;
    const NLSemanticValueView cap = entry->values[ref - 1];
    if (cap.type == 0 || cap.type > entry->type_count ||
        cap.reference.scope > entry->scope_count)
        return false;
    const NLSemanticTypeView type = entry->types[cap.type - 1].view;
    if (type.kind != NL_TYPE_REF || type.is_exclusive ||
        type.access != NL_ACCESS_WRITE ||
        type.target != entry->places[sink - 1].type ||
        cap.reference.place != sink ||
        cap.reference.provenance != NL_PROVENANCE_VALID ||
        !cap.reference.writable || cap.reference.scope == 0 ||
        !entry->scopes[cap.reference.scope - 1].active ||
        cap.reference.incarnation != entry->places[sink - 1].incarnation ||
        entry->values[entry->places[sink - 1].current_value - 1].variant != 1 ||
        entry->places[sink - 1].payload_occurrence != 0 ||
        call->custody_call.old_none != entry->places[sink - 1].current_value ||
        post->values[call->custody_call.old_none - 1].carrier !=
            NL_CARRIER_ENDED ||
        post->values[call->custody_call.old_none - 1].variant != 1 ||
        post->values[call->custody_call.old_none - 1].sum_payload != 0)
        return false;
    bool source_loan = false;
    for (size_t n = 0; n < arm->count; ++n) {
        const NLCheckedNodeView v = arm->nodes[n];
        source_loan |=
            v.kind == NL_CHECKED_LOAN_HEADER &&
            v.loan.ref_symbol == arg0->symbol &&
            v.loan.scope == cap.reference.scope && v.loan.source == custody &&
            v.loan.place == sink &&
            v.loan.incarnation == call->custody_call.sink_incarnation &&
            v.loan.access == NL_ACCESS_WRITE && v.loan.implicit_local &&
            !v.loan.from_ptr && v.loan.body_nonescape_proved;
    }
    if (!source_loan)
        return false;
    for (size_t i = 0; i < entry->scope_count; ++i)
        if (entry->scopes[i].active && i + 1 != cap.reference.scope)
            return false;
    for (size_t i = 0; i < entry->value_count; ++i) {
        const NLSemanticValueView v = entry->values[i];
        if (v.carrier != NL_CARRIER_ENDED &&
            (v.dependencies != NL_DEPENDENCY_FREE || v.value_dependency_count ||
             (entry->types[v.type - 1].view.kind == NL_TYPE_REF &&
              i + 1 != ref)))
            return false;
    }
    const NLValueId some = call->custody_call.new_some,
                    packet = call->custody_call.packet;
    const NLOccurrenceId occurrence = call->custody_call.occurrence;
    if (some == 0 || some > post->value_count ||
        occurrence <= entry->occurrence_count ||
        occurrence > post->occurrence_count ||
        post->places[sink - 1].current_value != some ||
        post->places[sink - 1].payload_occurrence != occurrence ||
        post->places[sink - 1].incarnation !=
            call->custody_call.sink_incarnation ||
        post->places[sink - 1].current_fact <= entry->last_value_fact ||
        post->values[some - 1].variant != 2 ||
        post->values[some - 1].sum_payload != packet ||
        !post->occurrences[occurrence - 1].live ||
        post->occurrences[occurrence - 1].root != sink)
        return false;
    const NLCheckedNodeView *policy =
        nl_checked_node_view(parent, parent->custody_join);
    const NLCheckedNodeView *producer =
        policy == NULL
            ? NULL
            : nl_checked_node_view(parent, policy->packet_fork.producer);
    if (producer == NULL || packet == 0 || packet > entry->value_count ||
        packet > post->value_count || producer->producer.root == 0 ||
        producer->producer.root > post->place_count ||
        producer->producer.range.region == 0 ||
        producer->producer.range.region > post->region_count ||
        producer->producer.domain == 0 ||
        producer->producer.domain > post->domain_count)
        return false;
    const NLSemanticPlaceView tail = post->places[producer->producer.root - 1];
    if (!tail.live || tail.incarnation != producer->producer.incarnation ||
        tail.governing_domain != producer->producer.domain ||
        tail.placement.region != producer->producer.range.region ||
        tail.placement.start != producer->producer.range.start ||
        tail.placement.length != producer->producer.range.length ||
        !post->regions[tail.placement.region - 1].view.live ||
        !post->domains[tail.governing_domain - 1].live ||
        post->values[packet - 1].field_count != 3 ||
        post->values[packet - 1].dependencies != NL_DEPENDENCY_FREE ||
        post->values[packet - 1].value_dependency_count != 0)
        return false;
    for (size_t i = 0; i < 3; ++i) {
        const NLValueId member = entry->values[packet - 1].fields[i];
        if (member == 0 || member > entry->value_count ||
            member > post->value_count ||
            post->values[packet - 1].fields[i] != member ||
            !node_unit_value_same(entry->values[member - 1],
                                  post->values[member - 1]))
            return false;
    }
    for (size_t i = 0; i < 2; ++i) {
        const NLSymbolId p = call->custody_call.parameters[i];
        if (p <= entry->binding_count || p > post->binding_count ||
            post->bindings[p - 1].view.availability != NL_CONSUMED ||
            (i == 1 && post->bindings[p - 1].view.value != packet))
            return false;
    }
    size_t first = 0;
    for (size_t n = 0; n < body->count; ++n) {
        const NLCheckedNodeView v = body->nodes[n];
        if (v.kind == NL_CHECKED_DESTROY || v.kind == NL_CHECKED_DEALLOCATE ||
            v.kind == NL_CHECKED_DOMAIN_FINALIZE)
            return false;
        if (v.custody_none_site == 1) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(body, v.initializer);
            if (init == NULL || init->kind != NL_CHECKED_IDENTIFIER ||
                init->value_use != NL_VALUE_CONSUMED ||
                init->results[0].value != call->custody_call.old_none ||
                v.item_count != 1)
                return false;
            ++first;
        }
    }
    return first == 1 && nl_sem_validate(entry) == NL_CHECK_OK &&
           nl_sem_validate(post) == NL_CHECK_OK &&
           nl_raw_validate(entry) == NL_CHECK_OK &&
           nl_raw_validate(post) == NL_CHECK_OK;
}

static bool custody_suffix_certificate(const NLCheckedFragment *parent,
                                       const NLCheckedFragment *arm,
                                       const NLCheckedFragment *suffix,
                                       bool adoption)
{
    const NLSemanticContext *entry = suffix->custody_entry,
                            *c = suffix->context;
    const NLCheckedNodeView *m = nl_checked_node_view(parent,
                                                      parent->custody_join),
                            *producer = nl_checked_node_view(
                                parent, m->packet_fork.producer);
    const NLValueId packet = m->packet_fork.packet;
    const NLSymbolId binding = parent->custody_binding;
    if (m == NULL || producer == NULL || entry == NULL || c == NULL ||
        binding == 0 || binding > entry->binding_count ||
        binding > c->binding_count ||
        suffix->custody_policy_world != arm->context ||
        suffix->custody_continuation_world != c)
        return false;
    const NLPlaceId sink = entry->bindings[binding - 1].view.place;
    if (sink == 0 || sink > entry->place_count || sink > c->place_count ||
        entry->places[sink - 1].current_value == 0 ||
        entry->places[sink - 1].current_value > entry->value_count)
        return false;
    if (entry->values[entry->places[sink - 1].current_value - 1].variant !=
            (adoption ? 2U : 1U) ||
        entry->values[entry->places[sink - 1].current_value - 1].sum_payload !=
            (adoption ? packet : 0))
        return false;
    if (suffix->packet_parent != parent ||
        suffix->packet_match != parent->custody_join || entry == NULL ||
        entry == arm->context || !nl_packet_same_entry(entry, arm->context) ||
        !nl_packet_same_entry(parent->packet_entry, suffix->packet_entry))
        return false;
    size_t extracts = 0, second = 0, receives = 0;
    NLValueId recovered = 0;
    for (size_t n = 0; n < suffix->count; ++n) {
        const NLCheckedNodeView v = suffix->nodes[n];
        if (v.custody_extraction.present) {
            const NLCheckedNodeView *a = nl_checked_node_view(suffix,
                                                              v.first_argument),
                                    *b = a == NULL
                                             ? NULL
                                             : nl_checked_node_view(
                                                   suffix, a->next_argument);
            if (a == NULL || b == NULL || v.argument_count != 2 ||
                a->kind != NL_CHECKED_IDENTIFIER || a->result_count != 1 ||
                a->results[0].value == 0 ||
                a->results[0].value > c->value_count ||
                b->kind != NL_CHECKED_SUM_CONSTRUCTOR || b->variant != 1 ||
                b->results[0].value != v.custody_extraction.new_sum ||
                v.result_count != 1 ||
                v.results[0].value != v.custody_extraction.old_sum)
                return false;
            const NLSemanticValueView ref = c->values[a->results[0].value - 1];
            if (c->types[ref.type - 1].view.kind != NL_TYPE_REF ||
                c->types[ref.type - 1].view.access != NL_ACCESS_WRITE ||
                ref.reference.provenance != NL_PROVENANCE_VALID ||
                !ref.reference.writable || ref.reference.place != sink ||
                ref.reference.incarnation !=
                    entry->places[sink - 1].incarnation)
                return false;
            bool source = false;
            for (size_t k = 0; k < suffix->count; ++k) {
                const NLCheckedNodeView loan = suffix->nodes[k];
                source |= loan.kind == NL_CHECKED_LOAN_HEADER &&
                          loan.loan.source == binding &&
                          loan.loan.scope == ref.reference.scope &&
                          loan.loan.place == sink &&
                          loan.loan.access == NL_ACCESS_WRITE &&
                          loan.loan.implicit_local &&
                          loan.loan.body_nonescape_proved;
            }
            if (!source || v.custody_extraction.new_sum == 0 ||
                v.custody_extraction.new_sum > c->value_count)
                return false;
            if (v.kind != NL_CHECKED_REPLACE ||
                v.custody_extraction.sink != sink ||
                v.custody_extraction.old_sum !=
                    entry->places[sink - 1].current_value ||
                v.custody_extraction.old_occurrence !=
                    entry->places[sink - 1].payload_occurrence ||
                v.custody_extraction.packet != (adoption ? packet : 0) ||
                v.custody_extraction.new_sum == 0 ||
                c->values[v.custody_extraction.new_sum - 1].variant != 1 ||
                c->values[v.custody_extraction.new_sum - 1].sum_payload != 0 ||
                (adoption &&
                 (v.custody_extraction.old_occurrence == 0 ||
                  v.custody_extraction.old_occurrence > c->occurrence_count ||
                  c->occurrences[v.custody_extraction.old_occurrence - 1]
                      .live)))
                return false;
            recovered = v.custody_extraction.old_sum;
            ++extracts;
        }
        if (v.custody_none_site == 2) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(suffix, v.initializer);
            if (init == NULL || init->symbol != binding ||
                init->value_use != NL_VALUE_CONSUMED || v.item_count != 1 ||
                v.custody_selected_variant != 1)
                return false;
            ++second;
        }
        if (v.kind == NL_CHECKED_AGGREGATE_BINDING &&
            v.packet_origin.ancestor != NULL) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(suffix, v.initializer);
            if (!adoption || init == NULL ||
                init->value_use != NL_VALUE_CONSUMED ||
                init->results[0].value != packet ||
                v.packet_origin.ancestor != parent ||
                v.packet_origin.world != c ||
                v.packet_origin.entry_world != suffix->packet_entry ||
                v.packet_origin.match != parent->custody_join ||
                v.packet_origin.packet != packet || v.argument_count != 3)
                return false;
            bool fields[3] = {false, false, false};
            NLCheckedNodeId receiver_id = v.first_argument;
            for (size_t field = 0; field < 3; ++field) {
                const NLCheckedNodeView *receiver =
                    nl_checked_node_view(suffix, receiver_id);
                if (receiver == NULL || receiver->kind != NL_CHECKED_RECEIVER ||
                    receiver->field_index >= 3 ||
                    fields[receiver->field_index] || receiver->symbol == 0 ||
                    receiver->symbol > c->binding_count ||
                    c->bindings[receiver->symbol - 1].view.value !=
                        parent->packet_entry->values[packet - 1]
                            .fields[receiver->field_index])
                    return false;
                fields[receiver->field_index] = true;
                receiver_id = receiver->next_argument;
            }
            if (receiver_id != 0)
                return false;
            ++receives;
        }
    }
    size_t recovery_matches = 0;
    for (size_t n = 0; n < suffix->count; ++n) {
        const NLCheckedNodeView v = suffix->nodes[n];
        const NLCheckedNodeView *init =
            nl_checked_node_view(suffix, v.initializer);
        if (v.kind == NL_CHECKED_MATCH && !v.custody_none_site &&
            v.custody_selected_variant) {
            if (init == NULL || init->results[0].value != recovered ||
                v.custody_selected_variant != (adoption ? 2U : 1U) ||
                v.item_count != 2)
                return false;
            ++recovery_matches;
        }
    }
    if (extracts != 1 || second != 1 || recovery_matches != 1 ||
        receives != (adoption ? 1U : 0U))
        return false;
    if (adoption) {
        const NLCheckedNodeView *call =
            nl_checked_node_view(suffix, suffix->owner_entry_call);
        const NLSemanticContext *actual =
            nl_checked_owner_entry(suffix, suffix->owner_entry_call);
        if (call == NULL || actual == NULL || !call->owner_call.entry_proved ||
            !call->owner_call.post_proved ||
            call->owner_call.root != producer->producer.root ||
            call->owner_call.incarnation != producer->producer.incarnation ||
            call->owner_call.range.region != producer->producer.range.region ||
            call->owner_call.domain != producer->producer.domain ||
            call->owner_call.inputs[1] != producer->producer.inputs[2] ||
            call->owner_call.inputs[2] != producer->producer.inputs[3] ||
            nl_owner_relations(actual, call->owner_call.inputs,
                               &call->owner_call.definition, call->span,
                               NULL) != NL_CHECK_OK)
            return false;
        const NLCheckedFragment *body =
            nl_checked_call_body(suffix, suffix->owner_entry_call);
        if (body == NULL ||
            !body->body_owner->owner_definition.definition_checked)
            return false;
        if (body->context != c || call->function == 0 ||
            call->function > actual->function_count ||
            !actual->functions[call->function - 1].owner_receiver ||
            body->body_owner != actual->functions[call->function - 1].body ||
            call->owner_call.range.start != producer->producer.range.start ||
            call->owner_call.range.length != producer->producer.range.length)
            return false;
        NLCheckedNodeId arg_id = call->first_argument;
        for (size_t i = 0; i < 3; ++i) {
            const NLCheckedNodeView *arg = nl_checked_node_view(suffix, arg_id);
            const NLSymbolId parameter = call->owner_call.parameters[i],
                             donor = call->owner_call.donor[i];
            if (arg == NULL || arg->kind != NL_CHECKED_IDENTIFIER ||
                arg->result_count != 1 || arg->symbol != donor ||
                arg->results[0].value != call->owner_call.inputs[i] ||
                arg->value_use !=
                    (i == 0 ? NL_VALUE_COPIED : NL_VALUE_CONSUMED) ||
                donor == 0 || donor > actual->binding_count ||
                donor > c->binding_count || parameter == 0 ||
                parameter > c->binding_count || parameter == donor ||
                c->bindings[parameter - 1].view.value !=
                    call->owner_call.inputs[i] ||
                (i != 0 &&
                 (actual->bindings[donor - 1].view.availability !=
                      NL_CONSUMED ||
                  c->bindings[donor - 1].view.availability != NL_CONSUMED ||
                  c->bindings[parameter - 1].view.availability != NL_CONSUMED)))
                return false;
            arg_id = arg->next_argument;
        }
        if (arg_id != 0)
            return false;
        size_t ends = 0, frees = 0;
        for (size_t n = 0; n < body->count; ++n) {
            ends += body->nodes[n].kind == NL_CHECKED_DESTROY;
            frees += body->nodes[n].kind == NL_CHECKED_DEALLOCATE;
        }
        if (ends != 1 || frees != 1)
            return false;
    } else if (suffix->owner_entry != NULL || suffix->owner_entry_call != 0)
        return false;
    const NLSemanticBindingView b = c->bindings[binding - 1].view;
    if (b.availability != NL_CONSUMED || !c->bindings[binding - 1].hidden ||
        c->places[sink - 1].live ||
        c->values[b.value - 1].carrier != NL_CARRIER_ENDED ||
        c->values[b.value - 1].variant != 1 ||
        c->values[b.value - 1].sum_payload != 0)
        return false;
    NLSemanticContext *normalized = NULL;
    if (nl_sem_clone(c, &normalized) != NL_CHECK_OK)
        return false;
    normalized->bindings[binding - 1].view.value =
        parent->custody_final_post->bindings[binding - 1].view.value;
    const bool same =
        nl_allocated_post_matches(parent->custody_final_post, normalized,
                                  nl_checked_node_view(suffix, suffix->root));
    nl_semantic_destroy(normalized);
    return same && nl_raw_validate(c) == NL_CHECK_OK;
}

bool nl_checked_custody_valid(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    const NLCheckedNodeView *m = nl_checked_node_view(f, id);
    if (m == NULL || m->kind != NL_CHECKED_MATCH || f->custody_join != id ||
        m->normal_arms != 2 || m->item_count != 2 ||
        m->normal_frame_unchanged || m->packet_fork.closed ||
        m->packet_fork.retained || f->custody_binding == 0 ||
        f->packet_match != id ||
        m->packet_fork.entry_world != f->packet_entry ||
        f->packet_entry == NULL || f->packet_retained_post == NULL ||
        f->custody_final_post == NULL ||
        !nl_checked_producer_valid(f, m->packet_fork.producer))
        return false;
    if (f->custody_binding > f->packet_entry->binding_count ||
        f->custody_floor > f->packet_entry->binding_count)
        return false;
    /* Reconstruct, don't trust the saved target flag/world. Temporary owns only
     * its new target; all original evidence is borrowed and stays immutable. */
    NLCheckedFragment reconstruction = *f;
    reconstruction.custody_final_post = NULL;
    Check reader = {.artifact = &reconstruction};
    if (!custody_final_target(&reader, f->custody_floor, m->span)) {
        nl_semantic_destroy(reconstruction.custody_final_post);
        return false;
    }
    const bool target = nl_packet_same_entry(reconstruction.custody_final_post,
                                             f->custody_final_post);
    nl_semantic_destroy(reconstruction.custody_final_post);
    if (!target)
        return false;
    bool adoption = false, refusal = false, variants[2] = {false, false};
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm = nl_checked_match_arm(f, id, i),
                                *suffix = f->custody_continuations[i];
        if (arm == NULL || suffix == NULL || arm->packet_parent != f ||
            arm->packet_match != id ||
            !nl_packet_same_entry(f->packet_entry, arm->packet_entry))
            return false;
        const NLCheckedNodeView *root = nl_checked_node_view(arm, arm->root);
        if (root == NULL || root->kind != NL_CHECKED_MATCH_ARM ||
            root->variant < 1 || root->variant > 2 ||
            variants[root->variant - 1] ||
            nl_control_exits_count(arm->exits) != 0 ||
            nl_control_exits_count(arm->loop_returns) != 0)
            return false;
        variants[root->variant - 1] = true;
        const bool adopted = arm->custody_call_id != 0;
        Check compare = {.artifact = (NLCheckedFragment *)f};
        if (adopted) {
            if (adoption ||
                !custody_call_certificate(f, arm, f->custody_binding) ||
                !custody_adopted_frame(&compare, arm, f->custody_binding))
                return false;
            adoption = true;
        } else {
            if (refusal ||
                !nl_packet_arm_closed(f, id, arm,
                                      nl_checked_node_view(arm, arm->root)))
                return false;
            refusal = true;
        }
        if (!custody_suffix_certificate(f, arm, suffix, adopted))
            return false;
    }
    return adoption && refusal;
}

static NLCheckedNodeId sum_match(Check *check, const NLSyntaxView *s)
{
    if (check->in_function_body || check->loop != NULL)
        return function_match(check, s);
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
    if (borrowed && c->places[root - 1].parent_aggregate != 0) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, s->span,
             "NODE-LINK-MATCH-PRECISION",
             "borrowed fixed-link match needs aggregate-aware occurrence "
             "guards; "
             "match a copied Option value in this slice");
        return 0;
    }
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
        if (a->data.arm.payload && !a->data.arm.wildcard &&
            !lexical_source_name(check, a->data.arm.binding))
            return 0;
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

/* §13.8 / Draft 17.19 bounded local source gates. The implicit authority is
 * the source-local place/current incarnation, never a domain-zero fixture or
 * ptr existence. Write remains ordinary/non-exclusive mutation authority. */
static NLCheckedNodeId source_local_loan(Check *check, const NLSyntaxView *s)
{
    NLSemanticContext *c = check->context;
    const NLSyntaxView *operand_node = nl_syntax_node_view(s->data.loan.source);
    const bool field = operand_node->kind == NL_SYNTAX_FIELD_DESIGNATOR;
    const NLSourceSpan operand =
        field ? operand_node->data.field_designator.base : operand_node->span;
    NLCheckedField field_evidence = {0};
    if (field &&
        !fixed_selection(check, operand_node, NL_ACCESS_WRITE, &field_evidence))
        return 0;
    const NLSymbolId source = available(check, operand);
    if (source == 0)
        return 0;
    if (c->bindings[source - 1].view.type == 2 || s->data.loan.is_exclusive)
        return allocated_domain_loan(check, s);
    const NLSemanticBindingView binding = c->bindings[source - 1].view;
    const NLSemanticTypeView type = c->types[binding.type - 1].view;
    const bool write = s->data.loan.access == NL_ACCESS_WRITE;
    NLPlaceId place = field ? field_evidence.child : binding.place;
    if (write && s->data.loan.from_ptr) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, operand,
             "LOCAL-LOAN-PROFILE",
             "bounded write loan requires a direct lexical local");
        return 0;
    }
    if (s->data.loan.from_ptr) {
        if (type.kind != NL_TYPE_PTR) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, operand, "P3-PTR-REQUIRED",
                 "loan_read_ptr requires a pointer local");
            return 0;
        }
        if (!concrete_ref(check, binding.value, operand) ||
            !reference_live(check, binding.value, operand))
            return 0;
        const NLSemanticValueView ptr = c->values[binding.value - 1];
        if (ptr.dependencies != NL_DEPENDENCY_FREE) {
            fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, operand,
                 "P3-DEPENDENCIES-UNSUPPORTED", "ptr dependencies need proof");
            return 0;
        }
        if (!ptr.reference.readable) {
            fail(check, NL_CHECK_SEMANTIC_ERROR, operand, "P3-ACCESS",
                 "ptr evidence does not permit ordinary read access");
            return 0;
        }
        place = ptr.reference.place;
    }
    const NLSemanticPlaceView root = c->places[place - 1];
    const NLSemanticPlaceView stability_root =
        field ? c->places[field_evidence.parent - 1] : root;
    bool visible = false;
    for (size_t i = check->namespace_floor; i < c->binding_count; ++i)
        if (!c->bindings[i].hidden &&
            c->bindings[i].view.availability == NL_AVAILABLE &&
            c->bindings[i].view.place ==
                (field ? field_evidence.parent : place))
            visible = true;
    const bool supported_type =
        root.type == nl_semantic_core_type(c, NL_TYPE_U8) || field ||
        (check->allocated_slice && custody_type(c, root.type)) ||
        (!write && nl_recursive_local_type(c, root.type));
    if (!stability_root.implicit_local || !visible || !root.live ||
        root.governing_domain != 0 || !stability_root.independent_root ||
        root.parent_sum != 0 || root.placement.region != 0 || !supported_type) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, operand,
             "LOCAL-LOAN-PROFILE",
             "requires a supported current lexical root or fixed field");
        return 0;
    }
    if (c->values[root.current_value - 1].dependencies != NL_DEPENDENCY_FREE) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, operand,
             "P3-DEPENDENCIES-UNSUPPORTED", "local dependencies need proof");
        return 0;
    }
    if (conflicts(check, place, false, false, 0, operand))
        return 0;
    char *name = source_name_copy(check, s->data.loan.binding);
    if (name == NULL)
        return 0;
    const size_t bindings = c->binding_count, scopes = c->scope_count;
    const NLTypeId ref_type = compound(check, NL_TYPE_REF, root.type,
                                       s->data.loan.access, false, s->span);
    NLScopeId scope = 0;
    NLSymbolId ref_symbol = 0;
    NLCheckedNodeId id = 0;
    if (ref_type == 0 ||
        !host(check, nl_sem_new_scope(c, 0, true, &scope), s->span))
        goto cleanup;
    const NLValueId ref = new_value(
        check,
        (NLSemanticValueView){.type = ref_type,
                              .reference = {.place = place,
                                            .incarnation = root.incarnation,
                                            .scope = scope,
                                            .provenance = NL_PROVENANCE_VALID,
                                            .readable = true,
                                            .writable = write}},
        s->span);
    if (ref == 0 ||
        !host(check, nl_sem_bind_in_scope(c, name, ref, bindings, &ref_symbol),
              s->data.loan.binding))
        goto cleanup;
    id = add(check,
             (NLCheckedNodeView){.kind = NL_CHECKED_LOAN_HEADER,
                                 .span = s->span,
                                 .field = field_evidence,
                                 .loan = {.source = source,
                                          .place = place,
                                          .incarnation = root.incarnation,
                                          .scope = scope,
                                          .access = s->data.loan.access,
                                          .prevent_lifetime_end = true,
                                          .implicit_local = true,
                                          .from_ptr = s->data.loan.from_ptr,
                                          .ref_symbol = ref_symbol}});
    if (id == 0)
        goto cleanup;
    /* Body locals (including the generated binder) end at the block boundary.
     * A copied tail package stays loose and is included in the exit check. */
    const bool saved_arm = check->has_arm_floor;
    const size_t saved_floor = check->arm_floor;
    const bool saved_loan = check->in_source_loan;
    LoopControl *saved_loop = check->loop;
    check->has_arm_floor = true;
    check->arm_floor = bindings;
    check->in_source_loan = true;
    check->loop = NULL;
    const NLCheckedNodeId body =
        source_block(check, nl_syntax_node_view(s->data.loan.body));
    check->has_arm_floor = saved_arm;
    check->arm_floor = saved_floor;
    check->in_source_loan = saved_loan;
    check->loop = saved_loop;
    if (body == 0) {
        id = 0;
        goto cleanup;
    }
    if (check->terminated) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span,
             "LOCAL-LOAN-PROFILE", "gate requires a normal exactly-once body");
        id = 0;
        goto cleanup;
    }
    const NLCheckStatus exit = nl_sem_function_exit(c, scopes, c->place_count);
    if (exit != NL_CHECK_OK) {
        fail(check, exit, s->span, "P8-EXIT-DEPENDENCY",
             "surviving state depends on the ending loan scope");
        id = 0;
        goto cleanup;
    }
    for (size_t i = scopes; i < c->scope_count; ++i)
        c->scopes[i].active = false;
    const NLCheckedNodeView result = *view(check, body);
    view(check, id)->initializer = body;
    view(check, id)->type = result.type;
    view(check, id)->result_count = result.result_count;
    memcpy(view(check, id)->results, result.results, sizeof(result.results));
    view(check, id)->loan.body_nonescape_proved = true;
    view(check, id)->loan.normal_result_forwarded = true;
cleanup:
    free(name);
    return id;
}

static NLCheckedNodeId allocated_raw(Check *check, const NLSyntaxView *syntax,
                                     NLRawOperationKind kind)
{
    if (!check->allocated_slice) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
             "ALLOCATED-SOURCE-PROFILE",
             "raw lifecycle operands require a checked allocation Some world");
        return 0;
    }
    NLSemanticContext *c = check->context;
    NLRawOperation operation = {
        .kind = kind, .source = check->source, .span = syntax->span};
    NLTypeId target = 0;
    if (kind != NL_RAW_DEALLOCATE) {
        target = check_type(check, syntax->data.call.type);
        if (target == 0)
            return 0;
        if (!nl_recursive_local_type(c, target)) {
            fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
                 "ALLOCATED-TARGET-PROFILE",
                 "slot target must be the completed recursive H");
            return 0;
        }
        operation.data.slot_target = target;
    }
    size_t count = kind == NL_RAW_DEALLOCATE ? 2 : 1;
    if (syntax->data.call.argument_count != count) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span, "ALLOCATED-ARITY",
             "incorrect lifecycle operand count");
        return 0;
    }
    NLCheckedNodeId id =
        add(check, (NLCheckedNodeView){.kind = kind == NL_RAW_DEALLOCATE
                                                   ? NL_CHECKED_DEALLOCATE
                                               : kind == NL_RAW_INTO_SLOT
                                                   ? NL_CHECKED_INTO_SLOT
                                                   : NL_CHECKED_ERASE_SLOT,
                                       .span = syntax->span});
    if (id == 0)
        return 0;
    NLValueId inputs[2] = {0};
    NLCheckedNodeId args[2] = {0};
    const NLSyntaxNode *operand = syntax->data.call.arguments;
    for (size_t i = 0; i < count; ++i) {
        operation.operands[i].span = nl_syntax_node_view(operand)->span;
        NLTypeId expected =
            kind == NL_RAW_DEALLOCATE
                ? nl_semantic_core_type(c, i == 0 ? NL_TYPE_ALLOCATION
                                                  : NL_TYPE_STORAGE)
            : kind == NL_RAW_INTO_SLOT
                ? nl_semantic_core_type(c, NL_TYPE_STORAGE)
                : compound(check, NL_TYPE_SLOT, target, NL_ACCESS_READ, false,
                           syntax->span);
        args[i] = argument(check, operand, expected);
        if (args[i] == 0 || (inputs[i] = one_result(check, args[i])) == 0)
            return 0;
        if (i == 0)
            view(check, id)->first_argument = args[i];
        else
            view(check, args[i - 1])->next_argument = args[i];
        operand = nl_syntax_next_argument(operand);
    }
    check->status = nl_raw_apply(c, &operation, inputs, view(check, id),
                                 &check->diagnostic);
    if (check->status != NL_CHECK_OK)
        return 0;
    view(check, id)->argument_count = count;
    for (size_t i = 0; i < count; ++i)
        end_temporary(check, inputs[i]);
    return id;
}

static NLCheckedNodeId allocated_ref(Check *check, const NLSyntaxView *syntax)
{
    if (!check->allocated_slice || syntax->data.call.argument_count != 2) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, syntax->span,
             "ALLOCATED-REF-PROFILE",
             "explicit allocated ref requires pointer and domain stability");
        return 0;
    }
    NLSemanticContext *c = check->context;
    const bool write = syntax->data.call.access == NL_ACCESS_WRITE;
    const NLSyntaxNode *pn = syntax->data.call.arguments,
                       *sn = nl_syntax_next_argument(pn);
    NLSymbolId p = available(check, nl_syntax_node_view(pn)->span),
               stable = available(check, nl_syntax_node_view(sn)->span);
    if (p == 0 || stable == 0)
        return 0;
    NLValueId pv = c->bindings[p - 1].view.value,
              sv = c->bindings[stable - 1].view.value;
    NLSemanticTypeView pt = c->types[c->values[pv - 1].type - 1].view,
                       st = c->types[c->values[sv - 1].type - 1].view;
    if (pt.kind != NL_TYPE_PTR || !nl_recursive_local_type(c, pt.target) ||
        st.kind != NL_TYPE_REF || st.is_exclusive) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span, "ALLOCATED-REF-TYPE",
             "requires H pointer and ordinary domain stability ref");
        return 0;
    }
    if (!concrete_ref(check, pv, syntax->span) ||
        !reference_live(check, pv, syntax->span))
        return 0;
    if (c->values[pv - 1].dependencies != NL_DEPENDENCY_FREE ||
        c->values[sv - 1].dependencies != NL_DEPENDENCY_FREE) {
        fail(check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, syntax->span,
             "ALLOCATED-REF-DEPENDENCY-PRECISION",
             "unproved dependencies cannot be erased during ref acquisition");
        return 0;
    }
    NLDomainId domain = domain_reference(check, sv, false, syntax->span);
    NLReferenceFacts facts = c->values[pv - 1].reference;
    if (domain == 0)
        return 0;
    if (c->places[facts.place - 1].governing_domain != domain ||
        !facts.readable) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, syntax->span,
             "ALLOCATED-DOMAIN-MISMATCH",
             "pointer requires matching readable live domain");
        return 0;
    }
    if (write) {
        const NLCheckStatus access = nl_allocated_write_access(c, pv);
        if (access != NL_CHECK_OK) {
            fail(check, access, syntax->span, "HEAP-ROOT-WRITE-ACCESS",
                 "write reloan requires proven allocated-root and ptr write "
                 "access");
            return 0;
        }
    }
    if (conflicts(check, facts.place, false, false, 0, syntax->span))
        return 0;
    facts.scope = c->values[sv - 1].reference.scope;
    facts.writable = write;
    NLTypeId type = compound(check, NL_TYPE_REF, pt.target,
                             syntax->data.call.access, false, syntax->span);
    if (type == 0)
        return 0;
    /* Preserve selected operands using the existing argument/Copy contract.
     * The ordinary domain ref is a scoped stability borrow, never its owner:
     * copying its capability package creates no new domain/lifetime authority.
     * These temporary packages end below; their immutable operand evidence
     * remains owned by this same arm artifact for native lowering. */
    NLTypeId stable_type =
        compound(check, NL_TYPE_REF, nl_semantic_domain_type(c), NL_ACCESS_READ,
                 false, syntax->span);
    if (stable_type == 0)
        return 0;
    NLCheckedNodeId pointer_arg = argument(check, pn, c->values[pv - 1].type);
    if (pointer_arg == 0)
        return 0;
    NLCheckedNodeId stability_arg = argument(check, sn, stable_type);
    if (stability_arg == 0)
        return 0;
    view(check, pointer_arg)->next_argument = stability_arg;
    NLValueId ref = new_value(
        check, (NLSemanticValueView){.type = type, .reference = facts},
        syntax->span);
    if (ref == 0)
        return 0;
    end_temporary(check, one_result(check, pointer_arg));
    end_temporary(check, one_result(check, stability_arg));
    return add(check,
               (NLCheckedNodeView){.kind = NL_CHECKED_REF_FROM_PTR,
                                   .first_argument = pointer_arg,
                                   .argument_count = 2,
                                   .span = syntax->span,
                                   .type = type,
                                   .result_count = 1,
                                   .results = {{type, ref}},
                                   .has_reference_result = true,
                                   .reference_result = facts,
                                   .lifetime_place = facts.place,
                                   .lifetime_incarnation = facts.incarnation,
                                   .lifetime_domain = domain,
                                   .lifetime_range =
                                       c->places[facts.place - 1].placement});
}

static NLCheckedNodeId allocated_domain_loan(Check *check,
                                             const NLSyntaxView *s)
{
    if (!check->allocated_slice || s->data.loan.access != NL_ACCESS_READ) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span,
             "ALLOCATED-DOMAIN-LOAN",
             "bounded domain loan requires read authority in Some world");
        return 0;
    }
    NLSemanticContext *c = check->context;
    size_t bindings = c->binding_count, scopes = c->scope_count;
    NLCheckedNodeId id = loan(check, s);
    if (id == 0)
        return 0;
    NLCheckedNodeView header = *view(check, id);
    c->scopes[header.loan.scope - 1].active = true;
    if (header.loan.domain == 0 || c->types[header.type - 1].view.target != 2) {
        fail(check, NL_CHECK_SEMANTIC_ERROR, s->span,
             "ALLOCATED-DOMAIN-REQUIRED", "loan requires LifetimeDomain owner");
        return 0;
    }
    if (s->data.loan.is_exclusive) {
        for (size_t i = 0; i < c->value_count; ++i) {
            NLSemanticValueView v = c->values[i];
            if (v.carrier != NL_CARRIER_ENDED &&
                c->types[v.type - 1].view.kind == NL_TYPE_REF &&
                v.reference.scope != 0 &&
                c->scopes[v.reference.scope - 1].active &&
                v.reference.place != 0 &&
                c->places[v.reference.place - 1].governing_domain ==
                    header.loan.domain) {
                fail(check, NL_CHECK_SEMANTIC_ERROR, s->span,
                     "ALLOCATED-DOMAIN-CONFLICT",
                     "live governed ref blocks exclusive ending authority");
                return 0;
            }
        }
    }
    NLValueId ref =
        new_value(check,
                  (NLSemanticValueView){
                      .type = header.type,
                      .reference = {.place = header.loan.place,
                                    .incarnation = header.loan.incarnation,
                                    .scope = header.loan.scope,
                                    .provenance = NL_PROVENANCE_VALID,
                                    .readable = true}},
                  s->span);
    char *name = source_name_copy(check, s->data.loan.binding);
    NLSymbolId symbol = 0;
    if (ref == 0 || name == NULL) {
        free(name);
        return 0;
    }
    NLCheckStatus status =
        nl_sem_bind_in_scope(c, name, ref, bindings, &symbol);
    free(name);
    if (!host(check, status, s->span))
        return 0;
    view(check, id)->loan.ref_symbol = symbol;
    bool saved_arm = check->has_arm_floor, saved_loan = check->in_source_loan;
    size_t saved_floor = check->arm_floor;
    LoopControl *saved_loop = check->loop;
    check->has_arm_floor = true;
    check->arm_floor = bindings;
    check->in_source_loan = true;
    check->loop = NULL;
    NLCheckedNodeId body =
        source_block(check, nl_syntax_node_view(s->data.loan.body));
    check->has_arm_floor = saved_arm;
    check->arm_floor = saved_floor;
    check->in_source_loan = saved_loan;
    check->loop = saved_loop;
    if (body == 0)
        return 0;
    if (!host(check, nl_sem_function_exit(c, scopes, c->place_count), s->span))
        return 0;
    for (size_t i = scopes; i < c->scope_count; ++i)
        c->scopes[i].active = false;
    NLCheckedNodeView result = *view(check, body);
    view(check, id)->initializer = body;
    view(check, id)->type = result.type;
    view(check, id)->result_count = result.result_count;
    memcpy(view(check, id)->results, result.results, sizeof(result.results));
    view(check, id)->loan.body_nonescape_proved = true;
    view(check, id)->loan.normal_result_forwarded = true;
    return id;
}

static NLCheckedNodeId loan(Check *check, const NLSyntaxView *syntax)
{
    if (!lexical_source_name(check, syntax->data.loan.binding))
        return 0;
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
    CHECK_SOURCE,
    CHECK_CLOSURE_EXPRESSION
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
         root->kind != NL_SYNTAX_OPTION_PTR &&
         root->kind != NL_SYNTAX_OPTION_LIVE_TAIL &&
         root->kind != NL_SYNTAX_OPTION_BACKING &&
         root->kind != NL_SYNTAX_TYPE_PTR &&
         root->kind != NL_SYNTAX_TYPE_REF) ||
        ((entry == CHECK_EXPRESSION || entry == CHECK_CLOSURE_EXPRESSION) &&
         root->kind != NL_SYNTAX_EXPR_NAME &&
         root->kind != NL_SYNTAX_EXPR_CALL) ||
        (entry == CHECK_BINDING && root->kind != NL_SYNTAX_BINDING) ||
        (entry == CHECK_SOURCE && root->kind != NL_SYNTAX_EXPR_NAME &&
         root->kind != NL_SYNTAX_EXPR_CALL && root->kind != NL_SYNTAX_BINDING &&
         root->kind != NL_SYNTAX_MULTI_BINDING &&
         root->kind != NL_SYNTAX_AGGREGATE_BINDING &&
         root->kind != NL_SYNTAX_AGGREGATE &&
         root->kind != NL_SYNTAX_SUM_CONSTRUCTOR &&
         root->kind != NL_SYNTAX_FIELD_DESIGNATOR &&
         root->kind != NL_SYNTAX_MATCH && root->kind != NL_SYNTAX_IF &&
         root->kind != NL_SYNTAX_LOOP && root->kind != NL_SYNTAX_BLOCK &&
         root->kind != NL_SYNTAX_STATEMENT &&
         root->kind != NL_SYNTAX_U8_LITERAL &&
         root->kind != NL_SYNTAX_LOCAL_READ_LOAN &&
         root->kind != NL_SYNTAX_LOCAL_WRITE_LOAN &&
         root->kind != NL_SYNTAX_ALLOCATED_TRY &&
         root->kind != NL_SYNTAX_ALLOCATED_INTO_SLOT &&
         root->kind != NL_SYNTAX_ALLOCATED_ERASE_SLOT &&
         root->kind != NL_SYNTAX_ALLOCATED_REF) ||
        (entry == CHECK_LOAN && root->kind != NL_SYNTAX_LOAN)) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    Check check = {.source = nl_syntax_tree_source(tree),
                   .closure_probe = entry == CHECK_CLOSURE_EXPRESSION};
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
    const NLCheckStatus dependency_status =
        nl_fixed_dependencies(check.context);
    if (dependency_status != NL_CHECK_OK) {
        fail(&check, dependency_status, root->span,
             "P3-DEPENDENCIES-UNSUPPORTED",
             "semantic dependency evidence is unknown, unsupported or stale");
        goto failure;
    }
    if (entry == CHECK_TYPE) {
        const NLTypeId type = check_type(&check, nl_syntax_tree_root(tree));
        if (type != 0) {
            check.artifact->root =
                add(&check, (NLCheckedNodeView){.kind = NL_CHECKED_TYPE,
                                                .span = root->span,
                                                .type = type});
        }
    } else if (entry == CHECK_EXPRESSION || entry == CHECK_CLOSURE_EXPRESSION) {
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
    if (!host(&check, nl_sem_validate(check.context), root->span) ||
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
    if (check->terminated)
        return id;
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
    if (!host(&check, nl_sem_validate(c), operation->span) ||
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
        if (v.dependencies != NL_DEPENDENCY_FREE) {
            check.status = NL_CHECK_ANALYSIS_PRECISION_LIMIT;
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
        check.status = nl_sem_validate(check.context);
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
           (t.kind == NL_TYPE_NOMINAL || t.kind == NL_TYPE_BOOL ||
            t.kind == NL_TYPE_UNIT || t.kind == NL_TYPE_BYTE ||
            t.kind == NL_TYPE_U8 || t.kind == NL_TYPE_USIZE ||
            t.kind == NL_TYPE_ADDR);
}
static bool body_signature_type(const NLSemanticContext *c, NLTypeId type,
                                bool parameter)
{
    if (body_plain_type(c, type))
        return true;
    const NLSemanticTypeView t = c->types[type - 1].view;
    if (t.kind == NL_TYPE_SUM) {
        for (size_t i = 0; i < t.variant_count; ++i) {
            NLTypeId payload = c->types[type - 1].variant_types[i];
            if (payload != 0 && !body_plain_type(c, payload))
                return false;
        }
        return true;
    }
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
    NLFunctionEntry *entry = &registration->context->functions[function_id - 1];
    if (entry->custody_recipient) {
        registration->status = nl_custody_definition(
            registration->context, entry->body,
            &entry->body->custody_definition, &registration->diagnostic);
        return registration->status == NL_CHECK_OK;
    }
    if (entry->owner_receiver || entry->owner_producer) {
        registration->status = nl_owner_definition(
            registration->context, entry->body,
            registration->context
                ->types[entry->parameters[entry->owner_producer ? 1 : 0] - 1]
                .view.target,
            &entry->body->owner_definition, &registration->diagnostic);
        return registration->status == NL_CHECK_OK;
    }
    Check definition = {
        .source =
            registration->context->functions[function_id - 1].body->source,
        .in_function_body = true,
        .definition = true,
        .closure_probe = registration->closure_probe,
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
    definition.function_result = function.result;
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
        if (t.kind == NL_TYPE_SUM) {
            value.variant = 1;
            NLTypeId payload_type = c->types[type - 1].variant_types[0];
            if (payload_type != 0) {
                value.sum_payload = new_value(
                    &definition, (NLSemanticValueView){.type = payload_type},
                    span);
                if (value.sum_payload == 0)
                    goto failure;
            }
        }
        values[i] = new_value(&definition, value, span);
        if (values[i] != 0 && value.sum_payload != 0) {
            c->values[value.sum_payload - 1].carrier = NL_CARRIER_SUM;
            c->values[value.sum_payload - 1].sum_owner = values[i];
        }
        if (values[i] == 0)
            goto failure;
    }
    const size_t scope_floor = c->scope_count, place_floor = c->place_count;
    definition.function_scope_floor = scope_floor;
    definition.function_place_floor = place_floor;
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
    if (!function_block(&definition, nl_syntax_node_view(nl_syntax_tree_root(
                                         function.body->syntax))))
        goto failure;
    if (!host(&definition, nl_sem_validate(c), span) ||
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
    if (!lexical_name(&check, name, strlen(name), root->span))
        goto failure;
    if (!body_signature_type(context, result, false))
        goto signature_limit;
    for (size_t i = 0; i < count; ++i) {
        if (parameters[i].name == NULL || parameters[i].name[0] == 0 ||
            parameters[i].type == 0 || parameters[i].type > context->type_count)
            return NL_CHECK_INTERNAL_ERROR;
        if (!lexical_name(&check, parameters[i].name,
                          strlen(parameters[i].name), root->span))
            goto failure;
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
    if (!host(&check, nl_sem_validate(check.context), root->span) ||
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

/* Narrow declaration collection, not a general call graph or module system. */
typedef struct {
    const NLSyntaxTree *tree;
    const NLSyntaxView *syntax;
    size_t input, function;
    char *name;
    /* Names owned by this collection until copied into a durable body. */
    NLFunctionParameter parameters[NL_SEMANTIC_MAX_PARAMETERS];
} FunctionDeclaration;

static int declaration_order(const void *a, const void *b)
{
    const FunctionDeclaration *left = a, *right = b;
    return strcmp(left->name, right->name);
}

/* Bounded source plumbing into the existing transactional registry. */
static bool register_avs(Check *check, const NLSyntaxView *s)
{
    if (!lexical_source_name(check, s->data.avs_struct.name))
        return false;
    char *name = source_name_copy(check, s->data.avs_struct.name);
    char *labels[2] = {NULL, NULL};
    NLAggregateField fields[2] = {{0}};
    bool ok = false;
    if (name == NULL)
        return false;
    if (s->data.avs_struct.count != 2)
        goto profile;
    const NLSyntaxNode *n = s->data.avs_struct.fields;
    for (size_t i = 0; i < 2; ++i) {
        if (n == NULL)
            goto profile;
        const NLSyntaxView *f = nl_syntax_node_view(n);
        const NLTypeId type = check_type(check, f->data.parameter.type);
        if (type == 0)
            goto cleanup;
        if (type != nl_semantic_core_type(check->context, NL_TYPE_U8))
            goto profile;
        labels[i] = source_name_copy(check, f->data.parameter.name);
        if (labels[i] == NULL)
            goto cleanup;
        fields[i] = (NLAggregateField){labels[i], type};
        n = nl_syntax_next_argument(n);
    }
    if (n != NULL)
        goto profile;
    /* Reuse the existing ordinary top-level namespace collision policy. */
    for (size_t i = 0; i < check->context->function_count; ++i)
        if (strcmp(name, check->context->functions[i].name) == 0)
            goto collision;
    for (size_t i = 0; i < check->context->binding_count; ++i)
        if (!check->context->bindings[i].hidden &&
            strcmp(name, check->context->bindings[i].name) == 0)
            goto collision;
    NLTypeId type = 0;
    const NLCheckStatus status =
        nl_semantic_register_aggregate(check->context, name, fields, 2, &type);
    if (status == NL_CHECK_SEMANTIC_ERROR)
        goto collision;
    ok = host(check, status, s->span);
    goto cleanup;
collision:
    fail(check, NL_CHECK_SEMANTIC_ERROR, s->span, "AVS-DECL-REGISTRATION",
         "aggregate name or fields conflict with established identities");
    goto cleanup;
profile:
    fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span, "AVS-DECL-PROFILE",
         "AVS declaration requires exactly two core u8 fields");
cleanup:
    free(name);
    free(labels[0]);
    free(labels[1]);
    return ok;
}

/* Opt-in staging of ONE ordinary nonCopy triad. Its nominal/field spelling
 * carries no Matched bit; primitive consumers check the actual constituents.
 * The enclosing unit transaction owns rollback of the registry on failure. */
static bool register_experimental_root_record(Check *check,
                                              const NLSyntaxView *s)
{
    if (!lexical_source_name(check, s->data.avs_struct.name))
        return false;
    char *name = source_name_copy(check, s->data.avs_struct.name);
    char *labels[3] = {0};
    NLAggregateField fields[3] = {{0}};
    bool ok = false;
    if (name == NULL)
        return false;
    const NLSyntaxNode *n = s->data.avs_struct.fields;
    if (s->data.avs_struct.count != 3)
        goto profile;
    for (size_t i = 0; i < 3; ++i) {
        if (n == NULL)
            goto profile;
        const NLSyntaxView *f = nl_syntax_node_view(n);
        const NLTypeId type = check_type(check, f->data.parameter.type);
        if (type == 0)
            goto cleanup;
        labels[i] = source_name_copy(check, f->data.parameter.name);
        if (labels[i] == NULL)
            goto cleanup;
        fields[i] = (NLAggregateField){labels[i], type};
        n = nl_syntax_next_argument(n);
    }
    if (n != NULL)
        goto profile;
    NLTypeId result = 0;
    const NLCheckStatus status = nl_semantic_register_aggregate(
        check->context, name, fields, 3, &result);
    if (status == NL_CHECK_SEMANTIC_ERROR) {
        fail(check, status, s->span, "P274-RECORD-DECLARATION",
             "ordinary record name or field declarations conflict");
        goto cleanup;
    }
    if (!host(check, status, s->span))
        goto cleanup;
    if (!nl_experimental_root_record_type(check->context, result))
        goto profile;
    ok = true;
    goto cleanup;
profile:
    fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span, "P274-RECORD-PROFILE",
         "experimental record requires ptr<H>,Allocation,LifetimeDomain");
cleanup:
    free(name);
    for (size_t i = 0; i < 3; ++i)
        free(labels[i]);
    return ok;
}

static bool register_recursive(Check *check, const NLSyntaxView *s)
{
    if (!lexical_source_name(check, s->data.avs_struct.name))
        return false;
    char *name = source_name_copy(check, s->data.avs_struct.name);
    char *labels[4] = {0};
    const size_t count = s->data.avs_struct.count;
    bool ok = false;
    if (name == NULL)
        return false;
    for (size_t i = 0; i < check->context->function_count; ++i)
        if (strcmp(name, check->context->functions[i].name) == 0)
            goto collision;
    for (size_t i = 0; i < check->context->binding_count; ++i)
        if (!check->context->bindings[i].hidden &&
            strcmp(name, check->context->bindings[i].name) == 0)
            goto collision;
    if (count != 2 && count != 4) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span, "REC-DECL-PROFILE",
             "H requires one link or exact three links");
        goto cleanup;
    }
    NLTypeId header = 0;
    const NLCheckStatus created =
        nl_recursive_header(check->context, name, &header);
    if (created == NL_CHECK_SEMANTIC_ERROR)
        goto collision;
    if (!host(check, created, s->span))
        goto cleanup;
    NLAggregateField fields[4] = {{0}};
    const NLSyntaxNode *n = s->data.avs_struct.fields;
    for (size_t i = 0; i < count; ++i) {
        const NLSyntaxView *f = nl_syntax_node_view(n);
        fields[i].type = check_type(check, f->data.parameter.type);
        if (fields[i].type == 0)
            goto cleanup;
        labels[i] = source_name_copy(check, f->data.parameter.name);
        if (labels[i] == NULL)
            goto cleanup;
        fields[i].name = labels[i];
        n = nl_syntax_next_argument(n);
    }
    const NLTypeId ptr =
        check->context->types[fields[0].type - 1].option_target;
    if (ptr == 0 || check->context->types[ptr - 1].view.target != header) {
        fail(check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span, "REC-SELF-TARGET",
             "link target must be the same nominal identity");
        goto cleanup;
    }
    ok = host(check,
              nl_recursive_complete(check->context, header, fields, count),
              s->span);
    goto cleanup;
collision:
    fail(check, NL_CHECK_SEMANTIC_ERROR, s->span, "REC-DECL-DUPLICATE",
         "recursive declaration conflicts with an established name");
cleanup:
    free(name);
    for (size_t i = 0; i < 4; ++i)
        free(labels[i]);
    return ok;
}

static NLCheckStatus
register_function_unit(NLSemanticContext *context,
                       const NLSyntaxTree *const *inputs, size_t count,
                       NLFunctionUnitDiagnostic *diagnostic, bool closure_probe)
{
    if (context == NULL || inputs == NULL || count == 0)
        return NL_CHECK_INTERNAL_ERROR;
    for (size_t i = 0; i < count; ++i) {
        if (inputs[i] == NULL ||
            nl_syntax_node_view(nl_syntax_tree_root(inputs[i]))->kind !=
                NL_SYNTAX_FUNCTION_UNIT)
            return NL_CHECK_INTERNAL_ERROR;
    }
    Check check = {.source = nl_syntax_tree_source(inputs[0]),
                   .closure_probe = closure_probe};
    FunctionDeclaration *declarations = NULL;
    const NLSyntaxView *recursive = NULL;
    const NLSyntaxView *experimental_record = NULL;
    size_t recursive_input = 0;
    size_t total = 0, input = 0;
    if (!host(&check, nl_sem_clone(context, &check.context), (NLSourceSpan){0}))
        goto failure;
    for (input = 0; input < count; ++input) {
        check.source = nl_syntax_tree_source(inputs[input]);
        const NLSyntaxView *root =
            nl_syntax_node_view(nl_syntax_tree_root(inputs[input]));
        for (const NLSyntaxNode *n = root->data.function_unit.declarations;
             n != NULL; n = nl_syntax_next_argument(n)) {
            const NLSyntaxView *s = nl_syntax_node_view(n);
            if (s->kind == NL_SYNTAX_EXPERIMENTAL_ROOT_STRUCT) {
                if (count != 1 || experimental_record != NULL) {
                    fail(&check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span,
                         "P274-RECORD-PROFILE",
                         "experiment admits one ordinary triad in one unit");
                    goto failure;
                }
                experimental_record = s;
                continue;
            }
            if (s->kind == NL_SYNTAX_RECURSIVE_STRUCT) {
                if (recursive != NULL) {
                    fail(&check, NL_CHECK_SEMANTIC_ERROR, s->span,
                         "REC-DECL-DUPLICATE",
                         "at most one bounded recursive declaration per "
                         "semantic unit");
                    goto failure;
                }
                recursive = s;
                recursive_input = input;
                continue;
            }
            if (s->kind == NL_SYNTAX_AVS_STRUCT) {
                if (count != 1 || n != root->data.function_unit.declarations) {
                    fail(&check, NL_CHECK_SEMANTIC_UNSUPPORTED, s->span,
                         "AVS-UNIT-PROFILE",
                         "AVS declarations require a single source unit");
                    goto failure;
                }
                if (!register_avs(&check, s))
                    goto failure;
                continue;
            }
            if (total == NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS) {
                (void)host(&check, NL_CHECK_RESOURCE_LIMIT, s->span);
                goto failure;
            }
            if (!lexical_source_name(&check, s->data.function.name))
                goto failure;
            FunctionDeclaration *storage =
                realloc(declarations, (total + 1) * sizeof(*storage));
            if (storage == NULL) {
                (void)host(&check, NL_CHECK_OUT_OF_MEMORY, s->span);
                goto failure;
            }
            declarations = storage;
            declarations[total] = (FunctionDeclaration){
                .tree = inputs[input], .syntax = s, .input = input};
            FunctionDeclaration *d = &declarations[total++];
            d->name = source_name_copy(&check, s->data.function.name);
            if (d->name == NULL)
                goto failure;
        }
    }
    if (recursive != NULL) {
        check.source = nl_syntax_tree_source(inputs[recursive_input]);
        input = recursive_input;
        if (!register_recursive(&check, recursive))
            goto failure;
        if (recursive->data.avs_struct.count == 4 &&
            (count != 1 || total != 1 ||
             strcmp(declarations[0].name, "main") != 0 ||
             declarations[0].syntax->data.function.count != 0 ||
             check_type(&check, declarations[0].syntax->data.function.result) !=
                 1)) {
            fail(&check, NL_CHECK_SEMANTIC_UNSUPPORTED, recursive->span,
                 "FIVE-ROOT-SOURCE-PROFILE",
                 "three-link H selects one parameterless main only");
            goto failure;
        }
    }
    if (!host(&check, nl_recursive_validate(check.context), (NLSourceSpan){0}))
        goto failure;
    if (experimental_record != NULL) {
        check.source = nl_syntax_tree_source(inputs[0]);
        input = 0;
        if (recursive == NULL) {
            fail(&check, NL_CHECK_SEMANTIC_UNSUPPORTED,
                 experimental_record->span, "P274-RECORD-PROFILE",
                 "experimental record requires a completed source H");
            goto failure;
        }
        if (!register_experimental_root_record(&check, experimental_record))
            goto failure;
    }
    if (total != 0)
        qsort(declarations, total, sizeof(*declarations), declaration_order);
    /* Exact signature installation is private until ALL definitions succeed. */
    size_t owner_receivers = 0, owner_producers = 0, custody_recipients = 0;
    for (size_t i = 0; i < total; ++i) {
        FunctionDeclaration *d = &declarations[i];
        input = d->input;
        check.source = nl_syntax_tree_source(d->tree);
        const NLSyntaxView *s = d->syntax;
        if (i != 0 && strcmp(d->name, declarations[i - 1].name) == 0)
            goto duplicate;
        for (size_t j = 0; j < check.context->function_count; ++j)
            if (strcmp(d->name, check.context->functions[j].name) == 0)
                goto duplicate;
        for (size_t j = 0; j < check.context->binding_count; ++j)
            if (!check.context->bindings[j].hidden &&
                strcmp(d->name, check.context->bindings[j].name) == 0)
                goto duplicate;
        /* Host-established nominal declarations share the ordinary top-level
         * namespace. Core type spellings do not become new reserved words. */
        for (size_t j = 2; j < check.context->type_count; ++j) {
            const NLTypeEntry *t = &check.context->types[j];
            if ((t->view.kind == NL_TYPE_NOMINAL ||
                 t->view.kind == NL_TYPE_SUM) &&
                t->name != NULL && strcmp(d->name, t->name) == 0)
                goto duplicate;
        }
        if (s->data.function.count > NL_SEMANTIC_MAX_PARAMETERS) {
            (void)host(&check, NL_CHECK_RESOURCE_LIMIT, s->span);
            goto failure;
        }
        NLTypeId types[NL_SEMANTIC_MAX_PARAMETERS] = {0};
        size_t parameter = 0;
        for (const NLSyntaxNode *n = s->data.function.parameters; n != NULL;
             n = nl_syntax_next_argument(n), ++parameter) {
            const NLSyntaxView *p = nl_syntax_node_view(n);
            if (!lexical_source_name(&check, p->data.parameter.name))
                goto failure;
            char *name = source_name_copy(&check, p->data.parameter.name);
            if (name == NULL)
                goto failure;
            d->parameters[parameter].name = name;
            for (size_t j = 0; j < parameter; ++j) {
                if (strcmp(name, d->parameters[j].name) == 0) {
                    fail(&check, NL_CHECK_SEMANTIC_ERROR,
                         p->data.parameter.name, "P11-DUPLICATE-PARAMETER",
                         "duplicate fn parameter name");
                    goto failure;
                }
            }
            const NLTypeId type = check_type(&check, p->data.parameter.type);
            if (type == 0)
                goto failure;
            types[parameter] = type;
            d->parameters[parameter].type = type;
        }
        const NLTypeId result = check_type(&check, s->data.function.result);
        if (result == 0)
            goto failure;
        const bool owner =
            nl_owner_signature(check.context, types, parameter, result);
        const bool producer =
            nl_producer_signature(check.context, types, parameter, result);
        const bool custody =
            nl_custody_signature(check.context, types, parameter, result);
        if (custody) {
            if (recursive == NULL || count != 1 || custody_recipients != 0 ||
                strcmp(d->name, "main") == 0) {
                fail(&check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, s->span,
                     "CUSTODY-SIGNATURE-PROFILE",
                     "requires one same-unit H custody recipient");
                goto failure;
            }
            ++custody_recipients;
        } else if (producer) {
            if (recursive == NULL || count != 1 || owner_producers != 0 ||
                strcmp(d->name, "main") == 0) {
                fail(&check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, s->span,
                     "P208-SIGNATURE-PROFILE",
                     "requires one same-unit H producer");
                goto failure;
            }
            ++owner_producers;
        } else if (owner) {
            if (recursive == NULL || count != 1 || owner_receivers != 0 ||
                strcmp(d->name, "main") == 0) {
                fail(&check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, s->span,
                     "P193-SIGNATURE-PROFILE",
                     "requires one same-unit H receiver");
                goto failure;
            }
            ++owner_receivers;
        } else {
            if (!body_signature_type(check.context, result, false))
                goto signature_limit;
            for (size_t j = 0; j < parameter; ++j)
                if (!body_signature_type(check.context, types[j], true))
                    goto signature_limit;
        }
        if (!host(&check,
                  nl_semantic_register_function(check.context, d->name, types,
                                                parameter, result, false,
                                                false),
                  s->span))
            goto failure;
        d->function = check.context->function_count;
        check.context->functions[d->function - 1].owner_receiver = owner;
        check.context->functions[d->function - 1].owner_producer = producer;
        check.context->functions[d->function - 1].custody_recipient = custody;
        continue;
    duplicate:
        fail(&check, NL_CHECK_SEMANTIC_ERROR, s->data.function.name,
             "P11-DUPLICATE-FUNCTION",
             "ordinary declaration name already established");
        goto failure;
    signature_limit:
        fail(&check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, s->span,
             "P8-SIGNATURE-PRECISION",
             "signature needs unsupported body analysis");
        goto failure;
    }
    if (owner_producers != 0 && owner_receivers != 1) {
        fail(&check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, (NLSourceSpan){0},
             "P208-UNIT-PROFILE",
             "live-tail profile requires a distinct known terminal receiver");
        goto failure;
    }
    if (custody_recipients != 0 &&
        (owner_producers != 1 || owner_receivers != 1)) {
        fail(&check, NL_CHECK_ANALYSIS_PRECISION_LIMIT, (NLSourceSpan){0},
             "CUSTODY-UNIT-PROFILE",
             "custody requires the independent producer and terminal receiver");
        goto failure;
    }
    /* Never interpret a source definition as a coarse signature-only call. */
    for (size_t i = 0; i < total; ++i) {
        const FunctionDeclaration *d = &declarations[i];
        input = d->input;
        check.source = nl_syntax_tree_source(d->tree);
        const NLSourceSpan span =
            nl_syntax_node_view(d->syntax->data.function.body)->span;
        if (!host(&check,
                  nl_body_create_span(
                      check.source, span, d->parameters,
                      d->syntax->data.function.count,
                      &check.context->functions[d->function - 1].body),
                  span))
            goto failure;
    }
    /* Symbolic obligations are checked before any ordinary definition walks a
     * known call, independent of declaration/name order or favorable callers.
     */
    for (size_t pass = 0; pass < 2; ++pass)
        for (size_t i = 0; i < total; ++i) {
            const FunctionDeclaration *d = &declarations[i];
            if ((check.context->functions[d->function - 1].owner_receiver ||
                 check.context->functions[d->function - 1].owner_producer ||
                 check.context->functions[d->function - 1].custody_recipient) !=
                (pass == 0))
                continue;
            input = d->input;
            if (!check_definition(&check, d->function)) {
                const size_t start =
                    nl_syntax_node_view(d->syntax->data.function.body)
                        ->span.start_byte;
                check.diagnostic.span.start_byte += start;
                check.diagnostic.span.end_byte += start;
                goto failure;
            }
        }
    check.source = nl_syntax_tree_source(inputs[0]);
    input = 0;
    if (!host(&check, nl_sem_validate(check.context), (NLSourceSpan){0}) ||
        !host(&check, nl_raw_validate(check.context), (NLSourceSpan){0}))
        goto failure;
    nl_sem_commit(context, check.context);
    check.context = NULL;
failure:
    for (size_t i = 0; i < total; ++i) {
        free(declarations[i].name);
        for (size_t j = 0; j < NL_SEMANTIC_MAX_PARAMETERS; ++j)
            free((void *)declarations[i].parameters[j].name);
    }
    free(declarations);
    nl_semantic_destroy(check.context);
    if (check.status != NL_CHECK_OK && diagnostic != NULL)
        *diagnostic = (NLFunctionUnitDiagnostic){input, check.diagnostic};
    return check.status;
}

NLCheckStatus nl_semantic_register_function_unit(
    NLSemanticContext *context, const NLSyntaxTree *const *inputs, size_t count,
    NLFunctionUnitDiagnostic *diagnostic)
{
    return register_function_unit(context, inputs, count, diagnostic, false);
}

NLCheckStatus nl_captured_closure_probe(const NLSyntaxTree *unit,
                                        NLCheckedFragment **out,
                                        NLCheckDiagnostic *diagnostic)
{
    if (unit == NULL || out == NULL || *out != NULL)
        return NL_CHECK_INTERNAL_ERROR;
    NLSemanticContext *context = NULL;
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *entry = NULL;
    NLCheckedFragment *artifact = NULL;
    NLCheckStatus s = nl_semantic_create(&context);
    const NLSyntaxTree *units[] = {unit};
    NLFunctionUnitDiagnostic d = {0};
    if (s != NL_CHECK_OK)
        goto done;
    s = register_function_unit(context, units, 1, &d, true);
    if (s != NL_CHECK_OK) {
        if (diagnostic != NULL)
            *diagnostic = d.diagnostic;
        goto done;
    }
    size_t bodies = 0;
    for (size_t i = 0; i < context->function_count; ++i) {
        const NLFunctionEntry f = context->functions[i];
        if (f.body == NULL)
            continue;
        ++bodies;
        if (strcmp(f.name, "main") != 0 || f.count != 0 || f.result != 1) {
            s = NL_CHECK_SEMANTIC_UNSUPPORTED;
            goto done;
        }
    }
    if (bodies != 1) {
        s = NL_CHECK_SEMANTIC_UNSUPPORTED;
        goto done;
    }
    if (nl_source_create("main()", 6, "captured-closure-probe", &source) !=
            NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(parser, &entry, NULL) !=
            NL_PARSE_OK) {
        s = NL_CHECK_OUT_OF_MEMORY;
        goto done;
    }
    s = check_fragment(context, entry, &artifact, diagnostic,
                       CHECK_CLOSURE_EXPRESSION);
    if (s == NL_CHECK_OK) {
        const NLCheckedFragment *body =
            nl_checked_call_body(artifact, nl_checked_root(artifact));
        if (body == NULL || body->captured_closure == NULL ||
            body->captured_closure->view.count != 0)
            s = NL_CHECK_SEMANTIC_UNSUPPORTED;
        else
            s = nl_checked_captured_closure_validate(body,
                                                     body->captured_match);
        if (s == NL_CHECK_OK) {
            artifact->destroy_context = nl_semantic_destroy;
            artifact->source = NULL; /* proof never interprets entry spelling */
            context = NULL;
            *out = artifact;
        } else
            nl_checked_destroy(artifact);
    }
done:
    nl_syntax_tree_destroy(entry);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    nl_semantic_destroy(context);
    return s;
}
