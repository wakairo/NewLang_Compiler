#ifndef NEWLANG_ARRAY4_H
#define NEWLANG_ARRAY4_H
#include "newlang/checked.h"
/* SPECULATIVE PR533 diagnostic/proof observation of a SOURCE-ISSUED placement
 * checkpoint. Borrowed IDs/pointers; never ownership, a live ref or authority.
 * The final lexical world may already be ended. Projection identity is the
 * qualified (original parent incarnation, index, child-place incarnation).
 * No element read/write/projection source API is introduced. */
typedef struct {
    size_t index;
    NLCheckedNodeId initializer;
    NLPlaceId place;
    NLIncarnationId incarnation;
    NLValueFactId fact;
    NLValueId value;
} NLArray4Element;
typedef struct NLArray4Construction {
    const NLCheckedFragment *origin;
    NLCheckedNodeId binding, constructor, receiver;
    NLSymbolId symbol;
    NLTypeId array_type, element_type;
    NLPlaceId root;
    NLIncarnationId incarnation;
    NLValueFactId fact;
    NLValueId value;
    NLArray4Element elements[4];
} NLArray4Construction;
/* Observation never creates a checkpoint. Success only for an owned captured
 * source constructor; failure leaves out unchanged. Source/AST input may have
 * been destroyed. All pointers borrow fragment ownership. */
bool nl_checked_array4_view(const NLCheckedFragment *,
                            NLArray4Construction *out);
/* Borrow the immutable actual captured world for public views; no mutation or
 * authority. */
const NLSemanticContext *nl_checked_array4_world(const NLCheckedFragment *);
/* Independently recheck source nodes, structural types, actual retained
 * placement world, current values and complete child relation. A caller may
 * supply a separate test copy; no mutation/commit or allocation is performed.
 * Validity describes the captured event, NOT current lifetime/owner rights. */
bool nl_checked_array4_valid(const NLCheckedFragment *,
                             const NLArray4Construction *);
/* Emit only the authenticated single-array unit entry. Success writes an
 * allocated C17 buffer; failure leaves both output slots untouched. No guest
 * observation/injection and no general Array ABI or element source operation.
 */
NLCheckStatus nl_checked_array4_c17(const NLCheckedFragment *, char **out,
                                    size_t *length);
#endif
