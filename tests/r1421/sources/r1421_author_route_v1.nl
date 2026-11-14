struct LedgerRoot {
    relay: Option<ptr<LedgerRoot>>,
    mark: u8,
}

fn author_route() -> u8 {
    match try_allocate_one<LedgerRoot>() {
        None => {
            u8(0)
        },
        Some(package) => {
            let OneBacking { allocation, raw } = package;
            let claim = into_slot<LedgerRoot>(raw);
            let domain = lifetime_domain();
            let origin = loan_read(domain) { |creation|
                initialize(claim, LedgerRoot {
                    relay: Option<ptr<LedgerRoot>>::None,
                    mark: u8(173),
                }, creation)
            };
            let observed = loan_read(domain) { |stable|
                let writer = ref_from_ptr(write, origin, stable);
                replace(writer@relay, Option<ptr<LedgerRoot>>::Some(origin));
                let baseline = ref_from_ptr(read, origin, stable);
                let route = read(baseline@relay);
                let selected = match route {
                    None => {
                        origin
                    },
                    Some(cached) => {
                        cached
                    },
                };
                let promised = unchecked ref_from_ptr(read, selected, stable);
                let echo = read(promised@relay);
                match echo {
                    None => {
                        u8(11)
                    },
                    Some(_) => {
                        u8(87)
                    },
                }
            };
            let empty = loan_exclusive_read(domain) { |ending|
                destroy(origin, ending)
            };
            let reclaimed = erase_slot<LedgerRoot>(empty);
            finalize_domain(domain);
            deallocate(allocation, reclaimed);
            observed
        },
    }
}
