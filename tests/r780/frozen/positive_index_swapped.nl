// R #780 independent calibrated A-shape variation (no Production fixture). 
// R-author variant: changed nominal order, field indexes, literals and originals.
struct CacheLink { next: Option<ptr<CacheLink>>, payload: u8, }
struct PulseFrame { guard: u8, phase: u8, }
fn revise_byte(w: ref<write,u8>, value: u8) -> u8 {
    let prior = replace(w, value);
    return prior;
}
fn handoff_tail(edge: ref<write,Option<ptr<CacheLink>>>, original: ptr<CacheLink>, backing: Allocation, lifetime: LifetimeDomain) -> LiveTail {
    let displaced = replace(edge, Option<ptr<CacheLink>>::None);
    let custody = LiveTail { owned_ptr: original, owned_allocation: backing, owned_domain: lifetime };
    return custody;
}
fn finish_tail(original: ptr<CacheLink>, backing: Allocation, lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |stable| let r = ref_from_ptr(read, original, stable); unit };
    let empty = loan_exclusive_read(lifetime) { |exclusive| destroy(original, exclusive) };
    let full = erase_slot<CacheLink>(empty);
    finalize_domain(lifetime);
    deallocate(backing, full);
    unit
}

fn main() -> unit {
    let panel = PulseFrame { guard: u8(185), phase: u8(27) };
    let reserve = PulseFrame { phase: u8(9), guard: u8(211) };
    let prior_panel = loan_write(panel@phase) { |rw| revise_byte(rw, u8(132)) };
    let prior_reserve = loan_write(reserve@guard) { |rw| replace(rw, prior_panel) };
    let panel_result = panel@phase;
    let panel_other = panel@guard;
    let reserve_result = reserve@guard;
    let reserve_other = reserve@phase;
    match try_allocate_one<CacheLink>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<CacheLink>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h| initialize(vacant_h, CacheLink { next: Option<ptr<CacheLink>>::None, payload: u8(37) }, stable_h) };
            match try_allocate_one<CacheLink>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |end_h| destroy(ptr_h, end_h) };
                    let full_h = erase_slot<CacheLink>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<CacheLink>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t| initialize(vacant_t, CacheLink { next: Option<ptr<CacheLink>>::None, payload: u8(218) }, stable_t) };
                    let prior_link = loan_read(life_h) { |stable_h| let head_w = ref_from_ptr(write, ptr_h, stable_h); replace(head_w@next, Option<ptr<CacheLink>>::Some(ptr_t)) };
                    let returned = loan_read(life_h) { |stable_h| let head_w2 = ref_from_ptr(write, ptr_h, stable_h); handoff_tail(head_w2@next, ptr_t, allocation_t, life_t) };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = returned;
                    loan_read(owned_domain) { |stable_t| let proof = ref_from_ptr(read, owned_ptr, stable_t); unit };
                    finish_tail(owned_ptr, owned_allocation, owned_domain);
                    let empty_h = loan_exclusive_read(life_h) { |end_h| destroy(ptr_h, end_h) };
                    let full_h = erase_slot<CacheLink>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
