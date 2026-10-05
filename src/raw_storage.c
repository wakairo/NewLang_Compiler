#include "semantic_internal.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

_Static_assert(CHAR_BIT == 8, "P4 host byte bookkeeping requires octets");

void nl_raw_dispose(NLSemanticContext *c)
{
    for (size_t i = 0; i < c->region_count; ++i) {
        free(c->regions[i].intervals);
    }
    free(c->regions);
}

NLCheckStatus nl_raw_clone(const NLSemanticContext *source,
                           NLSemanticContext *copy)
{
    if (source->region_count == 0) {
        return NL_CHECK_OK;
    }
    copy->regions = malloc(source->region_count * sizeof(*copy->regions));
    if (copy->regions == NULL) {
        return NL_CHECK_OUT_OF_MEMORY;
    }
    for (size_t i = 0; i < source->region_count; ++i) {
        const NLRawRegionEntry *const old = &source->regions[i];
        copy->regions[i] = (NLRawRegionEntry){.view = old->view};
        ++copy->region_count;
        if (old->count != 0) {
            copy->regions[i].intervals =
                malloc(old->count * sizeof(*old->intervals));
            if (copy->regions[i].intervals == NULL) {
                return NL_CHECK_OUT_OF_MEMORY;
            }
            for (size_t j = 0; j < old->count; ++j) {
                copy->regions[i].intervals[j] = old->intervals[j];
            }
        }
        copy->regions[i].count = old->count;
    }
    copy->raw_interval_count = source->raw_interval_count;
    return NL_CHECK_OK;
}

bool nl_semantic_backing_view(const NLSemanticContext *c,
                              NLBackingRegionId region,
                              NLSemanticBackingView *out)
{
    if (c == NULL || out == NULL || region == 0 || region > c->region_count) {
        return false;
    }
    *out = c->regions[region - 1].view;
    return true;
}
bool nl_semantic_raw_rep_view(const NLSemanticContext *c,
                              NLBackingRegionId region, size_t offset,
                              NLRawRepView *out)
{
    if (c == NULL || out == NULL || region == 0 || region > c->region_count) {
        return false;
    }
    const NLRawRegionEntry *const r = &c->regions[region - 1];
    if (!r->view.live || offset >= r->view.size) {
        return false;
    }
    for (size_t i = 0; i < r->count; ++i) {
        const NLRawInterval interval = r->intervals[i];
        if (offset >= interval.start &&
            offset - interval.start < interval.length) {
            *out = interval.state;
            return true;
        }
    }
    return false;
}

static bool range_valid(const NLSemanticContext *c, NLBackingRange range)
{
    if (range.region == 0 || range.region > c->region_count ||
        range.length == 0) {
        return false;
    }
    const NLSemanticBackingView r = c->regions[range.region - 1].view;
    return r.live && range.start <= r.size &&
           range.length <= r.size - range.start;
}
static bool overlap(NLBackingRange a, NLBackingRange b)
{
    return a.region == b.region && a.start < b.start + b.length &&
           b.start < a.start + a.length;
}

