// R756 fresh BLIND witness B (interposed T helper; T before H)
struct Metric { top:u8, base:u8 }
fn recalibrate(place:ref<write,u8>, next:u8) -> u8 {
    let before = replace(place,next);
    return before;
}
struct Stem { child:Option<ptr<Metric>>, payload:u8 }
fn extract_tail(edge: ref<write,Option<ptr<Stem>>>, held:ptr<Stem>, backing:Allocation, lifetime:LifetimeDomain) -> LiveTail {
    let previous_edge = replace(edge,Option<ptr<Stem>>::None);
    let returning = LiveTail { owned_ptr:held, owned_allocation:backing, owned_domain:lifetime };
    return returning;
}
fn release_tail(owned:ptr<Stem>, allocated:Allocation, domain:LifetimeDomain) -> unit {
    loan_read(domain) { |active| let observed = ref_from_ptr(read,owned,active); unit };
    let now_empty = loan_exclusive_read(domain) { |final| destroy(owned,final) };
    let whole = erase_slot<Stem>(now_empty);
    finalize_domain(domain);
    deallocate(allocated,whole);
    unit
}
fn main() -> unit {
    let first = Metric { top:u8(38), base:u8(64) };
    let second = Metric { base:u8(70), top:u8(11) };
    let old_first = loan_write(first@top) { |w| recalibrate(w,u8(82)) };
    let old_second = loan_write(second@base) { |w| replace(w,old_first) };
    let first_current = first@top;
    let first_sibling = first@base;
    let second_current = second@base;
    let second_sibling = second@top;

    match try_allocate_one<Stem>() {
        None => { unit },
        Some(front_bundle) => {
            let OneBacking { allocation,raw } = front_bundle;
            let front_alloc = allocation;
            let front_slot = into_slot<Stem>(raw);
            let front_domain = lifetime_domain();
            let front_ptr = loan_read(front_domain) { |stable_front|
                initialize(front_slot,Stem { child:Option<ptr<Stem>>::None, payload:u8(23) },stable_front)
            };
            match try_allocate_one<Stem>() {
                None => {
                    let front_empty = loan_exclusive_read(front_domain) { |end_front| destroy(front_ptr,end_front) };
                    let front_full = erase_slot<Stem>(front_empty);
                    finalize_domain(front_domain);
                    deallocate(front_alloc,front_full);
                    unit
                },
                Some(back_bundle) => {
                    let OneBacking { allocation,raw } = back_bundle;
                    let back_alloc = allocation;
                    let back_slot = into_slot<Stem>(raw);
                    let back_domain = lifetime_domain();
                    let back_ptr = loan_read(back_domain) { |stable_back|
                        initialize(back_slot,Stem { child:Option<ptr<Stem>>::None, payload:u8(29) },stable_back)
                    };
                    let replaced_empty = loan_read(front_domain) { |stable_front|
                        let front_write = ref_from_ptr(write,front_ptr,stable_front);
                        replace(front_write@child,Option<ptr<Stem>>::Some(back_ptr))
                    };
                    let previously_linked = loan_read(front_domain) { |stable_front|
                        let front_read = ref_from_ptr(read,front_ptr,stable_front);
                        read(front_read@child)
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
                        extract_tail(front_write@child,back_ptr,back_alloc,back_domain)
                    };
                    let LiveTail { owned_ptr,owned_allocation,owned_domain } = packet;
                    release_tail(owned_ptr,owned_allocation,owned_domain);
                    let front_empty = loan_exclusive_read(front_domain) { |end_front| destroy(front_ptr,end_front) };
                    let front_full = erase_slot<Stem>(front_empty);
                    finalize_domain(front_domain);
                    deallocate(front_alloc,front_full);
                    unit
                },
            }
        },
    }
}
