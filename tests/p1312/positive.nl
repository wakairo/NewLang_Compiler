struct Packet { peer: Option<ptr<Packet>>, byte: u8, }
struct Gauge { level: u8, stamp: u8, }

fn unhook(
    edge: ref<write, Option<ptr<Packet>>>,
    item: ptr<Packet>, allocation: Allocation, domain: LifetimeDomain
) -> LiveTail {
    let old_edge = replace(edge, Option<ptr<Packet>>::None);
    return LiveTail {
        owned_ptr: item, owned_allocation: allocation, owned_domain: domain
    };
}

fn dispose(item: ptr<Packet>, allocation: Allocation, domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let borrowed = ref_from_ptr(read, item, stable);
        unit
    };
    let slot = loan_exclusive_read(domain) { |ending| destroy(item, ending) };
    let storage = erase_slot<Packet>(slot);
    finalize_domain(domain);
    deallocate(allocation, storage);
    unit
}

fn cycle_byte(tick: usize) -> u8 {
    if (tick < usize(1)) { u8(49) } else { u8(117) }
}

fn main() -> unit {
    let gauge = Gauge { level: u8(41), stamp: u8(167) };
    let outcome = match try_allocate_one<Packet>() {
        None => { u8(27) },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let head_allocation = allocation;
            let head_slot = into_slot<Packet>(raw);
            let head_domain = lifetime_domain();
            let head_ptr = loan_read(head_domain) { |stable|
                initialize(head_slot,
                    Packet { peer: Option<ptr<Packet>>::None, byte: u8(19) },
                    stable)
            };
            match try_allocate_one<Packet>() {
                None => {
                    let slot = loan_exclusive_read(head_domain) { |ending|
                        destroy(head_ptr, ending)
                    };
                    let storage = erase_slot<Packet>(slot);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, storage);
                    u8(83)
                },
                Some(back_bundle) => {
                    let OneBacking { allocation, raw } = back_bundle;
                    let back_allocation = allocation;
                    let back_slot = into_slot<Packet>(raw);
                    let back_domain = lifetime_domain();
                    let back_ptr = loan_read(back_domain) { |stable|
                        initialize(back_slot,
                            Packet { peer: Option<ptr<Packet>>::None, byte: u8(212) },
                            stable)
                    };
                    let was_empty = loan_read(head_domain) { |stable|
                        let head_write = ref_from_ptr(write, head_ptr, stable);
                        replace(head_write@peer, Option<ptr<Packet>>::Some(back_ptr))
                    };
                    let payload = loan_read(back_domain) { |stable|
                        let back_read = ref_from_ptr(read, back_ptr, stable);
                        read(back_read@byte)
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(head_domain) { |stable|
                            let head_write = ref_from_ptr(write, head_ptr, stable);
                            unhook(head_write@peer, back_ptr, back_allocation, back_domain)
                        };
                    dispose(owned_ptr, owned_allocation, owned_domain);
                    let slot = loan_exclusive_read(head_domain) { |ending|
                        destroy(head_ptr, ending)
                    };
                    let storage = erase_slot<Packet>(slot);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, storage);
                    payload
                },
            }
        },
    };
    let saved = outcome;
    let terminal_old = loop (phase = usize(0)) {
        if (phase < usize(2)) {
            let next_byte = cycle_byte(phase);
            let previous = loan_write(gauge@level) { |port|
                replace(port, next_byte)
            };
            continue(phase + usize(1));
        } else {
            let old = loan_write(gauge@level) { |port|
                replace(port, saved)
            };
            break old;
        }
    };
    let current = gauge@level;
    let sibling = gauge@stamp;
    let receipt = terminal_old;
    unit
}
