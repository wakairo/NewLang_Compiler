// Single-unit batch handoff: heap buffer custody and independent device calibration.
struct QueueItem { queued: Option<ptr<QueueItem>>, tag: u8 }
struct Thresholds { low: u8, high: u8 }
fn tune(slot: ref<write,u8>, requested:u8)->u8 {
    let previous=replace(slot,requested);
    return previous;
}
fn release_batch(edge:ref<write,Option<ptr<QueueItem>>>,
                 address:ptr<QueueItem>, allocation:Allocation,
                 domain:LifetimeDomain)->LiveTail {
    let removed=replace(edge,Option<ptr<QueueItem>>::None);
    let owner=LiveTail{owned_ptr:address,owned_domain:domain,
                       owned_allocation:allocation};
    return owner;
}
fn finish_batch(address:ptr<QueueItem>,allocation:Allocation,
                domain:LifetimeDomain)->unit {
    loan_read(domain){|stable|let observed=ref_from_ptr(read,address,stable);unit};
    let slot=loan_exclusive_read(domain){|end|destroy(address,end)};
    let storage=erase_slot<QueueItem>(slot);
    finalize_domain(domain);
    deallocate(allocation,storage);
    unit
}
fn main()->unit {
    match try_allocate_one<QueueItem>() {
        None=>{unit},
        Some(first)=>{
            let OneBacking{allocation,raw}=first;
            let first_allocation=allocation;
            let first_slot=into_slot<QueueItem>(raw);
            let first_domain=lifetime_domain();
            let first_ptr=loan_read(first_domain){|stable|
                initialize(first_slot,QueueItem{queued:Option<ptr<QueueItem>>::None,
                    tag:u8(59)},stable)
            };
            match try_allocate_one<QueueItem>() {
                None=>{
                    let slot=loan_exclusive_read(first_domain){|end|destroy(first_ptr,end)};
                    let storage=erase_slot<QueueItem>(slot);
                    finalize_domain(first_domain);
                    deallocate(first_allocation,storage);
                    unit
                },
                Some(second)=>{
                    let OneBacking{allocation,raw}=second;
                    let second_allocation=allocation;
                    let second_slot=into_slot<QueueItem>(raw);
                    let second_domain=lifetime_domain();
                    let second_ptr=loan_read(second_domain){|stable|
                        initialize(second_slot,QueueItem{queued:Option<ptr<QueueItem>>::None,
                            tag:u8(211)},stable)
                    };
                    let empty_edge=loan_read(first_domain){|stable|
                        let original=ref_from_ptr(write,first_ptr,stable);
                        replace(original@queued,Option<ptr<QueueItem>>::Some(second_ptr))
                    };
                    let active=Thresholds{high:u8(193),low:u8(12)};
                    let backup=Thresholds{low:u8(88),high:u8(207)};
                    let saved=loan_write(active@high){|w|tune(w,u8(71))};
                    let displaced=loan_write(backup@low){|w|replace(w,saved)};
                    let current=active@high;
                    let sibling=active@low;
                    let archived=backup@low;
                    displaced;current;sibling;archived;
                    let LiveTail{owned_ptr,owned_allocation,owned_domain}=
                        loan_read(first_domain){|stable|
                            let original=ref_from_ptr(write,first_ptr,stable);
                            release_batch(original@queued,second_ptr,second_allocation,second_domain)
                        };
                    loan_read(owned_domain){|stable|
                        let returned=ref_from_ptr(read,owned_ptr,stable);
                        unit
                    };
                    finish_batch(owned_ptr,owned_allocation,owned_domain);
                    let slot=loan_exclusive_read(first_domain){|end|destroy(first_ptr,end)};
                    let storage=erase_slot<QueueItem>(slot);
                    finalize_domain(first_domain);
                    deallocate(first_allocation,storage);
                    unit
                },
            }
        },
    }
}
