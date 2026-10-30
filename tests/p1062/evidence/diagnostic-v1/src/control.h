#ifndef NEWLANG_CONTROL_H
#define NEWLANG_CONTROL_H

#include "newlang/checked.h"

/* Internal production proof substrate; not source-visible control constructs.
 * All out owner slots must initially be NULL. No API commits a public context.
 * Targets are live nominal handles, retained by evidence, never semantic IDs.
 */
#define NL_CONTROL_MAX_EXITS 64
#define NL_CONTROL_MAX_SLOTS 16

typedef enum {
    NL_TARGET_FUNCTION,
    NL_TARGET_LOOP
} NLControlTargetKind;
typedef struct NLControlTarget NLControlTarget;
NLCheckStatus nl_control_target_create(NLControlTargetKind, NLControlTarget **);
void nl_control_target_destroy(NLControlTarget *);
bool nl_control_target_same(const NLControlTarget *, const NLControlTarget *);

typedef struct NLControlState NLControlState;
/* Root construction clones borrowed context. Fork preserves ONLY root/public
 * prefix origin. Mutations use ordinary semantic APIs against the owned clone.
 * Equal post-fork numbers are not shared origins. */
NLCheckStatus nl_control_state_create(const NLSemanticContext *,
                                      NLControlState **);
NLCheckStatus nl_control_state_fork(const NLControlState *, NLControlState **);
/* Capture a checked transfer with the entry's nominal origin certificate. */
NLCheckStatus nl_control_state_capture(const NLControlState *,
                                       const NLSemanticContext *,
                                       NLControlState **);
/* Drop only ended iteration-local bindings/places/scopes after escape checking.
 */
NLCheckStatus nl_control_state_project(const NLControlState *,
                                       const NLSemanticContext *,
                                       NLControlState **);
NLSemanticContext *nl_control_state_context(NLControlState *); /* borrowed */
void nl_control_state_destroy(NLControlState *);

typedef enum {
    NL_EXIT_RETURN,
    NL_EXIT_CONTINUE,
    NL_EXIT_BREAK
} NLControlExitKind;
typedef struct NLControlExits NLControlExits;
const NLControlExits *
nl_checked_control_exits(const NLCheckedFragment *); /* borrowed */
typedef struct {
    NLControlExitKind kind;
    const NLControlTarget *target; /* borrowed from set */
    const NLControlState *state;   /* borrowed owned post-state */
    size_t count;
    NLCheckedResult values[NL_CONTROL_MAX_SLOTS]; /* IDs belong ONLY to state */
} NLControlExitView;
/* Append clones post-state and retains target; atomic on every failure.
 * Set union is a finite IF/MATCH alternative composition, never a chosen arm.
 * Empty set is valid: with normal=false it denotes zero-normal-exit.
 * NULL set denotes an empty set, allocation is lazy. */
NLCheckStatus nl_control_exit_append(NLControlExits **, NLControlExitKind,
                                     NLControlTarget *, const NLControlState *,
                                     const NLCheckedResult *, size_t);
NLCheckStatus nl_control_exits_union(NLControlExits **, const NLControlExits *);
size_t nl_control_exits_count(const NLControlExits *);
const NLControlExitView *nl_control_exit_view(const NLControlExits *, size_t);
void nl_control_exits_destroy(NLControlExits *);
typedef struct {
    bool normal;
    NLControlExits *exits;
} NLControlOutcome;
/* Finite alternatives (IF/MATCH) and sequential lexical-block composition.
 * Inputs borrowed, output owns copies; out must be empty. Atomic on failure.
 * then skips the second flow when first has no normal continuation. */
NLCheckStatus nl_control_outcome_join(const NLControlOutcome *,
                                      const NLControlOutcome *,
                                      NLControlOutcome *);
NLCheckStatus nl_control_outcome_then(const NLControlOutcome *,
                                      const NLControlOutcome *,
                                      NLControlOutcome *);
void nl_control_outcome_destroy(NLControlOutcome *);
/* All exit snapshots validated at boundary; Continue/Break cannot masquerade
 * as Return. Empty exits with no normal completion are legal, not Return unit.
 * Result joining remains the owning finite checker's responsibility. */
NLCheckStatus nl_control_function_boundary(const NLControlExits *,
                                           const NLControlTarget *,
                                           NLTypeId result, size_t scopes,
                                           size_t places);
bool nl_control_exits_only_return(const NLControlExits *,
                                  const NLControlTarget *);

/* Header keeps an owned entry certificate. Slot IDs describe loose carried
 * responsibilities in input state; no symbolic slot is minted as a package.
 * wide_copy=true abstracts flat Copy slots AND existing flat Copy current facts
 * to unknown. Exact non-Copy and outer availability are never widened.
 * Unrepresented dependencies/correlations -> analysis precision, not success.
 */
typedef struct NLLoopHeader NLLoopHeader;
NLCheckStatus nl_loop_header_create(const NLControlState *, NLControlTarget *,
                                    const NLValueId *, size_t count,
                                    bool wide_copy, NLLoopHeader **);
/* Source checker widens slots while keeping the captured memory frame exact. */
NLCheckStatus nl_loop_header_create_bounded(const NLControlState *,
                                            NLControlTarget *,
                                            const NLValueId *, size_t,
                                            bool wide_slots, bool wide_memory,
                                            NLLoopHeader **);
void nl_loop_header_destroy(NLLoopHeader *);
NLCheckStatus nl_loop_header_includes(const NLLoopHeader *,
                                      const NLControlState *, const NLValueId *,
                                      size_t count);
/* Every supplied Continue must target this header and pass inclusion. Return /
 * Break remain separate and do not feed recurrence. Must supply COMPLETE body
 * transfer evidence: this API does not prove the caller supplied every source
 * edge or that its transfer is sound for all inputs represented by H. */
NLCheckStatus nl_loop_header_closure(const NLLoopHeader *,
                                     const NLControlExits *);

#endif
