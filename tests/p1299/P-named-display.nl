// P1299: publish a completed sample through an ordinary stateful function.
struct PacketNode { chain: Option<ptr<PacketNode>>, datum: u8 }

fn publish_sample(destination: ref<write,u8>, requested: u8) -> u8 {
    let retired = replace(destination, requested);
    let receipt = retired;
    receipt
}

struct Display { level: u8, guard: u8 }

fn capture_previous(destination: ref<write,u8>, requested: u8) -> u8 {
    let displaced = replace(destination, requested);
    displaced
}

fn remove_tail(link: ref<write,Option<ptr<PacketNode>>>, item: ptr<PacketNode>,
               ticket: Allocation, lifetime: LifetimeDomain) -> LiveTail {
    let old_link = replace(link, Option<ptr<PacketNode>>::None);
    return LiveTail { owned_ptr: item, owned_allocation: ticket,
                      owned_domain: lifetime };
}

fn release_tail(item: ptr<PacketNode>, ticket: Allocation,
                lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |access|
        let original = ref_from_ptr(read, item, access);
        unit
    };
    let empty = loan_exclusive_read(lifetime) { |ending| destroy(item, ending) };
    let raw_bytes = erase_slot<PacketNode>(empty);
    finalize_domain(lifetime);
    deallocate(ticket, raw_bytes);
    unit
}

fn main() -> unit {
    let panel = Display { guard: u8(227), level: u8(103) };
    let archived = match try_allocate_one<PacketNode>() {
        None => { u8(19) },
        Some(head_memory) => {
            let OneBacking { allocation, raw } = head_memory;
            let head_ticket = allocation;
            let head_slot = into_slot<PacketNode>(raw);
            let head_lifetime = lifetime_domain();
            let head = loan_read(head_lifetime) { |making|
                initialize(head_slot,
                    PacketNode { datum: u8(239),
                                 chain: Option<ptr<PacketNode>>::None }, making)
            };
            match try_allocate_one<PacketNode>() {
                None => {
                    let head_empty = loan_exclusive_read(head_lifetime) { |ending|
                        destroy(head, ending)
                    };
                    let head_bytes = erase_slot<PacketNode>(head_empty);
                    finalize_domain(head_lifetime);
                    deallocate(head_ticket, head_bytes);
                    u8(43)
                },
                Some(tail_memory) => {
                    let OneBacking { allocation, raw } = tail_memory;
                    let tail_ticket = allocation;
                    let tail_slot = into_slot<PacketNode>(raw);
                    let tail_lifetime = lifetime_domain();
                    let tail = loan_read(tail_lifetime) { |making|
                        initialize(tail_slot,
                            PacketNode { datum: u8(149),
                                         chain: Option<ptr<PacketNode>>::None }, making)
                    };
                    let empty_link = loan_read(head_lifetime) { |access|
                        let node = ref_from_ptr(write, head, access);
                        replace(node@chain, Option<ptr<PacketNode>>::Some(tail))
                    };
                    let selected = loan_read(head_lifetime) { |access|
                        let node = ref_from_ptr(read, head, access);
                        read(node@chain)
                    };
                    let collected = match selected {
                        None => { u8(61) },
                        Some(found) => {
                            let sample = loan_read(tail_lifetime) { |access|
                                let node = ref_from_ptr(read, found, access);
                                read(node@datum)
                            };
                            let scratch = Display { guard: u8(193), level: u8(67) };
                            let displaced = loan_write(scratch@level) { |destination|
                                capture_previous(destination, sample)
                            };
                            let archived_copy = displaced;
                            archived_copy
                        }
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(head_lifetime) { |access|
                        let node = ref_from_ptr(write, head, access);
                        remove_tail(node@chain, tail, tail_ticket, tail_lifetime)
                    };
                    release_tail(owned_ptr, owned_allocation, owned_domain);
                    let head_empty = loan_exclusive_read(head_lifetime) { |ending|
                        destroy(head, ending)
                    };
                    let head_bytes = erase_slot<PacketNode>(head_empty);
                    finalize_domain(head_lifetime);
                    deallocate(head_ticket, head_bytes);
                    collected
                }
            }
        }
    };
    let forwarded = archived;
    let previous = loan_write(panel@level) { |destination|
        publish_sample(destination, forwarded)
    };
    let caller_old = previous;
    let caller_current = panel@level;
    let caller_sibling = panel@guard;
    unit
}
