#ifndef P866_SUPPORT_H
#define P866_SUPPORT_H
#include "../../src/copy_body_internal.h"
#include "../../src/if_field_internal.h"
#include "../support/node_checked.h"
typedef struct {
    TestNode source;
    NLCheckedFragment *caller, *body;
    NLCheckedNodeId call;
    NLCopyUnitView unit;
    NLIfReturnView returned;
} P866;
static inline bool p866_load(const char *path, P866 *out)
{
    CHECK(node_checked_load(path, &out->source));
    out->caller = (NLCheckedFragment *)nl_checked_call_body(
        out->source.entry, nl_checked_root(out->source.entry));
    size_t found = 0;
    for (size_t i = 1; i <= out->caller->count; ++i)
        if (nl_checked_copy_unit_view(out->caller, i, &out->unit)) {
            out->call = i;
            ++found;
        }
    CHECK(found == 1);
    out->body = (NLCheckedFragment *)out->unit.body;
    CHECK(nl_checked_if_return_view(out->body, out->unit.conditional,
                                    &out->returned));
    return true;
}
static inline bool p866_valid(P866 *o)
{
    return nl_checked_copy_unit_validate(o->caller, o->call, &o->unit) ==
               NL_CHECK_OK &&
           nl_checked_copy_body_parameters_validate(
               o->caller, o->call, &o->unit.parameters) == NL_CHECK_OK &&
           nl_checked_if_return_validate(o->body, o->unit.conditional,
                                         &o->returned) == NL_CHECK_OK;
}
#endif
