struct LedgerRoot {
    relay: Option<ptr<LedgerRoot>>,
    mark: u8,
}

fn wrong_domain() -> u8 {
    match try_allocate_one<LedgerRoot>() {
        None => {
            u8(0)
        },
        Some(package) => {
            let OneBacking { allocation, raw } = package;
            let claim = into_slot<LedgerRoot>(raw);
            let domain = lifetime_domain();
            let other_domain = lifetime_domain();
            let origin = loan_read(domain) { |creation|
                initialize(claim, LedgerRoot {
                    relay: Option<ptr<LedgerRoot>>::None,
                    mark: u8(173),
                }, creation)
            };
            let observed = loan_read(other_domain) { |foreign_stable|
                let forbidden = unchecked ref_from_ptr(read, origin, foreign_stable);
                let echo = read(forbidden@relay);
                match echo {
                    None => {
                        u8(19)
                    },
                    Some(_) => {
                        u8(23)
                    },
                }
            };
            finalize_domain(other_domain);
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
