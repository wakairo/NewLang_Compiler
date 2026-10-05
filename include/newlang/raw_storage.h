#ifndef NEWLANG_RAW_STORAGE_H
#define NEWLANG_RAW_STORAGE_H

#include "newlang/checked.h"

typedef enum {
    NL_RAW_UNSPECIFIED,
    NL_RAW_DEFINED
} NLRawRepValidity;
typedef struct {
    NLRawRepValidity validity;
    bool value_known; /* Defined != known constant */
    unsigned char value;
} NLRawRepView;
typedef struct {
    bool live;
    size_t size, alignment;
    bool ordinary_read, ordinary_write;
    bool address_known;
    size_t address; /* numeric observation only, no identity comparison */
} NLSemanticBackingView;
typedef struct {
    bool known;
    size_t value;
} NLRawQuantity;
typedef struct {
    NLSymbolId binding;
    NLValueId loose;
    NLSourceSpan span;
} NLRawOperand; /* exactly one of binding/loose; context-local IDs */

typedef enum {
    NL_RAW_ALLOCATE,
    NL_RAW_DEALLOCATE,
    NL_RAW_SPLIT,
    NL_RAW_MERGE,
    NL_RAW_INTO_SLOT,
    NL_RAW_ERASE_SLOT,
    NL_RAW_STORAGE_LEN,
    NL_RAW_STORAGE_ADDR,
    NL_RAW_STORAGE_READ_BYTE,
    NL_RAW_STORAGE_WRITE_BYTE,
    NL_RAW_COPY_BYTES,
    NL_RAW_BYTE_TO_U8,
    NL_RAW_U8_TO_BYTE
} NLRawOperationKind;
typedef struct {
    NLRawOperationKind kind;
    const NLSource *source; /* optional, borrowed for diagnostic/text access */
    NLSourceSpan span;
    NLRawOperand
        operands[2]; /* ordered: dealloc Allocation/Storage, copy dst/src */
    union {
        struct {
            NLRawQuantity size, alignment;
            bool ordinary_read, ordinary_write;
            bool address_known;
            size_t address;
        } allocate; /* successful allocator/target facts, not a platform API */
        NLRawQuantity split_at;
        NLTypeId slot_target;
        NLRawQuantity byte_offset;
        struct {
            NLRawQuantity offset;
            NLScalarValue value;
        } write;
        struct {
            NLRawQuantity dst_offset, src_offset, count;
        } copy;
        NLScalarValue conversion;
    } data;
} NLRawOperation;

/* Programmatic semantic entry; no source grammar, physical allocator or I/O.
 * Context owns region/representation summaries. Request/source are borrowed;
 * success returns one immutable caller-owned artifact. Binding/loose operands
 * use ordinary ownership rules; raw observations use compatible read refs to
 * the current Storage value. Scalar results are inline, not authority packages.
 * Complete success commits; every failure leaves context and owner unchanged.
 * Success leaves diagnostic untouched; NULL-source spans are placeholders.
 * Unknown dynamic proof inputs return precision limits. See P4 contract. */
NLCheckStatus nl_semantic_check_raw_operation(NLSemanticContext *,
                                              const NLRawOperation *,
                                              NLCheckedFragment **out,
                                              NLCheckDiagnostic *);
bool nl_semantic_backing_view(const NLSemanticContext *, NLBackingRegionId,
                              NLSemanticBackingView *);
/* Compiler fact inspection only, never a safe byte read or authority mint. */
bool nl_semantic_raw_rep_view(const NLSemanticContext *, NLBackingRegionId,
                              size_t offset, NLRawRepView *);

#endif
