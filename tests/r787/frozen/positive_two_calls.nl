// Fresh independent R #787 source A: two separately owned T calls within an H success arm.
struct TwinGauge { lower: u8, upper: u8, }
fn set_octet(w: ref<write,u8>, value: u8) -> u8 {
    let prior = replace(w, value);
    return prior;
}
struct TrailCell { next: Option<ptr<TrailCell>>, payload: u8, }
fn peel_tail(edge: ref<write,Option<ptr<TrailCell>>>, original: ptr<TrailCell>, backing: Allocation, lifetime: LifetimeDomain) -> LiveTail {
    let displaced = replace(edge, Option<ptr<TrailCell>>::None);
    let custody = LiveTail { owned_ptr: original, owned_allocation: backing, owned_domain: lifetime };
    return custody;
}
fn retire_tail(original: ptr<TrailCell>, backing: Allocation, lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |stable| let r = ref_from_ptr(read, original, stable); unit };
    let empty = loan_exclusive_read(lifetime) { |exclusive| destroy(original, exclusive) };
    let full = erase_slot<TrailCell>(empty);
    finalize_domain(lifetime);
    deallocate(backing, full);
    unit
}

fn main() -> unit {
    match try_allocate_one<TrailCell>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<TrailCell>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h| initialize(vacant_h, TrailCell { next: Option<ptr<TrailCell>>::None, payload: u8(53) }, stable_h) };
            match try_allocate_one<TrailCell>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |end_h| destroy(ptr_h, end_h) };
                    let full_h = erase_slot<TrailCell>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<TrailCell>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t| initialize(vacant_t, TrailCell { next: Option<ptr<TrailCell>>::None, payload: u8(197) }, stable_t) };
                    let prior_link = loan_read(life_h) { |stable_h| let head_w = ref_from_ptr(write, ptr_h, stable_h); replace(head_w@next, Option<ptr<TrailCell>>::Some(ptr_t)) };
                    let returned = loan_read(life_h) { |stable_h| let head_w2 = ref_from_ptr(write, ptr_h, stable_h); peel_tail(head_w2@next, ptr_t, allocation_t, life_t) };
                    let gauge0 = TwinGauge { upper: u8(174), lower: u8(21) };
                    let gauge1 = TwinGauge { lower: u8(88), upper: u8(196) };
                    let old_first = loan_write(gauge0@lower) { |edit| set_octet(edit, u8(64)) };
                    let old_second = loan_write(gauge1@upper) { |edit| set_octet(edit, u8(33)) };
                    let current_first = gauge0@lower;
                    let sibling_first = gauge0@upper;
                    let current_second = gauge1@upper;
                    let sibling_second = gauge1@lower;
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = returned;
                    loan_read(owned_domain) { |stable_t| let proof = ref_from_ptr(read, owned_ptr, stable_t); unit };
                    retire_tail(owned_ptr, owned_allocation, owned_domain);
                    let empty_h = loan_exclusive_read(life_h) { |end_h| destroy(ptr_h, end_h) };
                    let full_h = erase_slot<TrailCell>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