NLCheckStatus nl_raw_validate(const NLSemanticContext *c)
{
    if (c->region_count == 0) {
        return NL_CHECK_OK;
    }
    const size_t maximum = c->value_count + c->place_count;
    NLBackingRange *const ranges =
        malloc((maximum == 0 ? 1 : maximum) * sizeof(*ranges));
    size_t *const totals = malloc(c->region_count * 2 * sizeof(*totals));
    if (ranges == NULL || totals == NULL) {
        free(ranges);
        free(totals);
        return NL_CHECK_OUT_OF_MEMORY;
    }
    for (size_t i = 0; i < c->region_count * 2; ++i) {
        totals[i] = 0;
    }
    NLCheckStatus status = NL_CHECK_INTERNAL_ERROR;
    size_t count = 0, intervals = 0;
    for (size_t i = 0; i < c->value_count; ++i) {
        const NLSemanticValueView v = c->values[i];
        if (v.carrier == NL_CARRIER_ENDED) {
            continue;
        }
        const NLSemanticTypeKind kind = c->types[v.type - 1].view.kind;
        if (kind == NL_TYPE_ALLOCATION) {
            if (v.allocation_region == 0 ||
                v.allocation_region > c->region_count ||
                !c->regions[v.allocation_region - 1].view.live) {
                goto done;
            }
            ++totals[c->region_count + v.allocation_region - 1];
        }
        if (kind == NL_TYPE_STORAGE ||
            (kind == NL_TYPE_SLOT && v.occupancy.region != 0)) {
            if (!range_valid(c, v.occupancy)) {
                goto done;
            }
            ranges[count++] = v.occupancy;
        }
    }
    for (size_t i = 0; i < c->place_count; ++i) {
        const NLSemanticPlaceView p = c->places[i];
        if (p.placement.region == 0) {
            continue;
        }
        if (!p.live || !range_valid(c, p.placement)) {
            goto done;
        }
        const NLSemanticTypeView type = c->types[p.type - 1].view;
        if (!type.layout_known || type.size != p.placement.length) {
            goto done;
        }
        ranges[count++] = p.placement;
    }
    for (size_t i = 0; i < count; ++i) {
        const NLBackingRange range = ranges[i];
        size_t *const total = &totals[range.region - 1];
        if (range.length > c->regions[range.region - 1].view.size - *total) {
            goto done;
        }
        *total += range.length;
        for (size_t j = 0; j < i; ++j) {
            if (overlap(range, ranges[j])) {
                goto done;
            }
        }
    }
    for (size_t i = 0; i < c->region_count; ++i) {
        const NLRawRegionEntry *const r = &c->regions[i];
        if (!r->view.live) {
            if (r->count != 0 || totals[i] != 0 ||
                totals[c->region_count + i] != 0) {
                goto done;
            }
            continue;
        }
        if (totals[i] != r->view.size || totals[c->region_count + i] != 1) {
            goto done;
        }
        size_t end = 0;
        for (size_t j = 0; j < r->count; ++j) {
            const NLRawInterval interval = r->intervals[j];
            if (interval.start != end || interval.length == 0 ||
                interval.length > r->view.size - end) {
                goto done;
            }
            end += interval.length;
        }
        if (end != r->view.size) {
            goto done;
        }
        intervals += r->count;
    }
    if (intervals != c->raw_interval_count) {
        goto done;
    }
    status = NL_CHECK_OK;
done:
    free(ranges);
    free(totals);
    return status;
}

static bool state_equal(NLRawRepView a, NLRawRepView b)
{
    return a.validity == b.validity && a.value_known == b.value_known &&
           (!a.value_known || a.value == b.value);
}
static void append_interval(NLRawInterval *output, size_t *count,
                            NLRawInterval interval)
{
    if (interval.length == 0) {
        return;
    }
    if (*count != 0 &&
        output[*count - 1].start + output[*count - 1].length ==
            interval.start &&
        state_equal(output[*count - 1].state, interval.state)) {
        output[*count - 1].length += interval.length;
    } else {
        output[(*count)++] = interval;
    }
}

/* Source replacement is an owned/stable snapshot or one stack interval, never
 * an alias into the growable destination partition. Counts/ranges are bounded.
 */
