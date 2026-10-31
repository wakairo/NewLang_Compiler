struct Chain990 { onward: Option<ptr<Chain990>>, marker: u8 }
struct Meter990 { spare: u8, flow: u8 }

fn swap_flow(cell: ref<write,u8>, incoming: u8) -> u8 {
    let prior = replace(cell, incoming);
    return prior;
}

fn detach_Chain990(link: ref<write,Option<ptr<Chain990>>>,
                raw_tail: ptr<Chain990>, owner: Allocation,
                life: LifetimeDomain) -> LiveTail {
    let discarded = replace(link, Option<ptr<Chain990>>::None);
    let bundle = LiveTail {
        owned_ptr: raw_tail,
        owned_allocation: owner,
        owned_domain: life
    };
    return bundle;
}

fn finish_Chain990(raw_tail: ptr<Chain990>, owner: Allocation,
                life: LifetimeDomain) -> unit {
    loan_read(life) { |stable|
        let watched = ref_from_ptr(read, raw_tail, stable);
        unit
    };
    let vacant = loan_exclusive_read(life) { |ending|
        destroy(raw_tail, ending)
    };
    let storage = erase_slot<Chain990>(vacant);
    finalize_domain(life);
    deallocate(owner, storage);
    unit
}


fn main() -> unit {
    match try_allocate_one<Chain990>() {
        None => { unit },
        Some(head_box) => {
            let OneBacking { allocation, raw } = head_box;
            let head_owner = allocation;
            let head_slot = into_slot<Chain990>(raw);
            let head_life = lifetime_domain();
            let head = loan_read(head_life) { |stable|
                initialize(head_slot, Chain990 {
                    onward: Option<ptr<Chain990>>::None, marker: u8(47)
                }, stable)
            };
            match try_allocate_one<Chain990>() {
                None => {
                    let head_vacant = loan_exclusive_read(head_life) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<Chain990>(head_vacant);
                    finalize_domain(head_life);
                    deallocate(head_owner, head_storage);
                    unit
                },
                Some(tail_box) => {
                    let OneBacking { allocation, raw } = tail_box;
                    let tail_owner = allocation;
                    let tail_slot = into_slot<Chain990>(raw);
                    let tail_life = lifetime_domain();
                    let tail = loan_read(tail_life) { |stable|
                        initialize(tail_slot, Chain990 {
                            onward: Option<ptr<Chain990>>::None, marker: u8(193)
                        }, stable)
                    };
                    let previous_link = loan_read(head_life) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        replace(writer@onward, Option<ptr<Chain990>>::Some(tail))
                    };
                    let returned = loan_read(head_life) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        detach_Chain990(writer@onward, tail, tail_owner, tail_life)
                    };
                    let LiveTail { owned_ptr, owned_allocation,
                                   owned_domain } = returned;
                    finish_Chain990(owned_ptr, owned_allocation, owned_domain);
                    let head_vacant = loan_exclusive_read(head_life) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<Chain990>(head_vacant);
                    finalize_domain(head_life);
                    deallocate(head_owner, head_storage);
                    let primary = Meter990 { flow: u8(151), spare: u8(61) };
                    let secondary = Meter990 { spare: u8(233), flow: u8(29) };
                    let incoming = if (usize(11) == usize(11)) { u8(89) } else { u8(43) };
                    let old = loan_write(primary@flow) { |w|
                        swap_flow(w, incoming)
                    };
                    let displaced = loan_write(secondary@spare) { |w|
                        replace(w, old)
                    };
                    unit
                },
            }
        },
    }
}
