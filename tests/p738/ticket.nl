struct Ticket { edge: Option<ptr<Ticket>>, code: u8, }

fn unlink_ticket(
    head_link: ref<write, Option<ptr<Ticket>>>,
    p: ptr<Ticket>,
    a: Allocation,
    d: LifetimeDomain
) -> LiveTail {
    let old_link = replace(
        head_link, Option<ptr<Ticket>>::None);
    let transported = LiveTail {
        owned_domain: d,
        owned_allocation: a,
        owned_ptr: p
    };
    return transported;
}

fn dispose_ticket(
    tail_ptr: ptr<Ticket>,
    tail_allocation: Allocation,
    tail_life: LifetimeDomain
) -> unit {
    let empty_t = loan_exclusive_read(tail_life) { |ending_t|
        destroy(tail_ptr, ending_t)
    };
    let full_t = erase_slot<Ticket>(empty_t);
    finalize_domain(tail_life);
    deallocate(tail_allocation, full_t);
    unit
}

fn main() -> unit {
    match try_allocate_one<Ticket>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<Ticket>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(vacant_h,
                    Ticket { edge: Option<ptr<Ticket>>::None, code: u8(23) },
                    stable_h)
            };

            match try_allocate_one<Ticket>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Ticket>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<Ticket>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(vacant_t,
                            Ticket { edge: Option<ptr<Ticket>>::None,
                                   code: u8(199) },
                            stable_t)
                    };

                    let old_none = loan_read(life_h) { |stable_h|
                        let head_w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(head_w@edge,
                            Option<ptr<Ticket>>::Some(ptr_t))
                    };
                    let LiveTail {
                        owned_ptr,
                        owned_allocation,
                        owned_domain
                    } = loan_read(life_h) { |stable_h|
                        let head_w2 = ref_from_ptr(write, ptr_h, stable_h);
                        unlink_ticket(
                            head_w2@edge, ptr_t, allocation_t, life_t)
                    };

                    loan_read(owned_domain) { |stable_t|
                        let live_t = ref_from_ptr(read, owned_ptr, stable_t);
                        unit
                    };

                    dispose_ticket(
                        owned_ptr, owned_allocation, owned_domain);

                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Ticket>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
