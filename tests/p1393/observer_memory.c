/* Separate translation unit, no LTO. Additional read-only observer loads of
 * still-live A/B fields; not a claim about raw optimized machine loads. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct nl_node nl_node;
typedef struct {
    unsigned tag;
    const nl_node *ptr;
} nl_option;
struct nl_node {
    nl_option f0, f1, f2;
    uint8_t f3;
};
void p1393_observe_live_copy(const nl_option *field, const nl_option *copy)
{
    if (field->tag != copy->tag || field->ptr != copy->ptr)
        abort();
    printf("OBSERVER_LIVE_FIELD tag=%u ptr=%zx\n", field->tag,
           (size_t)(uintptr_t)field->ptr);
}
