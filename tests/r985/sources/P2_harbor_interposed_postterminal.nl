struct TwinGauge { upper: u8, lower: u8 }
fn exchange_value(target: ref<write,u8>, replacement: u8) -> u8 {
    return replace(target, replacement);
}
struct HarborHeap { successor: Option<ptr<HarborHeap>>, stamp: u8 }
fn hand_over(
    pointer_field: ref<write,Option<ptr<HarborHeap>>>,
    carried_ptr: ptr<HarborHeap>,
    carried_allocation: Allocation,
    carried_domain: LifetimeDomain
) -> LiveTail {
    let detached = replace(pointer_field, Option<ptr<HarborHeap>>::None);
    let packet = LiveTail {
        owned_allocation: carried_allocation,
        owned_domain: carried_domain,
        owned_ptr: carried_ptr
    };
    return packet;
}
fn terminal(
    carried_ptr: ptr<HarborHeap>,
    carried_allocation: Allocation,
    carried_domain: LifetimeDomain
) -> unit {
    loan_read(carried_domain) { |scope|
        let current_tail = ref_from_ptr(read, carried_ptr, scope);
        unit
    };
    let free_slot = loan_exclusive_read(carried_domain) { |end|
        destroy(carried_ptr, end)
    };
    let recovered = erase_slot<HarborHeap>(free_slot);
    finalize_domain(carried_domain);
    deallocate(carried_allocation, recovered);
    unit
}
fn main() -> unit {
    match try_allocate_one<HarborHeap>() {
        None => { unit },
        Some(lead_bundle) => {
            let OneBacking { allocation, raw } = lead_bundle;
            let lead_allocation = allocation;
            let lead_slot = into_slot<HarborHeap>(raw);
            let lead_domain = lifetime_domain();
            let lead_pointer = loan_read(lead_domain) { |s|
                initialize(lead_slot, HarborHeap {
                    stamp: u8(76), successor: Option<ptr<HarborHeap>>::None
                }, s)
            };
            match try_allocate_one<HarborHeap>() {
                None => {
                    let free_slot = loan_exclusive_read(lead_domain) { |e|
                        destroy(lead_pointer, e)
                    };
                    let restored = erase_slot<HarborHeap>(free_slot);
                    finalize_domain(lead_domain);
                    deallocate(lead_allocation, restored);
                    unit
                },
                Some(follow_bundle) => {
                    let OneBacking { allocation, raw } = follow_bundle;
                    let follow_allocation = allocation;
                    let follow_slot = into_slot<HarborHeap>(raw);
                    let follow_domain = lifetime_domain();
                    let follow_pointer = loan_read(follow_domain) { |s|
                        initialize(follow_slot, HarborHeap {
                            successor: Option<ptr<HarborHeap>>::None, stamp: u8(231)
                        }, s)
                    };
                    let formerly_none = loan_read(lead_domain) { |s|
                        let w = ref_from_ptr(write, lead_pointer, s);
                        replace(w@successor,
                            Option<ptr<HarborHeap>>::Some(follow_pointer))
                    };
                    let now_some = loan_read(lead_domain) { |s|
                        let r = ref_from_ptr(read, lead_pointer, s);
                        read(r@successor)
                    };
                    match now_some {
                        None => { unit },
                        Some(follow_link) => {
                            loan_read(follow_domain) { |s|
                                let r = ref_from_ptr(read, follow_link, s);
                                unit
                            }
                        },
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(lead_domain) { |s|
                            let w = ref_from_ptr(write, lead_pointer, s);
                            hand_over(w@successor, follow_pointer,
                                follow_allocation, follow_domain)
                        };
                    loan_read(owned_domain) { |s|
                        let still_live = ref_from_ptr(read, owned_ptr, s);
                        unit
                    };
                    terminal(owned_ptr, owned_allocation, owned_domain);
                    let recovered_lead = loan_exclusive_read(lead_domain) { |end|
                        destroy(lead_pointer, end)
                    };
                    let raw_lead = erase_slot<HarborHeap>(recovered_lead);
                    finalize_domain(lead_domain);
                    deallocate(lead_allocation, raw_lead);

                    let primary = TwinGauge { lower: u8(187), upper: u8(7) };
                    let secondary = TwinGauge { upper: u8(249), lower: u8(26) };
                    let prior = loan_write(primary@upper) { |w|
                        exchange_value(w, u8(54))
                    };
                    let displaced = loan_write(secondary@lower) { |w|
                        replace(w, prior)
                    };
                    let p_upper_now = primary@upper;
                    let p_lower_kept = primary@lower;
                    let s_lower_now = secondary@lower;
                    let s_upper_kept = secondary@upper;
                    unit
                },
            }
        },
    }
}
