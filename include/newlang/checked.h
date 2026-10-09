#ifndef NEWLANG_CHECKED_H
#define NEWLANG_CHECKED_H

#include "newlang/semantic.h"

typedef size_t NLCheckedNodeId; /* artifact-local; zero absent */
typedef enum {
    NL_CHECKED_TYPE,
    NL_CHECKED_IDENTIFIER,
    NL_CHECKED_BINDING,
    NL_CHECKED_REGISTERED_CALL,
    NL_CHECKED_DOMAIN_CREATE,
    NL_CHECKED_PTR_FROM_REF,
    NL_CHECKED_DOMAIN_FINALIZE,
    NL_CHECKED_INITIALIZE,
    NL_CHECKED_TAKE,
    NL_CHECKED_DESTROY,
    NL_CHECKED_REPLACE,
    NL_CHECKED_STORE,
    NL_CHECKED_SWAP,
    NL_CHECKED_LOAN_HEADER,
    NL_CHECKED_ALLOCATE,
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
    NL_CHECKED_U8_TO_BYTE,
    NL_CHECKED_BLOCK,
    NL_CHECKED_STATEMENT,
    NL_CHECKED_MULTI_BINDING,
    NL_CHECKED_RECEIVER,
    NL_CHECKED_AGGREGATE,
    NL_CHECKED_AGGREGATE_FIELD,
    NL_CHECKED_AGGREGATE_BINDING,
    NL_CHECKED_SUM_CONSTRUCTOR,
    NL_CHECKED_MATCH,
    NL_CHECKED_IF,
    NL_CHECKED_IF_ARM,
    NL_CHECKED_MATCH_ARM,
    NL_CHECKED_RETURN,
    NL_CHECKED_LOOP,
    NL_CHECKED_CONTINUE,
    NL_CHECKED_BREAK,
    NL_CHECKED_UNIT,
    NL_CHECKED_U8_LITERAL,
    NL_CHECKED_FIELD_READ,
    NL_CHECKED_TRY_ALLOCATE_ONE,
    NL_CHECKED_REF_FROM_PTR,
    NL_CHECKED_FIELD_REF, /* mode-preserving scoped H link projection */
    NL_CHECKED_LINK_READ  /* bounded Copy read through FIELD_REF */
} NLCheckedKind;
typedef enum {
    NL_VALUE_USE_NONE,
    NL_VALUE_COPIED,
    NL_VALUE_CONSUMED,
    NL_VALUE_REBORROWED,
    NL_VALUE_RECEIVED /* produced responsibility -> binding, never a Copy-use */
} NLValueUse;
typedef struct {
    NLTypeId type;
    NLValueId value;
} NLCheckedResult;
/* Point-in-time checked evidence; locals may be ended in the final context.
 * All IDs belong to this fragment's context, never a C address/offset. */
