// Single-unit batch handoff: heap buffer custody and independent device calibration.
struct BatchCell { next: Option<ptr<BatchCell>>, sample: u8 }
struct Calibration { offset: u8, limit: u8 }
fn tune(slot: ref<write,u8>, requested:u8)->u8 {
    let previous=replace(slot,requested);
    return previous;
}
fn release_batch(edge:ref<write,Option<ptr<BatchCell>>>,
                 address:ptr<BatchCell>, allocation:Allocation,
                 domain:LifetimeDomain)->LiveTail {
    let removed=replace(edge,Option<ptr<BatchCell>>::None);
    let owner=LiveTail{owned_ptr:address,owned_domain:domain,
                       owned_allocation:allocation};
    return owner;
}
fn finish_batch(address:ptr<BatchCell>,allocation:Allocation,
                domain:LifetimeDomain)->unit {
    let slot=loan_exclusive_read(domain){|end|destroy(address,end)};
    let storage=erase_slot<BatchCell>(slot);
    finalize_domain(domain);
    deallocate(allocation,storage);
    unit
}
fn main()->unit {
    let sensor=Calibration{offset:u8(26),limit:u8(118)};
    let log=Calibration{offset:u8(4),limit:u8(202)};
    let saved=loan_write(sensor@offset){|w|tune(w,u8(63))};
    let displaced=loan_write(log@limit){|w|replace(w,saved)};
    let current=sensor@offset;
    let sibling=sensor@limit;
    let archived=log@limit;
    displaced;current;sibling;archived;
    match try_allocate_one<BatchCell>() {
        None=>{unit},
        Some(first)=>{
            let OneBacking{allocation,raw}=first;
            let first_allocation=allocation;
            let first_slot=into_slot<BatchCell>(raw);
            let first_domain=lifetime_domain();
            let first_ptr=loan_read(first_domain){|stable|
                initialize(first_slot,BatchCell{next:Option<ptr<BatchCell>>::None,
                    sample:u8(37)},stable)
            };
            match try_allocate_one<BatchCell>() {
                None=>{
                    let slot=loan_exclusive_read(first_domain){|end|destroy(first_ptr,end)};
                    let storage=erase_slot<BatchCell>(slot);
                    finalize_domain(first_domain);
                    deallocate(first_allocation,storage);
                    unit
                },
                Some(second)=>{
                    let OneBacking{allocation,raw}=second;
                    let second_allocation=allocation;
                    let second_slot=into_slot<BatchCell>(raw);
                    let second_domain=lifetime_domain();
                    let second_ptr=loan_read(second_domain){|stable|
                        initialize(second_slot,BatchCell{next:Option<ptr<BatchCell>>::None,
                            sample:u8(151)},stable)
                    };
                    let empty_edge=loan_read(first_domain){|stable|
                        let original=ref_from_ptr(write,first_ptr,stable);
                        replace(original@next,Option<ptr<BatchCell>>::Some(second_ptr))
                    };
                    let LiveTail{owned_ptr,owned_allocation,owned_domain}=
                        loan_read(first_domain){|stable|
                            let original=ref_from_ptr(write,first_ptr,stable);
                            release_batch(original@next,second_ptr,second_allocation,first_domain)
                        };
                    loan_read(owned_domain){|stable|
                        let returned=ref_from_ptr(read,owned_ptr,stable);
                        unit
                    };
                    finish_batch(owned_ptr,owned_allocation,owned_domain);
                    let slot=loan_exclusive_read(first_domain){|end|destroy(first_ptr,end)};
                    let storage=erase_slot<BatchCell>(slot);
                    finalize_domain(first_domain);
                    deallocate(first_allocation,storage);
                    unit
                },
            }
        },
    }
}
