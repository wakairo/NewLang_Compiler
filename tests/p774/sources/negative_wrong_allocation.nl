struct TransferLink { successor:Option<ptr<TransferLink>>, stamp:u8 }
fn replace_setting(target:ref<write,u8>, desired:u8)->u8 {
    let previous=replace(target,desired);
    return previous;
}
struct PumpLimits { start:u8, ceiling:u8 }
fn detach_reserve(edge:ref<write,Option<ptr<TransferLink>>>,
                  address:ptr<TransferLink>,allocation:Allocation,
                  domain:LifetimeDomain)->LiveTail {
    let removed=replace(edge,Option<ptr<TransferLink>>::None);
    let reserve=LiveTail{owned_allocation:allocation,owned_ptr:address,
                         owned_domain:domain};
    return reserve;
}
fn retire_reserve(address:ptr<TransferLink>,allocation:Allocation,
                  domain:LifetimeDomain)->unit {
    let vacant=loan_exclusive_read(domain){|ending|destroy(address,ending)};
    let bytes=erase_slot<TransferLink>(vacant);
    finalize_domain(domain);
    deallocate(allocation,bytes);
    unit
}
fn main()->unit {
    let operating=PumpLimits{start:u8(23),ceiling:u8(144)};
    let journal=PumpLimits{start:u8(5),ceiling:u8(201)};
    let saved=loan_write(operating@start){|target|replace_setting(target,u8(87))};
    let displaced=loan_write(journal@ceiling){|target|replace(target,saved)};
    let applied=operating@start;
    let untouched=operating@ceiling;
    let recorded=journal@ceiling;
    displaced;applied;untouched;recorded;
    match try_allocate_one<TransferLink>() {
        None=>{unit},
        Some(front_bundle)=>{
            let OneBacking{allocation,raw}=front_bundle;
            let front_allocation=allocation;
            let front_slot=into_slot<TransferLink>(raw);
            let front_domain=lifetime_domain();
            let front_ptr=loan_read(front_domain){|stable|
                initialize(front_slot,TransferLink{successor:Option<ptr<TransferLink>>::None,
                    stamp:u8(41)},stable)
            };
            match try_allocate_one<TransferLink>() {
                None=>{
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<TransferLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
                Some(reserve_bundle)=>{
                    let OneBacking{allocation,raw}=reserve_bundle;
                    let reserve_allocation=allocation;
                    let reserve_slot=into_slot<TransferLink>(raw);
                    let reserve_domain=lifetime_domain();
                    let reserve_ptr=loan_read(reserve_domain){|stable|
                        initialize(reserve_slot,TransferLink{successor:Option<ptr<TransferLink>>::None,
                            stamp:u8(163)},stable)
                    };
                    let empty=loan_read(front_domain){|stable|
                        let front_write=ref_from_ptr(write,front_ptr,stable);
                        replace(front_write@successor,Option<ptr<TransferLink>>::Some(reserve_ptr))
                    };
                    let LiveTail{owned_ptr,owned_domain,owned_allocation}=
                        loan_read(front_domain){|stable|
                            let front_write=ref_from_ptr(write,front_ptr,stable);
                            detach_reserve(front_write@successor,reserve_ptr,
                                           reserve_allocation,reserve_domain)
                        };
                    loan_read(owned_domain){|stable|
                        let retained=ref_from_ptr(read,owned_ptr,stable);
                        unit
                    };
                    retire_reserve(owned_ptr,front_allocation,owned_domain);
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<TransferLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
            }
        },
    }
}
