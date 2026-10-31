struct ScoreCell { lower: u8, upper: u8 }
fn put_score(destination: ref<write,u8>, incoming: u8) -> u8 {
    let previous_score = replace(destination, incoming);
    let saved_score = previous_score;
    saved_score
}
fn carry_score(destination: ref<write,u8>, incoming: u8) -> u8 {
    return replace(destination, incoming);
}
struct SignalHop { next: Option<ptr<SignalHop>>, stamp: u8 }
fn detach_signal(edge: ref<write,Option<ptr<SignalHop>>>,
                target: ptr<SignalHop>, token: Allocation,
                lifetime: LifetimeDomain) -> LiveTail {
    let previous_link = replace(edge, Option<ptr<SignalHop>>::None);
    let detached = LiveTail{owned_ptr: target, owned_allocation: token,
                            owned_domain: lifetime};
    return detached;
}
fn retire_signal(target: ptr<SignalHop>, token: Allocation,
               lifetime: LifetimeDomain) -> unit {
    loan_read(lifetime) { |held|
        let readable = ref_from_ptr(read, target, held);
        unit
    };
    let vacant = loan_exclusive_read(lifetime) { |ending|
        destroy(target, ending)
    };
    let bytes = erase_slot<SignalHop>(vacant);
    finalize_domain(lifetime);
    deallocate(token, bytes);
    return unit;
}
fn main() -> unit {
    match try_allocate_one<SignalHop>() {
        None => { unit },
        Some(upstream_bundle) => {
            let OneBacking { allocation, raw } = upstream_bundle;
            let upstream_allocation = allocation;
            let upstream_slot = into_slot<SignalHop>(raw);
            let upstream_domain = lifetime_domain();
            let upstream = loan_read(upstream_domain) { |held|
                initialize(upstream_slot,
                    SignalHop{next: Option<ptr<SignalHop>>::None,
                               stamp: u8(13)}, held)
            };
            match try_allocate_one<SignalHop>() {
                None => {
                    let upstream_vacant = loan_exclusive_read(upstream_domain) { |ending|
                        destroy(upstream, ending)
                    };
                    let upstream_bytes = erase_slot<SignalHop>(upstream_vacant);
                    finalize_domain(upstream_domain);
                    deallocate(upstream_allocation, upstream_bytes);
                    unit
                },
                Some(downstream_bundle) => {
                    let OneBacking { allocation, raw } = downstream_bundle;
                    let downstream_allocation = allocation;
                    let downstream_slot = into_slot<SignalHop>(raw);
                    let downstream_domain = lifetime_domain();
                    let downstream = loan_read(downstream_domain) { |held|
                        initialize(downstream_slot,
                            SignalHop{next: Option<ptr<SignalHop>>::None,
                                       stamp: u8(187)}, held)
                    };
                    let previous_empty = loan_read(upstream_domain) { |held|
                        let writable = ref_from_ptr(write, upstream, held);
                        replace(writable@next,
                                Option<ptr<SignalHop>>::Some(downstream))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(upstream_domain) { |held|
                            let writable = ref_from_ptr(write, upstream, held);
                            detach_signal(writable@next, downstream,
                                         downstream_allocation, downstream_domain)
                        };
                    loan_read(owned_domain) { |held|
                        let readable = ref_from_ptr(read, owned_ptr, held);
                        unit
                    };
                    retire_signal(owned_ptr, owned_allocation, owned_domain);
                    let upstream_vacant = loan_exclusive_read(upstream_domain) { |ending|
                        destroy(upstream, ending)
                    };
                    let upstream_bytes = erase_slot<SignalHop>(upstream_vacant);
                    finalize_domain(upstream_domain);
                    deallocate(upstream_allocation, upstream_bytes);
                    unit
                },
            }
        },
    };
    let active_score = ScoreCell{lower: u8(58), upper: u8(141)};
    let spare_score = ScoreCell{upper: u8(94), lower: u8(220)};
    let before_left = loan_write(active_score@lower) { |target|
        put_score(target, u8(167))
    };
    let before_right = loan_write(spare_score@upper) { |target|
        carry_score(target, before_left)
    };
    let before_direct = loan_write(spare_score@lower) { |target|
        replace(target, before_right)
    };
    let observed_left = before_left;
    let observed_right = before_right;
    let observed_direct = before_direct;
    let after_left_west = active_score@lower;
    let after_left_east = active_score@upper;
    let after_right_west = spare_score@lower;
    let after_right_east = spare_score@upper;
    unit
}
