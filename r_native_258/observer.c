#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include "observer_state.h"
uintptr_t r258_addr[5]={0};
int r258_calls=0, r258_successes=0, r258_inits=0, r258_changes=0, r258_frees=0;
int r258_ended[5]={0},r258_freed[5]={0},r258_order[5]={0};
size_t r258_domain[5]={0};
static uintptr_t pending=0;
static int fail_site=-1;
extern void *__real_malloc(size_t);
extern void __real_free(void *);
void r258_fail(int code, const char *message){
  fprintf(stderr,"R258_OBSERVER_REJECT code=%d message=%s calls=%d success=%d frees=%d changes=%d\n",code,message,r258_calls,r258_successes,r258_frees,r258_changes);
  fflush(stderr);
  _Exit(code);
}
void *__wrap_malloc(size_t size){
  if(fail_site==-1) {
    const char *s=getenv("R258_FAIL_SITE");
    fail_site=s?atoi(s):0;
    if(fail_site<0 || fail_site>5) r258_fail(81,"bad failure site");
  }
  if(r258_calls>=5 || size!=56) r258_fail(82,"unexpected malloc/size");
  r258_calls++;
  if(r258_calls==fail_site) return NULL;
  void *p=__real_malloc(size);
  if(!p) r258_fail(82,"unexpected host OOM");
  for(int i=0;i<r258_successes;i++) if(r258_addr[i]==(uintptr_t)p)
    r258_fail(82,"same address for two simultaneously live heap objects");
  r258_addr[r258_successes++]=(uintptr_t)p;
  return p;
}
static int index_of(const void *p){
  for(int i=0;i<r258_successes;i++) if(r258_addr[i]==(uintptr_t)p) return i;
  return -1;
}
void r258_init(const void *p,size_t domain) {
  int index=index_of(p);
  if(index<0 || r258_inits>=r258_successes || index!=r258_inits)
     r258_fail(83,"init not on new distinct original region");
  for(int i=0;i<index;i++) if(r258_domain[i]==domain)
     r258_fail(83,"domain identity repeated");
  r258_domain[index]=domain;
  r258_inits++;
}
void r258_root(const void *p) {
  int index=index_of(p);
  if(index<0 || r258_freed[index] || r258_ended[index])
    r258_fail(85,"post-EndRoot or unknown pointer dereference/projection");
}
void r258_change(const void *p,const void *old) {
  (void)old;
  if(!p || r258_changes>=6 || r258_inits!=5 || r258_frees!=0)
    r258_fail(85,"change outside all-live checkpoint");
  r258_changes++;
}
void r258_end(const void *p,size_t domain) {
  int index=index_of(p);
  if(index<0 || r258_ended[index] || r258_freed[index] ||
     r258_domain[index]!=domain) r258_fail(86,"wrong EndRoot/domain");
  if(r258_successes==5 && r258_changes!=6)
    r258_fail(86,"EndRoot before complete six links");
  r258_ended[index]=1;
}
void r258_release(const void *allocation,const void *raw,size_t length) {
  int index=index_of(allocation);
  int wanted=r258_successes-1-r258_frees;
  if(index<0 || index!=wanted || allocation!=raw || length!=56 ||
     !r258_ended[index] || r258_freed[index] || pending)
    r258_fail(87,"original allocation/storage/domain or release order mismatch");
  pending=(uintptr_t)allocation;
}
void __wrap_free(void *p){
  int index=index_of(p);
  if(index<0 || !p || r258_freed[index] || pending!=(uintptr_t)p)
    r258_fail(88,"unmatched/double/early real free request (trapped before libc)");
  r258_order[r258_frees++]=index;
  r258_freed[index]=1;
  pending=0;
  __real_free(p);
}
void r258_finish(void) {
  int expected=fail_site?fail_site-1:5;
  int attempted=fail_site?fail_site:5;
  if(r258_calls!=attempted || r258_successes!=expected ||
     r258_inits!=expected || r258_frees!=expected || pending ||
     r258_changes!=(expected==5?6:0))
    r258_fail(89,"incomplete matching physical cleanup/checkpoint");
  for(int i=0;i<expected;i++) if(!r258_ended[i] || !r258_freed[i])
    r258_fail(89,"unended/unfreed original root");
  printf("R258_WORLD_PASS fail_site=%d calls=%d success=%d physical_frees=%d fields=%d addr:",fail_site,r258_calls,r258_successes,r258_frees,r258_changes);
  for(int i=0;i<expected;i++) printf(" 0x%" PRIxPTR,r258_addr[i]);
  printf(" order:");
  for(int i=0;i<expected;i++) printf(" %d",r258_order[i]+1);
  putchar('\n');
}
