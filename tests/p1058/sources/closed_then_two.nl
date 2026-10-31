struct ReadingPair { west: u8, east: u8 }
fn exchange_west(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    displaced
}
fn exchange_east(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    let copied = displaced;
    return copied;
}
struct RouteKnot { next: Option<ptr<RouteKnot>>, serial: u8 }
fn unlink_route(edge: ref<write,Option<ptr<RouteKnot>>>,
                target: ptr<RouteKnot>, token: Allocation,
                lifetime: LifetimeDomain) -> LiveTail {
    let previous_link = replace(edge, Option<ptr<RouteKnot>>::None);
    let detached = LiveTail{owned_ptr: target, owned_allocation: token,
                            owned_domain: lifetime};
    return detached;
}
fn finish_route(target: ptr<RouteKnot>, token: Allocation,
               lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |held|
        let readable = ref_from_ptr(read, target, held);
        unit
    };
    let vacant = loan_exclusive_read(lifetime) { |ending|
        destroy(target, ending)
    };
    let bytes = erase_slot<RouteKnot>(vacant);
    finalize_domain(lifetime);
    deallocate(token, bytes);
    unit
}
fn main() -> unit {
    match try_allocate_one<RouteKnot>() {
        None => { unit },
        Some(upstream_bundle) => {
            let OneBacking { allocation, raw } = upstream_bundle;
            let upstream_allocation = allocation;
            let upstream_slot = into_slot<RouteKnot>(raw);
            let upstream_domain = lifetime_domain();
            let upstream = loan_read(upstream_domain) { |held|
                initialize(upstream_slot,
                    RouteKnot{next: Option<ptr<RouteKnot>>::None,
                               serial: u8(29)}, held)
            };
            match try_allocate_one<RouteKnot>() {
                None => {
                    let upstream_vacant = loan_exclusive_read(upstream_domain) { |ending|
                        destroy(upstream, ending)
                    };
                    let upstream_bytes = erase_slot<RouteKnot>(upstream_vacant);
                    finalize_domain(upstream_domain);
                    deallocate(upstream_allocation, upstream_bytes);
                    unit
                },
                Some(downstream_bundle) => {
                    let OneBacking { allocation, raw } = downstream_bundle;
                    let downstream_allocation = allocation;
                    let downstream_slot = into_slot<RouteKnot>(raw);
                    let downstream_domain = lifetime_domain();
                    let downstream = loan_read(downstream_domain) { |held|
                        initialize(downstream_slot,
                            RouteKnot{next: Option<ptr<RouteKnot>>::None,
                                       serial: u8(173)}, held)
                    };
                    let previous_empty = loan_read(upstream_domain) { |held|
                        let writable = ref_from_ptr(write, upstream, held);
                        replace(writable@next,
                                Option<ptr<RouteKnot>>::Some(downstream))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(upstream_domain) { |held|
                            let writable = ref_from_ptr(write, upstream, held);
                            unlink_route(writable@next, downstream,
                                         downstream_allocation, downstream_domain)
                        };
                    loan_read(owned_domain) { |held|
                        let readable = ref_from_ptr(read, owned_ptr, held);
                        unit
                    };
                    finish_route(owned_ptr, owned_allocation, owned_domain);
                    let upstream_vacant = loan_exclusive_read(upstream_domain) { |ending|
                        destroy(upstream, ending)
                    };
                    let upstream_bytes = erase_slot<RouteKnot>(upstream_vacant);
                    finalize_domain(upstream_domain);
                    deallocate(upstream_allocation, upstream_bytes);
                    unit
                },
            }
        },
    };
    let left_reading = ReadingPair{west: u8(37), east: u8(209)};
    let right_reading = ReadingPair{east: u8(112), west: u8(68)};
    let before_left = loan_write(left_reading@west) { |target|
        exchange_west(target, u8(154))
    };
    let before_right = loan_write(right_reading@east) { |target|
        exchange_east(target, before_left)
    };
    let before_direct = loan_write(right_reading@west) { |target|
        replace(target, before_right)
    };
    let observed_left = before_left;
    let observed_right = before_right;
    let observed_direct = before_direct;
    let after_left_west = left_reading@west;
    let after_left_east = left_reading@east;
    let after_right_west = right_reading@west;
    let after_right_east = right_reading@east;
    unit
}
