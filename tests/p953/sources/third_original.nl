struct Span { lower: u8, upper: u8 }

fn replace_upper(location: ref<write,u8>, incoming: u8) -> u8 {
    let before = replace(location, incoming);
    return before;
}

struct Channel { onward: Option<ptr<Channel>>, code: u8 }

fn release_channel(child: ptr<Channel>, allocation: Allocation,
                   life: LifetimeDomain) -> unit {
    loan_read(life) { |stable|
        let view = ref_from_ptr(read, child, stable);
        unit
    };
    let empty = loan_exclusive_read(life) { |end_scope|
        destroy(child, end_scope)
    };
    let all_bytes = erase_slot<Channel>(empty);
    finalize_domain(life);
    deallocate(allocation, all_bytes);
    unit
}

fn package_channel(edge: ref<write,Option<ptr<Channel>>>,
                   child: ptr<Channel>, allocation: Allocation,
                   life: LifetimeDomain) -> LiveTail {
    let released = replace(edge, Option<ptr<Channel>>::None);
    let carried_owner = LiveTail {
        owned_ptr: child,
        owned_allocation: allocation,
        owned_domain: life
    };
    return carried_owner;
}

fn main() -> unit {
    match try_allocate_one<Channel>() {
        None => { unit },
        Some(origin_box) => {
            let OneBacking { allocation, raw } = origin_box;
            let origin_allocation = allocation;
            let origin_slot = into_slot<Channel>(raw);
            let origin_life = lifetime_domain();
            let origin_ptr = loan_read(origin_life) { |stable|
                initialize(origin_slot, Channel {
                    onward: Option<ptr<Channel>>::None, code: u8(59)
                }, stable)
            };
            match try_allocate_one<Channel>() {
                None => {
                    let voided = loan_exclusive_read(origin_life) { |ending|
                        destroy(origin_ptr, ending)
                    };
                    let bytes = erase_slot<Channel>(voided);
                    finalize_domain(origin_life);
                    deallocate(origin_allocation, bytes);
                    unit
                },
                Some(other_box) => {
                    let OneBacking { allocation, raw } = other_box;
                    let other_allocation = allocation;
                    let other_slot = into_slot<Channel>(raw);
                    let other_life = lifetime_domain();
                    let other_ptr = loan_read(other_life) { |stable|
                        initialize(other_slot, Channel {
                            onward: Option<ptr<Channel>>::None, code: u8(233)
                        }, stable)
                    };
                    let active = Span { upper: u8(189), lower: u8(61) };
                    let reserve = Span { lower: u8(224), upper: u8(11) };
                    let extra = Span { lower: u8(1), upper: u8(2) };
                    let choice = if (usize(4) == usize(19)) { u8(92) } else { u8(137) };
                    let prior = loan_write(active@upper) { |w|
                        replace_upper(w, choice)
                    };
                    let displaced = loan_write(reserve@lower) { |w|
                        replace(w, prior)
                    };
                    let was_none = loan_read(origin_life) { |stable|
                        let write_head = ref_from_ptr(write, origin_ptr, stable);
                        replace(write_head@onward,
                                Option<ptr<Channel>>::Some(other_ptr))
                    };
                    let received = loan_read(origin_life) { |stable|
                        let write_head = ref_from_ptr(write, origin_ptr, stable);
                        package_channel(write_head@onward, other_ptr,
                                        other_allocation, other_life)
                    };
                    let LiveTail { owned_ptr, owned_allocation,
                                   owned_domain } = received;
                    release_channel(owned_ptr, owned_allocation, owned_domain);
                    let vanished = loan_exclusive_read(origin_life) { |ending|
                        destroy(origin_ptr, ending)
                    };
                    let bytes = erase_slot<Channel>(vanished);
                    finalize_domain(origin_life);
                    deallocate(origin_allocation, bytes);
                    unit
                },
            }
        },
    }
}
