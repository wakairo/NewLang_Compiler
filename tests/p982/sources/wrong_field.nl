struct QueueCell { follow: Option<ptr<QueueCell>>, stamp: u8 }
struct Measure { hot: u8, cold: u8 }

fn exchange_byte(location: ref<write,u8>, incoming: u8) -> u8 {
    let saved = replace(location, incoming);
    return saved;
}

fn detach_cell(edge: ref<write,Option<ptr<QueueCell>>>,
               tail: ptr<QueueCell>, allocation: Allocation,
               domain: LifetimeDomain) -> LiveTail {
    let removed = replace(edge, Option<ptr<QueueCell>>::None);
    let carried = LiveTail {
        owned_ptr: tail,
        owned_allocation: allocation,
        owned_domain: domain
    };
    return carried;
}

fn release_cell(cell: ptr<QueueCell>, allocation: Allocation,
                domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let view = ref_from_ptr(read, cell, stable);
        unit
    };
    let vacant = loan_exclusive_read(domain) { |ending|
        destroy(cell, ending)
    };
    let storage = erase_slot<QueueCell>(vacant);
    finalize_domain(domain);
    deallocate(allocation, storage);
    unit
}

fn main() -> unit {
    match try_allocate_one<QueueCell>() {
        None => { unit },
        Some(head_box) => {
            let OneBacking { allocation, raw } = head_box;
            let head_allocation = allocation;
            let head_slot = into_slot<QueueCell>(raw);
            let head_domain = lifetime_domain();
            let head = loan_read(head_domain) { |stable|
                initialize(head_slot, QueueCell {
                    follow: Option<ptr<QueueCell>>::None, stamp: u8(53)
                }, stable)
            };
            match try_allocate_one<QueueCell>() {
                None => {
                    let vacant_head = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<QueueCell>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, head_storage);
                    unit
                },
                Some(tail_box) => {
                    let OneBacking { allocation, raw } = tail_box;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<QueueCell>(raw);
                    let tail_domain = lifetime_domain();
                    let tail = loan_read(tail_domain) { |stable|
                        initialize(tail_slot, QueueCell {
                            follow: Option<ptr<QueueCell>>::None, stamp: u8(187)
                        }, stable)
                    };
                    let previous_link = loan_read(head_domain) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        replace(writer@follow, Option<ptr<QueueCell>>::Some(tail))
                    };
                    let returned_tail = loan_read(head_domain) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        detach_cell(writer@follow, tail,
                                    tail_allocation, tail_domain)
                    };
                    let LiveTail { owned_ptr, owned_allocation,
                                   owned_domain } = returned_tail;
                    release_cell(owned_ptr, owned_allocation, owned_domain);
                    let vacant_head = loan_exclusive_read(head_domain) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<QueueCell>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, head_storage);
                    let meter = Measure { cold: u8(41), hot: u8(197) };
                    let backup = Measure { hot: u8(13), cold: u8(222) };
                    let chosen = if (usize(6) == usize(6)) { u8(86) } else { u8(172) };
                    let old = loan_write(backup@missing) { |w|
                        exchange_byte(w, chosen)
                    };
                    let displaced = loan_write(meter@cold) { |w|
                        replace(w, old)
                    };
                    unit
                },
            }
        },
    }
}
