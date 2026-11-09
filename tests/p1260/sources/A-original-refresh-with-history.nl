struct WorkLink { next: Option<ptr<WorkLink>>, stamp: u8 }

fn release_work(item: ptr<WorkLink>, backing: Allocation, age: LifetimeDomain) -> unit {
    loan_read(age) { |inspection|
        let readable = ref_from_ptr(read, item, inspection);
        unit
    };
    let vacant = loan_exclusive_read(age) { |ending| destroy(item, ending) };
    let complete = erase_slot<WorkLink>(vacant);
    finalize_domain(age);
    deallocate(backing, complete);
    unit
}

struct RefreshState { active: u8, previous: u8 }

fn requested_level(mode: usize) -> u8 {
    if (mode < usize(4)) { u8(197) } else { u8(83) }
}

fn warming_level(round: usize) -> u8 {
    if (round < usize(1)) { u8(41) } else { u8(88) }
}

fn remember_level(destination: ref<write,u8>, displaced: u8) -> u8 {
    let former_history = replace(destination, displaced);
    former_history
}

fn main() -> unit {
    let state = RefreshState { active: u8(23), previous: u8(164) };
    match try_allocate_one<WorkLink>() {
        None => { unit },
        Some(front) => {
            let OneBacking { allocation, raw } = front;
            let front_backing = allocation;
            let front_slot = into_slot<WorkLink>(raw);
            let front_age = lifetime_domain();
            let front_ptr = loan_read(front_age) { |front_guard|
                initialize(front_slot, WorkLink { next: Option<ptr<WorkLink>>::None, stamp: u8(72) }, front_guard)
            };
            match try_allocate_one<WorkLink>() {
                None => {
                    let empty_front = loan_exclusive_read(front_age) { |ending_front| destroy(front_ptr, ending_front) };
                    let front_bytes = erase_slot<WorkLink>(empty_front);
                    finalize_domain(front_age);
                    deallocate(front_backing, front_bytes);
                    unit
                },
                Some(back) => {
                    let OneBacking { allocation, raw } = back;
                    let back_backing = allocation;
                    let back_slot = into_slot<WorkLink>(raw);
                    let back_age = lifetime_domain();
                    let back_ptr = loan_read(back_age) { |back_guard|
                        initialize(back_slot, WorkLink { next: Option<ptr<WorkLink>>::None, stamp: u8(203) }, back_guard)
                    };
                    let vacant_link = loan_read(front_age) { |connecting|
                        let writable = ref_from_ptr(write, front_ptr, connecting);
                        replace(writable@next, Option<ptr<WorkLink>>::Some(back_ptr))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(front_age) { |disconnecting|
                        let writable = ref_from_ptr(write, front_ptr, disconnecting);
                        detach_work(writable@next, back_ptr, back_backing, back_age)
                    };
                    release_work(owned_ptr, owned_allocation, owned_domain);
                    let ended_front = loan_exclusive_read(front_age) { |last_front| destroy(front_ptr, last_front) };
                    let complete_front = erase_slot<WorkLink>(ended_front);
                    finalize_domain(front_age);
                    deallocate(front_backing, complete_front);
                    unit
                },
            };
            unit
        },
    };
    let selected = requested_level(usize(2));
    let saved_level = selected;
    let displaced = loop (round = usize(0)) {
        if (round < usize(3)) {
            let incoming = warming_level(round);
            let old_warming = loan_write(state@active) { |active_slot|
                replace(active_slot, incoming)
            };
            continue(round + usize(1));
        } else {
            let old_active = loan_write(state@active) { |active_slot|
                replace(active_slot, saved_level)
            };
            break old_active;
        }
    };
    let audit_copy = displaced;
    let former_history = loan_write(state@previous) { |history_slot|
        remember_level(history_slot, audit_copy)
    };
    let applied = state@active;
    let history = state@previous;
    unit
}

fn detach_work(link: ref<write,Option<ptr<WorkLink>>>, item: ptr<WorkLink>, backing: Allocation, age: LifetimeDomain) -> LiveTail {
    let removed = replace(link, Option<ptr<WorkLink>>::None);
    return LiveTail { owned_ptr: item, owned_allocation: backing, owned_domain: age };
}
