struct DispatchCell { pending: Option<ptr<DispatchCell>>, opcode: u8 }
struct PortConfig { channel: u8, limit: u8 }

fn exchange_channel(slot: ref<write,u8>, requested: u8) -> u8 {
    let displaced = replace(slot, requested);
    return displaced;
}
fn take_dispatch(link: ref<write,Option<ptr<DispatchCell>>>,
                 address: ptr<DispatchCell>, ticket: Allocation,
                 lifetime: LifetimeDomain) -> LiveTail {
    let detached = replace(link, Option<ptr<DispatchCell>>::None);
    let outgoing = LiveTail { owned_domain: lifetime,
                              owned_ptr: address, owned_allocation: ticket };
    return outgoing;
}
fn close_dispatch(address: ptr<DispatchCell>, ticket: Allocation,
                  lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |stable|
        let inspection = ref_from_ptr(read, address, stable);
        unit
    };
    let ended = loan_exclusive_read(lifetime) { |ending| destroy(address, ending) };
    let raw = erase_slot<DispatchCell>(ended);
    finalize_domain(lifetime);
    deallocate(ticket, raw);
    unit
}
fn main() -> unit {
    let live = PortConfig { limit: u8(122), channel: u8(39) };
    let journal = PortConfig { channel: u8(15), limit: u8(207) };
    let requested = if (usize(6) < usize(14)) { u8(166) } else { u8(27) };
    let old_channel = loan_write(live@channel) { |slot| exchange_channel(slot, requested) };
    let old_journal = loan_write(journal@limit) { |slot| replace(slot, old_channel) };
    let installed = live@channel;
    let preserved = live@limit;
    let archived = journal@limit;
    old_journal; installed; preserved; archived;
    match try_allocate_one<DispatchCell>() {
        None => { unit },
        Some(first) => {
            let OneBacking { allocation, raw } = first;
            let head_ticket = allocation;
            let head_slot = into_slot<DispatchCell>(raw);
            let head_lifetime = lifetime_domain();
            let head = loan_read(head_lifetime) { |stable|
                initialize(head_slot, DispatchCell {
                    pending: Option<ptr<DispatchCell>>::None, opcode: u8(65)
                }, stable)
            };
            match try_allocate_one<DispatchCell>() {
                None => {
                    let ended = loan_exclusive_read(head_lifetime) { |ending| destroy(head, ending) };
                    let head_bytes = erase_slot<DispatchCell>(ended);
                    finalize_domain(head_lifetime);
                    deallocate(head_ticket, head_bytes);
                    unit
                },
                Some(second) => {
                    let OneBacking { allocation, raw } = second;
                    let item_ticket = allocation;
                    let item_slot = into_slot<DispatchCell>(raw);
                    let item_lifetime = lifetime_domain();
                    let item = loan_read(item_lifetime) { |stable|
                        initialize(item_slot, DispatchCell {
                            pending: Option<ptr<DispatchCell>>::None, opcode: u8(193)
                        }, stable)
                    };
                    let was_empty = loan_read(head_lifetime) { |stable|
                        let writable = ref_from_ptr(write, head, stable);
                        replace(writable@pending, Option<ptr<DispatchCell>>::Some(item))
                    };
                    let transferred = loan_read(head_lifetime) { |stable|
                        let writable = ref_from_ptr(write, head, stable);
                        take_dispatch(writable@pending, item, item_ticket, item_lifetime)
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = transferred;
                    close_dispatch(owned_ptr, owned_allocation, owned_domain);
                    let ended = loan_exclusive_read(head_lifetime) { |ending| destroy(head, ending) };
                    let head_bytes = erase_slot<DispatchCell>(ended);
                    finalize_domain(head_lifetime);
                    deallocate(head_ticket, head_bytes);
                    unit
                },
            }
        },
    }
}
