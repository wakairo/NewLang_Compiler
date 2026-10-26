struct ThermalSettings { threshold: u8, fan: u8 }
fn exchange_fan(slot: ref<write,u8>, requested: u8) -> u8 {
    let displaced = replace(slot, requested);
    return displaced;
}
struct SampleCell { following: Option<ptr<SampleCell>>, sensor: u8 }
fn take_sample(link: ref<write,Option<ptr<SampleCell>>>,
               address: ptr<SampleCell>, ticket: Allocation,
               lifetime: LifetimeDomain) -> LiveTail {
    let detached = replace(link, Option<ptr<SampleCell>>::None);
    let outgoing = LiveTail { owned_allocation: ticket,
                              owned_domain: lifetime, owned_ptr: address };
    return outgoing;
}
fn close_sample(address: ptr<SampleCell>, ticket: Allocation,
                lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |stable|
        let inspection = ref_from_ptr(read, address, stable);
        unit
    };
    let ended = loan_exclusive_read(lifetime) { |ending| destroy(address, ending) };
    let raw = erase_slot<SampleCell>(ended);
    finalize_domain(lifetime);
    deallocate(ticket, raw);
    unit
}
fn main() -> unit {
    match try_allocate_one<SampleCell>() {
        None => { unit },
        Some(first) => {
            let OneBacking { allocation, raw } = first;
            let head_ticket = allocation;
            let head_slot = into_slot<SampleCell>(raw);
            let head_lifetime = lifetime_domain();
            let head = loan_read(head_lifetime) { |stable|
                initialize(head_slot, SampleCell {
                    following: Option<ptr<SampleCell>>::None, sensor: u8(74)
                }, stable)
            };
            match try_allocate_one<SampleCell>() {
                None => {
                    let ended = loan_exclusive_read(head_lifetime) { |ending| destroy(head, ending) };
                    let head_bytes = erase_slot<SampleCell>(ended);
                    finalize_domain(head_lifetime);
                    deallocate(head_ticket, head_bytes);
                    unit
                },
                Some(second) => {
                    let OneBacking { allocation, raw } = second;
                    let item_ticket = allocation;
                    let item_slot = into_slot<SampleCell>(raw);
                    let item_lifetime = lifetime_domain();
                    let item = loan_read(item_lifetime) { |stable|
                        initialize(item_slot, SampleCell {
                            following: Option<ptr<SampleCell>>::None, sensor: u8(218)
                        }, stable)
                    };
                    let was_empty = loan_read(head_lifetime) { |stable|
                        let writable = ref_from_ptr(write, head, stable);
                        replace(writable@following, Option<ptr<SampleCell>>::Some(item))
                    };
                    let transferred = loan_read(head_lifetime) { |stable|
                        let writable = ref_from_ptr(write, head, stable);
                        take_sample(writable@following, item, item_ticket, item_lifetime)
                    };
                    let active = ThermalSettings { fan: u8(184), threshold: u8(43) };
                    let recovery = ThermalSettings { threshold: u8(91), fan: u8(126) };
                    let requested = if (usize(29) == usize(11)) { u8(201) } else { u8(52) };
                    let previous_fan = loan_write(active@fan) { |slot| exchange_fan(slot, requested) };
                    let previous_recovery = loan_write(recovery@threshold) { |slot| replace(slot, previous_fan) };
                    let installed = active@fan;
                    let preserved = active@threshold;
                    let archived = recovery@threshold;
                    previous_recovery; installed; preserved; archived;
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = transferred;
                    close_sample(owned_ptr, owned_allocation, owned_domain);
                    let ended = loan_exclusive_read(head_lifetime) { |ending| destroy(head, ending) };
                    let head_bytes = erase_slot<SampleCell>(ended);
                    finalize_domain(head_lifetime);
                    deallocate(head_ticket, head_bytes);
                    unit
                },
            }
        },
    }
}
