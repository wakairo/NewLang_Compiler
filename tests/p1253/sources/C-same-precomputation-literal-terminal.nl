struct QueueCell { next: Option<ptr<QueueCell>>, tag: u8 }
struct SettingsPair { sample: u8, witness: u8 }

fn detach_cell(edge: ref<write,Option<ptr<QueueCell>>>, token: ptr<QueueCell>, handle: Allocation, domain: LifetimeDomain) -> LiveTail {
    let discarded = replace(edge, Option<ptr<QueueCell>>::None);
    return LiveTail { owned_ptr: token, owned_allocation: handle, owned_domain: domain };
}

fn release_cell(token: ptr<QueueCell>, handle: Allocation, domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let alive = ref_from_ptr(read, token, stable);
        unit
    };
    let empty = loan_exclusive_read(domain) { |ending| destroy(token, ending) };
    let full = erase_slot<QueueCell>(empty);
    finalize_domain(domain);
    deallocate(handle, full);
    unit
}

fn advance_code(index: usize) -> u8 {
    if (index < usize(1)) {
        u8(103)
    } else {
        u8(167)
    }
}

fn select_restore(seed: usize) -> u8 {
    if (seed == usize(0)) {
        u8(43)
    } else {
        u8(229)
    }
}

fn main() -> unit {
    let settings = SettingsPair { sample: u8(67), witness: u8(181) };
    match try_allocate_one<QueueCell>() {
        None => { unit },
        Some(first) => {
            let OneBacking { allocation, raw } = first;
            let head_allocation = allocation;
            let head_slot = into_slot<QueueCell>(raw);
            let head_domain = lifetime_domain();
            let head_ptr = loan_read(head_domain) { |stable_head|
                initialize(head_slot, QueueCell { next: Option<ptr<QueueCell>>::None, tag: u8(53) }, stable_head)
            };
            match try_allocate_one<QueueCell>() {
                None => {
                    let vacant_head = loan_exclusive_read(head_domain) { |ending_head| destroy(head_ptr, ending_head) };
                    let full_head = erase_slot<QueueCell>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, full_head);
                    unit
                },
                Some(second) => {
                    let OneBacking { allocation, raw } = second;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<QueueCell>(raw);
                    let tail_domain = lifetime_domain();
                    let tail_ptr = loan_read(tail_domain) { |stable_tail|
                        initialize(tail_slot, QueueCell { next: Option<ptr<QueueCell>>::None, tag: u8(227) }, stable_tail)
                    };
                    let old_none = loan_read(head_domain) { |stable_head|
                        let link_write = ref_from_ptr(write, head_ptr, stable_head);
                        replace(link_write@next, Option<ptr<QueueCell>>::Some(tail_ptr))
                    };
                    let linked = loan_read(head_domain) { |stable_head|
                        let link_read = ref_from_ptr(read, head_ptr, stable_head);
                        read(link_read@next)
                    };
                    match linked {
                        None => { unit },
                        Some(found) => {
                            loan_read(tail_domain) { |stable_tail|
                                let checked_tail = ref_from_ptr(read, found, stable_tail);
                                unit
                            }
                        },
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(head_domain) { |stable_head|
                        let link_write_again = ref_from_ptr(write, head_ptr, stable_head);
                        detach_cell(link_write_again@next, tail_ptr, tail_allocation, tail_domain)
                    };
                    loan_read(owned_domain) { |stable_tail|
                        let checked_return = ref_from_ptr(read, owned_ptr, stable_tail);
                        unit
                    };
                    release_cell(owned_ptr, owned_allocation, owned_domain);
                    let vacant_head = loan_exclusive_read(head_domain) { |ending_head| destroy(head_ptr, ending_head) };
                    let full_head = erase_slot<QueueCell>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, full_head);
                    unit
                },
            }
        },
    };
    let chosen_code = select_restore(usize(0));
    let saved_code = chosen_code;
    let answer = loop (turn = usize(0)) {
        if (turn < usize(2)) {
            let requested = advance_code(turn);
            let old_step = loan_write(settings@sample) { |slot| replace(slot, requested) };
            let prior_copy = old_step;
            prior_copy;
            let step_current = settings@sample;
            let step_sibling = settings@witness;
            step_current;
            step_sibling;
            continue(turn + usize(1));
        } else {
            let terminal_old = loan_write(settings@sample) { |slot| replace(slot, u8(43)) };
            let terminal_copy = terminal_old;
            break terminal_copy;
        }
    };
    let saved_answer = answer;
    let post_value = settings@sample;
    let post_witness = settings@witness;
    unit
}
