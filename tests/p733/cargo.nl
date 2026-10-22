struct Segment { chain: Option<ptr<Segment>>, marker: u8, }

fn unlink_segment(
    head_link: ref<write, Option<ptr<Segment>>>,
    p: ptr<Segment>,
    a: Allocation,
    d: LifetimeDomain
) -> LiveTail {
    let old_link = replace(
        head_link, Option<ptr<Segment>>::None);
    let cargo = LiveTail {
        owned_domain: d,
        owned_allocation: a,
        owned_ptr: p
    };
    return cargo;
}

fn release_segment(
    tail_ptr: ptr<Segment>,
    tail_allocation: Allocation,
    tail_life: LifetimeDomain
) -> unit {
    loan_read(tail_life) { |stable_t|
        let tail_r = ref_from_ptr(read, tail_ptr, stable_t);
        unit
    };
    let empty_t = loan_exclusive_read(tail_life) { |ending_t|
        destroy(tail_ptr, ending_t)
    };
    let full_t = erase_slot<Segment>(empty_t);
    finalize_domain(tail_life);
    deallocate(tail_allocation, full_t);
    unit
}

fn main() -> unit {
    match try_allocate_one<Segment>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<Segment>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(vacant_h,
                    Segment { chain: Option<ptr<Segment>>::None, marker: u8(29) },
                    stable_h)
            };

            match try_allocate_one<Segment>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Segment>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<Segment>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(vacant_t,
                            Segment { chain: Option<ptr<Segment>>::None,
                                   marker: u8(211) },
                            stable_t)
                    };

                    let old_none = loan_read(life_h) { |stable_h|
                        let head_w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(head_w@chain,
                            Option<ptr<Segment>>::Some(ptr_t))
                    };
                    let seen = loan_read(life_h) { |stable_h|
                        let head_r = ref_from_ptr(read, ptr_h, stable_h);
                        read(head_r@chain)
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
                    let LiveTail {
                        owned_ptr,
                        owned_allocation,
                        owned_domain
                    } = loan_read(life_h) { |stable_h|
                        let head_w2 = ref_from_ptr(write, ptr_h, stable_h);
                        unlink_segment(
                            head_w2@chain, ptr_t, allocation_t, life_t)
                    };

                    loan_read(owned_domain) { |stable_t|
                        let live_t = ref_from_ptr(read, owned_ptr, stable_t);
                        unit
                    };

                    release_segment(
                        owned_ptr, owned_allocation, owned_domain);

                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Segment>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
