// P338 desk-defined w2_scheduler_rotation: short faithful non-cJSON prefix; no source acceptance claim.
struct Node {
    next: Option<ptr<Node>>,
    prev: Option<ptr<Node>>,
    child: Option<ptr<Node>>,
    payload: u8,
}

fn main() -> unit {
    match try_allocate_one<Node>() {  // independent allocation site 1: cursor_x
        None => {
            unit
        },
        Some(bundle_cursor_x) => {
            let OneBacking { allocation, raw } = bundle_cursor_x;
            let allocation_cursor_x = allocation;
            let vacant_cursor_x = into_slot<Node>(raw);
            let life_cursor_x = lifetime_domain();
            let ptr_cursor_x = loan_read(life_cursor_x) { |stable_cursor_x|
                initialize(vacant_cursor_x, Node {
                    next: Option<ptr<Node>>::None,
                    prev: Option<ptr<Node>>::None,
                    child: Option<ptr<Node>>::None,
                    payload: u8(1)
                }, stable_cursor_x)
            };
            match try_allocate_one<Node>() {  // independent allocation site 2: cursor_y
                None => {
                    let empty_cursor_x = loan_exclusive_read(life_cursor_x) { |ending_cursor_x|
                        destroy(ptr_cursor_x, ending_cursor_x)
                    };
                    let full_cursor_x = erase_slot<Node>(empty_cursor_x);
                    finalize_domain(life_cursor_x);
                    deallocate(allocation_cursor_x, full_cursor_x);
                    unit
                },
                Some(bundle_cursor_y) => {
                    let OneBacking { allocation, raw } = bundle_cursor_y;
                    let allocation_cursor_y = allocation;
                    let vacant_cursor_y = into_slot<Node>(raw);
                    let life_cursor_y = lifetime_domain();
                    let ptr_cursor_y = loan_read(life_cursor_y) { |stable_cursor_y|
                        initialize(vacant_cursor_y, Node {
                            next: Option<ptr<Node>>::None,
                            prev: Option<ptr<Node>>::None,
                            child: Option<ptr<Node>>::None,
                            payload: u8(2)
                        }, stable_cursor_y)
                    };
                    match try_allocate_one<Node>() {  // independent allocation site 3: task_b
                        None => {
                            let empty_cursor_y = loan_exclusive_read(life_cursor_y) { |ending_cursor_y|
                                destroy(ptr_cursor_y, ending_cursor_y)
                            };
                            let full_cursor_y = erase_slot<Node>(empty_cursor_y);
                            finalize_domain(life_cursor_y);
                            deallocate(allocation_cursor_y, full_cursor_y);
                            let empty_cursor_x = loan_exclusive_read(life_cursor_x) { |ending_cursor_x|
                                destroy(ptr_cursor_x, ending_cursor_x)
                            };
                            let full_cursor_x = erase_slot<Node>(empty_cursor_x);
                            finalize_domain(life_cursor_x);
                            deallocate(allocation_cursor_x, full_cursor_x);
                            unit
                        },
                        Some(bundle_task_b) => {
                            let OneBacking { allocation, raw } = bundle_task_b;
                            let allocation_task_b = allocation;
                            let vacant_task_b = into_slot<Node>(raw);
                            let life_task_b = lifetime_domain();
                            let ptr_task_b = loan_read(life_task_b) { |stable_task_b|
                                initialize(vacant_task_b, Node {
                                    next: Option<ptr<Node>>::None,
                                    prev: Option<ptr<Node>>::None,
                                    child: Option<ptr<Node>>::None,
                                    payload: u8(3)
                                }, stable_task_b)
                            };
                            match try_allocate_one<Node>() {  // independent allocation site 4: task_c
                                None => {
                                    let empty_task_b = loan_exclusive_read(life_task_b) { |ending_task_b|
                                        destroy(ptr_task_b, ending_task_b)
                                    };
                                    let full_task_b = erase_slot<Node>(empty_task_b);
                                    finalize_domain(life_task_b);
                                    deallocate(allocation_task_b, full_task_b);
                                    let empty_cursor_y = loan_exclusive_read(life_cursor_y) { |ending_cursor_y|
                                        destroy(ptr_cursor_y, ending_cursor_y)
                                    };
                                    let full_cursor_y = erase_slot<Node>(empty_cursor_y);
                                    finalize_domain(life_cursor_y);
                                    deallocate(allocation_cursor_y, full_cursor_y);
                                    let empty_cursor_x = loan_exclusive_read(life_cursor_x) { |ending_cursor_x|
                                        destroy(ptr_cursor_x, ending_cursor_x)
                                    };
                                    let full_cursor_x = erase_slot<Node>(empty_cursor_x);
                                    finalize_domain(life_cursor_x);
                                    deallocate(allocation_cursor_x, full_cursor_x);
                                    unit
                                },
                                Some(bundle_task_c) => {
                                    let OneBacking { allocation, raw } = bundle_task_c;
                                    let allocation_task_c = allocation;
                                    let vacant_task_c = into_slot<Node>(raw);
                                    let life_task_c = lifetime_domain();
                                    let ptr_task_c = loan_read(life_task_c) { |stable_task_c|
                                        initialize(vacant_task_c, Node {
                                            next: Option<ptr<Node>>::None,
                                            prev: Option<ptr<Node>>::None,
                                            child: Option<ptr<Node>>::None,
                                            payload: u8(4)
                                        }, stable_task_c)
                                    };
                                    match try_allocate_one<Node>() {  // independent allocation site 5: task_d
                                        None => {
                                            let empty_task_c = loan_exclusive_read(life_task_c) { |ending_task_c|
                                                destroy(ptr_task_c, ending_task_c)
                                            };
                                            let full_task_c = erase_slot<Node>(empty_task_c);
                                            finalize_domain(life_task_c);
                                            deallocate(allocation_task_c, full_task_c);
                                            let empty_task_b = loan_exclusive_read(life_task_b) { |ending_task_b|
                                                destroy(ptr_task_b, ending_task_b)
                                            };
                                            let full_task_b = erase_slot<Node>(empty_task_b);
                                            finalize_domain(life_task_b);
                                            deallocate(allocation_task_b, full_task_b);
                                            let empty_cursor_y = loan_exclusive_read(life_cursor_y) { |ending_cursor_y|
                                                destroy(ptr_cursor_y, ending_cursor_y)
                                            };
                                            let full_cursor_y = erase_slot<Node>(empty_cursor_y);
                                            finalize_domain(life_cursor_y);
                                            deallocate(allocation_cursor_y, full_cursor_y);
                                            let empty_cursor_x = loan_exclusive_read(life_cursor_x) { |ending_cursor_x|
                                                destroy(ptr_cursor_x, ending_cursor_x)
                                            };
                                            let full_cursor_x = erase_slot<Node>(empty_cursor_x);
                                            finalize_domain(life_cursor_x);
                                            deallocate(allocation_cursor_x, full_cursor_x);
                                            unit
                                        },
                                        Some(bundle_task_d) => {
                                            let OneBacking { allocation, raw } = bundle_task_d;
                                            let allocation_task_d = allocation;
                                            let vacant_task_d = into_slot<Node>(raw);
                                            let life_task_d = lifetime_domain();
                                            let ptr_task_d = loan_read(life_task_d) { |stable_task_d|
                                                initialize(vacant_task_d, Node {
                                                    next: Option<ptr<Node>>::None,
                                                    prev: Option<ptr<Node>>::None,
                                                    child: Option<ptr<Node>>::None,
                                                    payload: u8(5)
                                                }, stable_task_d)
                                            };
                                            let old_1 = loan_read(life_cursor_x) { |stable_1|
                                                let write_1 = ref_from_ptr(write,ptr_cursor_x,stable_1);
                                                replace(write_1@next,Option<ptr<Node>>::Some(ptr_task_b))
                                            };
                                            let old_2 = loan_read(life_cursor_y) { |stable_2|
                                                let write_2 = ref_from_ptr(write,ptr_cursor_y,stable_2);
                                                replace(write_2@prev,Option<ptr<Node>>::Some(ptr_task_d))
                                            };
                                            let old_3 = loan_read(life_cursor_x) { |stable_3|
                                                let write_3 = ref_from_ptr(write,ptr_cursor_x,stable_3);
                                                replace(write_3@next,Option<ptr<Node>>::Some(ptr_task_c))
                                            };
                                            let old_4 = loan_read(life_cursor_y) { |stable_4|
                                                let write_4 = ref_from_ptr(write,ptr_cursor_y,stable_4);
                                                replace(write_4@prev,Option<ptr<Node>>::None)
                                            };
                                            let empty_task_d=loan_exclusive_read(life_task_d){|ending_task_d|destroy(ptr_task_d,ending_task_d)};
                                            let full_task_d=erase_slot<Node>(empty_task_d);
                                            finalize_domain(life_task_d); deallocate(allocation_task_d,full_task_d);
                                            let empty_task_c=loan_exclusive_read(life_task_c){|ending_task_c|destroy(ptr_task_c,ending_task_c)};
                                            let full_task_c=erase_slot<Node>(empty_task_c);
                                            finalize_domain(life_task_c); deallocate(allocation_task_c,full_task_c);
                                            let empty_task_b=loan_exclusive_read(life_task_b){|ending_task_b|destroy(ptr_task_b,ending_task_b)};
                                            let full_task_b=erase_slot<Node>(empty_task_b);
                                            finalize_domain(life_task_b); deallocate(allocation_task_b,full_task_b);
                                            let empty_cursor_y=loan_exclusive_read(life_cursor_y){|ending_cursor_y|destroy(ptr_cursor_y,ending_cursor_y)};
                                            let full_cursor_y=erase_slot<Node>(empty_cursor_y);
                                            finalize_domain(life_cursor_y); deallocate(allocation_cursor_y,full_cursor_y);
                                            let empty_cursor_x=loan_exclusive_read(life_cursor_x){|ending_cursor_x|destroy(ptr_cursor_x,ending_cursor_x)};
                                            let full_cursor_x=erase_slot<Node>(empty_cursor_x);
                                            finalize_domain(life_cursor_x); deallocate(allocation_cursor_x,full_cursor_x);
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
