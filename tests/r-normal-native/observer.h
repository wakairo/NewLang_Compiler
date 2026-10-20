#ifndef R656_OBSERVER_H
#define R656_OBSERVER_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

/* Read guest storage through const pointers. Never write guest memory. */
static void r656_pair(const char *stage, const uint8_t *left,
                      const uint8_t *right, unsigned expected_left,
                      unsigned expected_right)
{
    printf("%s left@%p=%u right@%p=%u\n", stage, (const void *)left,
           (unsigned)*left, (const void *)right, (unsigned)*right);
    assert(*left == expected_left);
    assert(*right == expected_right);
}

static void r656_disjoint(const void *first, const void *second, size_t size)
{
    const uintptr_t a = (uintptr_t)first;
    const uintptr_t b = (uintptr_t)second;
    printf("roots first@%p second@%p size=%zu\n", first, second, size);
    assert(a + size <= b || b + size <= a);
}

static void r656_copy(const char *name, unsigned actual, unsigned expected)
{
    printf("%s=%u\n", name, actual);
    assert(actual == expected);
}

#endif
