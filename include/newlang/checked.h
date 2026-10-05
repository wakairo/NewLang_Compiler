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
    NL_CHECKED_MATCH_ARM
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
    bool body_nonescape_proved; /* always false in P3 */
} NLCheckedLoanPlan;
typedef struct {
    NLCheckedKind kind;
    NLSourceSpan span, name, qualifier;
    size_t variant;
    bool borrowed_match;
    NLTypeId type;
    NLSymbolId symbol;
    size_t function; /* resolved prelude/registered signature identity */
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
    bool has_scalar_result; /* raw observations do not mint ValuePackages */
    NLScalarValue scalar_result;
    size_t raw_offsets[2], raw_count; /* resolved constant selections */
    NLCheckedLoanPlan loan;
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

/* Borrowed arm evidence in source order. Its semantic IDs belong exclusively
 * to nl_checked_context(arm), an artifact-owned hypothetical branch snapshot.
 * They never identify public-context values. The parent owns/destroys arms and
 * their contexts; source lifetime must cover both. Nested matches are outside
 * the bounded P6 slice. */
const NLCheckedFragment *nl_checked_match_arm(const NLCheckedFragment *,
                                              NLCheckedNodeId match,
                                              size_t index);

#endif
