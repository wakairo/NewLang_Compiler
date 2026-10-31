struct SwitchSample { falling: u8, rising: u8 }
fn rotate_sample(destination: ref<write,u8>, replacement: u8) -> u8 {
    let previous = replace(destination, replacement);
    return u8(83);
}
fn forward_sample(destination: ref<write,u8>, replacement: u8) -> u8 {
    return replace(destination, replacement);
}
struct CourierHop { next: Option<ptr<CourierHop>>, marker: u8 }
fn separate_hop(edge: ref<write,Option<ptr<CourierHop>>>,
                target: ptr<CourierHop>, token: Allocation,
                lifetime: LifetimeDomain) -> LiveTail {
    let previous_link = replace(edge, Option<ptr<CourierHop>>::None);
    let detached = LiveTail{owned_ptr: target, owned_allocation: token,
                            owned_domain: lifetime};
    return detached;
}
fn release_hop(target: ptr<CourierHop>, token: Allocation,
               lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |held|
        let readable = ref_from_ptr(read, target, held);
        unit
    };
    let vacant = loan_exclusive_read(lifetime) { |ending|
        destroy(target, ending)
    };
    let bytes = erase_slot<CourierHop>(vacant);
    finalize_domain(lifetime);
    deallocate(token, bytes);
    unit
}
fn main() -> unit {
    let sample = SwitchSample{rising: u8(211), falling: u8(83)};
    let displaced = loan_write(sample@falling) { |cell|
        rotate_sample(cell, u8(146))
    };
    let other_displaced = loan_write(sample@rising) { |cell|
        forward_sample(cell, displaced)
    };
    let observed_first = displaced;
    let observed_second = other_displaced;
    let after_falling = sample@falling;
    let after_rising = sample@rising;
    match try_allocate_one<CourierHop>() {
        None => { unit },
        Some(first_bundle) => {
            let OneBacking { allocation, raw } = first_bundle;
            let first_allocation = allocation;
            let first_slot = into_slot<CourierHop>(raw);
            let first_domain = lifetime_domain();
            let first = loan_read(first_domain) { |held|
                initialize(first_slot,
                    CourierHop{next: Option<ptr<CourierHop>>::None,
                               marker: u8(71)}, held)
            };
            match try_allocate_one<CourierHop>() {
                None => {
                    let first_vacant = loan_exclusive_read(first_domain) { |ending|
                        destroy(first, ending)
                    };
                    let first_bytes = erase_slot<CourierHop>(first_vacant);
                    finalize_domain(first_domain);
                    deallocate(first_allocation, first_bytes);
                    unit
                },
                Some(second_bundle) => {
                    let OneBacking { allocation, raw } = second_bundle;
                    let second_allocation = allocation;
                    let second_slot = into_slot<CourierHop>(raw);
                    let second_domain = lifetime_domain();
                    let second = loan_read(second_domain) { |held|
                        initialize(second_slot,
                            CourierHop{next: Option<ptr<CourierHop>>::None,
                                       marker: u8(193)}, held)
                    };
                    let previous_empty = loan_read(first_domain) { |held|
                        let writable = ref_from_ptr(write, first, held);
                        replace(writable@next,
                                Option<ptr<CourierHop>>::Some(second))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(first_domain) { |held|
                            let writable = ref_from_ptr(write, first, held);
                            separate_hop(writable@next, second,
                                         second_allocation, second_domain)
                        };
                    loan_read(owned_domain) { |held|
                        let readable = ref_from_ptr(read, owned_ptr, held);
                        unit
                    };
                    release_hop(owned_ptr, owned_allocation, owned_domain);
                    let first_vacant = loan_exclusive_read(first_domain) { |ending|
                        destroy(first, ending)
                    };
                    let first_bytes = erase_slot<CourierHop>(first_vacant);
                    finalize_domain(first_domain);
                    deallocate(first_allocation, first_bytes);
                    unit
                },
            }
        },
    }
}
