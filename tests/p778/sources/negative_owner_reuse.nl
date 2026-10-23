struct SpareLink { following:Option<ptr<SpareLink>>, sample:u8 }
fn adjust_trim(target:ref<write,u8>, desired:u8)->u8 {
    let previous=replace(target,desired);
    return previous;
}
struct SensorTrim { offset:u8, maximum:u8 }
fn release_spare(edge:ref<write,Option<ptr<SpareLink>>>,
                  address:ptr<SpareLink>,allocation:Allocation,
                  domain:LifetimeDomain)->LiveTail {
    let removed=replace(edge,Option<ptr<SpareLink>>::None);
    let reserve=LiveTail{owned_allocation:allocation,owned_ptr:address,
                         owned_domain:domain};
    return reserve;
}
fn dispose_spare(address:ptr<SpareLink>,allocation:Allocation,
                  domain:LifetimeDomain)->unit {
    loan_read(domain){|stable|let viewed=ref_from_ptr(read,address,stable);unit};
    let vacant=loan_exclusive_read(domain){|ending|destroy(address,ending)};
    let bytes=erase_slot<SpareLink>(vacant);
    finalize_domain(domain);
    deallocate(allocation,bytes);
    unit
}
fn main()->unit {
    let operating=SensorTrim{offset:u8(37),maximum:u8(156)};
    let journal=SensorTrim{offset:u8(9),maximum:u8(211)};
    let saved=loan_write(operating@offset){|target|adjust_trim(target,u8(101))};
    let displaced=loan_write(journal@maximum){|target|replace(target,saved)};
    let applied=operating@offset;
    let untouched=operating@maximum;
    let recorded=journal@maximum;
    displaced;applied;untouched;recorded;
    match try_allocate_one<SpareLink>() {
        None=>{unit},
        Some(front_bundle)=>{
            let OneBacking{allocation,raw}=front_bundle;
            let front_allocation=allocation;
            let front_slot=into_slot<SpareLink>(raw);
            let front_domain=lifetime_domain();
            let front_ptr=loan_read(front_domain){|stable|
                initialize(front_slot,SpareLink{following:Option<ptr<SpareLink>>::None,
                    sample:u8(53)},stable)
            };
            match try_allocate_one<SpareLink>() {
                None=>{
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<SpareLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
                Some(reserve_bundle)=>{
                    let OneBacking{allocation,raw}=reserve_bundle;
                    let reserve_allocation=allocation;
                    let reserve_slot=into_slot<SpareLink>(raw);
                    let reserve_domain=lifetime_domain();
                    let reserve_ptr=loan_read(reserve_domain){|stable|
                        initialize(reserve_slot,SpareLink{following:Option<ptr<SpareLink>>::None,
                            sample:u8(181)},stable)
                    };
                    let empty=loan_read(front_domain){|stable|
                        let front_write=ref_from_ptr(write,front_ptr,stable);
                        replace(front_write@following,Option<ptr<SpareLink>>::Some(reserve_ptr))
                    };
                    let held_spare=
                        loan_read(front_domain){|stable|
                            let front_write=ref_from_ptr(write,front_ptr,stable);
                            release_spare(front_write@following,reserve_ptr,
                                           reserve_allocation,reserve_domain)
                        };
                    let LiveTail{owned_ptr,owned_domain,owned_allocation}=held_spare;
                    let repeated=held_spare;
                    loan_read(owned_domain){|stable|
                        let retained=ref_from_ptr(read,owned_ptr,stable);
                        unit
                    };
                    dispose_spare(owned_ptr,owned_allocation,owned_domain);
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<SpareLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
            }
        },
    }
}
