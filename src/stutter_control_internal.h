#ifndef NEWLANG_STUTTER_CONTROL_INTERNAL_H
#define NEWLANG_STUTTER_CONTROL_INTERNAL_H
#include "control.h"

/* Private source-checker producer. Generic exit append cannot issue this
 * source-node/operand binding. This is provenance, not an induction issuer. */
NLCheckStatus
nl_control_source_continue_append(NLControlExits **, NLControlTarget *,
                                  const NLControlState *, NLCheckedResult,
                                  const NLCheckedFragment *, NLCheckedNodeId,
                                  const NLCheckedFragment *);
bool nl_control_source_terminal_valid(const NLCheckedFragment *,
                                      const NLControlTarget *,
                                      NLControlExitKind,
                                      const NLCheckedFragment *);
#endif
