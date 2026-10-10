#ifndef NEWLANG_SEMANTIC_INTERNAL_H
#define NEWLANG_SEMANTIC_INTERNAL_H

#include "control.h"
#include "newlang/captured_closure.h"
#include "newlang/checked.h"
#include "newlang/raw_storage.h"
#include "ordinary_name.h"

/* Borrowed spelling; only ordinary lexical names, never member labels.
 * Draft 17.14 preserves core/structural reasons without changing lexer tokens.
 */
bool nl_sem_lexical_name_admissible(const void *bytes, size_t length);
NLCheckStatus nl_owner_record_relations(const NLSemanticContext *, NLValueId,
                                        const NLTypedOwnerDefinition *);
struct NLFunctionBody;
NLCheckStatus nl_two_root_definition(const NLSemanticContext *,
                                     const struct NLFunctionBody *, NLTypeId,
                                     NLTwoRootDefinition *,
                                     NLCheckDiagnostic *);

typedef struct {
    size_t start, length;
    NLRawRepView state;
} NLRawInterval;
typedef struct {
    NLSemanticBackingView view;
    NLRawInterval *intervals; /* owning full-range partition */
    size_t count;
} NLRawRegionEntry;

typedef struct {
    char *name;
    bool incomplete, recursive_header;
    NLTypeId allocated_target, one_backing_target,
        live_tail_target; /* closed allocation/result registry */
    NLTypeId
        option_target; /* canonical Option instantiation argument, not name */
    NLSemanticTypeView view;
    char *field_names[NL_SEMANTIC_MAX_FIELDS];
    NLTypeId field_types[NL_SEMANTIC_MAX_FIELDS];
    char *variant_names[NL_SEMANTIC_MAX_VARIANTS];
    NLTypeId variant_types[NL_SEMANTIC_MAX_VARIANTS];
} NLTypeEntry;
typedef struct {
    char *name;
    NLSemanticBindingView view;
    bool hidden; /* lexical scope ended; stable historical IDs remain */
} NLBindingEntry;
typedef struct NLFunctionBody {
    size_t owners;
    NLSource *source;
    NLSyntaxTree *syntax;
    char *parameter_names[NL_SEMANTIC_MAX_PARAMETERS];
    size_t count;
    NLTypedOwnerDefinition owner_definition;
    NLTwoRootDefinition two_root_definition;
    NLCustodyDefinition custody_definition;
} NLFunctionBody; /* immutable plan; only ownership count is mutable */

