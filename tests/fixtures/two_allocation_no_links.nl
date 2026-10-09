struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main() -> unit {
    match try_allocate_one<Node>() {
        None => {
            unit
        },
        Some(bundle_src) => {
            let OneBacking { allocation, raw } = bundle_src;
            let allocation_src = allocation;
            let vacant_src = into_slot<Node>(raw);
            let life_src = lifetime_domain();
            let ptr_src = loan_read(life_src) { |stable_src|
                initialize(vacant_src, Node {
                    next: Option<ptr<Node>>::None,
                    payload: u8(1)
                }, stable_src)
            };
            match try_allocate_one<Node>() {
                None => {
                    let empty_src = loan_exclusive_read(life_src) { |ending_src|
                        destroy(ptr_src, ending_src)
                    };
                    let full_src = erase_slot<Node>(empty_src);
                    finalize_domain(life_src);
                    deallocate(allocation_src, full_src);
                    unit
                },
                Some(bundle_A) => {
                    let OneBacking { allocation, raw } = bundle_A;
                    let allocation_A = allocation;
                    let vacant_A = into_slot<Node>(raw);
                    let life_A = lifetime_domain();
                    let ptr_A = loan_read(life_A) { |stable_A|
                        initialize(vacant_A, Node {
                            next: Option<ptr<Node>>::None,
                            payload: u8(1)
                        }, stable_A)
                    };
                    let empty_A = loan_exclusive_read(life_A) { |ending_A|
                        destroy(ptr_A, ending_A)
                    };
                    let full_A = erase_slot<Node>(empty_A);
                    finalize_domain(life_A);
                    deallocate(allocation_A, full_A);
                    let empty_src = loan_exclusive_read(life_src) { |ending_src|
                        destroy(ptr_src, ending_src)
                    };
                    let full_src = erase_slot<Node>(empty_src);
                    finalize_domain(life_src);
                    deallocate(allocation_src, full_src);
                    unit
                },
            }
        },
    }
}
