#ifndef V536_RING4_C_H
#define V536_RING4_C_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct Ring4 Ring4; // Private tagged slots and metadata, unlike a naked C struct.
typedef struct { uint8_t *payload; uint8_t id; } OwnedFrameC;
typedef enum { RING_OK, RING_FULL, RING_EMPTY, RING_INVALID, RING_OOM } RingResult;
bool frame_make(OwnedFrameC *out, uint8_t id);
void frame_release(OwnedFrameC *owner, size_t *released);
Ring4 *ring_create(void);
RingResult ring_push(Ring4 *r, OwnedFrameC *input);
RingResult ring_pop(Ring4 *r, OwnedFrameC *empty_output);
RingResult ring_front_id(const Ring4 *r, uint8_t *out_id);
RingResult ring_finish(Ring4 **owned_ring);
#endif
