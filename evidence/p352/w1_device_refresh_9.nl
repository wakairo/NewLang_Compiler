// P338 desk-defined w1_device_refresh: short faithful non-cJSON prefix; no source acceptance claim.
struct Node {
    next: Option<ptr<Node>>,
    prev: Option<ptr<Node>>,
    child: Option<ptr<Node>>,
    payload: u8,
}

fn main() -> unit {
    match try_allocate_one<Node>() {  // independent allocation site 1: directory
        None => {
            unit
        },
        Some(bundle_directory) => {
            let OneBacking { allocation, raw } = bundle_directory;
            let allocation_directory = allocation;
            let vacant_directory = into_slot<Node>(raw);
            let life_directory = lifetime_domain();
            let ptr_directory = loan_read(life_directory) { |stable_directory|
                initialize(vacant_directory, Node {
                    next: Option<ptr<Node>>::None,
                    prev: Option<ptr<Node>>::None,
                    child: Option<ptr<Node>>::None,
                    payload: u8(1)
                }, stable_directory)
            };
            match try_allocate_one<Node>() {  // independent allocation site 2: slot2
                None => {
                    let empty_directory = loan_exclusive_read(life_directory) { |ending_directory|
                        destroy(ptr_directory, ending_directory)
                    };
                    let full_directory = erase_slot<Node>(empty_directory);
                    finalize_domain(life_directory);
                    deallocate(allocation_directory, full_directory);
                    unit
                },
                Some(bundle_slot2) => {
                    let OneBacking { allocation, raw } = bundle_slot2;
                    let allocation_slot2 = allocation;
                    let vacant_slot2 = into_slot<Node>(raw);
                    let life_slot2 = lifetime_domain();
                    let ptr_slot2 = loan_read(life_slot2) { |stable_slot2|
                        initialize(vacant_slot2, Node {
                            next: Option<ptr<Node>>::None,
                            prev: Option<ptr<Node>>::None,
                            child: Option<ptr<Node>>::None,
                            payload: u8(2)
                        }, stable_slot2)
                    };
                    match try_allocate_one<Node>() {  // independent allocation site 3: slot3
                        None => {
                            let empty_slot2 = loan_exclusive_read(life_slot2) { |ending_slot2|
                                destroy(ptr_slot2, ending_slot2)
                            };
                            let full_slot2 = erase_slot<Node>(empty_slot2);
                            finalize_domain(life_slot2);
                            deallocate(allocation_slot2, full_slot2);
                            let empty_directory = loan_exclusive_read(life_directory) { |ending_directory|
                                destroy(ptr_directory, ending_directory)
                            };
                            let full_directory = erase_slot<Node>(empty_directory);
                            finalize_domain(life_directory);
                            deallocate(allocation_directory, full_directory);
                            unit
                        },
                        Some(bundle_slot3) => {
                            let OneBacking { allocation, raw } = bundle_slot3;
                            let allocation_slot3 = allocation;
                            let vacant_slot3 = into_slot<Node>(raw);
                            let life_slot3 = lifetime_domain();
                            let ptr_slot3 = loan_read(life_slot3) { |stable_slot3|
                                initialize(vacant_slot3, Node {
                                    next: Option<ptr<Node>>::None,
                                    prev: Option<ptr<Node>>::None,
                                    child: Option<ptr<Node>>::None,
                                    payload: u8(3)
                                }, stable_slot3)
                            };
                            match try_allocate_one<Node>() {  // independent allocation site 4: slot4
                                None => {
                                    let empty_slot3 = loan_exclusive_read(life_slot3) { |ending_slot3|
                                        destroy(ptr_slot3, ending_slot3)
                                    };
                                    let full_slot3 = erase_slot<Node>(empty_slot3);
                                    finalize_domain(life_slot3);
                                    deallocate(allocation_slot3, full_slot3);
                                    let empty_slot2 = loan_exclusive_read(life_slot2) { |ending_slot2|
                                        destroy(ptr_slot2, ending_slot2)
                                    };
                                    let full_slot2 = erase_slot<Node>(empty_slot2);
                                    finalize_domain(life_slot2);
                                    deallocate(allocation_slot2, full_slot2);
                                    let empty_directory = loan_exclusive_read(life_directory) { |ending_directory|
                                        destroy(ptr_directory, ending_directory)
                                    };
                                    let full_directory = erase_slot<Node>(empty_directory);
                                    finalize_domain(life_directory);
                                    deallocate(allocation_directory, full_directory);
                                    unit
                                },
                                Some(bundle_slot4) => {
                                    let OneBacking { allocation, raw } = bundle_slot4;
                                    let allocation_slot4 = allocation;
                                    let vacant_slot4 = into_slot<Node>(raw);
                                    let life_slot4 = lifetime_domain();
                                    let ptr_slot4 = loan_read(life_slot4) { |stable_slot4|
                                        initialize(vacant_slot4, Node {
                                            next: Option<ptr<Node>>::None,
                                            prev: Option<ptr<Node>>::None,
                                            child: Option<ptr<Node>>::None,
                                            payload: u8(4)
                                        }, stable_slot4)
                                    };
                                    match try_allocate_one<Node>() {  // independent allocation site 5: slot5
                                        None => {
                                            let empty_slot4 = loan_exclusive_read(life_slot4) { |ending_slot4|
                                                destroy(ptr_slot4, ending_slot4)
                                            };
                                            let full_slot4 = erase_slot<Node>(empty_slot4);
                                            finalize_domain(life_slot4);
                                            deallocate(allocation_slot4, full_slot4);
                                            let empty_slot3 = loan_exclusive_read(life_slot3) { |ending_slot3|
                                                destroy(ptr_slot3, ending_slot3)
                                            };
                                            let full_slot3 = erase_slot<Node>(empty_slot3);
                                            finalize_domain(life_slot3);
                                            deallocate(allocation_slot3, full_slot3);
                                            let empty_slot2 = loan_exclusive_read(life_slot2) { |ending_slot2|
                                                destroy(ptr_slot2, ending_slot2)
                                            };
                                            let full_slot2 = erase_slot<Node>(empty_slot2);
                                            finalize_domain(life_slot2);
                                            deallocate(allocation_slot2, full_slot2);
                                            let empty_directory = loan_exclusive_read(life_directory) { |ending_directory|
                                                destroy(ptr_directory, ending_directory)
                                            };
                                            let full_directory = erase_slot<Node>(empty_directory);
                                            finalize_domain(life_directory);
                                            deallocate(allocation_directory, full_directory);
                                            unit
                                        },
                                        Some(bundle_slot5) => {
                                            let OneBacking { allocation, raw } = bundle_slot5;
                                            let allocation_slot5 = allocation;
                                            let vacant_slot5 = into_slot<Node>(raw);
                                            let life_slot5 = lifetime_domain();
                                            let ptr_slot5 = loan_read(life_slot5) { |stable_slot5|
                                                initialize(vacant_slot5, Node {
                                                    next: Option<ptr<Node>>::None,
                                                    prev: Option<ptr<Node>>::None,
                                                    child: Option<ptr<Node>>::None,
                                                    payload: u8(5)
                                                }, stable_slot5)
                                            };
                                            let old_1 = loan_read(life_directory) { |stable_1|
                                                let write_1 = ref_from_ptr(write,ptr_directory,stable_1);
                                                replace(write_1@child,Option<ptr<Node>>::Some(ptr_slot2))
                                            };
                                            let old_2 = loan_read(life_directory) { |stable_2|
                                                let write_2 = ref_from_ptr(write,ptr_directory,stable_2);
                                                replace(write_2@child,Option<ptr<Node>>::None)
                                            };
                                            let old_3 = loan_read(life_directory) { |stable_3|
                                                let write_3 = ref_from_ptr(write,ptr_directory,stable_3);
                                                replace(write_3@child,Option<ptr<Node>>::Some(ptr_slot3))
                                            };
                                            let old_4 = loan_read(life_directory) { |stable_4|
                                                let write_4 = ref_from_ptr(write,ptr_directory,stable_4);
                                                replace(write_4@child,Option<ptr<Node>>::None)
                                            };
                                            let old_5 = loan_read(life_directory) { |stable_5|
                                                let write_5 = ref_from_ptr(write,ptr_directory,stable_5);
                                                replace(write_5@child,Option<ptr<Node>>::Some(ptr_slot4))
                                            };
                                            let old_6 = loan_read(life_directory) { |stable_6|
                                                let write_6 = ref_from_ptr(write,ptr_directory,stable_6);
                                                replace(write_6@child,Option<ptr<Node>>::None)
                                            };
                                            let old_7 = loan_read(life_directory) { |stable_7|
                                                let write_7 = ref_from_ptr(write,ptr_directory,stable_7);
                                                replace(write_7@child,Option<ptr<Node>>::Some(ptr_slot5))
                                            };
                                            let old_8 = loan_read(life_directory) { |stable_8|
                                                let write_8 = ref_from_ptr(write,ptr_directory,stable_8);
                                                replace(write_8@child,Option<ptr<Node>>::None)
                                            };
                                            let old_9 = loan_read(life_directory) { |stable_9|
                                                let write_9 = ref_from_ptr(write,ptr_directory,stable_9);
                                                replace(write_9@child,Option<ptr<Node>>::Some(ptr_slot2))
                                            };
                                            let empty_slot5=loan_exclusive_read(life_slot5){|ending_slot5|destroy(ptr_slot5,ending_slot5)};
                                            let full_slot5=erase_slot<Node>(empty_slot5);
                                            finalize_domain(life_slot5); deallocate(allocation_slot5,full_slot5);
                                            let empty_slot4=loan_exclusive_read(life_slot4){|ending_slot4|destroy(ptr_slot4,ending_slot4)};
                                            let full_slot4=erase_slot<Node>(empty_slot4);
                                            finalize_domain(life_slot4); deallocate(allocation_slot4,full_slot4);
                                            let empty_slot3=loan_exclusive_read(life_slot3){|ending_slot3|destroy(ptr_slot3,ending_slot3)};
                                            let full_slot3=erase_slot<Node>(empty_slot3);
                                            finalize_domain(life_slot3); deallocate(allocation_slot3,full_slot3);
                                            let empty_slot2=loan_exclusive_read(life_slot2){|ending_slot2|destroy(ptr_slot2,ending_slot2)};
                                            let full_slot2=erase_slot<Node>(empty_slot2);
                                            finalize_domain(life_slot2); deallocate(allocation_slot2,full_slot2);
                                            let empty_directory=loan_exclusive_read(life_directory){|ending_directory|destroy(ptr_directory,ending_directory)};
                                            let full_directory=erase_slot<Node>(empty_directory);
                                            finalize_domain(life_directory); deallocate(allocation_directory,full_directory);
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
