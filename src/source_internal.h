#ifndef NEWLANG_SOURCE_INTERNAL_H
#define NEWLANG_SOURCE_INTERNAL_H
#include "newlang/source.h"
/* Copy only allocation headers; callers authenticate them before bytes. */
NLSourceView nl_source_storage_view(const NLSource *);
#endif
