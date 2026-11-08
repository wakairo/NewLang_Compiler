struct ChainLink { next: Option<ptr<ChainLink>>, tag: u8 }
struct ByteRegister { sample: u8, witness: u8 }

fn detach_link(edge: ref<write,Option<ptr<ChainLink>>>, token: ptr<ChainLink>, handle: Allocation, domain: LifetimeDomain) -> LiveTail {
    let discarded = replace(edge, Option<ptr<ChainLink>>::None);
    return LiveTail { owned_ptr: token, owned_allocation: handle, owned_domain: domain };
}

fn release_link(token: ptr<ChainLink>, handle: Allocation, domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable|
        let alive = ref_from_ptr(read, token, stable);
        unit
    };
    let empty = loan_exclusive_read(domain) { |ending| destroy(token, ending) };
    let full = erase_slot<ChainLink>(empty);
    finalize_domain(domain);
    deallocate(handle, full);
    unit
}

fn byte_at_phase(index: usize) -> u8 {
    if (index == usize(2)) {
        u8(211)
    } else {
        u8(83)
    }
}

fn main() -> unit {
    let register = ByteRegister { sample: u8(29), witness: u8(173) };
    match try_allocate_one<ChainLink>() {
        None => { unit },
        Some(first) => {
            let OneBacking { allocation, raw } = first;
            let head_allocation = allocation;
            let head_slot = into_slot<ChainLink>(raw);
            let head_domain = lifetime_domain();
            let head_ptr = loan_read(head_domain) { |stable_head|
                initialize(head_slot, ChainLink { next: Option<ptr<ChainLink>>::None, tag: u8(47) }, stable_head)
            };
            match try_allocate_one<ChainLink>() {
                None => {
                    let vacant_head = loan_exclusive_read(head_domain) { |ending_head| destroy(head_ptr, ending_head) };
                    let full_head = erase_slot<ChainLink>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, full_head);
                    unit
                },
                Some(second) => {
                    let OneBacking { allocation, raw } = second;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<ChainLink>(raw);
                    let tail_domain = lifetime_domain();
                    let tail_ptr = loan_read(tail_domain) { |stable_tail|
                        initialize(tail_slot, ChainLink { next: Option<ptr<ChainLink>>::None, tag: u8(191) }, stable_tail)
                    };
                    let old_none = loan_read(head_domain) { |stable_head|
                        let link_write = ref_from_ptr(write, head_ptr, stable_head);
                        replace(link_write@next, Option<ptr<ChainLink>>::Some(tail_ptr))
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
                        detach_link(link_write_again@next, tail_ptr, tail_allocation, tail_domain)
                    };
                    loan_read(owned_domain) { |stable_tail|
                        let checked_return = ref_from_ptr(read, owned_ptr, stable_tail);
                        unit
                    };
                    release_link(owned_ptr, owned_allocation, owned_domain);
                    let vacant_head = loan_exclusive_read(head_domain) { |ending_head| destroy(head_ptr, ending_head) };
                    let full_head = erase_slot<ChainLink>(vacant_head);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, full_head);
                    unit
                },
            }
        },
    };
    let answer = loop (turn = usize(0)) {
        if (turn < usize(2)) {
            let requested = byte_at_phase(turn);
            let old_step = loan_write(register@sample) { |slot| replace(slot, requested) };
            let prior_copy = old_step;
            prior_copy;
            let step_current = register@sample;
            let step_sibling = register@witness;
            step_current;
            step_sibling;
            continue(turn + usize(1));
        } else {
            let computed_new = byte_at_phase(turn);
            let copied_new = computed_new;
            let terminal_old = loan_write(register@witness) { |slot| replace(slot, copied_new) };
            let terminal_copy = terminal_old;
            break terminal_copy;
        }
    };
    let saved_answer = answer;
    let post_value = register@sample;
    let post_witness = register@witness;
    unit
}
