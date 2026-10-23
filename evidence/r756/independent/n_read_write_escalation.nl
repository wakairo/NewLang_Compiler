// R756 fresh BLIND witness A (independently constructed; H before T)
struct Reach { successor:Option<ptr<Reach>>, payload:u8 }
struct Ticket { incoming:u8, outgoing:u8 }
fn retune(place:ref<write,u8>, next:u8) -> u8 {
    let before = replace(place,next);
    return before;
}
fn separate_tail(edge: ref<write,Option<ptr<Reach>>>, held:ptr<Reach>, backing:Allocation, lifetime:LifetimeDomain) -> LiveTail {
    let previous_edge = replace(edge,Option<ptr<Reach>>::None);
    let returning = LiveTail { owned_ptr:held, owned_allocation:backing, owned_domain:lifetime };
    return returning;
}
fn retire_tail(owned:ptr<Reach>, allocated:Allocation, domain:LifetimeDomain) -> unit {
    loan_read(domain) { |active| let observed = ref_from_ptr(read,owned,active); unit };
    let now_empty = loan_exclusive_read(domain) { |final| destroy(owned,final) };
    let whole = erase_slot<Reach>(now_empty);
    finalize_domain(domain);
    deallocate(allocated,whole);
    unit
}
fn main() -> unit {
    let first = Ticket { incoming:u8(17), outgoing:u8(26) };
    let second = Ticket { outgoing:u8(91), incoming:u8(44) };
    let old_first = loan_read(first@incoming) { |w| retune(w,u8(63)) };
    let old_second = loan_write(second@outgoing) { |w| replace(w,old_first) };
    let first_current = first@incoming;
    let first_sibling = first@outgoing;
    let second_current = second@outgoing;
    let second_sibling = second@incoming;

    match try_allocate_one<Reach>() {
        None => { unit },
        Some(front_bundle) => {
            let OneBacking { allocation,raw } = front_bundle;
            let front_alloc = allocation;
            let front_slot = into_slot<Reach>(raw);
            let front_domain = lifetime_domain();
            let front_ptr = loan_read(front_domain) { |stable_front|
                initialize(front_slot,Reach { successor:Option<ptr<Reach>>::None, payload:u8(9) },stable_front)
            };
            match try_allocate_one<Reach>() {
                None => {
                    let front_empty = loan_exclusive_read(front_domain) { |end_front| destroy(front_ptr,end_front) };
                    let front_full = erase_slot<Reach>(front_empty);
                    finalize_domain(front_domain);
                    deallocate(front_alloc,front_full);
                    unit
                },
                Some(back_bundle) => {
                    let OneBacking { allocation,raw } = back_bundle;
                    let back_alloc = allocation;
                    let back_slot = into_slot<Reach>(raw);
                    let back_domain = lifetime_domain();
                    let back_ptr = loan_read(back_domain) { |stable_back|
                        initialize(back_slot,Reach { successor:Option<ptr<Reach>>::None, payload:u8(12) },stable_back)
                    };
                    let replaced_empty = loan_read(front_domain) { |stable_front|
                        let front_write = ref_from_ptr(write,front_ptr,stable_front);
                        replace(front_write@successor,Option<ptr<Reach>>::Some(back_ptr))
                    };
                    let previously_linked = loan_read(front_domain) { |stable_front|
                        let front_read = ref_from_ptr(read,front_ptr,stable_front);
                        read(front_read@successor)
                    };
                    match previously_linked {
                        None => { unit },
                        Some(found) => { loan_read(back_domain) { |stable_back|
                            let back_read = ref_from_ptr(read,found,stable_back);
                            unit
                        } }
                    };
                    let packet = loan_read(front_domain) { |stable_front|
                        let front_write = ref_from_ptr(write,front_ptr,stable_front);
                        separate_tail(front_write@successor,back_ptr,back_alloc,back_domain)
                    };
                    let LiveTail { owned_ptr,owned_allocation,owned_domain } = packet;
                    retire_tail(owned_ptr,owned_allocation,owned_domain);
                    let front_empty = loan_exclusive_read(front_domain) { |end_front| destroy(front_ptr,end_front) };
                    let front_full = erase_slot<Reach>(front_empty);
                    finalize_domain(front_domain);
                    deallocate(front_alloc,front_full);
                    unit
                },
            }
        },
    }
}