typedef struct {
    const char *name; /* static for prelude; owned for registered entries */
    NLCheckedKind kind;
    NLTypeId *parameters;
    size_t count;
    NLTypeId result;
    bool caller_effects, hidden_dependencies, owner_receiver, owner_producer,
        custody_recipient, experimental_root_receiver,
        experimental_two_receiver;
    NLFunctionBody *body; /* owned retained plan, NULL for signature-only */
} NLFunctionEntry;
struct NLSemanticContext {
    NLSemanticOccurrenceView *occurrences;
    size_t occurrence_count;
    NLTypeEntry *types;
    size_t type_count;
    NLBindingEntry *bindings;
    size_t binding_count;
    NLSemanticValueView *values;
    size_t value_count;
    NLSemanticPlaceView *places;
    size_t place_count;
    NLSemanticDomainView *domains;
    size_t domain_count;
    NLSemanticScopeView *scopes;
    size_t scope_count;
    NLFunctionEntry *functions;
    size_t function_count;
    NLIncarnationId last_incarnation;
    NLValueFactId last_value_fact;
    NLRawRegionEntry *regions;
    size_t region_count, raw_interval_count;
};
typedef struct {
    NLCheckedNodeId match;
    struct NLCheckedFragment *artifact; /* owns hypothetical arm context */
} NLCheckedArm;
typedef struct NLCapturedClosure {
    NLCapturedClosureView view;
    NLSemanticContext *ancestor_owned, *closed_owned;
    const NLSemanticContext *parent_world;
    struct {
        NLSemanticContext *entry; /* owned pre-grant fork snapshot */
        const NLSemanticContext *entry_origin; /* nominal owned fork anchor */
        const NLCheckedFragment *arm;          /* borrowed owning subtree */
        const NLSemanticContext *world;        /* exact owned arm identity */
    } branches[2];
} NLCapturedClosure;
typedef struct {
    NLCheckedNodeId node;
    const NLSemanticContext *world;
    NLSemanticContext *before, *after; /* owned point-in-time worlds */
    const NLSemanticContext *before_origin, *after_origin;
} NLCapturedChange;
struct NLCheckedFragment {
    NLTwoRootCallView two_root;
    const NLSemanticContext *two_entry_origin, *two_return_origin;
    NLCheckedNodeId two_call;
    void (*destroy_two_world)(NLSemanticContext *);
    NLWholeValueCallView whole_value;
    const NLSemanticContext *whole_entry_origin, *whole_return_origin,
        *whole_receive_origin;
    NLCheckedNodeId whole_call;
    void (*destroy_whole_world)(NLSemanticContext *);
    NLCapturedChange *field_changes[6];
    size_t field_change_count;
    void (*destroy_field_changes)(NLCheckedFragment *);
    NLCapturedClosure *captured_closure;
    void (*destroy_captured_closure)(NLCapturedClosure *);
    /* Parent owns entry/post; a child owns its pre-pattern entry and borrows
     * the enclosing fragment. Child evidence never outlives that owner tree. */
    NLSemanticContext *packet_entry, *packet_post, *packet_retained_post;
    void (*destroy_packet_world)(NLSemanticContext *);
    const struct NLCheckedFragment *packet_parent;
    NLCheckedNodeId packet_match;
    NLSemanticContext *custody_entry, *custody_post;
    void (*destroy_custody_world)(NLSemanticContext *);
    NLCheckedNodeId custody_call_id;
    const NLSemanticContext *custody_policy_world, *custody_continuation_world;
    NLCheckedFragment *custody_continuations[2];
    NLSemanticContext *custody_final_post;
    NLCheckedNodeId custody_join;
    NLSymbolId custody_binding;
    size_t custody_floor;
    NLSemanticContext *producer_entry,
        *producer_return; /* owned actual worlds */
    NLCheckedNodeId producer_call;
    void (*destroy_producer_world)(NLSemanticContext *);
    NLSemanticContext *owner_entry; /* exact pre-callee actual world snapshot */
    NLCheckedNodeId owner_entry_call;
    void (*destroy_owner_entry)(NLSemanticContext *);
    /* Nested allocation proof target; owned ancestor-only closed snapshot,
     * independent of either hypothetical arm and later lexical cleanup. */
    NLSemanticContext *captured_post;
    NLCheckedNodeId captured_match;
    void (*destroy_captured_post)(NLSemanticContext *);
    NLControlExits *loop_returns; /* owned early loop Return evidence */
    NLControlExits *exits;        /* owned complete finite control evidence */
    NLControlTarget *function_target; /* owned boundary identity, if any */
    const NLSource *source;
    const NLSemanticContext *context;
    NLCheckedNodeView *nodes;
    size_t count, capacity;
    NLCheckedNodeId root;
    NLCheckedArm *arms;
    size_t arm_count;
    NLValueFactId branch_fact_prefix; /* original current-value facts */
    size_t branch_value_prefix; /* proven original IDs for IF exit rebasing */
    void (*destroy_context)(NLSemanticContext *); /* arm snapshots only */
    struct NLCheckedFragment **bodies;
    NLCheckedNodeId *body_calls;
    size_t body_count;
    NLFunctionBody *body_owner;
    void (*release_body)(NLFunctionBody *);
};

NLCheckStatus nl_captured_change_begin(NLCheckedFragment *, NLCheckedNodeId,
                                       const NLSemanticContext *);
NLCheckStatus nl_captured_change_end(NLCheckedFragment *,
                                     const NLSemanticContext *);
NLCheckStatus nl_captured_closure_create(const NLSemanticContext *,
                                         NLCapturedClosure **);
