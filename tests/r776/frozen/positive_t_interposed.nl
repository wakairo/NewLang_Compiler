// Independent R #776 first-pass source. No producer fixture imported.
struct ControlBits { lower: u8, upper: u8, }
fn revise_byte(w: ref<write,u8>, value: u8) -> u8 {
    let prior = replace(w, value);
    return prior;
}
struct QueueAtom { next: Option<ptr<QueueAtom>>, payload: u8, }
fn handoff_tail(edge: ref<write,Option<ptr<QueueAtom>>>, original: ptr<QueueAtom>, backing: Allocation, lifetime: LifetimeDomain) -> LiveTail {
    let displaced = replace(edge, Option<ptr<QueueAtom>>::None);
    let custody = LiveTail { owned_ptr: original, owned_allocation: backing, owned_domain: lifetime };
    return custody;
}
fn finish_tail(original: ptr<QueueAtom>, backing: Allocation, lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |stable| let r = ref_from_ptr(read, original, stable); unit };
    let empty = loan_exclusive_read(lifetime) { |exclusive| destroy(original, exclusive) };
    let full = erase_slot<QueueAtom>(empty);
    finalize_domain(lifetime);
    deallocate(backing, full);
    unit
}

fn main() -> unit {
    match try_allocate_one<QueueAtom>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<QueueAtom>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h| initialize(vacant_h, QueueAtom { next: Option<ptr<QueueAtom>>::None, payload: u8(83) }, stable_h) };
            match try_allocate_one<QueueAtom>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |end_h| destroy(ptr_h, end_h) };
                    let full_h = erase_slot<QueueAtom>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<QueueAtom>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t| initialize(vacant_t, QueueAtom { next: Option<ptr<QueueAtom>>::None, payload: u8(237) }, stable_t) };
                    let prior_link = loan_read(life_h) { |stable_h| let head_w = ref_from_ptr(write, ptr_h, stable_h); replace(head_w@next, Option<ptr<QueueAtom>>::Some(ptr_t)) };
                    let returned = loan_read(life_h) { |stable_h| let head_w2 = ref_from_ptr(write, ptr_h, stable_h); handoff_tail(head_w2@next, ptr_t, allocation_t, life_t) };
                let panel = ControlBits { upper: u8(184), lower: u8(7) };
                let reserve = ControlBits { lower: u8(66), upper: u8(118) };
                let prior_panel = loan_write(panel@upper) { |rw| revise_byte(rw, u8(27)) };
                let prior_reserve = loan_write(reserve@lower) { |rw| replace(rw, prior_panel) };
                let panel_result = panel@upper;
                let panel_other = panel@lower;
                let reserve_result = reserve@lower;
                let reserve_other = reserve@upper;
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = returned;
                    loan_read(owned_domain) { |stable_t| let proof = ref_from_ptr(read, owned_ptr, stable_t); unit };
                    finish_tail(owned_ptr, owned_allocation, owned_domain);
                    let empty_h = loan_exclusive_read(life_h) { |end_h| destroy(ptr_h, end_h) };
                    let full_h = erase_slot<QueueAtom>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
