// P281 feasibility input: PROPOSED/UNADOPTED. No native/product PASS.
struct Node {
    next: Option<ptr<Node>>,
    prev: Option<ptr<Node>>,
    child: Option<ptr<Node>>,
    payload: u8,
}

struct LiveRoot { p: ptr<Node>, a: Allocation, d: LifetimeDomain, }
struct TreeFour { src: LiveRoot, first: LiveRoot, middle: LiveRoot, last: LiveRoot, }
struct TreeThree { src: LiveRoot, first: LiveRoot, last: LiveRoot, }
struct DetachResult { donor: TreeThree, detached: LiveRoot, }
struct TreeTwo { root: LiveRoot, child: LiveRoot, }

fn detach_middle(whole:TreeFour)->DetachResult {
    let TreeFour { src, first, middle, last } = whole;
    let LiveRoot { p:pa, a:aa, d:da } = first;
    let LiveRoot { p:pb, a:ab, d:db } = middle;
    let LiveRoot { p:pc, a:ac, d:dc } = last;
    // DETACH 1: A.next=C
    let old_A_next = loan_read(da) { |s_A|
        let w_A = ref_from_ptr(write, pa, s_A);
        replace(w_A@next, Option<ptr<Node>>::Some(pc))
    };
    // DETACH 2: C.prev=A
    let old_C_prev = loan_read(dc) { |s_C|
        let w_C = ref_from_ptr(write, pc, s_C);
        replace(w_C@prev, Option<ptr<Node>>::Some(pa))
    };
    // DETACH 3: B.prev=None
    let old_B_prev = loan_read(db) { |s_B_prev|
        let w_B_prev = ref_from_ptr(write, pb, s_B_prev);
        replace(w_B_prev@prev, Option<ptr<Node>>::None)
    };
    // DETACH 4: B.next=None
    let old_B_next = loan_read(db) { |s_B_next|
        let w_B_next = ref_from_ptr(write, pb, s_B_next);
        replace(w_B_next@next, Option<ptr<Node>>::None)
    };
    // All four stability scopes/derived refs have ended before repack.
    let donor = TreeThree { src:src,
        first:LiveRoot { p:pa, a:aa, d:da },
        last:LiveRoot { p:pc, a:ac, d:dc } };
    let detached = LiveRoot { p:pb, a:ab, d:db };
    let result = DetachResult { donor:donor, detached:detached };
    return result;
}

fn attach_whole(receiver:LiveRoot, detached:LiveRoot, ptr_B:ptr<Node>)->TreeTwo {
    let LiveRoot { p:pd, a:ad, d:dd } = receiver;
    let LiveRoot { p:pb, a:ab, d:db } = detached;
    // ADOPT 1: dst.child=B (a Copy link does not grant an owner)
    let old_dst_child = loan_read(dd) { |s_dst|
        let w_dst = ref_from_ptr(write, pd, s_dst);
        replace(w_dst@child, Option<ptr<Node>>::Some(ptr_B))
    };
    // ADOPT 2: B.prev=B
    let old_B_prev = loan_read(db) { |s_B|
        let w_B = ref_from_ptr(write, pb, s_B);
        replace(w_B@prev, Option<ptr<Node>>::Some(ptr_B))
    };
    let result = TreeTwo { root:LiveRoot { p:pd, a:ad, d:dd },
        child:LiveRoot { p:pb, a:ab, d:db } };
    return result;
}

fn finish_root(ticket:LiveRoot)->unit {
    let LiveRoot { p, a, d } = ticket;
    let empty = loan_exclusive_read(d) { |ending| destroy(p, ending) };
    let full = erase_slot<Node>(empty);
    finalize_domain(d);
    deallocate(a, full);
    unit
}
fn finish_two(whole:TreeTwo)->unit {
    let TreeTwo { root, child } = whole;
    finish_root(child);
    finish_root(root);
    unit
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
                                            let old_B_prev = loan_read(life_B) { |s_B_prev|
                                                let w_B_prev = ref_from_ptr(write, ptr_B, s_B_prev);
                                                replace(w_B_prev@prev, Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            let old_B_next = loan_read(life_B) { |s_B_next|
                                                let w_B_next = ref_from_ptr(write, ptr_B, s_B_next);
                                                replace(w_B_next@next, Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let old_C_prev = loan_read(life_C) { |s_C_prev|
                                                let w_C_prev = ref_from_ptr(write, ptr_C, s_C_prev);
                                                replace(w_C_prev@prev, Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            // All five originals exist; exactly six initial Changes above.
                                            let donor = TreeFour {
                                                src:LiveRoot { p:ptr_src, a:allocation_src, d:life_src },
                                                first:LiveRoot { p:ptr_A, a:allocation_A, d:life_A },
                                                middle:LiveRoot { p:ptr_B, a:allocation_B, d:life_B },
                                                last:LiveRoot { p:ptr_C, a:allocation_C, d:life_C }
                                            };
                                            let receiver = LiveRoot { p:ptr_dst, a:allocation_dst, d:life_dst };
                                            let result = detach_middle(donor);
                                            let DetachResult { donor:remaining, detached } = result;
                                            let adopted = attach_whole(receiver, detached, ptr_B);
                                            let TreeThree { src, first, last } = remaining;
                                            finish_root(first); // original A (#2) FIRST
                                            finish_root(last);  // original C (#4)
                                            finish_root(src);   // original src (#1)
                                            finish_two(adopted); // original B (#3), then dst (#5)
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