typedef struct {
    bool present, dependency_compatible;
    NLSymbolId base;
    NLTypeId nominal, type;
    size_t index;
    NLPlaceId parent, child;
    NLIncarnationId parent_incarnation, child_incarnation;
    NLValueFactId parent_fact, child_fact, parent_post_fact, child_post_fact;
    NLAccessSyntax access;
    NLValueId old_value, new_value;
    NLOccurrenceId payload_occurrence, post_payload_occurrence;
} NLCheckedField;
typedef struct {
    NLSymbolId source, stability;
    NLSourceSpan binding, body_open, body_interior, body_close;
    NLAccessSyntax access;
    bool is_exclusive;
    bool stability_weakened;
    NLPlaceId place;
    NLIncarnationId incarnation;
    NLDomainId domain;
    NLScopeId scope, dependency_scope;
    bool prevent_lifetime_end;
    bool prevent_conflicting_access;
    bool body_nonescape_proved; /* false for header-only P3 */
    bool implicit_local, from_ptr, normal_result_forwarded;
    NLSymbolId ref_symbol; /* resolved body-local binder, not source text */
} NLCheckedLoanPlan;
typedef struct {
    NLCheckedKind kind;
    NLSourceSpan span, name, qualifier;
    size_t variant;
    bool borrowed_match;
    /* A fallible trial has no common-context result package. Owned arm worlds
     * record conditional grants only; success is never a runtime assertion. */
    bool allocation_trial, allocation_success;
    NLTypeId allocation_target;
    size_t allocation_size, allocation_alignment;
    NLBackingRegionId backing;
    NLValueId allocation_authority, storage_authority;
    NLPlaceId lifetime_place;
    NLIncarnationId lifetime_incarnation;
    NLDomainId lifetime_domain;
    NLBackingRange lifetime_range;
    /* MATCH: every normal arm proved identical to the incoming public frame.
     * Only symbols in this prefix can be mapped to ancestor C carriers;
     * arm-local symbols belong to the separately owned arm context. */
    bool normal_frame_unchanged;
    size_t match_binding_prefix;
    /* Nested allocation only: BOTH owned arms proved this pre-fork head's
     * complete closure against a post-state derived solely from the parent.
     * IDs are ancestor-prefix identities in this fragment's world; arm-local
     * IDs remain qualified by nl_checked_match_arm's owned path. */
    /* One original LiveTail packet fork. Worlds and IDs are a qualified
     * ancestor-prefix mapping, never an arm-local owner/custody join. */
    struct {
        bool closed;
        const NLSemanticContext *entry_world, *post_world;
        NLCheckedNodeId producer;
        NLValueId packet;
        NLSymbolId binding;
    } packet_fork;
    /* Whole receiving in an arm: every inherited numeric ID is qualified
     * by this exact world and the enclosing match's common parent proof. */
    struct {
        const NLCheckedFragment *ancestor;
        const NLSemanticContext *world, *entry_world;
        NLCheckedNodeId match;
        NLValueId packet;
    } packet_origin;
    bool captured_frame_closed;
    NLBackingRegionId captured_backing;
    NLPlaceId captured_root;
    NLIncarnationId captured_incarnation;
    NLDomainId captured_domain;
    NLSymbolId captured_allocation, captured_domain_binding;
    NLTypeId type;
    NLSymbolId symbol;
    size_t function; /* resolved prelude/registered signature identity */
    bool body_backed;
    /* All IDs below belong to this artifact's world, including the separately
     * owned synchronous callee body. Never compare across match-arm worlds. */
    struct {
        NLTypedOwnerDefinition definition;
        bool entry_proved, post_proved;
        NLValueId inputs[3];
        NLSymbolId donor[3], parameters[3];
        NLPlaceId root;
        NLIncarnationId incarnation;
        NLBackingRange range;
        NLDomainId domain;
    } owner_call;
    /* §18.1b: separately owned entry/return worlds qualify every ID. */
    struct {
        NLTypedOwnerDefinition definition;
        bool entry_proved, return_proved;
        const NLSemanticContext *entry_world,
            *return_world; /* borrowed owned snapshots */
        NLValueId inputs[4], result;
        NLSymbolId donor[4], parameters[4];
        NLCheckedField head;
        NLPlaceId root;
        NLIncarnationId incarnation;
        NLBackingRange range;
        NLDomainId domain, head_domain;
        NLValueId head_before, head_after;
        NLValueFactId head_before_fact, head_after_fact;
    } producer;
    bool terminates; /* no normal outgoing edge; type 0 means absent, not never
                      */
    size_t normal_arms;       /* finite branch count, excludes return edges */
    NLCheckedResult returned; /* RETURN responsibility, not a normal result */
    bool header_inductive; /* LOOP only: abstract-input transfer + all backedges
                            */
    size_t continue_edges, break_edges, return_edges;
    NLValueUse value_use;
    bool contextually_weakened;
    NLTypeId parameter_type;
    NLScopeId reborrow_scope;
    NLCheckedNodeId first_argument, next_argument;
    size_t argument_count;
    NLCheckedNodeId initializer;
    NLCheckedNodeId first_item, next_item, tail;
    size_t item_count, field_index; /* field_index is declaration order */
    size_t
        result_count; /* 0 = unit/no responsibility; 1 or 2 separate values */
    NLCheckedResult results[2];
    bool has_scalar_result; /* U8_LITERAL owns checked value evidence; raw
                              observations do not mint ValuePackages. */
    NLScalarValue scalar_result;
    size_t raw_offsets[2], raw_count; /* resolved constant selections */
    NLCheckedLoanPlan loan;
    NLCheckedField field;
    bool has_reference_result;
    NLReferenceFacts reference_result; /* checked ptr_from_ref facts */
} NLCheckedNodeView;

