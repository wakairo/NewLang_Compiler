#ifndef NEWLANG_TEST_CHRONOLOGY_SHADOW
#error "P352 private corruption / fault hooks are test-build only"
#endif
extern unsigned nl_test_p352_phase;
void nl_test_p352_reset_faults(void);
NLCheckStatus nl_test_p352_primitive(const NLCheckedFragment *,
                                     const NLCapturedChange *);
