#ifndef R258_OBSERVER_STATE_H
#define R258_OBSERVER_STATE_H
#include <stdint.h>
#include <stddef.h>
extern uintptr_t r258_addr[5];
extern int r258_calls, r258_successes, r258_inits, r258_changes, r258_frees;
extern int r258_ended[5];
extern size_t r258_domain[5];
void r258_fail(int,const char*);
void r258_init(const void*,size_t);
void r258_change(const void*,const void*);
void r258_release(const void*,const void*,size_t);
void r258_end(const void*,size_t);
void r258_finish(void);
void r258_root(const void*);
#endif