/* Owned artifact, immutable after success. Nodes borrow artifact until destroy.
 * Copy views if retaining them across destruction. No syntax pointers, LLVM
 * objects, tuples or semantic ownership transfer on destruction. Result values
 * remain explicit loose responsibilities in the semantic context until bound/
 * forwarded to an operation; freeing artifact is not semantic discard. */
void nl_checked_destroy(NLCheckedFragment *);
NLCheckedNodeId nl_checked_root(const NLCheckedFragment *);
size_t nl_checked_node_count(const NLCheckedFragment *);
const NLCheckedNodeView *nl_checked_node_view(const NLCheckedFragment *,
                                              NLCheckedNodeId);
const NLSource *nl_checked_source(const NLCheckedFragment *);
const NLSemanticContext *nl_checked_context(const NLCheckedFragment *);
/* Producer snapshots are separately owned exact actual entry and return
 * worlds. Validation is read-only (link with the semantic library), requires
 * the matching synchronous checked body and rejects foreign cloned worlds.
 * Neither accessor nor validation mints or consumes source authority. */
/* Validates the owned parent/child lineage, both independently checked
 * terminal calls and the common parent-derived closed-tail postcondition. */
bool nl_checked_packet_fork_valid(const NLCheckedFragment *, NLCheckedNodeId);
bool nl_checked_producer_valid(const NLCheckedFragment *, NLCheckedNodeId);
const NLSemanticContext *nl_checked_producer_entry(const NLCheckedFragment *,
                                                   NLCheckedNodeId);
const NLSemanticContext *nl_checked_producer_return(const NLCheckedFragment *,
                                                    NLCheckedNodeId);
/* Terminal receiver actual entry, after argument evaluation, before effects. */
const NLSemanticContext *nl_checked_owner_entry(const NLCheckedFragment *,
                                                NLCheckedNodeId);
/* Borrowed immutable ancestor-prefix proof target, owned by fragment.
 * NULL unless this exact match proved changed captured closure. */
const NLSemanticContext *nl_checked_captured_post(const NLCheckedFragment *,
                                                  NLCheckedNodeId match);

/* Borrowed arm evidence in source order. Its semantic IDs belong exclusively
 * to nl_checked_context(arm), an artifact-owned hypothetical branch snapshot.
 * They never identify public-context values. The parent owns/destroys arms and
 * their contexts; source lifetime must cover both. Nested matches are outside
 * the bounded P6 slice; nested IF evidence is supported. */
const NLCheckedFragment *nl_checked_match_arm(const NLCheckedFragment *,
                                              NLCheckedNodeId match,
                                              size_t index);

/* IF arm snapshots use the same ownership/public-ID separation as match.
 * Index 0 is then, 1 is else. Nested IF evidence is recursively owned. */
const NLCheckedFragment *nl_checked_if_arm(const NLCheckedFragment *,
                                           NLCheckedNodeId conditional,
                                           size_t index);

/* Borrowed abstract iteration body. All IDs belong to its owned context;
 * header_inductive is published only after complete source transfer/closure. */
const NLCheckedFragment *nl_checked_loop_body(const NLCheckedFragment *,
                                              NLCheckedNodeId loop);

/* Owned body evidence retains its plan source. Semantic IDs use the SAME
 * public context as the call, never a hypothetical formal context. Namespace
 * locals are hidden/ended after completion; stable historical IDs remain. */
const NLCheckedFragment *nl_checked_call_body(const NLCheckedFragment *,
                                              NLCheckedNodeId call);

#endif