static NLCheckStatus replace_range(NLSemanticContext *c, NLBackingRange range,
                                   const NLRawInterval *replacement,
                                   size_t replacement_count)
{
    if (!range_valid(c, range)) {
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLRawRegionEntry *const r = &c->regions[range.region - 1];
    const size_t capacity = r->count + replacement_count + 2;
    NLRawInterval *const output = malloc(capacity * sizeof(*output));
    if (output == NULL) {
        return NL_CHECK_OUT_OF_MEMORY;
    }
    size_t count = 0;
    const size_t end = range.start + range.length;
    for (size_t i = 0; i < r->count; ++i) {
        const NLRawInterval old = r->intervals[i];
        if (old.start < range.start) {
            const size_t limit = old.start + old.length < range.start
                                     ? old.start + old.length
                                     : range.start;
            append_interval(
                output, &count,
                (NLRawInterval){old.start, limit - old.start, old.state});
        }
    }
    for (size_t i = 0; i < replacement_count; ++i) {
        NLRawInterval item = replacement[i];
        item.start += range.start;
        append_interval(output, &count, item);
    }
    for (size_t i = 0; i < r->count; ++i) {
        const NLRawInterval old = r->intervals[i];
        if (old.start + old.length > end) {
            const size_t start = old.start > end ? old.start : end;
            append_interval(output, &count,
                            (NLRawInterval){start,
                                            old.start + old.length - start,
                                            old.state});
        }
    }
    const size_t total = c->raw_interval_count - r->count + count;
    if (total > NL_SEMANTIC_MAX_ENTRIES) {
        free(output);
        return NL_CHECK_RESOURCE_LIMIT;
    }
    free(r->intervals);
    r->intervals = output;
    r->count = count;
    c->raw_interval_count = total;
    return NL_CHECK_OK;
}
static NLCheckStatus unspecified(NLSemanticContext *c, NLBackingRange range)
{
    const NLRawInterval state = {
        0, range.length, {NL_RAW_UNSPECIFIED, false, 0}};
    return replace_range(c, range, &state, 1);
}

NLCheckStatus nl_raw_start_root(NLSemanticContext *c, NLValueId slot,
                                NLPlaceId place)
{
    const NLBackingRange range = c->values[slot - 1].occupancy;
    if (range.region != 0) {
        const NLCheckStatus status = unspecified(c, range);
        if (status != NL_CHECK_OK) {
            return status;
        }
        c->places[place - 1].placement = range;
    }
    c->values[slot - 1].carrier = NL_CARRIER_ENDED;
    return NL_CHECK_OK;
}
NLCheckStatus nl_raw_end_root(NLSemanticContext *c, NLBackingRange range,
                              NLValueId slot)
{
    if (range.region == 0) {
        return NL_CHECK_OK;
    }
    const NLCheckStatus status = unspecified(c, range);
    if (status != NL_CHECK_OK) {
        return status;
    }
    c->values[slot - 1].occupancy = range;
    return NL_CHECK_OK;
}

typedef struct {
    NLSemanticContext *context;
    const NLRawOperation *operation;
    NLCheckDiagnostic *diagnostic;
} Raw;
static NLCheckStatus reject(Raw *raw, NLCheckStatus status, NLSourceSpan span,
                            const char *code, const char *message)
{
    const char *category =
        status == NL_CHECK_ANALYSIS_PRECISION_LIMIT ? "precision"
        : status == NL_CHECK_SEMANTIC_UNSUPPORTED   ? "unsupported"
        : status == NL_CHECK_OUT_OF_MEMORY || status == NL_CHECK_RESOURCE_LIMIT
            ? "host"
        : status == NL_CHECK_INTERNAL_ERROR ? "internal"
                                            : "semantic";
    if (raw->diagnostic != NULL) {
        *raw->diagnostic = (NLCheckDiagnostic){
            {NL_DIAG_ERROR, category, code, message, NULL, NULL, 0}, span};
    }
    return status;
}
static NLCheckStatus checked_host(Raw *raw, NLCheckStatus status)
{
    if (status == NL_CHECK_OK) {
        return status;
    }
    return reject(raw, status, raw->operation->span,
                  status == NL_CHECK_OUT_OF_MEMORY    ? "P4-OUT-OF-MEMORY"
                  : status == NL_CHECK_RESOURCE_LIMIT ? "P4-RESOURCE-LIMIT"
                                                      : "P4-INTERNAL",
                  "raw semantic host allocation/budget or invariant failure");
}
static NLCheckStatus quantity(Raw *raw, NLRawQuantity q, size_t *out)
{
    if (!q.known) {
        return reject(
            raw, NL_CHECK_ANALYSIS_PRECISION_LIMIT, raw->operation->span,
            "P4-DYNAMIC-PRECISION",
            "constant bounds/layout proof is required by the P4 slice");
    }
    *out = q.value;
    return NL_CHECK_OK;
}
static NLCheckStatus new_result(Raw *raw, NLCheckedNodeView *result,
                                NLSemanticValueView value)
{
    NLValueId id = 0;
    const NLCheckStatus status = nl_sem_new_value(raw->context, value, &id);
    if (status != NL_CHECK_OK) {
        return checked_host(raw, status);
    }
    result->results[result->result_count++] = (NLCheckedResult){value.type, id};
    if (result->result_count == 1) {
        result->type = value.type;
    }
    return NL_CHECK_OK;
}
static void end_value(NLSemanticContext *c, NLValueId value)
{
    c->values[value - 1].carrier = NL_CARRIER_ENDED;
    c->values[value - 1].owner_place = 0;
}
static NLCheckStatus raw_claim(Raw *raw, NLValueId id, size_t argument,
                               NLBackingRange *out)
{
    const NLSemanticValueView v = raw->context->values[id - 1];
    if (raw->context->types[v.type - 1].view.kind != NL_TYPE_STORAGE ||
        !range_valid(raw->context, v.occupancy)) {
        return reject(raw, NL_CHECK_SEMANTIC_ERROR,
                      raw->operation->operands[argument].span,
                      "P4-STORAGE-REQUIRED",
                      "operation requires a live raw Storage responsibility");
    }
    *out = v.occupancy;
    return NL_CHECK_OK;
}
static NLCheckStatus observed_claim(Raw *raw, NLValueId ref, size_t argument,
                                    NLBackingRange *out)
{
    const NLSemanticValueView v = raw->context->values[ref - 1];
    const NLValueId current =
        raw->context->places[v.reference.place - 1].current_value;
    return raw_claim(raw, current, argument, out);
}
static NLCheckStatus access_check(Raw *raw, NLBackingRange range, bool write,
                                  size_t argument)
{
    const NLSemanticBackingView view =
        raw->context->regions[range.region - 1].view;
    if (!(write ? view.ordinary_write : view.ordinary_read)) {
        return reject(
            raw, NL_CHECK_SEMANTIC_ERROR,
            raw->operation->operands[argument].span, "P4-BACKING-ACCESS",
            "backing does not permit the requested ordinary raw access");
    }
    return NL_CHECK_OK;
}
static NLCheckStatus bounds(Raw *raw, NLBackingRange claim, size_t offset,
                            size_t count, size_t argument)
{
    if (offset > claim.length || count > claim.length - offset) {
        return reject(raw, NL_CHECK_SEMANTIC_ERROR,
                      raw->operation->operands[argument].span, "P4-RAW-BOUNDS",
                      "selection is outside the current Storage range");
    }
    return NL_CHECK_OK;
}
static NLCheckStatus scalar_check(Raw *raw, NLScalarValue value,
                                  NLSemanticTypeKind expected)
{
    if (value.type != nl_semantic_core_type(raw->context, expected)) {
        return reject(raw, NL_CHECK_SEMANTIC_ERROR, raw->operation->span,
                      "P4-SCALAR-TYPE",
                      "scalar type differs from the selected raw operation");
    }
    if (value.known && value.value > 255) {
        return reject(raw, NL_CHECK_SEMANTIC_ERROR, raw->operation->span,
                      "P4-OCTET-DOMAIN",
                      "byte/u8 scalar must be in its 256-value domain");
    }
    return NL_CHECK_OK;
}

NLCheckStatus nl_raw_apply(NLSemanticContext *c, const NLRawOperation *op,
                           const NLValueId inputs[2], NLCheckedNodeView *result,
                           NLCheckDiagnostic *diagnostic)
{
    Raw raw = {c, op, diagnostic};
    NLCheckStatus status = NL_CHECK_OK;
    const NLTypeId storage_type = nl_semantic_core_type(c, NL_TYPE_STORAGE);
    result->type = nl_semantic_unit_type(c);
    if (op->kind == NL_RAW_ALLOCATE) {
        size_t size, alignment;
        if ((status = quantity(&raw, op->data.allocate.size, &size)) !=
                NL_CHECK_OK ||
            (status = quantity(&raw, op->data.allocate.alignment,
                               &alignment)) != NL_CHECK_OK) {
            return status;
        }
        if (size == 0) {
            return reject(&raw, NL_CHECK_SEMANTIC_UNSUPPORTED, op->span,
                          "P4-EMPTY-ALLOCATION-DEFERRED",
                          "empty allocation responsibility is outside the "
                          "closed P4 subset");
        }
        if (alignment == 0 ||
            (op->data.allocate.address_known &&
             (op->data.allocate.address % alignment != 0 ||
              size - 1 > SIZE_MAX - op->data.allocate.address))) {
            return reject(&raw, NL_CHECK_SEMANTIC_ERROR, op->span,
                          "P4-ALLOCATION-PROFILE",
                          "allocator alignment/address facts do not satisfy "
                          "the ordinary profile");
        }
        if (c->region_count >= NL_SEMANTIC_MAX_ENTRIES ||
            c->raw_interval_count >= NL_SEMANTIC_MAX_ENTRIES) {
            return checked_host(&raw, NL_CHECK_RESOURCE_LIMIT);
        }
        NLRawInterval *const initial = malloc(sizeof(*initial));
        if (initial == NULL) {
            return checked_host(&raw, NL_CHECK_OUT_OF_MEMORY);
        }
        *initial = (NLRawInterval){0, size, {NL_RAW_UNSPECIFIED, false, 0}};
        NLRawRegionEntry *const regions =
            realloc(c->regions, (c->region_count + 1) * sizeof(*regions));
        if (regions == NULL) {
            free(initial);
            return checked_host(&raw, NL_CHECK_OUT_OF_MEMORY);
        }
        c->regions = regions;
        c->regions[c->region_count++] = (NLRawRegionEntry){
            {true, size, alignment, op->data.allocate.ordinary_read,
             op->data.allocate.ordinary_write, op->data.allocate.address_known,
             op->data.allocate.address_known ? op->data.allocate.address : 0},
            initial,
            1};
        ++c->raw_interval_count;
        status =
            new_result(&raw, result,
                       (NLSemanticValueView){
                           .type = nl_semantic_core_type(c, NL_TYPE_ALLOCATION),
                           .allocation_region = c->region_count});
        if (status != NL_CHECK_OK) {
            return status;
        }
        return new_result(
            &raw, result,
            (NLSemanticValueView){.type = storage_type,
                                  .occupancy = {c->region_count, 0, size}});
    }
    if (op->kind == NL_RAW_BYTE_TO_U8 || op->kind == NL_RAW_U8_TO_BYTE) {
        const bool to_byte = op->kind == NL_RAW_U8_TO_BYTE;
        status = scalar_check(&raw, op->data.conversion,
                              to_byte ? NL_TYPE_U8 : NL_TYPE_BYTE);
        if (status != NL_CHECK_OK) {
            return status;
        }
        result->type =
            nl_semantic_core_type(c, to_byte ? NL_TYPE_BYTE : NL_TYPE_U8);
        result->has_scalar_result = true;
        result->scalar_result = (NLScalarValue){
            result->type, op->data.conversion.known,
            op->data.conversion.known ? op->data.conversion.value : 0};
        return NL_CHECK_OK;
    }
    if (op->kind == NL_RAW_ERASE_SLOT) {
        const NLSemanticValueView slot = c->values[inputs[0] - 1];
        if (c->types[slot.type - 1].view.kind == NL_TYPE_SLOT &&
            slot.occupancy.region == 0) {
            return reject(
                &raw, NL_CHECK_SEMANTIC_UNSUPPORTED, op->operands[0].span,
                "P4-ABSTRACT-SLOT-UNSUPPORTED",
                "legacy fixture slot lacks explicit backing range facts");
        }
        if (c->types[slot.type - 1].view.kind != NL_TYPE_SLOT ||
            !range_valid(c, slot.occupancy) || slot.slot_place == 0 ||
            c->places[slot.slot_place - 1].live) {
            return reject(&raw, NL_CHECK_SEMANTIC_ERROR, op->operands[0].span,
                          "P4-EMPTY-SLOT-REQUIRED",
                          "erase_slot requires a live-backed definitely-empty "
                          "slot responsibility");
        }
        end_value(c, inputs[0]);
        return new_result(&raw, result,
                          (NLSemanticValueView){.type = storage_type,
                                                .occupancy = slot.occupancy});
    }
    if (op->kind == NL_RAW_DEALLOCATE) {
        const NLSemanticValueView allocation = c->values[inputs[0] - 1];
        NLBackingRange range;
        if ((status = raw_claim(&raw, inputs[1], 1, &range)) != NL_CHECK_OK) {
            return status;
        }
        if (allocation.allocation_region != range.region) {
            return reject(&raw, NL_CHECK_SEMANTIC_ERROR, op->operands[0].span,
                          "P4-ALLOCATION-MISMATCH",
                          "Allocation and Storage have different BackingRegion "
                          "identities");
        }
        NLRawRegionEntry *const r = &c->regions[range.region - 1];
        if (range.start != 0 || range.length != r->view.size) {
            return reject(&raw, NL_CHECK_SEMANTIC_ERROR, op->operands[1].span,
                          "P4-FULL-RANGE-REQUIRED",
                          "deallocation requires full-range raw Storage");
        }
        end_value(c, inputs[0]);
        end_value(c, inputs[1]);
        r->view.live = false;
        c->raw_interval_count -= r->count;
        free(r->intervals);
        r->intervals = NULL;
        r->count = 0;
        return NL_CHECK_OK;
    }
    if (op->kind == NL_RAW_SPLIT || op->kind == NL_RAW_MERGE ||
        op->kind == NL_RAW_INTO_SLOT) {
        NLBackingRange first;
        if ((status = raw_claim(&raw, inputs[0], 0, &first)) != NL_CHECK_OK) {
            return status;
        }
        if (op->kind == NL_RAW_SPLIT) {
            size_t cut;
            if ((status = quantity(&raw, op->data.split_at, &cut)) !=
                NL_CHECK_OK) {
                return status;
            }
            if (cut > first.length) {
                return reject(&raw, NL_CHECK_SEMANTIC_ERROR, op->span,
                              "P4-SPLIT-BOUNDS",
                              "split cut exceeds Storage length");
            }
            if (cut == 0 || cut == first.length) {
                return reject(&raw, NL_CHECK_SEMANTIC_ERROR, op->span,
                              "P4-SPLIT-ENDPOINT",
                              "split cut must be strictly inside non-empty "
                              "Storage range");
            }
            end_value(c, inputs[0]);
            status =
                new_result(&raw, result,
                           (NLSemanticValueView){
                               .type = storage_type,
                               .occupancy = {first.region, first.start, cut}});
            if (status != NL_CHECK_OK) {
                return status;
            }
            return new_result(&raw, result,
                              (NLSemanticValueView){
                                  .type = storage_type,
                                  .occupancy = {first.region, first.start + cut,
                                                first.length - cut}});
        }
        if (op->kind == NL_RAW_MERGE) {
            NLBackingRange second;
            if ((status = raw_claim(&raw, inputs[1], 1, &second)) !=
                NL_CHECK_OK) {
                return status;
            }
            if (first.region != second.region) {
                return reject(&raw, NL_CHECK_SEMANTIC_ERROR,
                              op->operands[1].span, "P4-REGION-MISMATCH",
                              "merge requires the same BackingRegion identity, "
                              "not equal addresses");
            }
            if (overlap(first, second) ||
                (first.start + first.length != second.start &&
                 second.start + second.length != first.start)) {
                return reject(&raw, NL_CHECK_SEMANTIC_ERROR,
                              op->operands[1].span, "P4-NONADJACENT-MERGE",
                              "merge requires disjoint adjacent ranges with a "
                              "contiguous union");
            }
            const size_t start =
                first.start < second.start ? first.start : second.start;
            const size_t length =
                first.length +
                second.length; /* disjoint ranges within one bounded region */
            end_value(c, inputs[0]);
            end_value(c, inputs[1]);
            return new_result(&raw, result,
                              (NLSemanticValueView){
                                  .type = storage_type,
                                  .occupancy = {first.region, start, length}});
        }
        if (op->data.slot_target == 0 || op->data.slot_target > c->type_count) {
            return checked_host(&raw, NL_CHECK_INTERNAL_ERROR);
        }
        const NLSemanticTypeView target =
            c->types[op->data.slot_target - 1].view;
        if (!target.layout_known) {
            return reject(&raw, NL_CHECK_ANALYSIS_PRECISION_LIMIT, op->span,
                          "P4-LAYOUT-PRECISION",
                          "into_slot requires compiler-provided concrete "
                          "size/alignment facts");
        }
        if (target.size != first.length) {
            return reject(&raw, NL_CHECK_SEMANTIC_ERROR, op->operands[0].span,
                          "P4-SLOT-SIZE",
                          "Storage length must exactly equal sizeof(T); no "
                          "implicit tail split");
        }
        const NLSemanticBackingView r = c->regions[first.region - 1].view;
        if (r.address_known) {
            if ((r.address + first.start) % target.alignment != 0) {
                return reject(&raw, NL_CHECK_SEMANTIC_ERROR,
                              op->operands[0].span, "P4-SLOT-ALIGNMENT",
                              "Storage start does not satisfy alignof(T)");
            }
        } else if (r.alignment % target.alignment != 0 ||
                   first.start % target.alignment != 0) {
            return reject(&raw, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                          op->operands[0].span, "P4-ALIGNMENT-PRECISION",
                          "known backing alignment cannot prove this typed "
                          "start alignment");
        }
        NLTypeId type;
        NLPlaceId place;
        if ((status = nl_sem_compound(c, NL_TYPE_SLOT, op->data.slot_target,
                                      NL_ACCESS_READ, false, &type)) !=
                NL_CHECK_OK ||
            (status = nl_sem_new_place(c, op->data.slot_target, 0, true, 0,
                                       &place)) != NL_CHECK_OK) {
            return checked_host(&raw, status);
        }
        end_value(c, inputs[0]);
        return new_result(&raw, result,
                          (NLSemanticValueView){.type = type,
                                                .slot_place = place,
                                                .occupancy = first});
    }
    NLBackingRange first;
    if ((status = observed_claim(&raw, inputs[0], 0, &first)) != NL_CHECK_OK) {
        return status;
    }
    if (op->kind == NL_RAW_STORAGE_LEN || op->kind == NL_RAW_STORAGE_ADDR) {
        const bool address = op->kind == NL_RAW_STORAGE_ADDR;
        const NLSemanticBackingView r = c->regions[first.region - 1].view;
        result->type =
            nl_semantic_core_type(c, address ? NL_TYPE_ADDR : NL_TYPE_USIZE);
        result->has_scalar_result = true;
        result->scalar_result = (NLScalarValue){
            result->type, !address || r.address_known,
            address ? (r.address_known ? r.address + first.start : 0)
                    : first.length};
        return NL_CHECK_OK;
    }
    if (op->kind == NL_RAW_COPY_BYTES) {
        NLBackingRange source;
        size_t dst_offset, src_offset, count;
        if ((status = observed_claim(&raw, inputs[1], 1, &source)) !=
                NL_CHECK_OK ||
            (status = quantity(&raw, op->data.copy.dst_offset, &dst_offset)) !=
                NL_CHECK_OK ||
            (status = quantity(&raw, op->data.copy.src_offset, &src_offset)) !=
                NL_CHECK_OK ||
            (status = quantity(&raw, op->data.copy.count, &count)) !=
                NL_CHECK_OK ||
            (status = bounds(&raw, first, dst_offset, count, 0)) !=
                NL_CHECK_OK ||
            (status = bounds(&raw, source, src_offset, count, 1)) !=
                NL_CHECK_OK ||
            (status = access_check(&raw, first, true, 0)) != NL_CHECK_OK ||
            (status = access_check(&raw, source, false, 1)) != NL_CHECK_OK) {
            return status;
        }
        result->raw_offsets[0] = dst_offset;
        result->raw_offsets[1] = src_offset;
        result->raw_count = count;
        if (count == 0) {
            return NL_CHECK_OK;
        }
        const NLRawRegionEntry *const r = &c->regions[source.region - 1];
        NLRawInterval *const snapshot = malloc(r->count * sizeof(*snapshot));
        if (snapshot == NULL) {
            return checked_host(&raw, NL_CHECK_OUT_OF_MEMORY);
        }
        const size_t start = source.start + src_offset, end = start + count;
        size_t length = 0;
        for (size_t i = 0; i < r->count; ++i) {
            const NLRawInterval old = r->intervals[i];
            const size_t lo = old.start > start ? old.start : start;
            const size_t hi =
                old.start + old.length < end ? old.start + old.length : end;
            if (lo < hi) {
                snapshot[length++] =
                    (NLRawInterval){lo - start, hi - lo, old.state};
            }
        }
        status = replace_range(
            c, (NLBackingRange){first.region, first.start + dst_offset, count},
            snapshot, length);
        free(snapshot);
        return checked_host(&raw, status);
    }
    const bool write = op->kind == NL_RAW_STORAGE_WRITE_BYTE;
    size_t offset;
    if ((status = quantity(&raw,
                           write ? op->data.write.offset : op->data.byte_offset,
                           &offset)) != NL_CHECK_OK ||
        (status = bounds(&raw, first, offset, 1, 0)) != NL_CHECK_OK ||
        (status = access_check(&raw, first, write, 0)) != NL_CHECK_OK) {
        return status;
    }
    result->raw_offsets[0] = offset;
    result->raw_count = 1;
    if (write) {
        if ((status = scalar_check(&raw, op->data.write.value, NL_TYPE_BYTE)) !=
            NL_CHECK_OK) {
            return status;
        }
        const NLRawInterval state = {
            0,
            1,
            {NL_RAW_DEFINED, op->data.write.value.known,
             op->data.write.value.known
                 ? (unsigned char)op->data.write.value.value
                 : 0}};
        return checked_host(
            &raw,
            replace_range(
                c, (NLBackingRange){first.region, first.start + offset, 1},
                &state, 1));
    }
    NLRawRepView representation;
    if (!nl_semantic_raw_rep_view(c, first.region, first.start + offset,
                                  &representation)) {
        return checked_host(&raw, NL_CHECK_INTERNAL_ERROR);
    }
    if (representation.validity != NL_RAW_DEFINED) {
        return reject(&raw, NL_CHECK_SEMANTIC_ERROR, op->operands[0].span,
                      "P4-UNSPECIFIED-READ",
                      "Unspecified representation cannot be interpreted as a "
                      "semantic byte");
    }
    result->type = nl_semantic_core_type(c, NL_TYPE_BYTE);
    result->has_scalar_result = true;
    result->scalar_result =
        (NLScalarValue){result->type, representation.value_known,
                        representation.value_known ? representation.value : 0};
    return NL_CHECK_OK;
}
