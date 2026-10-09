/* Track R #230: independent native observer, source-preserving linker wrappers.
   Does not mutate app storage or repair an app semantic transition.
   Duplicate/invalid frees are reported and suppressed BEFORE platform free,
   solely to avoid invoking double-free UB in destructive tests. */
#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void *__real_malloc(size_t n);
extern void *__real_calloc(size_t n, size_t s);
extern void *__real_realloc(void *p, size_t n);
extern void __real_free(void *p);

struct r230_record { void *p; size_t n; int freed; unsigned id; };
static struct r230_record records[4096];
static unsigned count, attempts, frees, duplicate_frees, foreign_frees;
static unsigned fail_at, events;
static int configured;

__attribute__((no_instrument_function))
static void config(void) {
    if (configured) return;
    configured = 1;
    const char *p = getenv("R230_FAIL_ALLOCATION_N");
    if (p && *p) fail_at = (unsigned)strtoul(p, NULL, 10);
    fprintf(stderr, "R230 OBSERVER start fail_at=%u\n", fail_at);
}
__attribute__((no_instrument_function))
static void mark_allocation(void *p, size_t n, const char *kind) {
    if (p && count < 4096) {
        records[count] = (struct r230_record){p, n, 0, count+1};
        count++;
    }
    fprintf(stderr, "R230 ALLOC ordinal=%u kind=%s ptr=%p bytes=%zu %s\n",
            attempts, kind, p, n, p?"ok":"NULL");
}
__attribute__((no_instrument_function))
static int should_fail(const char *kind, size_t size) {
    config();
    attempts++;
    if (attempts != fail_at) return 0;
    fprintf(stderr, "R230 INJECT_FAIL ordinal=%u kind=%s bytes=%zu\n",
            attempts, kind, size);
    mark_allocation(NULL, size, kind);
    return 1;
}
__attribute__((no_instrument_function))
void *__wrap_malloc(size_t n) {
    if (should_fail("malloc", n)) return NULL;
    void *p=__real_malloc(n);
    mark_allocation(p,n,"malloc");
    return p;
}
__attribute__((no_instrument_function))
void *__wrap_calloc(size_t n, size_t s) {
    if (should_fail("calloc", n*s)) return NULL;
    void *p=__real_calloc(n,s);
    mark_allocation(p,n*s,"calloc");
    return p;
}
__attribute__((no_instrument_function))
void *__wrap_realloc(void *p, size_t n) {
    if (should_fail("realloc", n)) return NULL;
    void *out=__real_realloc(p,n);
    if (p && out) {
        for (unsigned i=count; i; --i)
            if (records[i-1].p==p && !records[i-1].freed) {
                records[i-1].freed=1; break;
            }
    }
    mark_allocation(out,n,"realloc");
    return out;
}
__attribute__((no_instrument_function))
void __wrap_free(void *p) {
    config();
    if (!p) {
        fprintf(stderr,"R230 FREE NULL\n");
        __real_free(p);
        return;
    }
    for (unsigned i=count;i;--i) {
        struct r230_record *r=&records[i-1];
        if (r->p!=p) continue;
        if (r->freed) {
            duplicate_frees++;
            fprintf(stderr,"R230 DOUBLE_FREE_SUPPRESSED ptr=%p id=%u\n",p,r->id);
            return;
        }
        r->freed=1;
        frees++;
        fprintf(stderr,"R230 FREE ptr=%p id=%u ordinal=%u bytes=%zu data=",
                p,r->id,frees,r->n);
        const unsigned char *b=(const unsigned char *)p;
        for(size_t k=0;k<r->n && k<32;++k) fprintf(stderr,"%02x",b[k]);
        fputc('\n',stderr);
        __real_free(p);
        return;
    }
    foreign_frees++;
    fprintf(stderr,"R230 UNTRACKED_FREE_SUPPRESSED ptr=%p\n",p);
}
__attribute__((no_instrument_function))
void __cyg_profile_func_enter(void *fn,void *caller) {
    if (++events>1500) return;
    Dl_info info={0};
    dladdr(fn,&info);
    fprintf(stderr,"R230 ENTER fn=%p name=%s caller=%p\n",
            fn,info.dli_sname?info.dli_sname:"(local)",caller);
}
__attribute__((no_instrument_function))
void __cyg_profile_func_exit(void *fn,void *caller) {
    if (++events>1500) return;
    Dl_info info={0};
    dladdr(fn,&info);
    fprintf(stderr,"R230 EXIT fn=%p name=%s caller=%p\n",
            fn,info.dli_sname?info.dli_sname:"(local)",caller);
}
__attribute__((destructor,no_instrument_function))
static void report(void) {
    fprintf(stderr,"R230 SUMMARY attempts=%u live_allocs=%u free_calls=%u "
            "duplicates=%u invalid=%u fail_at=%u\n",
            attempts,count,frees,duplicate_frees,foreign_frees,fail_at);
    for (unsigned i=0;i<count;++i) if (!records[i].freed)
        fprintf(stderr,"R230 LEAK ptr=%p id=%u bytes=%zu\n",
                records[i].p, records[i].id, records[i].n);
}
