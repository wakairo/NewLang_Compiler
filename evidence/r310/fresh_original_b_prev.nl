struct Node { next: Option<ptr<Node>>, prev: Option<ptr<Node>>, child: Option<ptr<Node>>, payload: u8, }
struct DetachedHolder { owned_ptr: ptr<Node>, owned_allocation: Allocation, owned_domain: LifetimeDomain, }
fn set_link(destination: ref<write,Option<ptr<Node>>>, replacement: Option<ptr<Node>>) -> unit {
    let displaced = replace(destination, replacement);
    unit
}
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
                    prev: Option<ptr<Node>>::None,
                    child: Option<ptr<Node>>::None,
                    payload: u8(17)
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
                            prev: Option<ptr<Node>>::None,
                            child: Option<ptr<Node>>::None,
                            payload: u8(18)
                        }, stable_A)
                    };
                    match try_allocate_one<Node>() {
                        None => {
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
                        Some(bundle_B) => {
                            let OneBacking { allocation, raw } = bundle_B;
                            let allocation_B = allocation;
                            let vacant_B = into_slot<Node>(raw);
                            let life_B = lifetime_domain();
                            let ptr_B = loan_read(life_B) { |stable_B|
                                initialize(vacant_B, Node {
                                    next: Option<ptr<Node>>::None,
                                    prev: Option<ptr<Node>>::None,
                                    child: Option<ptr<Node>>::None,
                                    payload: u8(19)
                                }, stable_B)
                            };
                            match try_allocate_one<Node>() {
                                None => {
                                    let empty_B = loan_exclusive_read(life_B) { |ending_B|
                                        destroy(ptr_B, ending_B)
                                    };
                                    let full_B = erase_slot<Node>(empty_B);
                                    finalize_domain(life_B);
                                    deallocate(allocation_B, full_B);
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
                                Some(bundle_C) => {
                                    let OneBacking { allocation, raw } = bundle_C;
                                    let allocation_C = allocation;
                                    let vacant_C = into_slot<Node>(raw);
                                    let life_C = lifetime_domain();
                                    let ptr_C = loan_read(life_C) { |stable_C|
                                        initialize(vacant_C, Node {
                                            next: Option<ptr<Node>>::None,
                                            prev: Option<ptr<Node>>::None,
                                            child: Option<ptr<Node>>::None,
                                            payload: u8(20)
                                        }, stable_C)
                                    };
                                    match try_allocate_one<Node>() {
                                        None => {
                                            let empty_C = loan_exclusive_read(life_C) { |ending_C|
                                                destroy(ptr_C, ending_C)
                                            };
                                            let full_C = erase_slot<Node>(empty_C);
                                            finalize_domain(life_C);
                                            deallocate(allocation_C, full_C);
                                            let empty_B = loan_exclusive_read(life_B) { |ending_B|
                                                destroy(ptr_B, ending_B)
                                            };
                                            let full_B = erase_slot<Node>(empty_B);
                                            finalize_domain(life_B);
                                            deallocate(allocation_B, full_B);
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
                                        Some(bundle_dst) => {
                                            let OneBacking { allocation, raw } = bundle_dst;
                                            let allocation_dst = allocation;
                                            let vacant_dst = into_slot<Node>(raw);
                                            let life_dst = lifetime_domain();
                                            let ptr_dst = loan_read(life_dst) { |stable_dst|
                                                initialize(vacant_dst, Node {
                                                    next: Option<ptr<Node>>::None,
                                                    prev: Option<ptr<Node>>::None,
                                                    child: Option<ptr<Node>>::None,
                                                    payload: u8(21)
                                                }, stable_dst)
                                            };
                                            let old_src_child = loan_read(life_src) { |stable_src_child|
                                                let write_src_child = ref_from_ptr(write, ptr_src, stable_src_child);
                                                replace(write_src_child@child, Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            let old_A_prev = loan_read(life_A) { |stable_A_prev|
                                                let write_A_prev = ref_from_ptr(write, ptr_A, stable_A_prev);
                                                replace(write_A_prev@prev, Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let old_A_next = loan_read(life_A) { |stable_A_next|
                                                let write_A_next = ref_from_ptr(write, ptr_A, stable_A_next);
                                                replace(write_A_next@next, Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            let old_B_prev = loan_read(life_B) { |stable_B_prev|
                                                let write_B_prev = ref_from_ptr(write, ptr_B, stable_B_prev);
                                                replace(write_B_prev@prev, Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            let old_B_next = loan_read(life_B) { |stable_B_next|
                                                let write_B_next = ref_from_ptr(write, ptr_B, stable_B_next);
                                                replace(write_B_next@next, Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let old_C_prev = loan_read(life_C) { |stable_C_prev|
                                                let write_C_prev = ref_from_ptr(write, ptr_C, stable_C_prev);
                                                replace(write_C_prev@prev, Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            let original_holder = DetachedHolder { owned_ptr: ptr_B, owned_allocation: allocation_B, owned_domain: life_B };
                                            let transferred_holder = original_holder;
                                            let DetachedHolder { owned_ptr, owned_allocation, owned_domain } = transferred_holder;
                                            let original_ptr_B = owned_ptr;
                                            let original_allocation_B = owned_allocation;
                                            let original_domain_B = owned_domain;
                                            let original_B_next_before = loan_read(original_domain_B) { |stable_before|
                                                let r_before = ref_from_ptr(read, original_ptr_B, stable_before);
                                                read(r_before@next)
                                            };
                                            let original_B_child_before = loan_read(original_domain_B) { |stable_child_before|
                                                let r_child_before = ref_from_ptr(read, original_ptr_B, stable_child_before);
                                                read(r_child_before@child)
                                            };
                                            let changed_prev = loan_read(original_domain_B) { |stable_change|
                                                let write_B_change = ref_from_ptr(write, original_ptr_B, stable_change);
                                                set_link(write_B_change@prev, Option<ptr<Node>>::None);
                                                unit
                                            };
                                            let original_B_next_after = loan_read(original_domain_B) { |stable_after|
                                                let r_after = ref_from_ptr(read, original_ptr_B, stable_after);
                                                read(r_after@next)
                                            };
                                            let original_B_child_after = loan_read(original_domain_B) { |stable_child_after|
                                                let r_child_after = ref_from_ptr(read, original_ptr_B, stable_child_after);
                                                read(r_child_after@child)
                                            };
                                            let empty_dst = loan_exclusive_read(life_dst) { |ending_dst|
                                                destroy(ptr_dst, ending_dst)
                                            };
                                            let full_dst = erase_slot<Node>(empty_dst);
                                            finalize_domain(life_dst);
                                            deallocate(allocation_dst, full_dst);
                                            let empty_C = loan_exclusive_read(life_C) { |ending_C|
                                                destroy(ptr_C, ending_C)
                                            };
                                            let full_C = erase_slot<Node>(empty_C);
                                            finalize_domain(life_C);
                                            deallocate(allocation_C, full_C);
                                            let empty_B = loan_exclusive_read(original_domain_B) { |ending_B|
                                                destroy(original_ptr_B, ending_B)
                                            };
                                            let full_B = erase_slot<Node>(empty_B);
                                            finalize_domain(original_domain_B);
                                            deallocate(original_allocation_B, full_B);
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
                        },
                    }
                },
            }
        },
    }
}
