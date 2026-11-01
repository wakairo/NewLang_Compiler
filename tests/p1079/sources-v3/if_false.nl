struct Readings { level: u8, cached: u8 }
fn overwrite_reading(dst: ref<write,u8>, incoming: u8) -> u8 {
    replace(dst, incoming)
}
fn retain_reading(dst: ref<write,u8>, incoming: u8) -> u8 {
    let previous = replace(dst, incoming);
    let saved = previous;
    return saved;
}
struct StepNode { next: Option<ptr<StepNode>>, marker: u8 }
fn unlink_step(edge: ref<write,Option<ptr<StepNode>>>, item: ptr<StepNode>,
               allocation: Allocation, domain: LifetimeDomain) -> LiveTail {
    let old_link = replace(edge, Option<ptr<StepNode>>::None);
    let packet = LiveTail{owned_ptr: item, owned_allocation: allocation, owned_domain: domain};
    return packet;
}
fn dispose_step(item: ptr<StepNode>, allocation: Allocation, domain: LifetimeDomain) -> unit {
    let vacancy = loan_exclusive_read(domain) { |ending| destroy(item, ending) };
    let released_storage = erase_slot<StepNode>(vacancy);
    finalize_domain(domain);
    deallocate(allocation, released_storage);
    unit
}
fn main() -> unit {
    match try_allocate_one<StepNode>() {
        None => { unit },
        Some(head_package) => {
            let OneBacking { allocation, raw } = head_package;
            let head_allocation = allocation;
            let head_slot = into_slot<StepNode>(raw);
            let head_domain = lifetime_domain();
            let head = loan_read(head_domain) { |scope|
                initialize(head_slot, StepNode{next: Option<ptr<StepNode>>::None, marker: u8(17)}, scope)
            };
            match try_allocate_one<StepNode>() {
                None => {
                    let head_vacancy = loan_exclusive_read(head_domain) { |ending| destroy(head, ending) };
                    let head_erased = erase_slot<StepNode>(head_vacancy);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, head_erased);
                    unit
                },
                Some(tail_package) => {
                    let OneBacking { allocation, raw } = tail_package;
                    let tail_allocation = allocation;
                    let tail_slot = into_slot<StepNode>(raw);
                    let tail_domain = lifetime_domain();
                    let tail = loan_read(tail_domain) { |scope|
                        initialize(tail_slot, StepNode{next: Option<ptr<StepNode>>::None, marker: u8(229)}, scope)
                    };
                    let prior_empty = loan_read(head_domain) { |scope|
                        let access_head = ref_from_ptr(write, head, scope);
                        replace(access_head@next, Option<ptr<StepNode>>::Some(tail))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(head_domain) { |scope|
                            let access_head = ref_from_ptr(write, head, scope);
                            unlink_step(access_head@next, tail, tail_allocation, tail_domain)
                        };
                    loan_read(owned_domain) { |scope|
                        let access_tail = ref_from_ptr(read, owned_ptr, scope);
                        unit
                    };
                    dispose_step(owned_ptr, owned_allocation, owned_domain);
                    let head_done = loan_exclusive_read(head_domain) { |ending| destroy(head, ending) };
                    let head_bytes = erase_slot<StepNode>(head_done);
                    finalize_domain(head_domain);
                    deallocate(head_allocation, head_bytes);
                    unit
                },
            }
        },
    };
    let primary = Readings{level: u8(47), cached: u8(129)};
    let secondary = Readings{cached: u8(83), level: u8(211)};
    let old_primary = loan_write(primary@level) { |writer| overwrite_reading(writer, u8(155)) };
    let old_secondary = loan_write(secondary@cached) { |writer| retain_reading(writer, old_primary) };
    let selected_old = if (usize(11) < usize(4)) {
        loan_write(secondary@level) { |writer| replace(writer, old_primary) }
    } else {
        loan_write(secondary@level) { |writer| replace(writer, old_secondary) }
    };
    let last_old = loan_write(primary@cached) { |writer| replace(writer, selected_old) };
    let observed_primary = old_primary;
    let observed_secondary = old_secondary;
    let observed_selected = selected_old;
    let observed_last = last_old;
    let final_primary_level = primary@level;
    let final_primary_cached = primary@cached;
    let final_secondary_level = secondary@level;
    let final_secondary_cached = secondary@cached;
    return unit;
}
