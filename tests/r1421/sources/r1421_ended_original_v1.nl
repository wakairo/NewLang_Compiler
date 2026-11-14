struct LedgerRoot {
    relay: Option<ptr<LedgerRoot>>,
    mark: u8,
}

fn ended_original() -> u8 {
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
            let empty = loan_exclusive_read(domain) { |ending|
                destroy(origin, ending)
            };
            let observed = loan_read(domain) { |stable|
                let forbidden = unchecked ref_from_ptr(read, origin, stable);
                let echo = read(forbidden@relay);
                match echo {
                    None => {
                        u8(29)
                    },
                    Some(_) => {
                        u8(31)
                    },
                }
            };
            let reclaimed = erase_slot<LedgerRoot>(empty);
            finalize_domain(domain);
            deallocate(allocation, reclaimed);
            observed
        },
    }
}