void nl_captured_closure_destroy(NLCapturedClosure *);
NLCheckStatus nl_captured_closure_finish(NLCheckedFragment *, NLCheckedNodeId);
/* Explicitly isolated checker-integration entry. No CLI/source admission
 * flag, host seed or allocation grant. Parses/checks the supplied real unit
 * through the same body/primitive transactions; result owns its context. */
NLCheckStatus nl_captured_closure_probe(const NLSyntaxTree *,
                                        NLCheckedFragment **,
                                        NLCheckDiagnostic *);

/* All ref consumers either iterate this may-set or explicitly reject it.
 * A concrete fact remains the P3 representation when reference_count == 0. */
static inline size_t nl_sem_ref_count(NLSemanticValueView v)
{
    return v.reference_count == 0 ? 1 : v.reference_count;
}
static inline NLReferenceFacts nl_sem_ref_fact(NLSemanticValueView v, size_t i)
{
    return v.reference_count == 0 ? v.reference : v.references[i];
}

NLCheckStatus nl_body_create(const NLSyntaxTree *, const NLFunctionParameter *,
                             size_t, NLFunctionBody **);
NLCheckStatus nl_body_create_span(const NLSource *, NLSourceSpan,
                                  const NLFunctionParameter *, size_t,
                                  NLFunctionBody **);
NLCheckStatus nl_body_retain(NLFunctionBody *);
void nl_body_release(NLFunctionBody *);
NLCheckStatus nl_live_tail_registry(NLSemanticContext *, NLTypeId, NLTypeId *);
NLCheckStatus nl_custody_registry(NLSemanticContext *, NLTypeId, NLTypeId *);
bool nl_custody_signature(const NLSemanticContext *, const NLTypeId *, size_t,
                          NLTypeId);
NLCheckStatus nl_custody_definition(const NLSemanticContext *, NLFunctionBody *,
                                    NLCustodyDefinition *, NLCheckDiagnostic *);
bool nl_producer_signature(const NLSemanticContext *, const NLTypeId *, size_t,
                           NLTypeId);
bool nl_owner_signature(const NLSemanticContext *, const NLTypeId *, size_t,
                        NLTypeId);
/* Independent symbolic checking: read-only registry, no concrete value/place
 * operations, no assumption that the three symbolic parameters correlate. */
NLCheckStatus nl_owner_definition(const NLSemanticContext *, NLFunctionBody *,
                                  NLTypeId, NLTypedOwnerDefinition *,
                                  NLCheckDiagnostic *);
NLCheckStatus nl_root_record_definition(const NLSemanticContext *,
                                        NLFunctionBody *, NLTypeId,
                                        NLTypedOwnerDefinition *,
                                        NLCheckDiagnostic *);
NLCheckStatus nl_owner_relations(const NLSemanticContext *, const NLValueId *,
                                 const NLTypedOwnerDefinition *, NLSourceSpan,
                                 NLCheckDiagnostic *);
/* Surviving packages after body/parameter cleanup may not reference newly
 * ending local scopes/places. Used by registration, real calls and fixtures. */
NLCheckStatus nl_sem_function_exit(const NLSemanticContext *,
                                   size_t scope_floor, size_t place_floor);

/* Concrete candidate state helpers; no general allocator/transaction DSL. */
NLCheckStatus nl_sem_clone(const NLSemanticContext *, NLSemanticContext **);
void nl_sem_commit(NLSemanticContext *, NLSemanticContext *);
NLCheckStatus nl_sem_compound(NLSemanticContext *, NLSemanticTypeKind, NLTypeId,
                              NLAccessSyntax, bool, NLTypeId *);
NLCheckStatus nl_sem_new_value(NLSemanticContext *, NLSemanticValueView,
                               NLValueId *);
NLCheckStatus nl_sem_new_place(NLSemanticContext *, NLTypeId, NLDomainId, bool,
                               NLValueId, NLPlaceId *);
NLCheckStatus nl_sem_install(NLSemanticContext *, NLPlaceId, NLValueId,
                             NLDomainId);
NLCheckStatus nl_sem_bind(NLSemanticContext *, const char *, NLValueId,
                          NLSymbolId *);
NLCheckStatus nl_sem_bind_in_scope(NLSemanticContext *, const char *, NLValueId,
                                   size_t binding_floor, NLSymbolId *);
