struct Counters { final: u8, initial: u8 }

fn shift_value(slot: ref<write,u8>, incoming: u8) -> u8 {
    return replace(slot, incoming);
}

struct LinkCell { link: Option<ptr<LinkCell>>, payload: u8 }

fn take_tail(
    edge: ref<write,Option<ptr<LinkCell>>>,
    tail: ptr<LinkCell>,
    original_allocation: Allocation,
    original_domain: LifetimeDomain
) -> LiveTail {
    let removed_link = replace(edge, Option<ptr<LinkCell>>::None);
    return LiveTail {
        owned_ptr: tail,
        owned_allocation: original_allocation,
        owned_domain: original_domain
    };
}

fn finish_tail(
    tail: ptr<LinkCell>,
    a: Allocation,
    d: LifetimeDomain
) -> unit {
    loan_read(d) { |stable|
        let current = ref_from_ptr(read, tail, stable);
        unit
    };
    let empty = loan_exclusive_read(d) { |ending|
        destroy(tail, ending)
    };
    let full = erase_slot<LinkCell>(empty);
    finalize_domain(d);
    deallocate(a, full);
    unit
}

fn main() -> unit {
    let source = Counters { initial: u8(17), final: u8(231) };
    let target = Counters { final: u8(109), initial: u8(62) };
    let copied_old = loan_write(source@final) { |w|
        shift_value(w, u8(45))
    };
    let replaced_initial = loan_write(target@initial) { |w|
        replace(w, copied_old)
    };
    let source_current = source@final;
    let target_current = target@initial;
    let source_sibling = source@initial;
    let target_sibling = target@final;
    match try_allocate_one<LinkCell>() {
        None => { unit },
        Some(first_bundle) => {
            let OneBacking { allocation, raw } = first_bundle;
            let first_allocation = allocation;
            let first_slot = into_slot<LinkCell>(raw);
            let first_domain = lifetime_domain();
            let first_ptr = loan_read(first_domain) { |stable_first|
                initialize(first_slot, LinkCell {
                    link: Option<ptr<LinkCell>>::None, payload: u8(73)
                }, stable_first)
            };
            match try_allocate_one<LinkCell>() {
                None => {
                    let first_empty = loan_exclusive_read(first_domain) { |ending_first|
                        destroy(first_ptr, ending_first)
                    };
                    let first_raw = erase_slot<LinkCell>(first_empty);
                    finalize_domain(first_domain);
                    deallocate(first_allocation, first_raw);
                    unit
                },
                Some(second_bundle) => {
                    let OneBacking { allocation, raw } = second_bundle;
                    let second_allocation = allocation;
                    let second_slot = into_slot<LinkCell>(raw);
                    let second_domain = lifetime_domain();
                    let second_ptr = loan_read(second_domain) { |stable_second|
                        initialize(second_slot, LinkCell {
                            link: Option<ptr<LinkCell>>::None, payload: u8(191)
                        }, stable_second)
                    };
                    let replaced_none = loan_read(first_domain) { |stable_first|
                        let link_writer = ref_from_ptr(write, first_ptr, stable_first);
                        replace(link_writer@link, Option<ptr<LinkCell>>::None)
                    };
                    let witnessed_link = loan_read(first_domain) { |stable_first|
                        let link_reader = ref_from_ptr(read, first_ptr, stable_first);
                        read(link_reader@link)
                    };
                    match witnessed_link {
                        None => { unit },
                        Some(observed_ptr) => {
                            loan_read(second_domain) { |stable_second|
                                let observed_tail = ref_from_ptr(read, observed_ptr, stable_second);
                                unit
                            }
                        },
                    };
                    let LiveTail {
                        owned_ptr, owned_allocation, owned_domain
                    } = loan_read(first_domain) { |stable_first|
                        let link_writer2 = ref_from_ptr(write, first_ptr, stable_first);
                        take_tail(link_writer2@link, second_ptr, second_allocation, second_domain)
                    };
                    loan_read(owned_domain) { |stable_second|
                        let still_current = ref_from_ptr(read, owned_ptr, stable_second);
                        unit
                    };
                    finish_tail(owned_ptr, owned_allocation, owned_domain);
                    let first_empty = loan_exclusive_read(first_domain) { |ending_first|
                        destroy(first_ptr, ending_first)
                    };
                    let first_raw = erase_slot<LinkCell>(first_empty);
                    finalize_domain(first_domain);
                    deallocate(first_allocation, first_raw);
                    unit
                },
            }
        },
    }
}
