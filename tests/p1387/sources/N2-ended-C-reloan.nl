struct Node {
    next: Option<ptr<Node>>,
    prev: Option<ptr<Node>>,
    child: Option<ptr<Node>>,
    payload: u8,
}

fn main() -> unit {
    match try_allocate_one<Node>() {  // independent allocation site 1: src
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
                    payload: u8(1)
                }, stable_src)
            };
            match try_allocate_one<Node>() {  // independent allocation site 2: A
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
                            payload: u8(2)
                        }, stable_A)
                    };
                    match try_allocate_one<Node>() {  // independent allocation site 3: B
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
                                    payload: u8(3)
                                }, stable_B)
                            };
                            match try_allocate_one<Node>() {  // independent allocation site 4: C
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
                                            payload: u8(4)
                                        }, stable_C)
                                    };
                                    match try_allocate_one<Node>() {  // independent allocation site 5: dst
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
                                                    payload: u8(5)
                                                }, stable_dst)
                                            };
                                            // Pre-detach checkpoint: exactly five original live roots;
                                            // distinct R_src/R_A/R_B/R_C/R_dst and D_src/D_A/D_B/D_C/D_dst.
                                            // dst.child stays None from its whole initial construction.
                                            let old_src_child = loan_read(life_src) { |stable_src_child|
                                                let w_src_child = ref_from_ptr(write, ptr_src, stable_src_child);
                                                replace(w_src_child@child,
                                                    Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            let old_A_prev = loan_read(life_A) { |stable_A_prev|
                                                let w_A_prev = ref_from_ptr(write, ptr_A, stable_A_prev);
                                                replace(w_A_prev@prev,
                                                    Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let old_A_next = loan_read(life_A) { |stable_A_next|
                                                let w_A_next = ref_from_ptr(write, ptr_A, stable_A_next);
                                                replace(w_A_next@next,
                                                    Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            let old_B_prev = loan_read(life_B) { |stable_B_prev|
                                                let w_B_prev = ref_from_ptr(write, ptr_B, stable_B_prev);
                                                replace(w_B_prev@prev,
                                                    Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            let old_B_next = loan_read(life_B) { |stable_B_next|
                                                let w_B_next = ref_from_ptr(write, ptr_B, stable_B_next);
                                                replace(w_B_next@next,
                                                    Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let old_C_prev = loan_read(life_C) { |stable_C_prev|
                                                let w_C_prev = ref_from_ptr(write, ptr_C, stable_C_prev);
                                                replace(w_C_prev@prev,
                                                    Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            // At this exact point seven current cJSON facts hold:
                                            // src.child=A; A.prev=C; A.next=B; B.prev=A;
                                            // B.next=C; C.prev=B; dst.child=None.
                                            // A.prev=C is the FIRST-CHILD TAIL SHORTCUT, not A.prev=None.
                                            // No B detach, no adoption, no other graph mutation.
                                            // P1386 C754 proposed-only source; all five original H roots remain live.
                                            let p1386_old_B_next = loan_read(life_B) { |p1386_stable_B|
                                                let p1386_write_B = ref_from_ptr(write, ptr_B, p1386_stable_B);
                                                replace(p1386_write_B@next, Option<ptr<Node>>::None)
                                            };
                                            let p1386_old_A_prev = loan_read(life_A) { |p1386_stable_A|
                                                let p1386_write_A = ref_from_ptr(write, ptr_A, p1386_stable_A);
                                                replace(p1386_write_A@prev, Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            let p1386_old_C_prev = loan_read(life_C) { |p1386_stable_C|
                                                let p1386_write_C = ref_from_ptr(write, ptr_C, p1386_stable_C);
                                                replace(p1386_write_C@prev, Option<ptr<Node>>::None)
                                            };
                                            
                                            // Test-end cleanup ONLY. Copy ptr links can dangle and
                                            // are never dereferenced or reloaned after target EndRoot.
                                            let empty_dst = loan_exclusive_read(life_dst) { |ending_dst|
                                                destroy(ptr_dst, ending_dst)
                                            };
                                            let full_dst = erase_slot<Node>(empty_dst);
                                            finalize_domain(life_dst);
                                            deallocate(allocation_dst, full_dst);
                                            let empty_C = loan_exclusive_read(life_C) { |ending_C|
                                                destroy(ptr_C, ending_C)
                                            };
                                            // Deliberate negative: C root ended above; live D_C does not resurrect C.
                                            loan_read(life_C) { |p1386_after_C_end_stable|
                                                let p1386_after_C_end_ref = ref_from_ptr(read, ptr_C, p1386_after_C_end_stable);
                                                unit
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
