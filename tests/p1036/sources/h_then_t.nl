struct DispatchLink { next: Option<ptr<DispatchLink>>, code: u8 }
struct CircuitLevels { idle: u8, trip: u8 }
fn record_trip(destination: ref<write,u8>, incoming: u8) -> u8 {
    let prior = replace(destination, incoming);
    return prior;
}
fn record_idle(destination: ref<write,u8>, incoming: u8) -> u8 {
    return replace(destination, incoming);
}
fn revisit_trip(destination: ref<write,u8>, incoming: u8) -> u8 {
    let prior = replace(destination, incoming);
    return prior;
}
fn detach_dispatch(edge: ref<write,Option<ptr<DispatchLink>>>,
                   pointer: ptr<DispatchLink>, allocation: Allocation,
                   domain: LifetimeDomain) -> LiveTail {
    let prior_link = replace(edge, Option<ptr<DispatchLink>>::None);
    let detached = LiveTail{owned_ptr: pointer, owned_allocation: allocation,
                            owned_domain: domain};
    return detached;
}
fn finish_dispatch(pointer: ptr<DispatchLink>, allocation: Allocation,
                   domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let readable = ref_from_ptr(read, pointer, stable);
        unit
    };
    let vacancy = loan_exclusive_read(domain) { |ending|
        destroy(pointer, ending)
    };
    let storage = erase_slot<DispatchLink>(vacancy);
    finalize_domain(domain);
    deallocate(allocation, storage);
    unit
}
fn main() -> unit {
    let levels = CircuitLevels{trip: u8(173), idle: u8(37)};
    let first_old = loan_write(levels@idle) { |slot|
        record_trip(slot, u8(94))
    };
    let second_old = loan_write(levels@trip) { |slot|
        record_idle(slot, first_old)
    };
    let third_old = loan_write(levels@idle) { |slot|
        revisit_trip(slot, second_old)
    };
    let observed_third = third_old;
    let final_idle = levels@idle;
    let final_trip = levels@trip;
    match try_allocate_one<DispatchLink>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let head_allocation = allocation;
            let head_slot = into_slot<DispatchLink>(raw);
            let head_domain = lifetime_domain();
            let head = loan_read(head_domain) { |stable|
                initialize(head_slot,
                    DispatchLink{next: Option<ptr<DispatchLink>>::None,
                                 code: u8(61)}, stable)
            };
            match try_allocate_one<DispatchLink>() {
                None => {
                    let vacancy = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let storage = erase_slot<DispatchLink>(vacancy);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, storage);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<DispatchLink>(raw);
                    let tail_domain = lifetime_domain();
                    let tail = loan_read(tail_domain) { |stable|
                        initialize(tail_slot,
                            DispatchLink{next: Option<ptr<DispatchLink>>::None,
                                         code: u8(209)}, stable)
                    };
                    let prior_empty = loan_read(head_domain) { |stable|
                        let writable = ref_from_ptr(write, head, stable);
                        replace(writable@next,
                                Option<ptr<DispatchLink>>::Some(tail))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(head_domain) { |stable|
                            let writable = ref_from_ptr(write, head, stable);
                            detach_dispatch(writable@next, tail,
                                            tail_allocation, tail_domain)
                        };
                    loan_read(owned_domain) { |stable|
                        let readable = ref_from_ptr(read, owned_ptr, stable);
                        unit
                    };
                    finish_dispatch(owned_ptr, owned_allocation, owned_domain);
                    let vacancy = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let storage = erase_slot<DispatchLink>(vacancy);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, storage);
                    unit
                },
            }
        },
    }
}
