#ifndef NEWLANG_SCALAR_VIEW_INTERNAL_H
#define NEWLANG_SCALAR_VIEW_INTERNAL_H

#include "semantic_internal.h"
#include <stdio.h>
#include <string.h>

/* Read-only scalar/source consistency; no issuer, ownership or runtime state.
 */
static bool copy_if_plain(NLSemanticValueView v, NLTypeId type)
{
    return v.type == type && v.dependencies == NL_DEPENDENCY_FREE &&
           v.value_dependency_count == 0 && v.reference_count == 0 &&
           v.reference.place == 0 && v.reference.scope == 0 &&
           v.reference.incarnation == 0 &&
           v.reference.provenance == NL_PROVENANCE_UNKNOWN &&
           !v.reference.readable && !v.reference.writable &&
           v.reference.occurrence_dependency == 0 && v.domain == 0 &&
           v.slot_place == 0 && v.allocation_region == 0 &&
           v.occupancy.region == 0 && v.field_count == 0 &&
           v.aggregate_owner == 0 && v.sum_owner == 0 && v.sum_payload == 0 &&
           v.variant == 0;
}
static bool copy_if_gap(const NLSource *source, size_t start, size_t end,
                        const char *expected)
{
    NLSourceView bytes;
    if (!nl_source_view(source, (NLSourceSpan){start, end}, &bytes))
        return false;
    for (size_t i = 0; i < bytes.length; ++i) {
        const unsigned char ch = bytes.bytes[i];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            continue;
        if (*expected == 0 || ch != (unsigned char)*expected++)
            return false;
    }
    return *expected == 0;
}
static bool copy_if_literal(const NLCheckedFragment *f,
                            const NLCheckedNodeView *v)
{
    NLSemanticValueView value;
    NLSourceView text;
    if (v == NULL || v->kind != NL_CHECKED_U8_LITERAL || v->result_count != 1 ||
        !v->has_scalar_result || !v->scalar_result.known ||
        v->scalar_result.type != v->type || v->scalar_result.value > 255 ||
        !nl_semantic_value_view(nl_checked_context(f), v->results[0].value,
                                &value) ||
        !copy_if_plain(value, v->type) || !value.scalar_known ||
        value.scalar_value != v->scalar_result.value ||
        !nl_source_view(nl_checked_source(f), v->span, &text) ||
        text.length < 5 || text.bytes[0] != 'u' ||
        text.bytes[text.length - 1] != ')')
        return false;
    /* Read-only consistency with the already parsed source; no source
     * spelling is emitted as C and no new syntax is admitted. */
    size_t i = 0, number = 0, digits = 0;
    const char *prefix = "u8(";
    while (i < text.length && *prefix != 0) {
        unsigned char ch = text.bytes[i++];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            continue;
        if (ch != (unsigned char)*prefix++)
            return false;
    }
    if (*prefix != 0)
        return false;
    while (i < text.length) {
        unsigned char ch = text.bytes[i];
        if (ch < '0' || ch > '9')
            break;
        if (number > (255U - (ch - '0')) / 10)
            return false;
        number = number * 10 + ch - '0';
        ++i;
        ++digits;
    }
    return digits != 0 && number == value.scalar_value &&
           copy_if_gap(nl_checked_source(f), v->span.start_byte + i,
                       v->span.end_byte, ")");
}
static bool copy_if_arm_origin(const NLCheckedFragment *parent,
                               NLCheckedNodeId id, size_t index,
                               const NLCheckedFragment *arm)
{
    if (parent == NULL || arm == NULL || arm->context == NULL ||
        arm->if_parent != parent || arm->if_parent_node != id ||
        arm->if_arm_index != index || arm->source != parent->source)
        return false;
    size_t found = 0;
    for (size_t i = 0; i < parent->arm_count; ++i) {
        const NLCheckedArm *issued = &parent->arms[i];
        if (issued->match == id && found++ == index)
            return issued->artifact == arm &&
                   issued->context_origin == arm->context;
    }
    return false;
}

#endif