NLCheckStatus nl_sem_copy_value(NLSemanticContext *, NLValueId, NLValueId *);
void nl_sem_end_value(NLSemanticContext *, NLValueId);
NLCheckStatus nl_sem_new_domain(NLSemanticContext *, NLDomainId *, NLValueId *);
NLCheckStatus nl_sem_new_scope(NLSemanticContext *, NLScopeId, bool,
                               NLScopeId *);
NLCheckStatus nl_sem_fresh_fact(NLSemanticContext *, NLValueFactId *);
bool nl_fixed_type(const NLSemanticContext *, NLTypeId);
bool nl_recursive_local_type(const NLSemanticContext *, NLTypeId);
/* Syntax/shape admission only; NEVER an original-root correlation grant.
 * False in every canonical/default build. */
bool nl_experimental_root_record_type(const NLSemanticContext *, NLTypeId);
/* Structural custody shapes, never a p/R/A/D matching assertion. */
bool nl_experimental_nested_type(const NLSemanticContext *, NLTypeId);
bool nl_experimental_owner_aggregate_type(const NLSemanticContext *, NLTypeId);
bool nl_experimental_value_type(const NLSemanticContext *, NLTypeId);
NLCheckStatus nl_whole_call_validate(const NLCheckedFragment *,
                                     NLCheckedNodeId);
/* Read-only write-admission predicate, not a mint/seed operation. Caller still
 * checks explicit domain stability, dependencies and scope/conflicts. */
NLCheckStatus nl_allocated_write_access(const NLSemanticContext *, NLValueId);
/* Private transaction helpers. No header is committed until unit completion.
 * Strings/arrays are borrowed for the call; successful types own copies. */
NLCheckStatus nl_sem_nominal(NLSemanticContext *, const char *, bool, bool,
                             NLTypeId *);
NLCheckStatus nl_recursive_header(NLSemanticContext *, const char *,
                                  NLTypeId *);
NLCheckStatus nl_recursive_option(NLSemanticContext *, NLTypeId, NLTypeId *);
NLCheckStatus nl_recursive_complete(NLSemanticContext *, NLTypeId,
                                    const NLAggregateField *, size_t);
NLCheckStatus nl_recursive_validate(const NLSemanticContext *);
bool nl_recursive_value_type(const NLSemanticContext *, NLTypeId);
NLCheckStatus nl_fixed_attach(NLSemanticContext *, NLPlaceId);
void nl_fixed_detach(NLSemanticContext *, NLPlaceId);
NLCheckStatus nl_fixed_validate(const NLSemanticContext *);
NLCheckStatus nl_sem_validate(const NLSemanticContext *);
bool nl_fixed_overlap(const NLSemanticContext *, NLPlaceId, NLPlaceId);
bool nl_fixed_live(const NLSemanticContext *, NLPlaceId);
NLCheckStatus nl_fixed_dependencies(const NLSemanticContext *);
NLCheckStatus nl_fixed_change_dependencies(const NLSemanticContext *, NLPlaceId,
                                           NLValueId discarded);
NLCheckStatus nl_fixed_end_dependencies(const NLSemanticContext *, NLPlaceId,
                                        size_t ending_binding_floor);
NLCheckStatus nl_fixed_change(NLSemanticContext *, NLPlaceId, NLValueId, bool);
static inline bool nl_fixed_frame_same(NLSemanticPlaceView a,
                                       NLSemanticPlaceView b)
{
    if (a.parent_aggregate != b.parent_aggregate ||
        a.parent_incarnation != b.parent_incarnation ||
        a.parent_field_index != b.parent_field_index ||
        a.fixed_field_count != b.fixed_field_count)
        return false;
    for (size_t i = 0; i < NL_SEMANTIC_MAX_FIELDS; ++i)
        if (a.fixed_fields[i] != b.fixed_fields[i])
            return false;
    return true;
}
NLCheckStatus nl_checked_add(NLCheckedFragment *, NLCheckedNodeView,
                             NLCheckedNodeId *);

