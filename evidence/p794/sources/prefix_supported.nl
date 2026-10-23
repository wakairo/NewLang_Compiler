struct ControlStamp { code:u8, mark:u8 }
fn exchange_code(target:ref<write,u8>, desired:u8)->u8 {
    let previous=replace(target,desired);
    return previous;
}
struct MessageLink { next:Option<ptr<MessageLink>>, reading:u8 }
fn deliver_payload(edge:ref<write,Option<ptr<MessageLink>>>,
                  address:ptr<MessageLink>,allocation:Allocation,
                  domain:LifetimeDomain)->LiveTail {
    let removed=replace(edge,Option<ptr<MessageLink>>::None);
    let reserve=LiveTail{owned_allocation:allocation,owned_ptr:address,
                         owned_domain:domain};
    return reserve;
}
fn discard_payload(address:ptr<MessageLink>,allocation:Allocation,
                  domain:LifetimeDomain)->unit {
    loan_read(domain){|stable|let viewed=ref_from_ptr(read,address,stable);unit};
    let vacant=loan_exclusive_read(domain){|ending|destroy(address,ending)};
    let bytes=erase_slot<MessageLink>(vacant);
    finalize_domain(domain);
    deallocate(allocation,bytes);
    unit
}
fn main()->unit {
    let operating=ControlStamp{code:u8(42),mark:u8(17)};
    let journal=ControlStamp{code:u8(213),mark:u8(66)};
    let saved=loan_write(operating@code){|target|exchange_code(target,u8(98))};
    let displaced=loan_write(journal@mark){|target|exchange_code(target,saved)};
    let applied=operating@code;
    let untouched=operating@mark;
    let recorded=journal@mark;
    displaced;applied;untouched;recorded;
    match try_allocate_one<MessageLink>() {
        None=>{unit},
        Some(front_bundle)=>{
            let OneBacking{allocation,raw}=front_bundle;
            let front_allocation=allocation;
            let front_slot=into_slot<MessageLink>(raw);
            let front_domain=lifetime_domain();
            let front_ptr=loan_read(front_domain){|stable|
                initialize(front_slot,MessageLink{next:Option<ptr<MessageLink>>::None,
                    reading:u8(81)},stable)
            };
            match try_allocate_one<MessageLink>() {
                None=>{
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<MessageLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
                Some(reserve_bundle)=>{
                    let OneBacking{allocation,raw}=reserve_bundle;
                    let reserve_allocation=allocation;
                    let reserve_slot=into_slot<MessageLink>(raw);
                    let reserve_domain=lifetime_domain();
                    let reserve_ptr=loan_read(reserve_domain){|stable|
                        initialize(reserve_slot,MessageLink{next:Option<ptr<MessageLink>>::None,
                            reading:u8(143)},stable)
                    };
                    let empty=loan_read(front_domain){|stable|
                        let front_write=ref_from_ptr(write,front_ptr,stable);
                        replace(front_write@next,Option<ptr<MessageLink>>::Some(reserve_ptr))
                    };
                    let held_spare=
                        loan_read(front_domain){|stable|
                            let front_write=ref_from_ptr(write,front_ptr,stable);
                            deliver_payload(front_write@next,reserve_ptr,
                                           reserve_allocation,reserve_domain)
                        };
                    let LiveTail{owned_ptr,owned_domain,owned_allocation}=held_spare;
                    loan_read(owned_domain){|stable|
                        let retained=ref_from_ptr(read,owned_ptr,stable);
                        unit
                    };
                    discard_payload(owned_ptr,owned_allocation,owned_domain);
                    let vacant=loan_exclusive_read(front_domain){|ending|destroy(front_ptr,ending)};
                    let bytes=erase_slot<MessageLink>(vacant);
                    finalize_domain(front_domain);
                    deallocate(front_allocation,bytes);
                    unit
                },
            }
        },
    }
}
