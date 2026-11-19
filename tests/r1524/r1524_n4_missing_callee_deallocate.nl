struct SignalCell {
    chain: Option<ptr<SignalCell>>,
    datum: u8,
}

struct ControlPair {
    setting: u8,
    seal: u8,
}

fn exchange_setting(access: ref<write,u8>, incoming: u8) -> u8 {
    let departed = replace(access, incoming);
    return departed;
}

fn retire_cell(address: ptr<SignalCell>, token: Allocation, epoch: LifetimeDomain) -> unit {
    let vacant = loan_exclusive_read(epoch) { |ending|
        destroy(address, ending)
    };
    let bytes = erase_slot<SignalCell>(vacant);
    finalize_domain(epoch);
    unit
}

fn main() -> unit {
    let control = ControlPair { setting: u8(29), seal: u8(101) };
    let saved_setting = loan_write(control@setting) { |access|
        exchange_setting(access, u8(149))
    };
    let intermediate_setting = control@setting;
    let intermediate_seal = control@seal;

    match try_allocate_one<SignalCell>() {
        None => { unit },
        Some(initial_bundle) => {
            let OneBacking { allocation, raw } = initial_bundle;
            let initial_allocation = allocation;
            let initial_vacant = into_slot<SignalCell>(raw);
            let initial_domain = lifetime_domain();
            let initial_ptr = loan_read(initial_domain) { |stability|
                initialize(
                    initial_vacant,
                    SignalCell { chain: Option<ptr<SignalCell>>::None, datum: saved_setting },
                    stability
                )
            };

            match try_allocate_one<SignalCell>() {
                None => {
                    let initial_empty = loan_exclusive_read(initial_domain) { |ending|
                        destroy(initial_ptr, ending)
                    };
                    let initial_bytes = erase_slot<SignalCell>(initial_empty);
                    finalize_domain(initial_domain);
                    deallocate(initial_allocation, initial_bytes);
                    unit
                },
                Some(following_bundle) => {
                    let OneBacking { allocation, raw } = following_bundle;
                    let following_allocation = allocation;
                    let following_vacant = into_slot<SignalCell>(raw);
                    let following_domain = lifetime_domain();
                    let following_ptr = loan_read(following_domain) { |stability|
                        initialize(
                            following_vacant,
                            SignalCell { chain: Option<ptr<SignalCell>>::None, datum: u8(211) },
                            stability
                        )
                    };

                    let selected_datum = loan_read(following_domain) { |stability|
                        let view = ref_from_ptr(read, following_ptr, stability);
                        read(view@datum)
                    };
                    retire_cell(following_ptr, following_allocation, following_domain);

                    let initial_empty = loan_exclusive_read(initial_domain) { |ending|
                        destroy(initial_ptr, ending)
                    };
                    let initial_bytes = erase_slot<SignalCell>(initial_empty);
                    finalize_domain(initial_domain);
                    deallocate(initial_allocation, initial_bytes);

                    let displaced_setting = loan_write(control@setting) { |access|
                        exchange_setting(access, selected_datum)
                    };
                    unit
                },
            }
        },
    };
    let final_setting = control@setting;
    let final_seal = control@seal;
    unit
}
