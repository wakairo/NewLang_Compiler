/* Compile the IMMUTABLE original translation unit as a TEST ONLY object.
 * Replace its archive member only in a copy linked to the dedicated harness.
 * No production/header edit, no newlangc linkage, no public ABI change. */
#ifndef NEWLANG_TEST_CHRONOLOGY_SHADOW
#error "Never compile this wrapper into the production library"
#endif
#include "../../src/captured_closure.c"
#include "shadow.h"
NLCheckStatus nl_test_p350_primitive(const NLCheckedFragment *f,
                                     const NLCapturedChange *change)
{
    return change_validate(f, change);
}
