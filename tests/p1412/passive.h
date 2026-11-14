/* Test-only passive callbacks: never write a guest node or repair a value. */
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#define NEED(x) do { if (!(x)) { \
    fprintf(stderr,"P1412 observer line %d: %s\n",__LINE__,#x); exit(77); \
} } while (0)
size_t p1412_requests(void);
uintptr_t p1412_address(size_t);
static uintptr_t addresses[2];
static const nl_node *nodes[2];
static size_t domains[2], phases[2], trials, arms, ends, frees;
static size_t roots, fields, changes, copies, choices, active_scope;
static size_t index_of(const void *p)
{
    for (size_t i=0;i<2;++i)
        if (p && addresses[i] == (uintptr_t)p) return i;
    NEED(false); return 0;
}
static void observe_trial(void *p,size_t n,size_t a)
{
    NEED(trials<2 && n==24 && a==8 && p1412_requests()==trials+1);
    NEED(p1412_address(trials)==(uintptr_t)p);
    addresses[trials]=(uintptr_t)p;
    phases[trials]=p?1u:99u;
    if (p && trials) NEED(addresses[0] != addresses[1] && phases[0]==4);
    ++trials;
}
static void observe_arm(size_t variant)
{
    NEED(trials && arms+1==trials && variant==(addresses[trials-1]?2u:1u));
    ++arms;
}
static void observe_slot(const void *p,size_t n)
{
    size_t i=index_of(p); NEED(phases[i]==1 && n==24);phases[i]=2;
}
static void observe_domain(size_t d)
{
    NEED(trials && d && phases[trials-1]==2);
    domains[trials-1]=d;phases[trials-1]=3;
    if (trials==2) NEED(domains[0]!=domains[1]);
}
static void observe_init(const nl_node *p,size_t d)
{
    size_t i=index_of(p);
    NEED(phases[i]==3 && domains[i]==d && p->f0.tag==0 && !p->f0.ptr &&
         p->f1==(i?99:41));
    nodes[i]=p;phases[i]=4;
}
static void observe_root(const nl_node *p,size_t d,unsigned write,
                          size_t r,size_t inc,size_t scope)
{
    size_t i=index_of(p);
    NEED(roots<3 && phases[i]==4 && domains[i]==d && r && inc && scope &&
         !active_scope && i==(roots?1u:0u) && write==(roots<2?1u:0u));
    active_scope=scope;++roots;
}
static void observe_field(const nl_node *p,const nl_node_option *f,
                           unsigned write,size_t nominal,size_t index,
                           size_t child,size_t inc,size_t scope)
{
    size_t i=index_of(p);
    NEED(fields<2 && phases[i]==4 && f==&p->f0 && !index && nominal && child &&
         inc && active_scope==scope && write==(fields?0u:1u) &&
         i==(fields?1u:0u));++fields;
}
static void observe_change(const nl_node_option *field,
                            const nl_node_option *old,size_t ordinal)
{
    NEED(!changes && phases[0]==4 && phases[1]==4 && ordinal==1 &&
         field==&nodes[0]->f0 && old->tag==0 && !old->ptr &&
         field->tag==1 && field->ptr==nodes[1]);++changes;
}
static void observe_choice(size_t branch,const nl_node *p)
{
    NEED(!choices && changes==1 && branch==TEST_BRANCH && !active_scope &&
         phases[0]==4 && phases[1]==4 && p==nodes[1]);++choices;
}
static void observe_copy(const nl_node_option *field,
                          const nl_node_option *copy)
{
    NEED(!copies && choices==1 && phases[1]==4 && field==&nodes[1]->f0 &&
         field->tag==0 && !field->ptr && copy->tag==0 && !copy->ptr);++copies;
}
static void observe_scope(size_t scope)
{
    NEED(active_scope && active_scope==scope);active_scope=0;
}
static void observe_end(const nl_node *p,size_t d)
{
    size_t i=index_of(p);
    NEED(phases[i]==4 && domains[i]==d && !active_scope &&
         i==(TEST_WORLD==2 && !ends?1u:0u));
    if (TEST_WORLD==2) NEED(copies==1 && choices==1);
    phases[i]=5;++ends;
}
static void observe_raw(const void *p,size_t n)
{
    size_t i=index_of(p);NEED(phases[i]==5 && n==24);phases[i]=6;
}
static void observe_final(size_t d)
{
    size_t i=domains[0]==d?0u:1u;NEED(d && domains[i]==d && phases[i]==6);
    phases[i]=7;
}
static void observe_release(const void *p,const void *raw,size_t n)
{
    size_t i=index_of(p);NEED(p==raw && n==24 && phases[i]==7);phases[i]=8;
}
void p1412_before_free(void *p)
{
    size_t i=index_of(p);NEED(phases[i]==8 && frees+1<=(size_t)TEST_WORLD);phases[i]=9;
    ++frees;
}
static void observe_finish(void)
{
    NEED(trials==(TEST_WORLD?2u:1u) && arms==trials && frees==TEST_WORLD &&
         ends==TEST_WORLD && !active_scope);
    for(size_t i=0;i!=(size_t)TEST_WORLD;++i) NEED(phases[i]==9);
    NEED(roots==(TEST_WORLD==2?3u:0u) && fields==(TEST_WORLD==2?2u:0u) &&
         changes==(TEST_WORLD==2?1u:0u) && copies==(TEST_WORLD==2?1u:0u) &&
         choices==(TEST_WORLD==2?1u:0u));
    printf("H%u cleanup=%zu actual-selected-B=%zu initial-None-copy=%zu PASS\n",
           (unsigned)TEST_WORLD,frees,choices,copies);
}
#undef NL_NODE_ARM
#define NL_NODE_ARM(a,p) observe_choice(a,p)
#define NL_HEAP_TRIAL(p,n,a) observe_trial(p,n,a)
#define NL_HEAP_ARM(v) observe_arm(v)
#define NL_HEAP_SLOT(p,n) observe_slot(p,n)
#define NL_HEAP_DOMAIN(d) observe_domain(d)
#define NL_HEAP_INITIALIZE(p,d) observe_init(p,d)
#define NL_HEAP_ROOT(p,d,w,r,i,s) observe_root(p,d,w,r,i,s)
#define NL_HEAP_FIELD(p,f,w,n,i,c,j,s) observe_field(p,f,w,n,i,c,j,s)
#define NL_HEAP_CHANGE(f,o,i) observe_change(f,o,i)
#define NL_HEAP_COPY(f,c) observe_copy(f,c)
#define NL_HEAP_SCOPE_END(s) observe_scope(s)
#define NL_HEAP_END(p,d) observe_end(p,d)
#define NL_HEAP_RAW(p,n) observe_raw(p,n)
#define NL_HEAP_FINALIZE(d) observe_final(d)
#define NL_HEAP_RELEASE(p,r,n) observe_release(p,r,n)
#define NL_HEAP_FINISH() observe_finish()
