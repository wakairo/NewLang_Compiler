#ifndef NEWLANG_ADDRESS_BODY_INTERNAL_H
#define NEWLANG_ADDRESS_BODY_INTERNAL_H

#include "newlang/semantic.h"

struct NLFunctionBody;

NLCheckStatus nl_body_create_address_main_span(const NLSource *, NLSourceSpan,
                                               struct NLFunctionBody **);

#endif
