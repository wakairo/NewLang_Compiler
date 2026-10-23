struct PressureTrim { idle:u8, limit:u8 }
fn set_pressure(target:ref<write,u8>, desired:u8)->u8 {
    let previous=replace(target,desired);
    return previous;
}
struct BenchLink { next:Option<ptr<BenchLink>>, reading:u8 }
fn release_spare(edge:ref<write,Option<ptr<BenchLink>>>,
                  address:ptr<BenchLink>,allocation:Allocation,
                  domain:LifetimeDomain)->LiveTail {
    let removed=replace(edge,Option<ptr<BenchLink>>::None);
    let reserve=LiveTail{owned_allocation:allocation,owned_ptr:address,
                         owned_domain:domain};
    return reserve;
}
fn dispose_spare(address:ptr<BenchLink>,allocation:Allocation,
                  domain:LifetimeDomain)->unit {
    loan_read(domain){|stable|let viewed=ref_from_ptr(read,address,stable);unit};
    let vacant=loan_exclusive_read(domain){|ending|destroy(address,ending)};
    let bytes=erase_slot<BenchLink>(vacant);
    finalize_domain(domain);
    deallocate(allocation,bytes);
    unit
}
fn main()->unit {
    match try_allocate_one<BenchLink>() {
        None=>{unit},
        Some(front_bundle)=>{
            let OneBacking{allocation,raw}=front_bundle;
            let front_allocation=allocation;
            let front_slot=into_slot<BenchLink>(raw);
            let front_domain=lifetime_domain();
            let front_ptr=loan_read(front_domain){|stable|
                initialize(front_slot,BenchLink{next:Option<ptr<BenchLink>>::None,
                    reading:u8(105)},stable)
            };
            match try_allocate_one<BenchLink>() {
                None=>{
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<BenchLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
                Some(reserve_bundle)=>{
                    let OneBacking{allocation,raw}=reserve_bundle;
                    let reserve_allocation=allocation;
                    let reserve_slot=into_slot<BenchLink>(raw);
                    let reserve_domain=lifetime_domain();
                    let reserve_ptr=loan_read(reserve_domain){|stable|
                        initialize(reserve_slot,BenchLink{next:Option<ptr<BenchLink>>::None,
                            reading:u8(249)},stable)
                    };
                    let empty=loan_read(front_domain){|stable|
                        let front_write=ref_from_ptr(write,front_ptr,stable);
                        replace(front_write@next,Option<ptr<BenchLink>>::Some(reserve_ptr))
                    };
                    let held_spare=
                        loan_read(front_domain){|stable|
                            let front_write=ref_from_ptr(write,front_ptr,stable);
                            release_spare(front_write@next,reserve_ptr,
                                           reserve_allocation,reserve_domain)
                        };
                    let operating=PressureTrim{limit:u8(237),idle:u8(57)};
                    let journal=PressureTrim{limit:u8(203),idle:u8(14)};
                    let saved=loan_write(operating@limit){|target|set_pressure(target,u8(97))};
                    let displaced=loan_write(journal@idle){|target|set_pressure(target,saved)};
                    let applied=operating@limit;
                    let untouched=operating@idle;
                    let recorded=journal@idle;
                    displaced;applied;untouched;recorded;
                    let LiveTail{owned_ptr,owned_domain,owned_allocation}=held_spare;
                    loan_read(owned_domain){|stable|
                        let retained=ref_from_ptr(read,front_ptr,stable);
                        unit
                    };
                    dispose_spare(owned_ptr,owned_allocation,owned_domain);
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<BenchLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
            }
        },
    }
}
