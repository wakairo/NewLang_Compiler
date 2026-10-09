#ifndef R258_NATIVE_OBSERVER_H
#define R258_NATIVE_OBSERVER_H
/* Independent observer for C emitted in THIS R run; no P harness included. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
extern uintptr_t r258_addr[5];
extern int r258_calls, r258_successes, r258_inits, r258_changes, r258_frees;
extern int r258_ended[5];
extern size_t r258_domain[5];
void r258_fail(int, const char *);
void r258_init(const void *, size_t);
void r258_change(const void *, const void *);
void r258_release(const void *, const void *, size_t);
void r258_end(const void *, size_t);
void r258_finish(void);
void r258_root(const void *);
static inline void r258_topology(void) {
  const nl_node *n[5];
  for (int i=0;i<5;i++) {
    if (!r258_addr[i]) r258_fail(84,"missing root");
    n[i]=(const nl_node *)r258_addr[i];
    if (n[i]->f3!=(uint8_t)(i+1)) r258_fail(84,"wrong payload");
    /* Three physically separate fields and real placement. */
    if ((const void *)&n[i]->f0==(const void *)&n[i]->f1 ||
        (const void *)&n[i]->f1==(const void *)&n[i]->f2)
      r258_fail(84,"overlapping sibling fields");
  }
#define R258_EDGE(from,field,to) do { \
   if(n[from]->field.tag!=1 || n[from]->field.ptr!=n[to]) r258_fail(84,"wrong live edge"); \
  } while(0)
  R258_EDGE(0,f2,1); R258_EDGE(1,f1,3); R258_EDGE(1,f0,2);
  R258_EDGE(2,f1,1); R258_EDGE(2,f0,3); R258_EDGE(3,f1,2);
#undef R258_EDGE
  if (n[4]->f2.tag!=0 || n[4]->f2.ptr!=NULL)
    r258_fail(84,"wrong destination None/tag");
  for (int i=0;i<5;i++){
    if(i!=0 && i!=1 && i!=2 && i!=3 && i!=4) r258_fail(84,"bad root");
    if (i==0 && (n[i]->f0.tag || n[i]->f1.tag)) r258_fail(84,"src siblings non-None");
    if (i==3 && (n[i]->f0.tag || n[i]->f2.tag)) r258_fail(84,"C siblings non-None");
    if (i==4 && (n[i]->f0.tag || n[i]->f1.tag)) r258_fail(84,"dst siblings non-None");
    if (i==1 && n[i]->f2.tag) r258_fail(84,"A child non-None");
    if (i==2 && n[i]->f2.tag) r258_fail(84,"B child non-None");
    if (i==3 && n[i]->f2.tag) r258_fail(84,"C child non-None");
    if (i==4 && n[i]->f2.tag) r258_fail(84,"dst child non-None");
    if (i==0 && n[i]->f0.tag) r258_fail(84,"src next non-None");
  }
  puts("R258_TOPOLOGY_PASS: 5 distinct live roots; 7 edge/None facts; payload 1..5");
}
static inline void r258_field(const nl_node *root,const nl_option *f,
                              unsigned idx) {
  r258_root(root);
  if (idx>2) r258_fail(84,"field index not 0..2");
  const nl_option *wanted=idx==0?&root->f0:(idx==1?&root->f1:&root->f2);
  if (f!=wanted) r258_fail(84,"wrong physical field offset");
}
static inline void r258_note_change(const nl_option *current,
                                    const nl_option *old) {
  if (!current || !old) r258_fail(84,"null change");
  if (old->tag!=0 || old->ptr!=NULL) r258_fail(84,"unexpected old Option");
  if (current->tag!=1 || current->ptr==NULL)
    r258_fail(84,"wrong Some tag or pointer");
  r258_change(current,old);
  if(r258_changes==6) r258_topology();
}
#define NL_FIVE_TRIAL(...) ((void)0)
#define NL_FIVE_ARM(...) ((void)0)
#define NL_FIVE_SLOT(...) ((void)0)
#define NL_FIVE_RAW(...) ((void)0)
#define NL_FIVE_DOMAIN(...) ((void)0)
#define NL_FIVE_INIT(p,d,...) r258_init((p),(d))
#define NL_FIVE_LOAN(...) ((void)0)
#define NL_FIVE_SCOPE_END(...) ((void)0)
#define NL_FIVE_ROOT(p,...) r258_root((p))
#define NL_FIVE_FIELD(p,f,m,i,...) r258_field((p),(f),(i))
#define NL_FIVE_CHANGE(p,old,...) r258_note_change((p),(old))
#define NL_FIVE_END(p,d,...) r258_end((p),(d))
#define NL_FIVE_FINALIZE(...) ((void)0)
#define NL_FIVE_RELEASE(h,b,n) r258_release((h),(b),(n))
#define NL_FIVE_FINISH() r258_finish()
#endif
