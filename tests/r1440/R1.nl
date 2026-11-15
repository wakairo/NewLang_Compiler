struct Cell { jump: Option<ptr<Cell>>, stamp: u8, }
struct Meter { peak: u8, floor: u8, }

fn retune(slot: ref<write,u8>, requested: u8) -> u8 {
    let prior = replace(slot, requested);
    return prior;
}

fn detach_tail(
    head_jump: ref<write,Option<ptr<Cell>>>,
    p: ptr<Cell>,
    a: Allocation,
    d: LifetimeDomain
) -> LiveTail {
    let previous_link = replace(head_jump, Option<ptr<Cell>>::None);
    return LiveTail {
        owned_ptr: p,
        owned_allocation: a,
        owned_domain: d
    };
}

fn release_tail(
    p: ptr<Cell>,
    a: Allocation,
    d: LifetimeDomain
) -> unit {
    loan_read(d) { |stable_tail|
        let live_tail = ref_from_ptr(read, p, stable_tail);
        unit
    };
    let empty_tail = loan_exclusive_read(d) { |ending_tail|
        destroy(p, ending_tail)
    };
    let raw_tail = erase_slot<Cell>(empty_tail);
    finalize_domain(d);
    deallocate(a, raw_tail);
    unit
}

fn main() -> unit {
    let gauge = Meter { peak: u8(84), floor: u8(12) };
    let journal = Meter { peak: u8(4), floor: u8(96) };

    match try_allocate_one<Cell>() {
        None => { unit },
        Some(head_package) => {
            let OneBacking { allocation, raw } = head_package;
            let allocation_head = allocation;
            let vacant_head = into_slot<Cell>(raw);
            let domain_head = lifetime_domain();
            let ptr_head = loan_read(domain_head) { |stable_head|
                initialize(
                    vacant_head,
                    Cell { jump: Option<ptr<Cell>>::None, stamp: u8(19) },
                    stable_head
                )
            };

            match try_allocate_one<Cell>() {
                None => {
                    let empty_head = loan_exclusive_read(domain_head) { |ending_head|
                        destroy(ptr_head, ending_head)
                    };
                    let raw_head = erase_slot<Cell>(empty_head);
                    finalize_domain(domain_head);
                    deallocate(allocation_head, raw_head);
                    unit
                },
                Some(tail_package) => {
                    let OneBacking { allocation, raw } = tail_package;
                    let allocation_tail = allocation;
                    let vacant_tail = into_slot<Cell>(raw);
                    let domain_tail = lifetime_domain();
                    let ptr_tail = loan_read(domain_tail) { |stable_tail|
                        initialize(
                            vacant_tail,
                            Cell { jump: Option<ptr<Cell>>::None, stamp: u8(53) },
                            stable_tail
                        )
                    };

                    let old_none = loan_read(domain_head) { |stable_head|
                        let head_writer = ref_from_ptr(write, ptr_head, stable_head);
                        replace(head_writer@jump, Option<ptr<Cell>>::Some(ptr_tail))
                    };
                    let copied_link = loan_read(domain_head) { |stable_head|
                        let head_reader = ref_from_ptr(read, ptr_head, stable_head);
                        read(head_reader@jump)
                    };
                    let linked = match copied_link {
                        None => { usize(0) == usize(1) },
                        Some(found) => {
                            loan_read(domain_tail) { |stable_tail|
                                let target = unchecked ref_from_ptr(read, found, stable_tail);
                                let actual_stamp = read(target@stamp);
                                unit
                            };
                            usize(3) == usize(3)
                        },
                    };
                    let choice = if (linked) {
                        gauge@peak
                    } else {
                        gauge@floor
                    };
                    let displaced = loan_write(gauge@floor) { |w|
                        retune(w, choice)
                    };
                    let journal_old = loan_write(journal@peak) { |w|
                        replace(w, displaced)
                    };

                    let LiveTail {
                        owned_ptr,
                        owned_allocation,
                        owned_domain
                    } = loan_read(domain_head) { |stable_head|
                        let head_writer2 = ref_from_ptr(write, ptr_head, stable_head);
                        detach_tail(head_writer2@jump, ptr_tail, allocation_tail, domain_tail)
                    };

                    release_tail(owned_ptr, owned_allocation, owned_domain);

                    let empty_head = loan_exclusive_read(domain_head) { |ending_head|
                        destroy(ptr_head, ending_head)
                    };
                    let raw_head = erase_slot<Cell>(empty_head);
                    finalize_domain(domain_head);
                    deallocate(allocation_head, raw_head);
                    unit
                },
            }
        },
    }
}
