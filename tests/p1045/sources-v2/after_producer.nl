struct GaugeSample { low: u8, high: u8 }
fn take_reading(writable: ref<write,u8>, next_reading: u8) -> u8 {
    return replace(writable, next_reading);
}
struct RelayLink { next: Option<ptr<RelayLink>>, stamp: u8 }
fn save_reading(destination: ref<write,u8>, old_reading: u8) -> u8 {
    let displaced = replace(destination, old_reading);
    return displaced;
}
fn unlink_relay(edge: ref<write,Option<ptr<RelayLink>>>,
                address: ptr<RelayLink>, allocation: Allocation,
                domain: LifetimeDomain) -> LiveTail {
    let old_link = replace(edge, Option<ptr<RelayLink>>::None);
    let detached = LiveTail{owned_ptr: address, owned_domain: domain,
                            owned_allocation: allocation};
    return detached;
}
fn close_relay(address: ptr<RelayLink>, allocation: Allocation,
               domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let original_view = ref_from_ptr(read, address, stable);
        unit
    };
    let empty = loan_exclusive_read(domain) { |ending|
        destroy(address, ending)
    };
    let freed_storage = erase_slot<RelayLink>(empty);
    finalize_domain(domain);
    deallocate(allocation, freed_storage);
    unit
}
fn main() -> unit {
    match try_allocate_one<RelayLink>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let head_allocation = allocation;
            let head_slot = into_slot<RelayLink>(raw);
            let head_domain = lifetime_domain();
            let head = loan_read(head_domain) { |stable|
                initialize(head_slot,
                    RelayLink{next: Option<ptr<RelayLink>>::None,
                              stamp: u8(53)}, stable)
            };
            match try_allocate_one<RelayLink>() {
                None => {
                    let empty = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let freed_storage = erase_slot<RelayLink>(empty);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, freed_storage);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<RelayLink>(raw);
                    let tail_domain = lifetime_domain();
                    let tail = loan_read(tail_domain) { |stable|
                        initialize(tail_slot,
                            RelayLink{next: Option<ptr<RelayLink>>::None,
                                      stamp: u8(197)}, stable)
                    };
                    let old_empty = loan_read(head_domain) { |stable|
                        let writable = ref_from_ptr(write, head, stable);
                        replace(writable@next,
                                Option<ptr<RelayLink>>::Some(tail))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(head_domain) { |stable|
                            let writable = ref_from_ptr(write, head, stable);
                            unlink_relay(writable@next, tail,
                                         tail_allocation, tail_domain)
                        };
                    let source_reading = GaugeSample{low: u8(19), high: u8(149)};
                    let saved_reading = GaugeSample{high: u8(67), low: u8(222)};
                    let previous_sample = loan_write(source_reading@low) { |writable|
                        take_reading(writable, u8(88))
                    };
                    let previous_saved = loan_write(saved_reading@high) { |writable|
                        save_reading(writable, previous_sample)
                    };
                    let observed_source_low = source_reading@low;
                    let observed_source_high = source_reading@high;
                    let observed_saved_low = saved_reading@low;
                    let observed_saved_high = saved_reading@high;
                    loan_read(owned_domain) { |stable|
                        let original_view = ref_from_ptr(read, owned_ptr, stable);
                        unit
                    };
                    close_relay(owned_ptr, owned_allocation, owned_domain);
                    let empty = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let freed_storage = erase_slot<RelayLink>(empty);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, freed_storage);
                    unit
                },
            }
        },
    }
}
