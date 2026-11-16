// P1455 A: actual H Bool gates its read scalar before a genuine T installer.
struct MessageCell { next: Option<ptr<MessageCell>>, payload: u8 }
struct MeterState { reading: u8, spare: u8 }
fn install_result(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    displaced
}

fn gate_byte(enabled: bool, incoming: u8) -> u8 {
    let payload_copy = incoming;
    if (enabled) { payload_copy } else { u8(0) }
}

fn detach_tail(edge: ref<write,Option<ptr<MessageCell>>>, tail: ptr<MessageCell>,
               allocation: Allocation, domain: LifetimeDomain) -> LiveTail {
    let detached_link = replace(edge, Option<ptr<MessageCell>>::None);
    return LiveTail { owned_ptr: tail, owned_allocation: allocation,
                      owned_domain: domain };
}
fn finish_tail(cell: ptr<MessageCell>, allocation: Allocation,
               domain: LifetimeDomain) -> unit {
    loan_read(domain) { |held|
        let accessible = ref_from_ptr(read, cell, held);
        unit
    };
    let empty = loan_exclusive_read(domain) { |ending| destroy(cell, ending) };
    let storage = erase_slot<MessageCell>(empty);
    finalize_domain(domain);
    deallocate(allocation, storage);
    unit
}
fn main() -> unit {
    let completed = match try_allocate_one<MessageCell>() {
        None => { u8(5) },
        Some(front_bundle) => {
            let OneBacking { allocation, raw } = front_bundle;
            let front_allocation = allocation;
            let front_slot = into_slot<MessageCell>(raw);
            let front_domain = lifetime_domain();
            let front = loan_read(front_domain) { |starting|
                initialize(front_slot,
                    MessageCell { next: Option<ptr<MessageCell>>::None, payload: u8(59) },
                    starting)
            };
            match try_allocate_one<MessageCell>() {
                None => {
                    let empty = loan_exclusive_read(front_domain) { |ending|
                        destroy(front, ending)
                    };
                    let storage = erase_slot<MessageCell>(empty);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, storage);
                    u8(17)
                },
                Some(back_bundle) => {
                    let OneBacking { allocation, raw } = back_bundle;
                    let back_allocation = allocation;
                    let back_slot = into_slot<MessageCell>(raw);
                    let back_domain = lifetime_domain();
                    let back = loan_read(back_domain) { |starting|
                        initialize(back_slot,
                            MessageCell { next: Option<ptr<MessageCell>>::None, payload: u8(173) },
                            starting)
                    };
                    let old_link = loan_read(front_domain) { |connecting|
                        let edge = ref_from_ptr(write, front, connecting);
                        replace(edge@next, Option<ptr<MessageCell>>::Some(back))
                    };
                    let state = loan_read(front_domain) { |observing|
                        let reader = ref_from_ptr(read, front, observing);
                        read(reader@next)
                    };
                    let payloadd = match state {
                        None => { u8(23) },
                        Some(selected_back) => {
                            let prefer_front = match state {
                                None => { usize(1) < usize(0) },
                                Some(pointer_copy) => { usize(0) < usize(1) }
                            };
                            let chosen = if (prefer_front) {
                                loan_read(back_domain) { |reading_front|
                                    let front_reader = ref_from_ptr(read, front, reading_front);
                                    read(front_reader@payload)
                                }
                            } else {
                                loan_read(back_domain) { |reading_back|
                                    let back_reader = ref_from_ptr(read, selected_back, reading_back);
                                    read(back_reader@payload)
                                }
                            };
                            let delivered = gate_byte(prefer_front, chosen);
                            let display = MeterState { reading: u8(23), spare: u8(229) };
                            let previous = loan_write(display@reading) { |writer|
                                install_result(writer, delivered)
                            };
                            let old_copy = previous;
                            let current = display@reading;
                            let sibling = display@spare;
                            old_copy
                        }
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(front_domain) { |disconnecting|
                            let edge = ref_from_ptr(write, front, disconnecting);
                            detach_tail(edge@next, back, back_allocation, back_domain)
                        };
                    finish_tail(owned_ptr, owned_allocation, owned_domain);
                    let empty = loan_exclusive_read(front_domain) { |ending|
                        destroy(front, ending)
                    };
                    let storage = erase_slot<MessageCell>(empty);
                    finalize_domain(front_domain);
                    deallocate(front_allocation, storage);
                    payloadd
                }
            }
        }
    };
    let retained = completed;
    retained;
    unit
}
