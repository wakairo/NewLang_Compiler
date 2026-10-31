struct HandoffLink { next: Option<ptr<HandoffLink>>, serial: u8 }
fn take_link(edge: ref<write,Option<ptr<HandoffLink>>>,
                target: ptr<HandoffLink>, token: Allocation,
                lifetime: LifetimeDomain) -> LiveTail {
    let previous_link = replace(edge, Option<ptr<HandoffLink>>::None);
    let detached = LiveTail{owned_ptr: target, owned_allocation: token,
                            owned_domain: lifetime};
    return detached;
}
fn close_link(target: ptr<HandoffLink>, token: Allocation,
               lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |held|
        let readable = ref_from_ptr(read, target, held);
        unit
    };
    let vacant = loan_exclusive_read(lifetime) { |ending|
        destroy(target, ending)
    };
    let bytes = erase_slot<HandoffLink>(vacant);
    finalize_domain(lifetime);
    deallocate(token, bytes);
    unit
}
struct SensorDuet { east: u8, west: u8 }
fn relay_west(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    displaced
}
fn relay_east(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    let copied = displaced;
    return copied;
}
fn main() -> unit {
    match try_allocate_one<HandoffLink>() {
Some(upstream_bundle) => {
            let OneBacking { allocation, raw } = upstream_bundle;
            let upstream_allocation = allocation;
            let upstream_slot = into_slot<HandoffLink>(raw);
            let upstream_domain = lifetime_domain();
            let upstream = loan_read(upstream_domain) { |held|
                initialize(upstream_slot,
                    HandoffLink{next: Option<ptr<HandoffLink>>::None,
                               serial: u8(101)}, held)
            };
            match try_allocate_one<HandoffLink>() {
Some(downstream_bundle) => {
                    let OneBacking { allocation, raw } = downstream_bundle;
                    let downstream_allocation = allocation;
                    let downstream_slot = into_slot<HandoffLink>(raw);
                    let downstream_domain = lifetime_domain();
                    let downstream = loan_read(downstream_domain) { |held|
                        initialize(downstream_slot,
                            HandoffLink{next: Option<ptr<HandoffLink>>::None,
                                       serial: u8(47)}, held)
                    };
                    let previous_empty = loan_read(upstream_domain) { |held|
                        let writable = ref_from_ptr(write, upstream, held);
                        replace(writable@next,
                                Option<ptr<HandoffLink>>::Some(downstream))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(upstream_domain) { |held|
                            let writable = ref_from_ptr(write, upstream, held);
                            take_link(writable@next, downstream,
                                         downstream_allocation, downstream_domain)
                        };
                    loan_read(owned_domain) { |held|
                        let readable = ref_from_ptr(read, owned_ptr, held);
                        unit
                    };
                    close_link(owned_ptr, owned_allocation, owned_domain);
                    let upstream_vacant = loan_exclusive_read(upstream_domain) { |ending|
                        destroy(upstream, ending)
                    };
                    let upstream_bytes = erase_slot<HandoffLink>(upstream_vacant);
                    finalize_domain(upstream_domain);
                    deallocate(upstream_allocation, upstream_bytes);
                    unit
                },
None => {
                    let upstream_vacant = loan_exclusive_read(upstream_domain) { |ending|
                        destroy(upstream, ending)
                    };
                    let upstream_bytes = erase_slot<HandoffLink>(upstream_vacant);
                    finalize_domain(upstream_domain);
                    deallocate(upstream_allocation, upstream_bytes);
                    unit
                },
}
        },
None => { unit },
};
    let stored_south = SensorDuet{west: u8(16), east: u8(250)};
    let stored_north = SensorDuet{east: u8(9), west: u8(193)};
    let before_left = loan_write(stored_south@west) { |target|
        relay_west(target, u8(77))
    };
    let before_right = loan_write(stored_north@east) { |target|
        relay_east(target, before_left)
    };
    let before_direct = loan_write(stored_north@west) { |target|
        replace(target, before_right)
    };
    let observed_left = before_left;
    let observed_right = before_right;
    let observed_direct = before_direct;
    let after_left_west = stored_south@west;
    let after_left_east = stored_south@east;
    let after_right_west = stored_north@west;
    let after_right_east = stored_north@east;
    return unit;
}
