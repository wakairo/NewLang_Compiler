struct Dial990 { pulse: u8, park: u8 }
struct Relay990 { onward: Option<ptr<Relay990>>, marker: u8 }

fn swap_pulse(cell: ref<write,u8>, incoming: u8) -> u8 {
    return replace(cell, incoming);
    return u8(19);
}

fn detach_Relay990(link: ref<write,Option<ptr<Relay990>>>,
                raw_tail: ptr<Relay990>, owner: Allocation,
                life: LifetimeDomain) -> LiveTail {
    let discarded = replace(link, Option<ptr<Relay990>>::None);
    let bundle = LiveTail {
        owned_ptr: raw_tail,
        owned_allocation: owner,
        owned_domain: life
    };
    return bundle;
}

fn finish_Relay990(raw_tail: ptr<Relay990>, owner: Allocation,
                life: LifetimeDomain) -> unit {
    loan_read(life) { |stable|
        let watched = ref_from_ptr(read, raw_tail, stable);
        unit
    };
    let vacant = loan_exclusive_read(life) { |ending|
        destroy(raw_tail, ending)
    };
    let storage = erase_slot<Relay990>(vacant);
    finalize_domain(life);
    deallocate(owner, storage);
    unit
}


fn main() -> unit {
    let left = Dial990 { park: u8(17), pulse: u8(83) };
    let right = Dial990 { pulse: u8(121), park: u8(201) };
    let old = loan_write(right@pulse) { |w|
        swap_pulse(w, u8(37))
    };
    let displaced = loan_write(left@park) { |w|
        replace(w, old)
    };
    match try_allocate_one<Relay990>() {
        None => { unit },
        Some(head_box) => {
            let OneBacking { allocation, raw } = head_box;
            let head_owner = allocation;
            let head_slot = into_slot<Relay990>(raw);
            let head_life = lifetime_domain();
            let head = loan_read(head_life) { |stable|
                initialize(head_slot, Relay990 {
                    onward: Option<ptr<Relay990>>::None, marker: u8(97)
                }, stable)
            };
            match try_allocate_one<Relay990>() {
                None => {
                    let head_vacant = loan_exclusive_read(head_life) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<Relay990>(head_vacant);
                    finalize_domain(head_life);
                    deallocate(head_owner, head_storage);
                    unit
                },
                Some(tail_box) => {
                    let OneBacking { allocation, raw } = tail_box;
                    let tail_owner = allocation;
                    let tail_slot = into_slot<Relay990>(raw);
                    let tail_life = lifetime_domain();
                    let tail = loan_read(tail_life) { |stable|
                        initialize(tail_slot, Relay990 {
                            onward: Option<ptr<Relay990>>::None, marker: u8(173)
                        }, stable)
                    };
                    let previous_link = loan_read(head_life) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        replace(writer@onward, Option<ptr<Relay990>>::Some(tail))
                    };
                    let returned = loan_read(head_life) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        detach_Relay990(writer@onward, tail, tail_owner, tail_life)
                    };
                    let LiveTail { owned_ptr, owned_allocation,
                                   owned_domain } = returned;
                    finish_Relay990(owned_ptr, owned_allocation, owned_domain);
                    let head_vacant = loan_exclusive_read(head_life) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<Relay990>(head_vacant);
                    finalize_domain(head_life);
                    deallocate(head_owner, head_storage);
                    unit
                },
            }
        },
    }
}
