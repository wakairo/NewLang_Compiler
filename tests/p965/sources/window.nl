struct Window { ceiling: u8, floor: u8 }

fn adjust_ceiling(location: ref<write,u8>, incoming: u8) -> u8 {
    let before = replace(location, incoming);
    return before;
}

struct WorkItem { onward: Option<ptr<WorkItem>>, code: u8 }

fn dispose_work(child: ptr<WorkItem>, allocation: Allocation,
                   life: LifetimeDomain) -> unit {
    loan_read(life) { |stable|
        let view = ref_from_ptr(read, child, stable);
        unit
    };
    let empty = loan_exclusive_read(life) { |end_scope|
        destroy(child, end_scope)
    };
    let all_bytes = erase_slot<WorkItem>(empty);
    finalize_domain(life);
    deallocate(allocation, all_bytes);
    unit
}

fn take_work(edge: ref<write,Option<ptr<WorkItem>>>,
                   child: ptr<WorkItem>, allocation: Allocation,
                   life: LifetimeDomain) -> LiveTail {
    let released = replace(edge, Option<ptr<WorkItem>>::None);
    let carried_owner = LiveTail {
        owned_ptr: child,
        owned_allocation: allocation,
        owned_domain: life
    };
    return carried_owner;
}

fn main() -> unit {
    match try_allocate_one<WorkItem>() {
        None => { unit },
        Some(origin_box) => {
            let OneBacking { allocation, raw } = origin_box;
            let origin_allocation = allocation;
            let origin_slot = into_slot<WorkItem>(raw);
            let origin_life = lifetime_domain();
            let origin_ptr = loan_read(origin_life) { |stable|
                initialize(origin_slot, WorkItem {
                    onward: Option<ptr<WorkItem>>::None, code: u8(41)
                }, stable)
            };
            match try_allocate_one<WorkItem>() {
                None => {
                    let voided = loan_exclusive_read(origin_life) { |ending|
                        destroy(origin_ptr, ending)
                    };
                    let bytes = erase_slot<WorkItem>(voided);
                    finalize_domain(origin_life);
                    deallocate(origin_allocation, bytes);
                    unit
                },
                Some(other_box) => {
                    let OneBacking { allocation, raw } = other_box;
                    let other_allocation = allocation;
                    let other_slot = into_slot<WorkItem>(raw);
                    let other_life = lifetime_domain();
                    let other_ptr = loan_read(other_life) { |stable|
                        initialize(other_slot, WorkItem {
                            onward: Option<ptr<WorkItem>>::None, code: u8(197)
                        }, stable)
                    };
                    let active = Window { ceiling: u8(155), floor: u8(73) };
                    let reserve = Window { floor: u8(209), ceiling: u8(17) };
                    let choice = if (usize(4) < usize(19)) { u8(128) } else { u8(66) };
                    let prior = loan_write(active@ceiling) { |w|
                        adjust_ceiling(w, choice)
                    };
                    let displaced = loan_write(reserve@floor) { |w|
                        replace(w, prior)
                    };
                    let was_none = loan_read(origin_life) { |stable|
                        let write_head = ref_from_ptr(write, origin_ptr, stable);
                        replace(write_head@onward,
                                Option<ptr<WorkItem>>::Some(other_ptr))
                    };
                    let received = loan_read(origin_life) { |stable|
                        let write_head = ref_from_ptr(write, origin_ptr, stable);
                        take_work(write_head@onward, other_ptr,
                                        other_allocation, other_life)
                    };
                    let LiveTail { owned_ptr, owned_allocation,
                                   owned_domain } = received;
                    dispose_work(owned_ptr, owned_allocation, owned_domain);
                    let vanished = loan_exclusive_read(origin_life) { |ending|
                        destroy(origin_ptr, ending)
                    };
                    let bytes = erase_slot<WorkItem>(vanished);
                    finalize_domain(origin_life);
                    deallocate(origin_allocation, bytes);
                    unit
                },
            }
        },
    }
}
