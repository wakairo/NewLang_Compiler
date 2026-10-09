/* Issue #264: reference driver only; pinned upstream stb_ds.h remains untouched. */
#include "observer.h"
#include "hooks.h"
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef STB264_HOOK
#define STBDS_REALLOC(context,p,n) stb264_realloc((p),(n))
#define STBDS_FREE(context,p) stb264_free((p))
#endif
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

static void require(int condition, const char *what) {
    if (!condition) { fprintf(stderr,"DRIVER_FAIL|%s\n",what); exit(91); }
}
static uint32_t checksum(uint32_t id) { return 17u*id+3u; }
static void checkpoint(const char *label, struct Rec *a,
                       const uint32_t *expected, size_t n, size_t cap) {
    size_t real_n=(size_t)stbds_arrlenu(a), real_c=(size_t)stbds_arrcap(a);
    const void *base=a ? (const void *)stbds_header(a) : NULL;
    require(real_n==n && real_c==cap,"LEN_CAP_ORACLE");
    if(a) {
        uintptr_t addr=(uintptr_t)a, b=(uintptr_t)base;
        require(addr==b+sizeof(stbds_array_header),"HEADER_BEFORE_ITEM");
        require(addr % _Alignof(struct Rec)==0,"ITEM_ALIGNMENT");
        require(stbds_header(a)->hash_table==NULL,"ARRAY_HASH_TABLE_NOT_NULL");
        require(stbds_header(a)->temp==0,"ARRAY_TEMP_NOT_ZERO");
    }
    for (size_t i=0;i<n;++i)
        require(a[i].id==expected[i] && a[i].checksum==checksum(expected[i]),
                "FULL_IDS_AND_CHECKSUMS");
#ifdef STB264_OBSERVE
    require(stb264_check(label,a,base,sizeof(stbds_array_header),real_n,real_c,
                         expected,n,cap),"INDEPENDENT_OBSERVER_REJECTED");
#endif
    printf("CHECKPOINT|%s|base=0x%" PRIxPTR "|item=0x%" PRIxPTR
           "|len=%zu|cap=%zu|header_bytes=%zu|elem_bytes=%zu\n",
           label,(uintptr_t)base,(uintptr_t)a,real_n,real_c,
           sizeof(stbds_array_header),sizeof(struct Rec));
}
int main(int argc, char **argv) {
    int probes= argc>1 && strcmp(argv[1],"probes")==0;
#if !(defined(STB264_HOOK) && defined(STB264_FORCE) && defined(STB264_OBSERVE))
    if (probes) { fprintf(stderr,"PROBE_MODE_ONLY_FOR_FORCE_OBS\n"); return 2; }
#endif
    struct Rec *a=NULL;
    uint32_t ids[12]={0};
    size_t actual_grow_events=0;
    printf("BUILD|mode=%s|observe=%d|gcc_clang_detect=%s|H=%zu|E=%zu|align_H=%zu|align_E=%zu\n",
#ifdef STB264_FORCE
           "FORCE",
#elif defined(STB264_HOOK)
           "OBSERVED_DEFAULT",
#else
           "DEFAULT",
#endif
#ifdef STB264_OBSERVE
           1,
#else
           0,
#endif
#ifdef __clang__
           "clang",
#else
           "gcc",
#endif
           sizeof(stbds_array_header),sizeof(struct Rec),
           _Alignof(stbds_array_header),_Alignof(struct Rec));
    checkpoint("empty",a,ids,0,0);
    for(uint32_t i=0;i<12;++i) {
        size_t old_c=(size_t)stbds_arrcap(a);
        uintptr_t old_base=a ? (uintptr_t)stbds_header(a) : 0u;
        struct Rec r={i,checksum(i)};
        stbds_arrput(a,r); /* ORIGINAL pinned upstream macro */
        ids[i]=i;
        size_t new_c=(size_t)stbds_arrcap(a);
        if(new_c!=old_c) {
            ++actual_grow_events;
            printf("SOURCE_GROW|at_id=%" PRIu32 "|old_cap=%zu|new_cap=%zu"
                   "|old_base_snapshot=0x%" PRIxPTR "|current_base=0x%" PRIxPTR
                   "|request_bytes=%zu\n",i,old_c,new_c,old_base,
                   (uintptr_t)stbds_header(a),
                   sizeof(stbds_array_header)+sizeof(struct Rec)*new_c);
        }
        size_t expected_cap= i<4 ? 4u : (i<8 ? 8u : 16u);
        char stage[32]; (void)snprintf(stage,sizeof(stage),"append_%u",(unsigned)i);
        checkpoint(stage,a,ids,(size_t)i+1u,expected_cap);
    }
    require(actual_grow_events==3,"THREE_SOURCE_GROWTH_EVENTS");
    require(a[7].id==7 && a[7].checksum==122,"READ_INDEX_SEVEN");
    stbds_arrdeln(a,4,3); /* ORIGINAL upstream memmove ordered deletion */
    static const uint32_t after_del[]={0,1,2,3,7,8,9,10,11};
    checkpoint("after_del",a,after_del,9,16);

#if defined(STB264_HOOK) && defined(STB264_FORCE) && defined(STB264_OBSERVE)
    if(probes) {
        /* All mutations/probes run after a GOOD reference observation and
           restore live data/metadata before the original pop/free. */
        require(stb264_probe_bad_base((const void*)a),"PROBE_BAD_ELEMENT_FREE");
        require(stb264_probe_bad_base((const char*)stbds_header(a)+1),
                "PROBE_BAD_HEADER_OFFSET");
        require(stb264_probe_missing_release(),"PROBE_MISSING_FINAL_FREE");
        require(stb264_probe_bad_growth_count(1),"PROBE_GROWTH_COUNT");

        size_t old_len=stbds_header(a)->length;
        stbds_header(a)->length=old_len+1; /* within cap; observer sees mismatch first */
        require(!stb264_check("mutated_length",a,stbds_header(a),
                             sizeof(stbds_array_header),stbds_header(a)->length,
                             stbds_header(a)->capacity,after_del,9,16),
                "PROBE_MISSING_LENGTH_DECREMENT_NOT_DETECTED");
        printf("PROBE_DETECTED|WRONG_DELETION_LENGTH\n");
        stbds_header(a)->length=old_len;

        struct Rec saved[9];
        memcpy(saved,a,sizeof(saved));
        /* Safe wrong-range memmove MUTATION of driver-side current data only.
           Neither original stb_ds.h nor its successful reference call changes. */
        memmove(&a[4],&a[6],2*sizeof(struct Rec));
        require(!stb264_check("mutated_shift",a,stbds_header(a),
                             sizeof(stbds_array_header),stbds_header(a)->length,
                             stbds_header(a)->capacity,after_del,9,16),
                "PROBE_WRONG_MEMMOVE_NOT_DETECTED");
        printf("PROBE_DETECTED|WRONG_MEMMOVE_RANGE\n");
        memcpy(a,saved,sizeof(saved)); /* explicit reset of safe observer mutation */
        checkpoint("restored_after_probes",a,after_del,9,16);
    }
#endif
    struct Rec popped=stbds_arrpop(a); /* ORIGINAL upstream macro */
    require(popped.id==11 && popped.checksum==190,"POPPED_VALUE");
    static const uint32_t after_pop[]={0,1,2,3,7,8,9,10};
    checkpoint("after_pop",a,after_pop,8,16);
    printf("POPPED|id=%" PRIu32 "|checksum=%" PRIu32 "\n",
           popped.id,popped.checksum);

    unsigned final_id=0;
#ifdef STB264_HOOK
    final_id=stb264_final_free_id(); /* 0 until guest actual free */
#endif
    (void)final_id;
    stbds_arrfree(a); /* ORIGINAL upstream macro; sends HEADER to STBDS_FREE */
    require(a==NULL && stbds_arrlen(a)==0,"FINAL_NULL_AND_ZERO_LEN");
    checkpoint("after_free",a,ids,0,0);
#ifdef STB264_HOOK
    stb264_assert_final();
#if defined(STB264_FORCE) && defined(STB264_OBSERVE)
    if(probes) {
        unsigned released=stb264_final_free_id();
        require(released!=0 && stb264_probe_double_by_id(released),
                "PROBE_DOUBLE_FREE_NOT_DETECTED");
    }
#endif
#endif
    printf("RESULT|PASS|grow_events=%zu|original_stb_ds_h=1|probes=%d\n",
           actual_grow_events,probes);
    return 0;
}
