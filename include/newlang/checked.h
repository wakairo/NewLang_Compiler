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
    NL_CHECKED_LOAN_HEADER
} NLCheckedKind;
typedef enum {
    NL_VALUE_USE_NONE,
    NL_VALUE_COPIED,
    NL_VALUE_CONSUMED,
    NL_VALUE_REBORROWED
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
    NLSourceSpan span, name;
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
    size_t
        result_count; /* 0 = unit/no responsibility; 1 or 2 separate values */
    NLCheckedResult results[2];
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

#endif
