#ifndef NEWLANG_CAPTURED_TRACE_INTERNAL_H
#define NEWLANG_CAPTURED_TRACE_INTERNAL_H

/* Storage/consumer bounds only. Authority is rederived by the existing
 * captured closure validator, including the exact ordered nine-event shape. */
enum {
    NL_CAPTURED_SETUP_CHANGES = 6,
    NL_CAPTURED_UNLINK_CHANGES = 9,
    NL_CAPTURED_CHANGE_CAPACITY = NL_CAPTURED_UNLINK_CHANGES
};

#endif
