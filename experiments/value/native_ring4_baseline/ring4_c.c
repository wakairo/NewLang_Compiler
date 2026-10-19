#include "ring4_c.h"
#include <stdlib.h>
#include <string.h>
#define CAP 4u
// Exactly four fixed tagged envelopes; each live value holds the ORIGINAL payload heap address.
typedef struct { bool occupied; OwnedFrameC owner; } Slot;
struct Ring4 { Slot slots[CAP]; size_t head, len; };

bool frame_make(OwnedFrameC *out, uint8_t id) {
    if (!out || out->payload) return false;
    uint8_t *bytes = malloc(16);
    if (!bytes) return false;
    memset(bytes, id, 16);
    out->payload = bytes; out->id = id;
    return true;
}
void frame_release(OwnedFrameC *owner, size_t *released) {
    if (!owner || !owner->payload) return;
    free(owner->payload);
    owner->payload = NULL;
    owner->id = 0;
    if (released) ++*released;
}
Ring4 *ring_create(void) { return calloc(1, sizeof(Ring4)); }
RingResult ring_push(Ring4 *r, OwnedFrameC *input) {
    if (!r || !input || !input->payload) return RING_INVALID;
    if (r->len == CAP) return RING_FULL; // input remains EXACTLY in caller ownership
    if (r->head >= CAP || r->len > CAP) return RING_INVALID;
    const size_t tail = (r->head + r->len) % CAP;
    if (r->slots[tail].occupied) return RING_INVALID;
    r->slots[tail].owner = *input; // C *convention*, NOT a compiler-enforced move
    input->payload = NULL;
    input->id = 0;
    r->slots[tail].occupied = true;
    ++r->len;
    return RING_OK;
}
RingResult ring_pop(Ring4 *r, OwnedFrameC *empty_output) {
    if (!r || !empty_output || empty_output->payload) return RING_INVALID;
    if (r->len == 0) return RING_EMPTY;
    if (r->head >= CAP || r->len > CAP || !r->slots[r->head].occupied) return RING_INVALID;
    Slot *slot = &r->slots[r->head];
    *empty_output = slot->owner; // exact existing owner packet returned
    slot->owner.payload = NULL;
    slot->owner.id = 0;
    slot->occupied = false;
    r->head = (r->head + 1) % CAP;
    --r->len;
    return RING_OK;
}
RingResult ring_front_id(const Ring4 *r, uint8_t *out_id) {
    if (!r || !out_id) return RING_INVALID;
    if (r->len == 0) return RING_EMPTY;
    if (r->head >= CAP || !r->slots[r->head].occupied) return RING_INVALID;
    *out_id = r->slots[r->head].owner.id;
    return RING_OK;
}
RingResult ring_finish(Ring4 **owned_ring) {
    if (!owned_ring || !*owned_ring) return RING_INVALID;
    Ring4 *r = *owned_ring;
    if (r->len) return RING_FULL; // cannot silently drop live caller-owned frames
    for (size_t i=0; i<CAP; ++i) if (r->slots[i].occupied) return RING_INVALID;
    free(r); *owned_ring = NULL;
    return RING_OK;
}
