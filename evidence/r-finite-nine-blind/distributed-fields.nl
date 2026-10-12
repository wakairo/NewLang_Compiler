// R359 independently authored physical-five-root witness; not a list policy.
struct Beacon {
    next: Option<ptr<Beacon>>,
    prev: Option<ptr<Beacon>>,
    child: Option<ptr<Beacon>>,
    payload: u8,
}
fn main() -> unit {
    match try_allocate_one<Beacon>() {
        None => {
            unit
        },
        Some(grant_North) => {
            let OneBacking { allocation, raw } = grant_North;
            let allocation_North = allocation;
            let slot_North = into_slot<Beacon>(raw);
            let domain_North = lifetime_domain();
            let ptr_North = loan_read(domain_North) { |stable_init_North|
                initialize(slot_North, Beacon {
                    next: Option<ptr<Beacon>>::None,
                    prev: Option<ptr<Beacon>>::None,
                    child: Option<ptr<Beacon>>::None,
                    payload: u8(31)
                }, stable_init_North)
            };
            match try_allocate_one<Beacon>() {
                None => {
                    let vacant_North = loan_exclusive_read(domain_North) { |ending_North| destroy(ptr_North, ending_North) };
                    let raw_North = erase_slot<Beacon>(vacant_North);
                    finalize_domain(domain_North);
                    deallocate(allocation_North, raw_North);
                    unit
                },
                Some(grant_East) => {
                    let OneBacking { allocation, raw } = grant_East;
                    let allocation_East = allocation;
                    let slot_East = into_slot<Beacon>(raw);
                    let domain_East = lifetime_domain();
                    let ptr_East = loan_read(domain_East) { |stable_init_East|
                        initialize(slot_East, Beacon {
                            next: Option<ptr<Beacon>>::None,
                            prev: Option<ptr<Beacon>>::None,
                            child: Option<ptr<Beacon>>::None,
                            payload: u8(32)
                        }, stable_init_East)
                    };
                    match try_allocate_one<Beacon>() {
                        None => {
                            let vacant_East = loan_exclusive_read(domain_East) { |ending_East| destroy(ptr_East, ending_East) };
                            let raw_East = erase_slot<Beacon>(vacant_East);
                            finalize_domain(domain_East);
                            deallocate(allocation_East, raw_East);
                            let vacant_North = loan_exclusive_read(domain_North) { |ending_North| destroy(ptr_North, ending_North) };
                            let raw_North = erase_slot<Beacon>(vacant_North);
                            finalize_domain(domain_North);
                            deallocate(allocation_North, raw_North);
                            unit
                        },
                        Some(grant_South) => {
                            let OneBacking { allocation, raw } = grant_South;
                            let allocation_South = allocation;
                            let slot_South = into_slot<Beacon>(raw);
                            let domain_South = lifetime_domain();
                            let ptr_South = loan_read(domain_South) { |stable_init_South|
                                initialize(slot_South, Beacon {
                                    next: Option<ptr<Beacon>>::None,
                                    prev: Option<ptr<Beacon>>::None,
                                    child: Option<ptr<Beacon>>::None,
                                    payload: u8(33)
                                }, stable_init_South)
                            };
                            match try_allocate_one<Beacon>() {
                                None => {
                                    let vacant_South = loan_exclusive_read(domain_South) { |ending_South| destroy(ptr_South, ending_South) };
                                    let raw_South = erase_slot<Beacon>(vacant_South);
                                    finalize_domain(domain_South);
                                    deallocate(allocation_South, raw_South);
                                    let vacant_East = loan_exclusive_read(domain_East) { |ending_East| destroy(ptr_East, ending_East) };
                                    let raw_East = erase_slot<Beacon>(vacant_East);
                                    finalize_domain(domain_East);
                                    deallocate(allocation_East, raw_East);
                                    let vacant_North = loan_exclusive_read(domain_North) { |ending_North| destroy(ptr_North, ending_North) };
                                    let raw_North = erase_slot<Beacon>(vacant_North);
                                    finalize_domain(domain_North);
                                    deallocate(allocation_North, raw_North);
                                    unit
                                },
                                Some(grant_West) => {
                                    let OneBacking { allocation, raw } = grant_West;
                                    let allocation_West = allocation;
                                    let slot_West = into_slot<Beacon>(raw);
                                    let domain_West = lifetime_domain();
                                    let ptr_West = loan_read(domain_West) { |stable_init_West|
                                        initialize(slot_West, Beacon {
                                            next: Option<ptr<Beacon>>::None,
                                            prev: Option<ptr<Beacon>>::None,
                                            child: Option<ptr<Beacon>>::None,
                                            payload: u8(34)
                                        }, stable_init_West)
                                    };
                                    match try_allocate_one<Beacon>() {
                                        None => {
                                            let vacant_West = loan_exclusive_read(domain_West) { |ending_West| destroy(ptr_West, ending_West) };
                                            let raw_West = erase_slot<Beacon>(vacant_West);
                                            finalize_domain(domain_West);
                                            deallocate(allocation_West, raw_West);
                                            let vacant_South = loan_exclusive_read(domain_South) { |ending_South| destroy(ptr_South, ending_South) };
                                            let raw_South = erase_slot<Beacon>(vacant_South);
                                            finalize_domain(domain_South);
                                            deallocate(allocation_South, raw_South);
                                            let vacant_East = loan_exclusive_read(domain_East) { |ending_East| destroy(ptr_East, ending_East) };
                                            let raw_East = erase_slot<Beacon>(vacant_East);
                                            finalize_domain(domain_East);
                                            deallocate(allocation_East, raw_East);
                                            let vacant_North = loan_exclusive_read(domain_North) { |ending_North| destroy(ptr_North, ending_North) };
                                            let raw_North = erase_slot<Beacon>(vacant_North);
                                            finalize_domain(domain_North);
                                            deallocate(allocation_North, raw_North);
                                            unit
                                        },
                                        Some(grant_Center) => {
                                            let OneBacking { allocation, raw } = grant_Center;
                                            let allocation_Center = allocation;
                                            let slot_Center = into_slot<Beacon>(raw);
                                            let domain_Center = lifetime_domain();
                                            let ptr_Center = loan_read(domain_Center) { |stable_init_Center|
                                                initialize(slot_Center, Beacon {
                                                    next: Option<ptr<Beacon>>::None,
                                                    prev: Option<ptr<Beacon>>::None,
                                                    child: Option<ptr<Beacon>>::None,
                                                    payload: u8(35)
                                                }, stable_init_Center)
                                            };
                                            let prior_0 = loan_read(domain_Center) { |stable_0|
                                                let scoped_0 = ref_from_ptr(write, ptr_Center, stable_0);
                                                replace(scoped_0@child, Option<ptr<Beacon>>::Some(ptr_North))
                                            };
                                            let prior_1 = loan_read(domain_West) { |stable_1|
                                                let scoped_1 = ref_from_ptr(write, ptr_West, stable_1);
                                                replace(scoped_1@next, Option<ptr<Beacon>>::Some(ptr_South))
                                            };
                                            let prior_2 = loan_read(domain_South) { |stable_2|
                                                let scoped_2 = ref_from_ptr(write, ptr_South, stable_2);
                                                replace(scoped_2@prev, Option<ptr<Beacon>>::Some(ptr_East))
                                            };
                                            let prior_3 = loan_read(domain_East) { |stable_3|
                                                let scoped_3 = ref_from_ptr(write, ptr_East, stable_3);
                                                replace(scoped_3@child, Option<ptr<Beacon>>::Some(ptr_West))
                                            };
                                            let prior_4 = loan_read(domain_North) { |stable_4|
                                                let scoped_4 = ref_from_ptr(write, ptr_North, stable_4);
                                                replace(scoped_4@prev, Option<ptr<Beacon>>::Some(ptr_Center))
                                            };
                                            let prior_5 = loan_read(domain_Center) { |stable_5|
                                                let scoped_5 = ref_from_ptr(write, ptr_Center, stable_5);
                                                replace(scoped_5@next, Option<ptr<Beacon>>::Some(ptr_South))
                                            };
                                            let prior_6 = loan_read(domain_South) { |stable_6|
                                                let scoped_6 = ref_from_ptr(write, ptr_South, stable_6);
                                                replace(scoped_6@child, Option<ptr<Beacon>>::Some(ptr_North))
                                            };
                                            let prior_7 = loan_read(domain_West) { |stable_7|
                                                let scoped_7 = ref_from_ptr(write, ptr_West, stable_7);
                                                replace(scoped_7@prev, Option<ptr<Beacon>>::Some(ptr_Center))
                                            };
                                            let prior_8 = loan_read(domain_East) { |stable_8|
                                                let scoped_8 = ref_from_ptr(write, ptr_East, stable_8);
                                                replace(scoped_8@next, Option<ptr<Beacon>>::Some(ptr_North))
                                            };
                                            let vacant_Center = loan_exclusive_read(domain_Center) { |ending_Center| destroy(ptr_Center, ending_Center) };
                                            let raw_Center = erase_slot<Beacon>(vacant_Center);
                                            finalize_domain(domain_Center);
                                            deallocate(allocation_Center, raw_Center);
                                            let vacant_West = loan_exclusive_read(domain_West) { |ending_West| destroy(ptr_West, ending_West) };
                                            let raw_West = erase_slot<Beacon>(vacant_West);
                                            finalize_domain(domain_West);
                                            deallocate(allocation_West, raw_West);
                                            let vacant_South = loan_exclusive_read(domain_South) { |ending_South| destroy(ptr_South, ending_South) };
                                            let raw_South = erase_slot<Beacon>(vacant_South);
                                            finalize_domain(domain_South);
                                            deallocate(allocation_South, raw_South);
                                            let vacant_East = loan_exclusive_read(domain_East) { |ending_East| destroy(ptr_East, ending_East) };
                                            let raw_East = erase_slot<Beacon>(vacant_East);
                                            finalize_domain(domain_East);
                                            deallocate(allocation_East, raw_East);
                                            let vacant_North = loan_exclusive_read(domain_North) { |ending_North| destroy(ptr_North, ending_North) };
                                            let raw_North = erase_slot<Beacon>(vacant_North);
                                            finalize_domain(domain_North);
                                            deallocate(allocation_North, raw_North);
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
