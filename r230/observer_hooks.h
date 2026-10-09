/* Independent Track R #230 read-only inspection hooks.
 * Written from actual frozen generated-C hook invocation sites, not P tests.
 * Accesses physical C values; never rewrites application heap/stack.
 */
#ifndef R230_OBSERVER_HOOKS_H
#define R230_OBSERVER_HOOKS_H
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
static unsigned r230_hook_errors;
static int r230_policy;
static int r230_has_original;
static nl_live_tail r230_original;

static void r230_check(int pred, const char *where, const char *reason) {
    if (!pred) {
        ++r230_hook_errors;
        fprintf(stderr, "R230 HOOK_FAIL at=%s reason=%s\n", where, reason);
    }
}
static int r230_same(nl_live_tail p, nl_live_tail q) {
    return p.ptr==q.ptr && p.allocation.handle==q.allocation.handle
           && p.domain.token==q.domain.token;
}
static void r230_link_change(nl_node_option *after,
                             const nl_node_option *before,int kind) {
    fprintf(stderr, "R230 LINK change=%d old_tag=%u old_ptr=%p now_tag=%u now_ptr=%p\n",
            kind, before->tag, (const void *)before->ptr,
            after->tag,(const void *)after->ptr);
    if(kind==2) {
        r230_check(before->tag==1,"detach","old link not Some");
        r230_check(after->tag==0 && after->ptr==NULL,
                   "detach","head still links to tail");
    }
}
static void r230_live_send(const nl_node *p,void *a,size_t d) {
    r230_original=(nl_live_tail){p,{a},{d}};
    r230_has_original=1;
    fprintf(stderr, "R230 LIVE_SEND ptr=%p allocation=%p domain=%zu\n",
            (const void *)p,a,d);
    r230_check(p!=NULL && a==(void *)p,"send","ptr/allocation disagree");
}
static void r230_live_enter(nl_node_option *link,const nl_node *p,
                            void *a,size_t d) {
    fprintf(stderr,
            "R230 PRODUCER_ENTER link=%p tag=%u link_ptr=%p tail=%p A=%p D=%zu\n",
            (void *)link,link->tag,(const void *)link->ptr,(const void *)p,a,d);
    r230_check(link->tag==1 && link->ptr==p,
               "producer","head link does not hold original tail");
}
static void r230_live_return(nl_live_tail p) {
    fprintf(stderr,"R230 PRODUCER_RETURN ptr=%p allocation=%p domain=%zu\n",
            (const void *)p.ptr,p.allocation.handle,p.domain.token);
    r230_check(r230_has_original && r230_same(p,r230_original),
               "producer return","packet not original ptr/A/D");
}
static void r230_custody_enter(const nl_custody *s,nl_live_tail p) {
    fprintf(stderr,"R230 RECIPIENT_ENTER slot=%p tag=%u packet_ptr=%p A=%p D=%zu\n",
            (const void *)s,s->tag,(const void *)p.ptr,p.allocation.handle,
            p.domain.token);
    r230_check(s->tag==0,"recipient","sink not exactly None on entry");
    r230_check(r230_has_original && r230_same(p,r230_original),
               "recipient","packet identity lost before call");
}
static void r230_custody_stored(const nl_custody *s) {
    fprintf(stderr,"R230 RECIPIENT_RETURN slot=%p tag=%u ptr=%p A=%p D=%zu\n",
            (const void *)s,s->tag,(const void *)s->packet.ptr,
            s->packet.allocation.handle,s->packet.domain.token);
    r230_check(s->tag==1,"recipient return","slot is not Some");
    r230_check(r230_same(s->packet,r230_original),
               "recipient return","slot packet not original");
}
static void r230_custody_returned(const nl_custody *s) {
    fprintf(stderr,"R230 AFTER_RECIPIENT slot=%p tag=%u ptr=%p A=%p D=%zu\n",
            (const void *)s,s->tag,(const void *)s->packet.ptr,
            s->packet.allocation.handle,s->packet.domain.token);
    r230_check(s->tag==1,"after call","recipient result not durable");
    r230_check(r230_same(s->packet,r230_original),
               "after call","original identity lost after frame return");
}
static void r230_custody_extract(const nl_custody *current,nl_custody previous) {
    fprintf(stderr,"R230 EXTRACT current_tag=%u old_tag=%u old_ptr=%p A=%p D=%zu\n",
            current->tag,previous.tag,(const void *)previous.packet.ptr,
            previous.packet.allocation.handle,previous.packet.domain.token);
    r230_check(current->tag==0,"extract","caller slot not set to None");
    if(r230_policy==1) {
        r230_check(previous.tag==1,"extract","old value not Some on accept");
        r230_check(r230_same(previous.packet,r230_original),
                   "extract","original owner not recovered");
    } else if(r230_policy==2) {
        r230_check(previous.tag==0,"extract","refused path held owner");
    }
}
static void r230_custody_none(int site,nl_custody val) {
    fprintf(stderr,"R230 EMPTY_CONSUME site=%d tag=%u\n",site,val.tag);
    r230_check(val.tag==0,"None-only consuming match","non-None silently dropped");
}
static void r230_owner_receiver(const nl_node *p,void *a,size_t d) {
    fprintf(stderr,"R230 RECEIVER_ENTER ptr=%p allocation=%p domain=%zu\n",
            (const void *)p,a,d);
    r230_check(r230_has_original && p==r230_original.ptr &&
               a==r230_original.allocation.handle &&
               d==r230_original.domain.token,
               "receiver","not original tail ptr/A/D");
}
static void r230_release(void *a,const unsigned char *s,size_t n) {
    fprintf(stderr,"R230 RELEASE allocation=%p storage=%p bytes=%zu\n",
            a,(const void *)s,n);
    r230_check(a==(void *)s && n==24,"release",
               "allocation vs storage pointer or full extent mismatch");
}
/* Unused projection-stage macros are explicitly neutral; no app state edits. */
#define NL_HEAP_TRIAL(p,n,a) ((void)0)
#define NL_HEAP_ARM(...) ((void)0)
#define NL_HEAP_DOMAIN(...) ((void)0)
#define NL_HEAP_SLOT(...) ((void)0)
#define NL_HEAP_INITIALIZE(...) ((void)0)
#define NL_HEAP_RELOAN(...) ((void)0)
#define NL_HEAP_ROOT(...) ((void)0)
#define NL_HEAP_FIELD(...) ((void)0)
#define NL_HEAP_COPY(...) ((void)0)
#define NL_HEAP_CHANGE(p,b,k) r230_link_change((p),(b),(k))
#define NL_HEAP_SCOPE_END(...) ((void)0)
#define NL_HEAP_END(...) ((void)0)
#define NL_HEAP_RAW(...) ((void)0)
#define NL_HEAP_FINALIZE(...) ((void)0)
#define NL_HEAP_RELEASE(a,s,n) r230_release((a),(s),(n))
#define NL_HEAP_FINISH() ((void)0)
#define NL_HEAP_HANDOFF(...) ((void)0)
#define NL_HEAP_RECEIVER_ENTER(p,a,d,...) r230_owner_receiver((p),(a),(d))
#define NL_HEAP_RECEIVER_EXIT() ((void)0)
#define NL_HEAP_RETURNED(...) ((void)0)
#define NL_LIVE_SEND(p,a,d,...) r230_live_send((p),(a),(d))
#define NL_LIVE_ENTER(l,p,a,d) r230_live_enter((l),(p),(a),(d))
#define NL_LIVE_RETURN(p) r230_live_return((p))
#define NL_LIVE_RECEIVE(...) ((void)0)
#define NL_CUSTODY_ENTER(s,p) r230_custody_enter((s),(p))
#define NL_CUSTODY_STORED(s,...) r230_custody_stored((s))
#define NL_CUSTODY_RETURNED(s,...) r230_custody_returned((s))
#define NL_CUSTODY_EXTRACT(s,old) r230_custody_extract((s),(old))
#define NL_CUSTODY_NONE(site,v) r230_custody_none((site),(v))
#define NL_CUSTODY_POLICY(p) do {r230_policy=(p);fprintf(stderr,"R230 POLICY %d\n",r230_policy);}while(0)
#define NL_CUSTODY_LOAN(...) ((void)0)
#endif
