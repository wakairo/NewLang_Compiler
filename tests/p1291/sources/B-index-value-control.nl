// P1291 C416 independent value/index control: genuine old-u8 result survives completed H matches and cleanup.
struct CaptureNode { link: Option<ptr<CaptureNode>>, captured: u8 }
struct DisplayState { spare: u8, current: u8 }

fn accept_captured(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    displaced
}
fn detach_report(edge: ref<write,Option<ptr<CaptureNode>>>, back: ptr<CaptureNode>,
                 allocation: Allocation, domain: LifetimeDomain) -> LiveTail {
    let unlinked = replace(edge, Option<ptr<CaptureNode>>::None);
    return LiveTail { owned_ptr: back, owned_allocation: allocation,
                      owned_domain: domain };
}
fn release_report(back: ptr<CaptureNode>, allocation: Allocation,
                  domain: LifetimeDomain) -> unit {
    loan_read(domain) { |held|
        let live = ref_from_ptr(read, back, held);
        unit
    };
    let empty = loan_exclusive_read(domain) { |ending| destroy(back, ending) };
    let bytes = erase_slot<CaptureNode>(empty);
    finalize_domain(domain);
    deallocate(allocation, bytes);
    unit
}
fn main() -> unit {
    let completed = match try_allocate_one<CaptureNode>() {
        None => { u8(7) },
        Some(front_packet) => {
            let OneBacking { allocation, raw } = front_packet;
            let front_allocation = allocation;
            let front_slot = into_slot<CaptureNode>(raw);
            let front_domain = lifetime_domain();
            let front_ptr = loan_read(front_domain) { |initializing|
                initialize(front_slot,
                    CaptureNode { link: Option<ptr<CaptureNode>>::None,
                                 captured: u8(227) }, initializing)
            };
            match try_allocate_one<CaptureNode>() {
                None => {
                    let empty_front = loan_exclusive_read(front_domain) { |ending|
                        destroy(front_ptr, ending)
                    };
                    let bytes = erase_slot<CaptureNode>(empty_front);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, bytes);
                    u8(19)
                },
                Some(back_packet) => {
                    let OneBacking { allocation, raw } = back_packet;
                    let back_allocation = allocation;
                    let back_slot = into_slot<CaptureNode>(raw);
                    let back_domain = lifetime_domain();
                    let back_ptr = loan_read(back_domain) { |initializing|
                        initialize(back_slot,
                            CaptureNode { link: Option<ptr<CaptureNode>>::None,
                                         captured: u8(131) }, initializing)
                    };
                    let prior_link = loan_read(front_domain) { |attaching|
                        let writer = ref_from_ptr(write, front_ptr, attaching);
                        replace(writer@link, Option<ptr<CaptureNode>>::Some(back_ptr))
                    };
                    let observed_link = loan_read(front_domain) { |observing|
                        let front_reader = ref_from_ptr(read, front_ptr, observing);
                        read(front_reader@link)
                    };
                    let arm_copy = match observed_link {
                        None => { u8(31) },
                        Some(selected_back) => {
                            let observed_byte = loan_read(back_domain) { |reading_back|
                                let payload_reader = ref_from_ptr(read, selected_back, reading_back);
                                read(payload_reader@captured)
                            };
                            let source_copy = observed_byte;
                            let record = DisplayState { current: u8(61), spare: u8(181) };
                            let old_selected = loan_write(record@current) { |writing_record|
                                accept_captured(writing_record, source_copy)
                            };
                            let old_copy = old_selected;
                            let current_byte = record@current;
                            let sibling_byte = record@spare;
                            old_copy
                        }
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(front_domain) { |detaching|
                        let writer = ref_from_ptr(write, front_ptr, detaching);
                        detach_report(writer@link, back_ptr, back_allocation, back_domain)
                    };
                    release_report(owned_ptr, owned_allocation, owned_domain);
                    let empty_front = loan_exclusive_read(front_domain) { |ending|
                        destroy(front_ptr, ending)
                    };
                    let bytes = erase_slot<CaptureNode>(empty_front);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, bytes);
                    arm_copy
                }
            }
        }
    };
    let result_copy = completed;
    let continuation_copy = result_copy;
    unit
}
