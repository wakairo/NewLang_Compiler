#include "ring4_c.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    OwnedFrameC a={0},b={0},c={0},d={0},e={0},f={0},out={0};
    assert(frame_make(&a,'A') && frame_make(&b,'B') && frame_make(&c,'C'));
    assert(frame_make(&d,'D') && frame_make(&e,'E') && frame_make(&f,'F'));
    uint8_t *original[] = {a.payload,b.payload,c.payload,d.payload,e.payload,f.payload};
    for(size_t i=0;i<6;++i) for(size_t j=i+1;j<6;++j) assert(original[i]!=original[j]);
    size_t released=0;
    Ring4 *ring=ring_create(); assert(ring);
    assert(ring_pop(ring,&out)==RING_EMPTY);
    assert(ring_push(ring,&a)==RING_OK && a.payload==NULL);
    assert(ring_push(ring,&b)==RING_OK && b.payload==NULL);
    assert(ring_push(ring,&c)==RING_OK && c.payload==NULL);
    assert(ring_push(ring,&d)==RING_OK && d.payload==NULL);
    assert(ring_push(ring,&e)==RING_FULL && e.payload==original[4]);
    uint8_t id=0; assert(ring_front_id(ring,&id)==RING_OK && id=='A');
    for(size_t i=0;i<2;++i) {
        assert(ring_pop(ring,&out)==RING_OK);
        assert(out.payload==original[i] && out.id==(uint8_t)('A'+i));
        frame_release(&out,&released);
    }
    assert(released==2);
    assert(ring_push(ring,&e)==RING_OK && e.payload==NULL);
    assert(ring_push(ring,&f)==RING_OK && f.payload==NULL);
    for(size_t i=2;i<6;++i) {
        assert(ring_pop(ring,&out)==RING_OK);
        assert(out.payload==original[i] && out.id==(uint8_t)('A'+i));
        frame_release(&out,&released);
    }
    assert(ring_pop(ring,&out)==RING_EMPTY);
    assert(ring_finish(&ring)==RING_OK && ring==NULL);
    assert(released==6);
    puts("C17_RING4_SCENARIO_OK released=6 original_heap_addresses_unchanged");
    return 0;
}
