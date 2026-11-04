#ifndef R1151_PASSIVE_OBSERVER_H
#define R1151_PASSIVE_OBSERVER_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static const uint8_t *r1151_first;
static const uint8_t *r1151_second;

/* Only called while the original main-body object is live. No guest values,
 * conditions, stores or addresses are produced by these observations. */
static void r1151_record(const char *tag, size_t value, const uint8_t *address)
{
    printf("R1151\t%s\t%zu\t%p\t%p\t%u\t%u\t%p\n", tag, value,
           (void *)r1151_first, (void *)r1151_second, (unsigned)*r1151_first,
           (unsigned)*r1151_second, (void *)address);
}

#endif
