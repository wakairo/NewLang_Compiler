struct Pocket { edge: Option<ptr<Pocket>>, mark: u8 }
struct Panel { left: u8, right: u8 }

fn revise(slot: ref<write, u8>, incoming: u8) -> u8 {
    let preceding = replace(slot, incoming);
    preceding
}

fn main() -> unit {
    let pane = Panel { right: u8(118), left: u8(9) };
    let first_previous = loan_write(pane@right) { |w|
        revise(w, u8(33))
    };
    match try_allocate_one<Pocket>() {
        None => { unit },
        Some(outer_bundle) => {
            let OneBacking { allocation, raw } = outer_bundle;
            let owner_outer = allocation;
            let raw_outer = into_slot<Pocket>(raw);
            let region_outer = lifetime_domain();
            let address_outer = loan_read(region_outer) { |borrow_outer|
                initialize(raw_outer, Pocket {
                    edge: Option<ptr<Pocket>>::None, mark: u8(66)
                }, borrow_outer)
            };
            match try_allocate_one<Pocket>() {
                None => {
                    let terminated_outer = loan_exclusive_read(region_outer) { |release_outer|
                        destroy(address_outer, release_outer)
                    };
                    let full_outer = erase_slot<Pocket>(terminated_outer);
                    finalize_domain(region_outer);
                    deallocate(owner_outer, full_outer);
                    unit
                },
                Some(inner_bundle) => {
                    let OneBacking { allocation, raw } = inner_bundle;
                    let owner_inner = allocation;
                    let raw_inner = into_slot<Pocket>(raw);
                    let region_inner = lifetime_domain();
                    let address_inner = loan_read(region_inner) { |borrow_inner|
                        initialize(raw_inner, Pocket {
                            edge: Option<ptr<Pocket>>::None, mark: u8(229)
                        }, borrow_inner)
                    };
                    let fetched = loan_read(region_inner) { |borrow_inner|
                        let observed_inner = ref_from_ptr(read, address_inner, borrow_inner);
                        read(observed_inner@mark)
                    };
                    let terminated_inner = loan_exclusive_read(region_inner) { |release_inner|
                        destroy(address_inner, release_inner)
                    };
                    let full_inner = erase_slot<Pocket>(terminated_inner);
                    finalize_domain(region_inner);
                    deallocate(owner_inner, full_inner);
                    let terminated_outer = loan_exclusive_read(region_outer) { |release_outer|
                        destroy(address_outer, release_outer)
                    };
                    let full_outer = erase_slot<Pocket>(terminated_outer);
                    finalize_domain(region_outer);
                    deallocate(owner_outer, full_outer);
                    let second_previous = loan_write(pane@right) { |w|
                        revise(w, fetched)
                    };
                    let old_at_success = second_previous;
                    unit
                },
            }
        },
    };
    let old_at_start = first_previous;
    let current_right = pane@right;
    let unaffected_left = pane@left;
    unit
}
