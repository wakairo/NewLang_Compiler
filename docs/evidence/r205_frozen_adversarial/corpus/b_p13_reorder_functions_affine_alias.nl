struct Node { next: Option<ptr<Node>>, payload: u8, }

fn main() -> unit {
    match try_allocate_one<Node>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<Node>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(
                    vacant_h,
                    Node { next: Option<ptr<Node>>::None, payload: u8(1) },
                    stable_h
                )
            };

            match try_allocate_one<Node>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Node>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<Node>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(
                            vacant_t,
                            Node { next: Option<ptr<Node>>::None, payload: u8(2) },
                            stable_t
                        )
                    };

                    let old_none = loan_read(life_h) { |stable_h|
                        let root_w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(root_w@next, Option<ptr<Node>>::Some(ptr_t))
                    };
                    let seen = loan_read(life_h) { |stable_h|
                        let root_r = ref_from_ptr(read, ptr_h, stable_h);
                        read(root_r@next)
                    };
                    match seen {
                        None => { unit },
                        Some(q) => {
                            loan_read(life_t) { |stable_t|
                                let tail_r = ref_from_ptr(read, q, stable_t);
                                unit
                            }
                        },
                    };

                    let old_some = loan_read(life_h) { |stable_h|
                        let root_w2 = ref_from_ptr(write, ptr_h, stable_h);
                        replace(root_w2@next, Option<ptr<Node>>::None)
                    };

                                        let metamorphic_alloc = allocation_t;
                    let metamorphic_domain = life_t;
                    receive_and_release_tail(ptr_t, metamorphic_alloc, metamorphic_domain);

                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Node>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}

fn receive_and_release_tail(
    tail_ptr: ptr<Node>,
    tail_allocation: Allocation,
    tail_life: LifetimeDomain
) -> unit {
    loan_read(tail_life) { |stable_t|
        let root_r = ref_from_ptr(read, tail_ptr, stable_t);
        unit
    };

    let empty_t = loan_exclusive_read(tail_life) { |ending_t|
        destroy(tail_ptr, ending_t)
    };
    let full_t = erase_slot<Node>(empty_t);
    finalize_domain(tail_life);
    deallocate(tail_allocation, full_t);
    unit
}
