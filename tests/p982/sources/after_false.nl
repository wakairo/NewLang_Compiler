struct RelayCell { successor: Option<ptr<RelayCell>>, byte: u8 }
struct Dial { cold: u8, hot: u8 }

fn exchange_byte(location: ref<write,u8>, incoming: u8) -> u8 {
    let saved = replace(location, incoming);
    return saved;
}

fn detach_cell(edge: ref<write,Option<ptr<RelayCell>>>,
               tail: ptr<RelayCell>, allocation: Allocation,
               domain: LifetimeDomain) -> LiveTail {
    let removed = replace(edge, Option<ptr<RelayCell>>::None);
    let carried = LiveTail {
        owned_ptr: tail,
        owned_allocation: allocation,
        owned_domain: domain
    };
    return carried;
}

fn release_cell(cell: ptr<RelayCell>, allocation: Allocation,
                domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let view = ref_from_ptr(read, cell, stable);
        unit
    };
    let vacant = loan_exclusive_read(domain) { |ending|
        destroy(cell, ending)
    };
    let storage = erase_slot<RelayCell>(vacant);
    finalize_domain(domain);
    deallocate(allocation, storage);
    unit
}

fn main() -> unit {
    match try_allocate_one<RelayCell>() {
        None => { unit },
        Some(head_box) => {
            let OneBacking { allocation, raw } = head_box;
            let head_allocation = allocation;
            let head_slot = into_slot<RelayCell>(raw);
            let head_domain = lifetime_domain();
            let head = loan_read(head_domain) { |stable|
                initialize(head_slot, RelayCell {
                    successor: Option<ptr<RelayCell>>::None, byte: u8(67)
                }, stable)
            };
            match try_allocate_one<RelayCell>() {
                None => {
                    let vacant_head = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<RelayCell>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, head_storage);
                    unit
                },
                Some(tail_box) => {
                    let OneBacking { allocation, raw } = tail_box;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<RelayCell>(raw);
                    let tail_domain = lifetime_domain();
                    let tail = loan_read(tail_domain) { |stable|
                        initialize(tail_slot, RelayCell {
                            successor: Option<ptr<RelayCell>>::None, byte: u8(149)
                        }, stable)
                    };
                    let previous_link = loan_read(head_domain) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        replace(writer@successor, Option<ptr<RelayCell>>::Some(tail))
                    };
                    let returned_tail = loan_read(head_domain) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        detach_cell(writer@successor, tail,
                                    tail_allocation, tail_domain)
                    };
                    let LiveTail { owned_ptr, owned_allocation,
                                   owned_domain } = returned_tail;
                    release_cell(owned_ptr, owned_allocation, owned_domain);
                    let vacant_head = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<RelayCell>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, head_storage);
                    let meter = Dial { hot: u8(103), cold: u8(37) };
                    let backup = Dial { cold: u8(211), hot: u8(5) };
                    let chosen = if (usize(7) == usize(10)) { u8(181) } else { u8(23) };
                    let old = loan_write(meter@hot) { |w|
                        exchange_byte(w, chosen)
                    };
                    let displaced = loan_write(backup@cold) { |w|
                        replace(w, old)
                    };
                    unit
                },
            }
        },
    }
}
