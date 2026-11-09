// P1280 C380/C383 independent complete source, Draft17.35 eligibility hypothesis.
struct SignalNode { following: Option<ptr<SignalNode>>, reading: u8 }
struct ReceiptPair { selected: u8, unchanged: u8 }

fn store_reading(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    displaced
}
fn detach_signal(edge: ref<write,Option<ptr<SignalNode>>>, back: ptr<SignalNode>,
                 allocation: Allocation, domain: LifetimeDomain) -> LiveTail {
    let unlinked = replace(edge, Option<ptr<SignalNode>>::None);
    return LiveTail { owned_ptr: back, owned_allocation: allocation,
                      owned_domain: domain };
}
fn release_signal(back: ptr<SignalNode>, allocation: Allocation,
                  domain: LifetimeDomain) -> unit {
    loan_read(domain) { |held|
        let live = ref_from_ptr(read, back, held);
        unit
    };
    let empty = loan_exclusive_read(domain) { |ending| destroy(back, ending) };
    let bytes = erase_slot<SignalNode>(empty);
    finalize_domain(domain);
    deallocate(allocation, bytes);
    unit
}
fn main() -> unit {
    match try_allocate_one<SignalNode>() {
        None => { unit },
        Some(front_packet) => {
            let OneBacking { allocation, raw } = front_packet;
            let front_allocation = allocation;
            let front_slot = into_slot<SignalNode>(raw);
            let front_domain = lifetime_domain();
            let front_ptr = loan_read(front_domain) { |initializing|
                initialize(front_slot,
                    SignalNode { following: Option<ptr<SignalNode>>::None,
                                 reading: u8(231) }, initializing)
            };
            match try_allocate_one<SignalNode>() {
                None => {
                    let empty_front = loan_exclusive_read(front_domain) { |ending|
                        destroy(front_ptr, ending)
                    };
                    let bytes = erase_slot<SignalNode>(empty_front);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, bytes);
                    unit
                },
                Some(back_packet) => {
                    let OneBacking { allocation, raw } = back_packet;
                    let back_allocation = allocation;
                    let back_slot = into_slot<SignalNode>(raw);
                    let back_domain = lifetime_domain();
                    let back_ptr = loan_read(back_domain) { |initializing|
                        initialize(back_slot,
                            SignalNode { following: Option<ptr<SignalNode>>::None,
                                         reading: u8(83) }, initializing)
                    };
                    let prior_link = loan_read(front_domain) { |attaching|
                        let writer = ref_from_ptr(write, front_ptr, attaching);
                        replace(writer@following, Option<ptr<SignalNode>>::Some(back_ptr))
                    };
                    let observed_link = loan_read(front_domain) { |observing|
                        let front_reader = ref_from_ptr(read, front_ptr, observing);
                        read(front_reader@following)
                    };
                    match observed_link {
                        None => { unit },
                        Some(selected_back) => {
                            let observed_byte = loan_read(back_domain) { |reading_back|
                                let payload_reader = ref_from_ptr(read, selected_back, reading_back);
                                read(payload_reader@reading)
                            };
                            let source_copy = observed_byte;
                            let record = ReceiptPair { selected: u8(31), unchanged: u8(127) };
                            let old_selected = loan_write(record@selected) { |writing_record|
                                store_reading(writing_record, source_copy)
                            };
                            let old_copy = old_selected;
                            let current_byte = record@selected;
                            let sibling_byte = record@unchanged;
                            unit
                        }
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(front_domain) { |detaching|
                        let writer = ref_from_ptr(write, front_ptr, detaching);
                        detach_signal(writer@following, back_ptr, back_allocation, back_domain)
                    };
                    release_signal(owned_ptr, owned_allocation, owned_domain);
                    let empty_front = loan_exclusive_read(front_domain) { |ending|
                        destroy(front_ptr, ending)
                    };
                    let bytes = erase_slot<SignalNode>(empty_front);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, bytes);
                    unit
                }
            }
        }
    };
    unit
}
