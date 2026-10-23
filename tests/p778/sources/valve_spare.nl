struct ValveTrim { floor:u8, cap:u8 }
fn adjust_valve(target:ref<write,u8>, desired:u8)->u8 {
    let previous=replace(target,desired);
    return previous;
}
struct ValveLink { chain:Option<ptr<ValveLink>>, code:u8 }
fn release_spare(edge:ref<write,Option<ptr<ValveLink>>>,
                  address:ptr<ValveLink>,allocation:Allocation,
                  domain:LifetimeDomain)->LiveTail {
    let removed=replace(edge,Option<ptr<ValveLink>>::None);
    let reserve=LiveTail{owned_allocation:allocation,owned_ptr:address,
                         owned_domain:domain};
    return reserve;
}
fn dispose_spare(address:ptr<ValveLink>,allocation:Allocation,
                  domain:LifetimeDomain)->unit {
    loan_read(domain){|stable|let viewed=ref_from_ptr(read,address,stable);unit};
    let vacant=loan_exclusive_read(domain){|ending|destroy(address,ending)};
    let bytes=erase_slot<ValveLink>(vacant);
    finalize_domain(domain);
    deallocate(allocation,bytes);
    unit
}
fn main()->unit {
    let operating=ValveTrim{cap:u8(226),floor:u8(43)};
    let journal=ValveTrim{cap:u8(198),floor:u8(12)};
    let saved=loan_write(operating@cap){|target|adjust_valve(target,u8(83))};
    let displaced=loan_write(journal@floor){|target|adjust_valve(target,saved)};
    let applied=operating@cap;
    let untouched=operating@floor;
    let recorded=journal@floor;
    displaced;applied;untouched;recorded;
    match try_allocate_one<ValveLink>() {
        None=>{unit},
        Some(front_bundle)=>{
            let OneBacking{allocation,raw}=front_bundle;
            let front_allocation=allocation;
            let front_slot=into_slot<ValveLink>(raw);
            let front_domain=lifetime_domain();
            let front_ptr=loan_read(front_domain){|stable|
                initialize(front_slot,ValveLink{chain:Option<ptr<ValveLink>>::None,
                    code:u8(89)},stable)
            };
            match try_allocate_one<ValveLink>() {
                None=>{
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<ValveLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
                Some(reserve_bundle)=>{
                    let OneBacking{allocation,raw}=reserve_bundle;
                    let reserve_allocation=allocation;
                    let reserve_slot=into_slot<ValveLink>(raw);
                    let reserve_domain=lifetime_domain();
                    let reserve_ptr=loan_read(reserve_domain){|stable|
                        initialize(reserve_slot,ValveLink{chain:Option<ptr<ValveLink>>::None,
                            code:u8(241)},stable)
                    };
                    let empty=loan_read(front_domain){|stable|
                        let front_write=ref_from_ptr(write,front_ptr,stable);
                        replace(front_write@chain,Option<ptr<ValveLink>>::Some(reserve_ptr))
                    };
                    let held_spare=
                        loan_read(front_domain){|stable|
                            let front_write=ref_from_ptr(write,front_ptr,stable);
                            release_spare(front_write@chain,reserve_ptr,
                                           reserve_allocation,reserve_domain)
                        };
                    let LiveTail{owned_ptr,owned_domain,owned_allocation}=held_spare;
                    loan_read(owned_domain){|stable|
                        let retained=ref_from_ptr(read,owned_ptr,stable);
                        unit
                    };
                    dispose_spare(owned_ptr,owned_allocation,owned_domain);
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<ValveLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
            }
        },
    }
}
