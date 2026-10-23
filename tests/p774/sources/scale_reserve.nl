struct Scale { alarm:u8, baseline:u8 }
fn replace_setting(target:ref<write,u8>, desired:u8)->u8 {
    let previous=replace(target,desired);
    return previous;
}
struct SpareCell { following:Option<ptr<SpareCell>>, marker:u8 }
fn detach_reserve(edge:ref<write,Option<ptr<SpareCell>>>,
                  address:ptr<SpareCell>,allocation:Allocation,
                  domain:LifetimeDomain)->LiveTail {
    let removed=replace(edge,Option<ptr<SpareCell>>::None);
    let reserve=LiveTail{owned_allocation:allocation,owned_ptr:address,
                         owned_domain:domain};
    return reserve;
}
fn retire_reserve(address:ptr<SpareCell>,allocation:Allocation,
                  domain:LifetimeDomain)->unit {
    let vacant=loan_exclusive_read(domain){|ending|destroy(address,ending)};
    let bytes=erase_slot<SpareCell>(vacant);
    finalize_domain(domain);
    deallocate(allocation,bytes);
    unit
}
fn main()->unit {
    let operating=Scale{baseline:u8(218),alarm:u8(31)};
    let journal=Scale{baseline:u8(190),alarm:u8(8)};
    let saved=loan_write(operating@baseline){|target|replace_setting(target,u8(69))};
    let displaced=loan_write(journal@alarm){|target|replace_setting(target,saved)};
    let applied=operating@baseline;
    let untouched=operating@alarm;
    let recorded=journal@alarm;
    displaced;applied;untouched;recorded;
    match try_allocate_one<SpareCell>() {
        None=>{unit},
        Some(front_bundle)=>{
            let OneBacking{allocation,raw}=front_bundle;
            let front_allocation=allocation;
            let front_slot=into_slot<SpareCell>(raw);
            let front_domain=lifetime_domain();
            let front_ptr=loan_read(front_domain){|stable|
                initialize(front_slot,SpareCell{following:Option<ptr<SpareCell>>::None,
                    marker:u8(76)},stable)
            };
            match try_allocate_one<SpareCell>() {
                None=>{
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<SpareCell>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
                Some(reserve_bundle)=>{
                    let OneBacking{allocation,raw}=reserve_bundle;
                    let reserve_allocation=allocation;
                    let reserve_slot=into_slot<SpareCell>(raw);
                    let reserve_domain=lifetime_domain();
                    let reserve_ptr=loan_read(reserve_domain){|stable|
                        initialize(reserve_slot,SpareCell{following:Option<ptr<SpareCell>>::None,
                            marker:u8(229)},stable)
                    };
                    let empty=loan_read(front_domain){|stable|
                        let front_write=ref_from_ptr(write,front_ptr,stable);
                        replace(front_write@following,Option<ptr<SpareCell>>::Some(reserve_ptr))
                    };
                    let LiveTail{owned_ptr,owned_domain,owned_allocation}=
                        loan_read(front_domain){|stable|
                            let front_write=ref_from_ptr(write,front_ptr,stable);
                            detach_reserve(front_write@following,reserve_ptr,
                                           reserve_allocation,reserve_domain)
                        };
                    loan_read(owned_domain){|stable|
                        let retained=ref_from_ptr(read,owned_ptr,stable);
                        unit
                    };
                    retire_reserve(owned_ptr,owned_allocation,owned_domain);
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<SpareCell>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
            }
        },
    }
}
