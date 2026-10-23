// Fresh independent R #787 source B: one named call on second declared field, after H ownership transfer.
struct ArcTicket { left: u8, right: u8, }
fn update_byte(w: ref<write,u8>, value: u8) -> u8 {
    let prior = replace(w, value);
    return prior;
}
struct LedgerArc { next: Option<ptr<LedgerArc>>, payload: u8, }
fn unhook_tail(edge: ref<write,Option<ptr<LedgerArc>>>, original: ptr<LedgerArc>, backing: Allocation, lifetime: LifetimeDomain) -> LiveTail {
    let displaced = replace(edge, Option<ptr<LedgerArc>>::None);
    let custody = LiveTail { owned_ptr: original, owned_allocation: backing, owned_domain: lifetime };
    return custody;
}
fn release_tail(original: ptr<LedgerArc>, backing: Allocation, lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |stable| let r = ref_from_ptr(read, original, stable); unit };
    let empty = loan_exclusive_read(lifetime) { |exclusive| destroy(original, exclusive) };
    let full = erase_slot<LedgerArc>(empty);
    finalize_domain(lifetime);
    deallocate(backing, full);
    unit
}

fn main() -> unit {
    match try_allocate_one<LedgerArc>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<LedgerArc>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h| initialize(vacant_h, LedgerArc { next: Option<ptr<LedgerArc>>::None, payload: u8(29) }, stable_h) };
            match try_allocate_one<LedgerArc>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |end_h| destroy(ptr_h, end_h) };
                    let full_h = erase_slot<LedgerArc>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<LedgerArc>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t| initialize(vacant_t, LedgerArc { next: Option<ptr<LedgerArc>>::None, payload: u8(231) }, stable_t) };
                    let prior_link = loan_read(life_h) { |stable_h| let head_w = ref_from_ptr(write, ptr_h, stable_h); replace(head_w@next, Option<ptr<LedgerArc>>::Some(ptr_t)) };
                    let returned = loan_read(life_h) { |stable_h| let head_w2 = ref_from_ptr(write, ptr_h, stable_h); unhook_tail(head_w2@next, ptr_t, allocation_t, life_t) };
                    let ticket = ArcTicket { right: u8(177), left: u8(43) };
                    let before = loan_read(ticket@right) { |edit| update_byte(edit, u8(91)) };
                    let after = ticket@right;
                    let untouched = ticket@left;
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = returned;
                    loan_read(owned_domain) { |stable_t| let proof = ref_from_ptr(read, owned_ptr, stable_t); unit };
                    release_tail(owned_ptr, owned_allocation, owned_domain);
                    let empty_h = loan_exclusive_read(life_h) { |end_h| destroy(ptr_h, end_h) };
                    let full_h = erase_slot<LedgerArc>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
