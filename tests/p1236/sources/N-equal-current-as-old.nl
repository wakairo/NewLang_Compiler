struct HeapLink { next: Option<ptr<HeapLink>>, tag: u8 }
struct LedgerPair { active: u8, marker: u8 }

fn take_link(edge: ref<write,Option<ptr<HeapLink>>>, token: ptr<HeapLink>, handle: Allocation, domain: LifetimeDomain) -> LiveTail {
    let discarded = replace(edge, Option<ptr<HeapLink>>::None);
    return LiveTail { owned_ptr: token, owned_allocation: handle, owned_domain: domain };
}

fn finish_link(token: ptr<HeapLink>, handle: Allocation, domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let alive = ref_from_ptr(read, token, stable);
        unit
    };
    let empty = loan_exclusive_read(domain) { |ending| destroy(token, ending) };
    let full = erase_slot<HeapLink>(empty);
    finalize_domain(domain);
    deallocate(handle, full);
    unit
}

fn next_byte(index: usize) -> u8 {
    if (index < usize(1)) {
        u8(79)
    } else {
        u8(157)
    }
}

fn main() -> unit {
    let original = LedgerPair { active: u8(37), marker: u8(223) };
    match try_allocate_one<HeapLink>() {
        None => { unit },
        Some(first) => {
            let OneBacking { allocation, raw } = first;
            let head_allocation = allocation;
            let head_slot = into_slot<HeapLink>(raw);
            let head_domain = lifetime_domain();
            let head_ptr = loan_read(head_domain) { |stable_head|
                initialize(head_slot, HeapLink { next: Option<ptr<HeapLink>>::None, tag: u8(43) }, stable_head)
            };
            match try_allocate_one<HeapLink>() {
                None => {
                    let vacant_head = loan_exclusive_read(head_domain) { |ending_head| destroy(head_ptr, ending_head) };
                    let full_head = erase_slot<HeapLink>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, full_head);
                    unit
                },
                Some(second) => {
                    let OneBacking { allocation, raw } = second;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<HeapLink>(raw);
                    let tail_domain = lifetime_domain();
                    let tail_ptr = loan_read(tail_domain) { |stable_tail|
                        initialize(tail_slot, HeapLink { next: Option<ptr<HeapLink>>::None, tag: u8(181) }, stable_tail)
                    };
                    let old_none = loan_read(head_domain) { |stable_head|
                        let link_write = ref_from_ptr(write, head_ptr, stable_head);
                        replace(link_write@next, Option<ptr<HeapLink>>::Some(tail_ptr))
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
                        take_link(link_write_again@next, tail_ptr, tail_allocation, tail_domain)
                    };
                    loan_read(owned_domain) { |stable_tail|
                        let checked_return = ref_from_ptr(read, owned_ptr, stable_tail);
                        unit
                    };
                    finish_link(owned_ptr, owned_allocation, owned_domain);
                    let vacant_head = loan_exclusive_read(head_domain) { |ending_head| destroy(head_ptr, ending_head) };
                    let full_head = erase_slot<HeapLink>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, full_head);
                    unit
                },
            }
        },
    };
    let answer = loop (turn = usize(0)) {
        if (turn < usize(2)) {
            let requested = next_byte(turn);
            let old_step = loan_write(original@active) { |slot| replace(slot, requested) };
            let prior_copy = old_step;
            prior_copy;
            let step_current = original@active;
            let step_sibling = original@marker;
            step_current;
            step_sibling;
            continue(turn + usize(1));
        } else {
            let terminal_old = loan_write(original@active) { |slot| replace(slot, u8(157)) };
            let terminal_copy = original@active;
            break terminal_copy;
        }
    };
    let saved_answer = answer;
    let post_value = original@active;
    let post_marker = original@marker;
    unit
}
