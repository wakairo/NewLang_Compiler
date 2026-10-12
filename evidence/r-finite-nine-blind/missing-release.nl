// R359 independently authored physical-five-root witness; not a list policy.
struct Relay {
    next: Option<ptr<Relay>>,
    prev: Option<ptr<Relay>>,
    child: Option<ptr<Relay>>,
    payload: u8,
}
fn main() -> unit {
    match try_allocate_one<Relay>() {
        None => {
            unit
        },
        Some(grant_Quartz) => {
            let OneBacking { allocation, raw } = grant_Quartz;
            let allocation_Quartz = allocation;
            let slot_Quartz = into_slot<Relay>(raw);
            let domain_Quartz = lifetime_domain();
            let ptr_Quartz = loan_read(domain_Quartz) { |stable_init_Quartz|
                initialize(slot_Quartz, Relay {
                    next: Option<ptr<Relay>>::None,
                    prev: Option<ptr<Relay>>::None,
                    child: Option<ptr<Relay>>::None,
                    payload: u8(31)
                }, stable_init_Quartz)
            };
            match try_allocate_one<Relay>() {
                None => {
                    let vacant_Quartz = loan_exclusive_read(domain_Quartz) { |ending_Quartz| destroy(ptr_Quartz, ending_Quartz) };
                    let raw_Quartz = erase_slot<Relay>(vacant_Quartz);
                    finalize_domain(domain_Quartz);
                    deallocate(allocation_Quartz, raw_Quartz);
                    unit
                },
                Some(grant_Cedar) => {
                    let OneBacking { allocation, raw } = grant_Cedar;
                    let allocation_Cedar = allocation;
                    let slot_Cedar = into_slot<Relay>(raw);
                    let domain_Cedar = lifetime_domain();
                    let ptr_Cedar = loan_read(domain_Cedar) { |stable_init_Cedar|
                        initialize(slot_Cedar, Relay {
                            next: Option<ptr<Relay>>::None,
                            prev: Option<ptr<Relay>>::None,
                            child: Option<ptr<Relay>>::None,
                            payload: u8(32)
                        }, stable_init_Cedar)
                    };
                    match try_allocate_one<Relay>() {
                        None => {
                            let vacant_Cedar = loan_exclusive_read(domain_Cedar) { |ending_Cedar| destroy(ptr_Cedar, ending_Cedar) };
                            let raw_Cedar = erase_slot<Relay>(vacant_Cedar);
                            finalize_domain(domain_Cedar);
                            deallocate(allocation_Cedar, raw_Cedar);
                            let vacant_Quartz = loan_exclusive_read(domain_Quartz) { |ending_Quartz| destroy(ptr_Quartz, ending_Quartz) };
                            let raw_Quartz = erase_slot<Relay>(vacant_Quartz);
                            finalize_domain(domain_Quartz);
                            deallocate(allocation_Quartz, raw_Quartz);
                            unit
                        },
                        Some(grant_Maple) => {
                            let OneBacking { allocation, raw } = grant_Maple;
                            let allocation_Maple = allocation;
                            let slot_Maple = into_slot<Relay>(raw);
                            let domain_Maple = lifetime_domain();
                            let ptr_Maple = loan_read(domain_Maple) { |stable_init_Maple|
                                initialize(slot_Maple, Relay {
                                    next: Option<ptr<Relay>>::None,
                                    prev: Option<ptr<Relay>>::None,
                                    child: Option<ptr<Relay>>::None,
                                    payload: u8(33)
                                }, stable_init_Maple)
                            };
                            match try_allocate_one<Relay>() {
                                None => {
                                    let vacant_Maple = loan_exclusive_read(domain_Maple) { |ending_Maple| destroy(ptr_Maple, ending_Maple) };
                                    let raw_Maple = erase_slot<Relay>(vacant_Maple);
                                    finalize_domain(domain_Maple);
                                    deallocate(allocation_Maple, raw_Maple);
                                    let vacant_Cedar = loan_exclusive_read(domain_Cedar) { |ending_Cedar| destroy(ptr_Cedar, ending_Cedar) };
                                    let raw_Cedar = erase_slot<Relay>(vacant_Cedar);
                                    finalize_domain(domain_Cedar);
                                    deallocate(allocation_Cedar, raw_Cedar);
                                    let vacant_Quartz = loan_exclusive_read(domain_Quartz) { |ending_Quartz| destroy(ptr_Quartz, ending_Quartz) };
                                    let raw_Quartz = erase_slot<Relay>(vacant_Quartz);
                                    finalize_domain(domain_Quartz);
                                    deallocate(allocation_Quartz, raw_Quartz);
                                    unit
                                },
                                Some(grant_Birch) => {
                                    let OneBacking { allocation, raw } = grant_Birch;
                                    let allocation_Birch = allocation;
                                    let slot_Birch = into_slot<Relay>(raw);
                                    let domain_Birch = lifetime_domain();
                                    let ptr_Birch = loan_read(domain_Birch) { |stable_init_Birch|
                                        initialize(slot_Birch, Relay {
                                            next: Option<ptr<Relay>>::None,
                                            prev: Option<ptr<Relay>>::None,
                                            child: Option<ptr<Relay>>::None,
                                            payload: u8(34)
                                        }, stable_init_Birch)
                                    };
                                    match try_allocate_one<Relay>() {
                                        None => {
                                            let vacant_Birch = loan_exclusive_read(domain_Birch) { |ending_Birch| destroy(ptr_Birch, ending_Birch) };
                                            let raw_Birch = erase_slot<Relay>(vacant_Birch);
                                            finalize_domain(domain_Birch);
                                            deallocate(allocation_Birch, raw_Birch);
                                            let vacant_Maple = loan_exclusive_read(domain_Maple) { |ending_Maple| destroy(ptr_Maple, ending_Maple) };
                                            let raw_Maple = erase_slot<Relay>(vacant_Maple);
                                            finalize_domain(domain_Maple);
                                            deallocate(allocation_Maple, raw_Maple);
                                            let vacant_Cedar = loan_exclusive_read(domain_Cedar) { |ending_Cedar| destroy(ptr_Cedar, ending_Cedar) };
                                            let raw_Cedar = erase_slot<Relay>(vacant_Cedar);
                                            finalize_domain(domain_Cedar);
                                            deallocate(allocation_Cedar, raw_Cedar);
                                            let vacant_Quartz = loan_exclusive_read(domain_Quartz) { |ending_Quartz| destroy(ptr_Quartz, ending_Quartz) };
                                            let raw_Quartz = erase_slot<Relay>(vacant_Quartz);
                                            finalize_domain(domain_Quartz);
                                            deallocate(allocation_Quartz, raw_Quartz);
                                            unit
                                        },
                                        Some(grant_Onyx) => {
                                            let OneBacking { allocation, raw } = grant_Onyx;
                                            let allocation_Onyx = allocation;
                                            let slot_Onyx = into_slot<Relay>(raw);
                                            let domain_Onyx = lifetime_domain();
                                            let ptr_Onyx = loan_read(domain_Onyx) { |stable_init_Onyx|
                                                initialize(slot_Onyx, Relay {
                                                    next: Option<ptr<Relay>>::None,
                                                    prev: Option<ptr<Relay>>::None,
                                                    child: Option<ptr<Relay>>::None,
                                                    payload: u8(35)
                                                }, stable_init_Onyx)
                                            };
                                            let prior_0 = loan_read(domain_Quartz) { |stable_0|
                                                let scoped_0 = ref_from_ptr(write, ptr_Quartz, stable_0);
                                                replace(scoped_0@next, Option<ptr<Relay>>::Some(ptr_Cedar))
                                            };
                                            let prior_1 = loan_read(domain_Quartz) { |stable_1|
                                                let scoped_1 = ref_from_ptr(write, ptr_Quartz, stable_1);
                                                replace(scoped_1@next, Option<ptr<Relay>>::Some(ptr_Cedar))
                                            };
                                            let prior_2 = loan_read(domain_Quartz) { |stable_2|
                                                let scoped_2 = ref_from_ptr(write, ptr_Quartz, stable_2);
                                                replace(scoped_2@next, Option<ptr<Relay>>::Some(ptr_Maple))
                                            };
                                            let prior_3 = loan_read(domain_Quartz) { |stable_3|
                                                let scoped_3 = ref_from_ptr(write, ptr_Quartz, stable_3);
                                                replace(scoped_3@prev, Option<ptr<Relay>>::Some(ptr_Birch))
                                            };
                                            let prior_4 = loan_read(domain_Quartz) { |stable_4|
                                                let scoped_4 = ref_from_ptr(write, ptr_Quartz, stable_4);
                                                replace(scoped_4@child, Option<ptr<Relay>>::Some(ptr_Onyx))
                                            };
                                            let prior_5 = loan_read(domain_Cedar) { |stable_5|
                                                let scoped_5 = ref_from_ptr(write, ptr_Cedar, stable_5);
                                                replace(scoped_5@next, Option<ptr<Relay>>::Some(ptr_Maple))
                                            };
                                            let prior_6 = loan_read(domain_Maple) { |stable_6|
                                                let scoped_6 = ref_from_ptr(write, ptr_Maple, stable_6);
                                                replace(scoped_6@prev, Option<ptr<Relay>>::Some(ptr_Quartz))
                                            };
                                            let prior_7 = loan_read(domain_Birch) { |stable_7|
                                                let scoped_7 = ref_from_ptr(write, ptr_Birch, stable_7);
                                                replace(scoped_7@child, Option<ptr<Relay>>::Some(ptr_Cedar))
                                            };
                                            let prior_8 = loan_read(domain_Onyx) { |stable_8|
                                                let scoped_8 = ref_from_ptr(write, ptr_Onyx, stable_8);
                                                replace(scoped_8@prev, Option<ptr<Relay>>::Some(ptr_Birch))
                                            };
                                            let vacant_Onyx = loan_exclusive_read(domain_Onyx) { |ending_Onyx| destroy(ptr_Onyx, ending_Onyx) };
                                            let raw_Onyx = erase_slot<Relay>(vacant_Onyx);
                                            finalize_domain(domain_Onyx);
                                            // deliberately omit Onyx original release (invalid resource discharge)
                                            let vacant_Birch = loan_exclusive_read(domain_Birch) { |ending_Birch| destroy(ptr_Birch, ending_Birch) };
                                            let raw_Birch = erase_slot<Relay>(vacant_Birch);
                                            finalize_domain(domain_Birch);
                                            deallocate(allocation_Birch, raw_Birch);
                                            let vacant_Maple = loan_exclusive_read(domain_Maple) { |ending_Maple| destroy(ptr_Maple, ending_Maple) };
                                            let raw_Maple = erase_slot<Relay>(vacant_Maple);
                                            finalize_domain(domain_Maple);
                                            deallocate(allocation_Maple, raw_Maple);
                                            let vacant_Cedar = loan_exclusive_read(domain_Cedar) { |ending_Cedar| destroy(ptr_Cedar, ending_Cedar) };
                                            let raw_Cedar = erase_slot<Relay>(vacant_Cedar);
                                            finalize_domain(domain_Cedar);
                                            deallocate(allocation_Cedar, raw_Cedar);
                                            let vacant_Quartz = loan_exclusive_read(domain_Quartz) { |ending_Quartz| destroy(ptr_Quartz, ending_Quartz) };
                                            let raw_Quartz = erase_slot<Relay>(vacant_Quartz);
                                            finalize_domain(domain_Quartz);
                                            deallocate(allocation_Quartz, raw_Quartz);
                                            unit
                                        },
                                    }
                                },
                            }
                        },
                    }
                },
            }
        },
    }
}
