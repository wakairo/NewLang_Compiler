#ifndef NEWLANG_TEST_RAW_STORAGE_CHECK_H
#define NEWLANG_TEST_RAW_STORAGE_CHECK_H

#include "semantic_check.h"
#include <stdint.h>

static inline NLRawQuantity raw_known(size_t value)
{
    return (NLRawQuantity){true, value};
}
static inline NLRawOperand raw_binding(NLSymbolId binding)
{
    return (NLRawOperand){.binding = binding};
}
static inline NLRawOperand raw_loose(NLValueId value)
{
    return (NLRawOperand){.loose = value};
}
static inline bool raw_run(NLSemanticContext *c, NLRawOperation op,
                           NLCheckedNodeView *result)
{
    NLCheckedFragment *artifact = NULL;
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status =
        nl_semantic_check_raw_operation(c, &op, &artifact, &diagnostic);
    if (status != NL_CHECK_OK) {
        fprintf(stderr, "raw operation %d: %s\n", (int)op.kind,
                diagnostic.diagnostic.code);
    }
    CHECK(status == NL_CHECK_OK && artifact != NULL);
    CHECK(nl_checked_context(artifact) == c &&
          nl_checked_source(artifact) == op.source);
    const NLCheckedNodeView *const root =
        nl_checked_node_view(artifact, nl_checked_root(artifact));
    CHECK(root != NULL);
    if (result != NULL) {
        *result = *root;
    }
    nl_checked_destroy(artifact);
    return true;
}
static inline bool raw_rejected(NLSemanticContext *c, NLRawOperation op,
                                NLCheckStatus status, const char *code)
{
    TestState before;
    CHECK(test_state(c, &before));
    NLCheckDiagnostic first = {0};
    for (size_t repeat = 0; repeat < 2; ++repeat) {
        NLCheckedFragment *artifact = NULL;
        NLCheckDiagnostic d = {0};
        CHECK(nl_semantic_check_raw_operation(c, &op, &artifact, &d) == status);
        CHECK(artifact == NULL && d.diagnostic.severity == NL_DIAG_ERROR);
        if (code != NULL) {
            if (strcmp(d.diagnostic.code, code) != 0) {
                fprintf(stderr, "raw expected %s, received %s\n", code,
                        d.diagnostic.code);
            }
            CHECK(strcmp(d.diagnostic.code, code) == 0);
        }
        CHECK(test_unchanged(c, &before));
        if (repeat == 0) {
            first = d;
        } else {
            CHECK(strcmp(first.diagnostic.code, d.diagnostic.code) == 0 &&
                  strcmp(first.diagnostic.message, d.diagnostic.message) == 0 &&
                  strcmp(first.diagnostic.category, d.diagnostic.category) ==
                      0 &&
                  first.span.start_byte == d.span.start_byte &&
                  first.span.end_byte == d.span.end_byte);
        }
    }
    return true;
}
static inline bool raw_allocate(NLSemanticContext *c, size_t size, size_t align,
                                bool read, bool write,
                                const char *allocation_name,
                                const char *storage_name,
                                NLSymbolId *allocation, NLSymbolId *storage,
                                NLBackingRegionId *region)
{
    NLCheckedNodeView result;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_ALLOCATE,
                                   .data.allocate = {.size = {true, size},
                                                     .alignment = {true, align},
                                                     .ordinary_read = read,
                                                     .ordinary_write = write,
                                                     .address_known = true,
                                                     .address = 4096}},
                  &result));
    CHECK(result.result_count == 2 && !result.has_scalar_result);
    CHECK(nl_semantic_bind_result(c, allocation_name, result.results[0].value,
                                  allocation) == NL_CHECK_OK);
    CHECK(nl_semantic_bind_result(c, storage_name, result.results[1].value,
                                  storage) == NL_CHECK_OK);
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(c, result.results[0].value, &v));
    if (region != NULL) {
        *region = v.allocation_region;
    }
    return true;
}
static inline bool raw_ref(NLSemanticContext *c, const char *name,
                           NLSymbolId storage, NLSymbolId *ref,
                           NLScopeId *scope)
{
    NLSemanticBindingView b;
    CHECK(nl_semantic_binding_view(c, storage, &b));
    return test_reference(c, name, b.place, NL_TYPE_REF, NL_ACCESS_READ, false,
                          ref, scope);
}
static inline bool raw_write(NLSemanticContext *c, NLSymbolId ref,
                             size_t offset, bool known, size_t value)
{
    return raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_STORAGE_WRITE_BYTE,
                         .operands = {raw_binding(ref)},
                         .data.write = {{true, offset},
                                        {nl_semantic_core_type(c, NL_TYPE_BYTE),
                                         known, value}}},
        NULL);
}
static inline bool raw_read(NLSemanticContext *c, NLSymbolId ref, size_t offset,
                            bool known, size_t value)
{
    NLCheckedNodeView result;
    CHECK(raw_run(c,
                  (NLRawOperation){.kind = NL_RAW_STORAGE_READ_BYTE,
                                   .operands = {raw_binding(ref)},
                                   .data.byte_offset = {true, offset}},
                  &result));
    CHECK(result.result_count == 0 && result.has_scalar_result &&
          result.scalar_result.type == nl_semantic_core_type(c, NL_TYPE_BYTE) &&
          result.scalar_result.known == known &&
          result.scalar_result.value == value);
    return true;
}
static inline bool raw_copy(NLSemanticContext *c, NLSymbolId dst,
                            size_t dst_offset, NLSymbolId src,
                            size_t src_offset, size_t count)
{
    return raw_run(
        c,
        (NLRawOperation){.kind = NL_RAW_COPY_BYTES,
                         .operands = {raw_binding(dst), raw_binding(src)},
                         .data.copy = {{true, dst_offset},
                                       {true, src_offset},
                                       {true, count}}},
        NULL);
}
static inline bool raw_unspecified(NLSemanticContext *c, NLSymbolId ref,
                                   size_t offset)
{
    return raw_rejected(c,
                        (NLRawOperation){.kind = NL_RAW_STORAGE_READ_BYTE,
                                         .operands = {raw_binding(ref)},
                                         .data.byte_offset = {true, offset}},
                        NL_CHECK_SEMANTIC_ERROR, "P4-UNSPECIFIED-READ");
}
static inline bool raw_bind(NLSemanticContext *c, const char *name,
                            NLValueId value, NLSymbolId *symbol)
{
    CHECK(nl_semantic_bind_result(c, name, value, symbol) == NL_CHECK_OK);
    return true;
}
#endif
