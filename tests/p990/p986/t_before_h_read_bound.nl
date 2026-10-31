struct Gauge { reserve: u8, signal: u8 }
struct Pulse { onward: Option<ptr<Pulse>>, marker: u8 }

fn trade_signal(cell: ref<write,u8>, incoming: u8) -> u8 {
    let prior = replace(cell, incoming);
    return prior;
}

fn detach_pulse(link: ref<write,Option<ptr<Pulse>>>,
                raw_tail: ptr<Pulse>, owner: Allocation,
                life: LifetimeDomain) -> LiveTail {
    let discarded = replace(link, Option<ptr<Pulse>>::None);
    return LiveTail {
        owned_ptr: raw_tail,
        owned_allocation: owner,
        owned_domain: life
    };
}

fn finish_pulse(raw_tail: ptr<Pulse>, owner: Allocation,
                life: LifetimeDomain) -> unit {
    loan_read(life) { |stable|
        let watched = ref_from_ptr(read, raw_tail, stable);
        unit
    };
    let vacant = loan_exclusive_read(life) { |ending|
        destroy(raw_tail, ending)
    };
    let storage = erase_slot<Pulse>(vacant);
    finalize_domain(life);
    deallocate(owner, storage);
    unit
}

fn main() -> unit {
    let front = Gauge { signal: u8(107), reserve: u8(31) };
    let back = Gauge { reserve: u8(59), signal: u8(219) };
    let was = loan_write(front@signal) { |w|
        trade_signal(w, u8(73))
    };
    let displaced = loan_write(back@reserve) { |w|
        replace(w, was)
    };
    match try_allocate_one<Pulse>() {
        None => { unit },
        Some(head_box) => {
            let OneBacking { allocation, raw } = head_box;
            let head_owner = allocation;
            let head_slot = into_slot<Pulse>(raw);
            let head_life = lifetime_domain();
            let head = loan_read(head_life) { |stable|
                initialize(head_slot, Pulse {
                    onward: Option<ptr<Pulse>>::None, marker: u8(81)
                }, stable)
            };
            match try_allocate_one<Pulse>() {
                None => {
                    let head_vacant = loan_exclusive_read(head_life) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<Pulse>(head_vacant);
                    finalize_domain(head_life);
                    deallocate(head_owner, head_storage);
                    unit
                },
                Some(tail_box) => {
                    let OneBacking { allocation, raw } = tail_box;
                    let tail_owner = allocation;
                    let tail_slot = into_slot<Pulse>(raw);
                    let tail_life = lifetime_domain();
                    let tail = loan_read(tail_life) { |stable|
                        initialize(tail_slot, Pulse {
                            onward: Option<ptr<Pulse>>::None, marker: u8(163)
                        }, stable)
                    };
                    let previous_link = loan_read(head_life) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        replace(writer@onward, Option<ptr<Pulse>>::Some(tail))
                    };
                    let observed_link = loan_read(head_life) { |stable|
                        let watcher = ref_from_ptr(read, head, stable);
                        read(watcher@onward)
                    };
                    match observed_link {
                        None => { unit },
                        Some(seen_tail) => {
                            loan_read(tail_life) { |stable|
                                let sample = ref_from_ptr(read, seen_tail, stable);
                                unit
                            };
                            unit
                        },
                    };
                    let returned = loan_read(head_life) { |stable|
                        let writer = ref_from_ptr(write, head, stable);
                        detach_pulse(writer@onward, tail, tail_owner, tail_life)
                    };
                    let LiveTail { owned_ptr, owned_allocation,
                                   owned_domain } = returned;
                    loan_read(owned_domain) { |stable|
                        let returned_view = ref_from_ptr(read, owned_ptr, stable);
                        unit
                    };
                    finish_pulse(owned_ptr, owned_allocation, owned_domain);
                    let head_vacant = loan_exclusive_read(head_life) { |ending|
                        destroy(head, ending)
                    };
                    let head_storage = erase_slot<Pulse>(head_vacant);
                    finalize_domain(head_life);
                    deallocate(head_owner, head_storage);
                    unit
                },
            }
        },
    }
}
