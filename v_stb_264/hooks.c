#include "hooks.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

struct Block { uintptr_t addr; size_t size; unsigned id; int live; };
static struct Block blocks[24];
static size_t used, calls, nonnull_grows, moves, physical_frees, bad_probes;
static unsigned next_id=1, final_free_id;
static void die(const char *reason) {
    fprintf(stderr,"HOOK_FAIL|%s\n",reason); exit(90);
}
static struct Block *lookup(uintptr_t address) {
    for (size_t i=0; i<used; ++i)
        if (blocks[i].live && blocks[i].addr==address) return &blocks[i];
    return NULL;
}
static struct Block *get_or_die(const void *p) {
    struct Block *b=lookup((uintptr_t)p);
    if (!b) die("MISSING_CURRENT_HEADER_BASE");
    return b;
}
static size_t live_count(void) {
    size_t n=0; for(size_t i=0;i<used;++i) if(blocks[i].live) ++n; return n;
}
static unsigned enroll(uintptr_t address, size_t size) {
    if (used>=sizeof(blocks)/sizeof(blocks[0])) die("OBSERVER_TABLE_FULL");
    if (lookup(address)) die("DUPLICATE_LIVE_BASE");
    unsigned id=next_id++;
    blocks[used++]=(struct Block){address,size,id,1};
    return id;
}
void *stb264_realloc(void *p, size_t bytes) {
    if (bytes==0) die("ZERO_BYTE_REALLOC_NOT_PRE_REGISTERED");
    ++calls;
    uintptr_t old_addr=(uintptr_t)p;   /* snapshot while p is still valid */
    size_t old_bytes=0;
    unsigned old_id=0;
    struct Block *old=NULL;
    if (p) {
        old=get_or_die(p); ++nonnull_grows;
        old_bytes=old->size; old_id=old->id;
    }
#ifdef STB264_FORCE
    /* The freshly allocated block coexists with old: physical move is forced. */
    void *new_p=malloc(bytes);
    if(!new_p) die("UNEXPECTED_ALLOC_FAILURE_OUTSIDE_C_REFERENCE");
    if(old) {
        memcpy(new_p,p,old_bytes < bytes ? old_bytes : bytes);
        if (new_p==p) die("FORCED_MOVE_MUST_BE_DISTINCT");
        ++moves;
        old->live=0;
        free(p);
        ++physical_frees;
    }
#else
    /* Observed-default mode still delegates to genuine libc realloc. */
    void *new_p=realloc(p,bytes);
    if(!new_p) die("UNEXPECTED_REALLOC_FAILURE_OUTSIDE_C_REFERENCE");
    if(old) {
        if ((uintptr_t)new_p!=old_addr) ++moves;
        old->live=0;
    }
#endif
    unsigned id=enroll((uintptr_t)new_p,bytes);
    printf("ALLOC_EVENT|call=%zu|old_id=%u|new_id=%u|old_addr=0x%" PRIxPTR
           "|new_addr=0x%" PRIxPTR "|old_bytes=%zu|new_bytes=%zu|moved=%d\n",
           calls,old_id,id,old_addr,(uintptr_t)new_p,old_bytes,bytes,
           old!=NULL && old_addr!=(uintptr_t)new_p);
    return new_p;
}
void stb264_free(void *p) {
    if(!p) die("NULL_FREE_NOT_PART_OF_REFERENCE");
    struct Block *b=get_or_die(p);
    unsigned id=b->id;
    uintptr_t addr=b->addr;
    size_t n=b->size;
    b->live=0;
    ++physical_frees;
    final_free_id=id;
    free(p);
    printf("FREE_EVENT|id=%u|base_addr=0x%" PRIxPTR "|bytes=%zu|live=%zu\n",
           id,addr,n,live_count());
}
int stb264_probe_bad_base(const void *p) {
    if(lookup((uintptr_t)p)) return 0;
    ++bad_probes;
    printf("PROBE_DETECTED|BAD_FREE_BASE|request=0x%" PRIxPTR
           "|before_libc_free=1\n",(uintptr_t)p);
    return 1;
}
int stb264_probe_double_by_id(unsigned id) {
    for(size_t i=0;i<used;++i) if(blocks[i].id==id) {
        if(blocks[i].live) return 0;
        ++bad_probes;
        printf("PROBE_DETECTED|DOUBLE_FREE_ID|id=%u|before_libc_free=1\n",id);
        return 1;
    }
    die("DUPLICATE_PROBE_ID_NOT_FOUND");return 0;
}
int stb264_probe_missing_release(void) {
    if (live_count()==0) return 0;
    ++bad_probes;
    printf("PROBE_DETECTED|MISSING_FINAL_RELEASE|live=%zu\n",live_count());
    return 1;
}
int stb264_probe_bad_growth_count(size_t bogus) {
    if (bogus==nonnull_grows) return 0;
    ++bad_probes;
    printf("PROBE_DETECTED|INCORRECT_GROWTH_COUNT|claimed=%zu|actual=%zu\n",
           bogus,nonnull_grows);
    return 1;
}
unsigned stb264_final_free_id(void) {return final_free_id;}
void stb264_assert_final(void) {
    if (calls!=3 || nonnull_grows!=2 || live_count()!=0) die("BAD_FINAL_ALLOCATOR_COUNTS");
#ifdef STB264_FORCE
    if (moves!=2 || physical_frees!=3) die("FORCE_NOT_TWO_PHYSICAL_MOVES");
#else
    if (physical_frees!=1) die("BAD_FINAL_LIBC_RELEASE_COUNT");
#endif
    printf("HOOK_FINAL|calls=%zu|nonnull=%zu|physical_moves=%zu"
           "|hook_physical_frees=%zu|live=%zu|detected_probes=%zu\n",
           calls,nonnull_grows,moves,physical_frees,live_count(),bad_probes);
}
