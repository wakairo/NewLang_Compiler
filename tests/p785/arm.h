#ifndef P785_ARM_H
#define P785_ARM_H
#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>
typedef struct {
    NLCheckedFragment *caller, *body, *parent;
    NLCheckedNodeId call, match;
    size_t index;
} ArmCall;
static inline bool collect(NLCheckedFragment *f, NLCheckedFragment *parent,
                           NLCheckedNodeId match, size_t index,
                           ArmCall found[8], size_t *count, size_t depth)
{
    CHECK(f && depth < 20);
    for (size_t i = 0; i < f->body_count; ++i) {
        NLCheckedFragment *b = f->bodies[i];
        if (b->copy_body && b->copy_body->formal_write && parent) {
            CHECK(*count < 8);
            found[(*count)++] =
                (ArmCall){f, b, parent, f->body_calls[i], match, index};
        }
        CHECK(collect(b, NULL, 0, 0, found, count, depth + 1));
    }
    for (size_t i = 0; i < f->arm_count; ++i) {
        size_t arm_index = 0;
        for (size_t j = 0; j < i; ++j)
            arm_index += f->arms[j].match == f->arms[i].match;
        CHECK(collect(f->arms[i].artifact, f, f->arms[i].match, arm_index,
                      found, count, depth + 1));
    }
    return true;
}
static inline bool valid(ArmCall a)
{
    NLCopyBodyView v;
    NLCopyBodyParameters p;
    return nl_checked_copy_body_view(a.caller, a.call, &v) &&
           nl_checked_copy_body_parameters(a.caller, a.call, &p) &&
           nl_checked_copy_body_validate(a.caller, a.call, &v) == NL_CHECK_OK &&
           nl_checked_copy_body_parameters_validate(a.caller, a.call, &p) ==
               NL_CHECK_OK;
}
static inline bool scalar(const NLSemanticContext *c, NLValueId id,
                          unsigned expected)
{
    NLSemanticValueView v;
    return nl_semantic_value_view(c, id, &v) && v.scalar_known &&
           v.scalar_value == expected;
}
#endif
