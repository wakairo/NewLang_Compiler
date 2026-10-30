struct ThermoPair { warm: u8, cold: u8 }
fn update_probe(address: ref<write,u8>, incoming: u8) -> u8 {
    let previous = replace(address, incoming);
    let alias_a = previous;
    let alias_b = alias_a;
    alias_b
}
fn relay_probe(address: ref<write,u8>, incoming: u8) -> u8 {
    return replace(address, incoming);
}
struct DispatchCell { next: Option<ptr<DispatchCell>>, hopcode: u8 }
fn handoff_cell(edge: ref<write,Option<ptr<DispatchCell>>>,
                target: ptr<DispatchCell>, token: Allocation,
                lifetime: LifetimeDomain) -> LiveTail {
    let previous_link = replace(edge, Option<ptr<DispatchCell>>::None);
    let detached = LiveTail{owned_ptr: target, owned_allocation: token,
                            owned_domain: lifetime};
    return detached;
}
fn finish_cell(target: ptr<DispatchCell>, token: Allocation,
               lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |held|
        let readable = ref_from_ptr(read, target, held);
        unit
    };
    let vacant = loan_exclusive_read(lifetime) { |ending|
        destroy(target, ending)
    };
    let bytes = erase_slot<DispatchCell>(vacant);
    finalize_domain(lifetime);
    deallocate(token, bytes);
    unit
}
fn main() -> unit {
    let record = ThermoPair{warm: u8(188), cold: u8(43)};
    let displaced = loan_write(record@cold) { |cell|
        update_probe(cell, u8(107))
    };
    let other_displaced = loan_write(record@warm) { |cell|
        relay_probe(cell, displaced)
    };
    let observed_first = displaced;
    let observed_second = other_displaced;
    let after_falling = record@cold;
    let after_rising = record@warm;
    match try_allocate_one<DispatchCell>() {
        None => { unit },
        Some(first_bundle) => {
            let OneBacking { allocation, raw } = first_bundle;
            let first_allocation = allocation;
            let first_slot = into_slot<DispatchCell>(raw);
            let first_domain = lifetime_domain();
            let first = loan_read(first_domain) { |held|
                initialize(first_slot,
                    DispatchCell{next: Option<ptr<DispatchCell>>::None,
                               hopcode: u8(59)}, held)
            };
            match try_allocate_one<DispatchCell>() {
                None => {
                    let first_vacant = loan_exclusive_read(first_domain) { |ending|
                        destroy(first, ending)
                    };
                    let first_bytes = erase_slot<DispatchCell>(first_vacant);
                    finalize_domain(first_domain);
                    deallocate(first_allocation, first_bytes);
                    unit
                },
                Some(second_bundle) => {
                    let OneBacking { allocation, raw } = second_bundle;
                    let second_allocation = allocation;
                    let second_slot = into_slot<DispatchCell>(raw);
                    let second_domain = lifetime_domain();
                    let second = loan_read(second_domain) { |held|
                        initialize(second_slot,
                            DispatchCell{next: Option<ptr<DispatchCell>>::None,
                                       hopcode: u8(181)}, held)
                    };
                    let previous_empty = loan_read(first_domain) { |held|
                        let writable = ref_from_ptr(write, first, held);
                        replace(writable@next,
                                Option<ptr<DispatchCell>>::Some(second))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(first_domain) { |held|
                            let writable = ref_from_ptr(write, first, held);
                            handoff_cell(writable@next, second,
                                         second_allocation, second_domain)
                        };
                    loan_read(owned_domain) { |held|
                        let readable = ref_from_ptr(read, owned_ptr, held);
                        unit
                    };
                    finish_cell(owned_ptr, owned_allocation, owned_domain);
                    let first_vacant = loan_exclusive_read(first_domain) { |ending|
                        destroy(first, ending)
                    };
                    let first_bytes = erase_slot<DispatchCell>(first_vacant);
                    finalize_domain(first_domain);
                    deallocate(first_allocation, first_bytes);
                    unit
                },
            }
        },
    }
}
