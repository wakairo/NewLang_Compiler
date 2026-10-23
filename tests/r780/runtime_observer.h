/* R #780: passive original-pointer, T-currentness, and affine-transfer checks.
 * No writes to guest values, no platform allocation/free substitution.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#define R_ASSERT(p) do { if (!(p)) { fprintf(stderr,"R780 observer:%d: %s\n", __LINE__, #p); abort(); } } while (0)
extern size_t test_malloc_calls(void);
extern size_t test_free_calls(void);
extern uintptr_t test_allocated_address(size_t);
static uint8_t *r_t[2][2];
static const unsigned r_before[2][2]={{B00,B01},{B10,B11}};
static const unsigned r_after[2][2]={{A00,A01},{A10,A11}};
static const unsigned r_old[2]={OLD0,OLD1};
static const unsigned r_new[2]={NEW0,NEW1};
static const size_t r_index[2]={IDX0,IDX1};
static size_t r_binds,r_stores,r_calls,r_trials,r_allocs,r_frees;
static const nl_node *r_nodes[2];
static size_t r_domains[2],r_dcount,r_changes,r_local,r_sent,r_enter,r_return,r_receive,r_caller,r_handoff,r_receiver,r_receiver_exit;
static size_t r_end[2],r_raw[2],r_final[2],r_released[2],r_freed[2];
static bool r_producer,r_receiver_active;
static const nl_live_tail *r_local_ptr,*r_caller_ptr;
static int r_region(const void *p) {
    for (int i=0;i<2;i++) if (p && p==r_nodes[i]) return i;
    R_ASSERT(false); return 0;
}
static void r_packet(const nl_live_tail *p) {
    R_ASSERT(r_nodes[1] && p->ptr==r_nodes[1] &&
       p->allocation.handle==(void *)r_nodes[1] && p->domain.token==r_domains[1]);
}
static void r780_node_bind(size_t frame,size_t symbol,size_t kind,const void *p) {
    (void)frame;(void)symbol;
    if (r_receive==1 && r_caller==0 && kind==4) {
        r_caller_ptr=(const nl_live_tail *)p;
        r_packet(r_caller_ptr); r_caller++;
    }
}
static void r_copy_bind(size_t frame,size_t symbol,uint8_t *a,uint8_t *b) {
    (void)frame;(void)symbol;
    R_ASSERT(r_binds<2 && r_trials==0 && a && b && a!=b);
    size_t i=r_binds++;
    r_t[i][0]=a;r_t[i][1]=b;
    R_ASSERT(*a==r_before[i][0] && *b==r_before[i][1]);
    if (i) for(int k=0;k<2;k++) for(int j=0;j<2;j++)
        R_ASSERT(r_t[0][k]!=r_t[1][j]);
}
static void r_store(uint8_t *field,uint8_t old,uint8_t next,bool call) {
    size_t i=r_stores++;
    R_ASSERT(i<2 && r_binds==2 && r_trials==0 &&
             field==r_t[i][r_index[i]] && old==r_old[i] && next==r_new[i] &&
             *field==next && *r_t[i][1-r_index[i]]==r_before[i][1-r_index[i]]);
    if(call)r_calls++;
}
static void r_trial(void *p,size_t n,size_t a) {
    R_ASSERT(r_trials<2 && r_stores==2 && n==24 && a==8 &&
       test_malloc_calls()==r_trials+1 && (uintptr_t)p==test_allocated_address(r_trials));
    if(p)r_allocs++;
    r_trials++;
}
static void r_domain(size_t d) {
    R_ASSERT(r_dcount<2 && d!=0 && (!r_dcount || d!=r_domains[0]));
    r_domains[r_dcount++]=d;
}
static void r_initialize(const nl_node *p,size_t d) {
    R_ASSERT(r_nodes[0]==NULL || r_nodes[1]==NULL);
    size_t i=r_nodes[0]?1:0;
    R_ASSERT(p && d==r_domains[i] &&
       (uintptr_t)p==test_allocated_address(i) &&
       p->f1==(i ? TAIL_PAYLOAD : HEAD_PAYLOAD) &&
       p->f0.tag==0 && p->f0.ptr==NULL);
    r_nodes[i]=p;
    if(i) R_ASSERT(r_nodes[0]!=r_nodes[1]);
}
static void r_change(const nl_node_option *f,const nl_node_option *old,size_t step) {
    R_ASSERT(r_nodes[0] && r_nodes[1] && f==&r_nodes[0]->f0 && old!=f &&
       step==r_changes+1);
    if(r_changes==0) R_ASSERT(!r_producer && old->tag==0 &&
                             f->tag==1 && f->ptr==r_nodes[1]);
    else R_ASSERT(r_producer && old->tag==1 && old->ptr==r_nodes[1] &&
                  f->tag==0 && f->ptr==NULL);
    r_changes++;
}
static void r_live_send(const nl_node *p,void *a,size_t d,
                        const nl_allocation *old_a,const nl_domain *old_d) {
    R_ASSERT(r_sent++==0 && p==r_nodes[1] && a==(void *)p &&
       d==r_domains[1] && old_a->handle==NULL && old_d->token==0);
}
static void r_live_enter(nl_node_option *edge,const nl_node *p,void *a,size_t d) {
    R_ASSERT(r_sent==1 && r_enter++==0 && edge==&r_nodes[0]->f0 &&
       edge->tag==1 && edge->ptr==p && p==r_nodes[1] &&
       a==(void *)p && d==r_domains[1]);r_producer=true;
}
static void r_owner_local(const nl_live_tail *p) {
    R_ASSERT(r_producer && r_local++==0);
    r_local_ptr=p;r_packet(p);
}
static void r_live_return(nl_live_tail p) {
    R_ASSERT(r_producer && r_return++==0 && r_local==1 && r_changes==2);
    r_packet(&p);R_ASSERT(r_local_ptr && !r_local_ptr->ptr &&
       !r_local_ptr->allocation.handle && !r_local_ptr->domain.token);
    r_producer=false;
}
static void r_live_receive(nl_live_tail p,const nl_allocation *a,const nl_domain *d) {
    R_ASSERT(r_return==1 && r_receive++==0 && !a->handle && !d->token);
    r_packet(&p);
}
static void r_handoff_call(const nl_node *p,void *a,size_t d,
                         const nl_allocation *old_a,const nl_domain *old_d) {
    R_ASSERT(r_caller==1 && r_caller_ptr && !r_caller_ptr->ptr &&
       !r_caller_ptr->allocation.handle && !r_caller_ptr->domain.token &&
       p==r_nodes[1] && a==(void *)p && d==r_domains[1] &&
       !old_a->handle && !old_d->token);
    r_handoff++;
}
static void r_receiver_enter(const nl_node *p,void *a,size_t d,
                    size_t frame,size_t ps,size_t as,size_t ds,
                    const nl_allocation *ap,const nl_domain *dp) {
    (void)frame;(void)ps;(void)as;(void)ds;
    R_ASSERT(r_handoff==1 && r_receiver++==0 && p==r_nodes[1] &&
       a==(void *)p && d==r_domains[1] && ap->handle==a && dp->token==d);
    r_receiver_active=true;
}
static void r_receiver_leave(void) {
    R_ASSERT(r_receiver_active && r_freed[1]==1 && r_receiver_exit++==0);
    r_receiver_active=false;
}
static void r_returned(const nl_allocation *a,const nl_domain *d) {
    R_ASSERT(r_receiver_exit==1 && !a->handle && !d->token);
}
static void r_end_node(const nl_node *p,size_t d) {
    int i=r_region(p);
    R_ASSERT(d==r_domains[i] && r_end[i]++==0 &&
       p->f0.tag==0 && p->f0.ptr==NULL &&
       p->f1==(i?TAIL_PAYLOAD:HEAD_PAYLOAD));
    if(i)R_ASSERT(r_receiver_active && r_handoff==1);
    else if(r_nodes[1])R_ASSERT(!r_receiver_active && r_freed[1]==1);
}
static void r_raw_slot(const void *p,size_t n) {
    int i=r_region(p);R_ASSERT(n==24 && r_end[i]==1 && r_raw[i]++==0);
}
static void r_finalize_domain(size_t d) {
    int i=(d==r_domains[0]?0:1);
    R_ASSERT(d==r_domains[i] && r_raw[i]==1 && r_final[i]++==0);
}
static void r_release(const void *p,const void *b,size_t n) {
    int i=r_region(p);R_ASSERT(p==b && n==24 && r_final[i]==1 &&
       r_released[i]++==0);
}
void test_observe_free(uintptr_t p) {
    int i=r_region((void*)p);
    R_ASSERT(r_released[i]==1 && r_freed[i]++==0);
    if(i==0 && r_nodes[1]) R_ASSERT(r_freed[1]==1);
    if(i==1) R_ASSERT(r_receiver_active);
    r_frees++;
}
static void r_finish(void) {
    R_ASSERT(r_binds==2 && r_stores==2 && r_calls==1);
    for(size_t i=0;i<2;i++)for(size_t j=0;j<2;j++)
        R_ASSERT(*r_t[i][j]==r_after[i][j]);
    R_ASSERT(r_trials==test_malloc_calls() && r_frees==test_free_calls() &&
       r_frees==r_allocs);
    for(size_t i=0;i<r_allocs;i++)
        R_ASSERT(r_freed[i]==1);
    if(r_allocs==2)R_ASSERT(r_sent==1 && r_enter==1 && r_local==1 &&
       r_return==1 && r_receive==1 && r_caller==1 && r_handoff==1 &&
       r_receiver==1 && r_receiver_exit==1 && r_changes==2);
    else R_ASSERT(!r_sent && !r_enter && !r_local && !r_return &&
      !r_receive && !r_caller && !r_handoff && !r_receiver && !r_changes);
    printf("R780_NATIVE PASS T=%u,%u/%u,%u old=%u,%u calls=%zu trials=%zu allocated=%zu freed=%zu transfer=%zu caller=%zu receiver=%zu ptr0=%p ptr1=%p\n",
      *r_t[0][0],*r_t[0][1],*r_t[1][0],*r_t[1][1],
      r_old[0],r_old[1],r_calls,r_trials,r_allocs,r_frees,r_return,r_caller,r_receiver,
      (void*)r_nodes[0],(void*)r_nodes[1]);
}
#define NL_MIXED_COPY_BIND(f,s,a,b) r_copy_bind(f,s,a,b)
#define NL_MIXED_COPY_CALL(p,o,n) r_store(p,o,n,true)
#define NL_MIXED_COPY_STORE(p,o,n) r_store(p,o,n,false)
#define NL_HEAP_TRIAL(p,n,a) r_trial(p,n,a)
#define NL_HEAP_DOMAIN(d) r_domain(d)
#define NL_HEAP_INITIALIZE(p,d) r_initialize(p,d)
#define NL_HEAP_CHANGE(p,o,k) r_change(p,o,k)
#define NL_HEAP_END(p,d) r_end_node(p,d)
#define NL_HEAP_RAW(p,n) r_raw_slot(p,n)
#define NL_HEAP_FINALIZE(d) r_finalize_domain(d)
#define NL_HEAP_RELEASE(p,b,n) r_release(p,b,n)
#define NL_HEAP_FINISH() r_finish()
#define NL_HEAP_HANDOFF(p,a,d,ap,dp) r_handoff_call(p,a,d,ap,dp)
#define NL_HEAP_RECEIVER_ENTER(p,a,d,f,ps,as,ds,ap,dp) r_receiver_enter(p,a,d,f,ps,as,ds,ap,dp)
#define NL_HEAP_RECEIVER_EXIT() r_receiver_leave()
#define NL_HEAP_RETURNED(a,d) r_returned(a,d)
#define NL_LIVE_SEND(p,a,d,ap,dp) r_live_send(p,a,d,ap,dp)
#define NL_LIVE_ENTER(e,p,a,d) r_live_enter(e,p,a,d)
#define NL_LIVE_RETURN(p) r_live_return(p)
#define NL_LIVE_RECEIVE(p,a,d) r_live_receive(p,a,d)
#define NL_MIXED_OWNER_LOCAL(p) r_owner_local(p)
