struct Spool { next: Option<ptr<Spool>>, stamp: u8 }

fn dispose_spool(item: ptr<Spool>, backing: Allocation, age: LifetimeDomain) -> unit {
    loan_read(age) { |inspection|
        let readable = ref_from_ptr(read, item, inspection);
        unit
    };
    let vacant = loan_exclusive_read(age) { |ending| destroy(item, ending) };
    let complete = erase_slot<Spool>(vacant);
    finalize_domain(age);
    deallocate(backing, complete);
    unit
}

struct RestorationState { previous: u8, active: u8 }

fn choose_target(mode: usize) -> u8 {
    if (mode == usize(0)) { u8(219) } else { u8(151) }
}

fn tick_value(round: usize) -> u8 {
    if (round == usize(0)) { u8(137) } else { u8(219) }
}

fn archive_byte(destination: ref<write,u8>, displaced: u8) -> u8 {
    let former_history = replace(destination, displaced);
    return former_history;
}

fn main() -> unit {
    let session = RestorationState { active: u8(31), previous: u8(151) };
    match try_allocate_one<Spool>() {
        None => { unit },
        Some(front) => {
            let OneBacking { allocation, raw } = front;
            let front_backing = allocation;
            let front_slot = into_slot<Spool>(raw);
            let front_age = lifetime_domain();
            let front_ptr = loan_read(front_age) { |front_guard|
                initialize(front_slot, Spool { next: Option<ptr<Spool>>::None, stamp: u8(72) }, front_guard)
            };
            match try_allocate_one<Spool>() {
                None => {
                    let empty_front = loan_exclusive_read(front_age) { |ending_front| destroy(front_ptr, ending_front) };
                    let front_bytes = erase_slot<Spool>(empty_front);
                    finalize_domain(front_age);
                    deallocate(front_backing, front_bytes);
                    unit
                },
                Some(back) => {
                    let OneBacking { allocation, raw } = back;
                    let back_backing = allocation;
                    let back_slot = into_slot<Spool>(raw);
                    let back_age = lifetime_domain();
                    let back_ptr = loan_read(back_age) { |back_guard|
                        initialize(back_slot, Spool { next: Option<ptr<Spool>>::None, stamp: u8(203) }, back_guard)
                    };
                    let vacant_link = loan_read(front_age) { |connecting|
                        let writable = ref_from_ptr(write, front_ptr, connecting);
                        replace(writable@next, Option<ptr<Spool>>::Some(back_ptr))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(front_age) { |disconnecting|
                        let writable = ref_from_ptr(write, front_ptr, disconnecting);
                        detach_spool(writable@next, back_ptr, back_backing, back_age)
                    };
                    dispose_spool(owned_ptr, owned_allocation, owned_domain);
                    let ended_front = loan_exclusive_read(front_age) { |last_front| destroy(front_ptr, last_front) };
                    let complete_front = erase_slot<Spool>(ended_front);
                    finalize_domain(front_age);
                    deallocate(front_backing, complete_front);
                    unit
                },
            };
            unit
        },
    };
    let target = choose_target(usize(3));
    let saved_target = target;
    let displaced = loop (round = usize(0)) {
        if (round < usize(2)) {
            let incoming = tick_value(round);
            let old_warming = loan_write(session@active) { |active_slot|
                replace(active_slot, incoming)
            };
            continue(round + usize(1));
        } else {
            let old_active = loan_write(session@active) { |active_slot|
                replace(active_slot, saved_target)
            };
            break old_active;
        }
    };
    let audit_copy = displaced;
    let former_history = loan_write(session@previous) { |history_slot|
        archive_byte(history_slot, audit_copy)
    };
    let replaced_policy = loan_write(session@active) { |restoring_slot|
        archive_byte(restoring_slot, former_history)
    };
    let replaced_policy_copy = replaced_policy;
    let applied = session@active;
    let history = session@previous;
    unit
}

fn detach_spool(link: ref<write,Option<ptr<Spool>>>, item: ptr<Spool>, backing: Allocation, age: LifetimeDomain) -> LiveTail {
    let removed = replace(link, Option<ptr<Spool>>::None);
    return LiveTail { owned_ptr: item, owned_allocation: backing, owned_domain: age };
}