NLCheckStatus nl_sum_validate(const NLSemanticContext *);
NLCheckStatus nl_sum_attach(NLSemanticContext *, NLPlaceId);
void nl_sum_detach(NLSemanticContext *, NLPlaceId);
NLCheckStatus nl_sum_payload_changed(NLSemanticContext *, NLPlaceId);
bool nl_sum_authority(const NLSemanticContext *, NLTypeId);

void nl_raw_dispose(NLSemanticContext *);
NLCheckStatus nl_raw_clone(const NLSemanticContext *, NLSemanticContext *);
NLCheckStatus nl_raw_validate(const NLSemanticContext *);
NLCheckStatus nl_raw_start_root(NLSemanticContext *, NLValueId slot, NLPlaceId);
NLCheckStatus nl_raw_end_root(NLSemanticContext *, NLBackingRange,
                              NLValueId slot);
NLCheckStatus nl_raw_apply(NLSemanticContext *, const NLRawOperation *,
                           const NLValueId inputs[2], NLCheckedNodeView *,
                           NLCheckDiagnostic *);

/* Trusted host-known ordinary-function boundary, INTERNAL ONLY. The caller
 * constructs/owns the semantic setup; these floors define which scopes/places
 * end at function exit. No source declaration/context admission is exposed.
 * Clone/check/commit; failure leaves context and *out unchanged. Tree/source
 * borrowed, source must outlive the owned returned checked artifact.
 */
typedef struct {
    NLTypeId result;
    size_t bindings, scopes, places;
} NLFunctionBoundary;
NLCheckStatus nl_sem_check_function_block(NLSemanticContext *,
                                          const NLSyntaxTree *,
                                          NLFunctionBoundary,
                                          NLCheckedFragment **,
                                          NLCheckDiagnostic *);

/* Private-candidate operations; caller rolls back the entire candidate on
 * failure. */
NLCheckStatus nl_allocated_registry(NLSemanticContext *, NLTypeId, NLTypeId *);
/* Derive a CLOSED prefix from the incoming single live allocated H only.
 * This is a comparison target, never source cleanup or an arm import.
 * Success owns *out; failure leaves output/certificate untouched. */
NLCheckStatus nl_allocated_closed_prefix(const NLSemanticContext *,
                                         NLSemanticContext **,
                                         NLCheckedNodeView *);
/* Internal proof comparisons allocate nothing. Raw integrity is separately
 * status-checked at construction and fail-closed in public validation. */
bool nl_producer_valid(const NLCheckedFragment *, NLCheckedNodeId, bool raw);
bool nl_packet_same_entry(const NLSemanticContext *, const NLSemanticContext *);
NLCheckStatus nl_packet_fork_prepare(NLCheckedFragment *, NLCheckedNodeId,
                                     const NLSemanticContext *);
bool nl_packet_inherited(const NLCheckedFragment *, NLValueId);
/* Read-only pre-consumption relation; no sink/alias proof or transfer grant. */
bool nl_packet_available_inherited(const NLCheckedFragment *, NLValueId);
NLCheckStatus nl_packet_retained(const NLCheckedFragment *, NLCheckedNodeId,
                                 const NLSemanticContext *,
                                 NLSemanticContext **);
bool nl_packet_arm_retained(const NLCheckedFragment *, NLCheckedNodeId,
                            const NLCheckedFragment *,
                            const NLCheckedNodeView *);
bool nl_packet_receiving_after_join(const NLCheckedFragment *,
                                    const NLSemanticContext *, NLValueId);
NLCheckStatus nl_packet_closed(const NLCheckedFragment *, NLCheckedNodeId,
                               const NLSemanticContext *, NLSemanticContext **);
bool nl_packet_arm_closed(const NLCheckedFragment *, NLCheckedNodeId,
                          const NLCheckedFragment *, const NLCheckedNodeView *);
bool nl_allocated_post_matches(const NLSemanticContext *,
                               const NLSemanticContext *,
                               const NLCheckedNodeView *);
NLCheckStatus nl_allocated_grant(NLSemanticContext *, NLTypeId, bool,
                                 NLCheckedNodeView *, NLCheckDiagnostic *);
#endif
