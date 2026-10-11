struct Node { next: Option<ptr<Node>>, prev: Option<ptr<Node>>, child: Option<ptr<Node>>, payload: u8, }
struct DetachedHolder { node: ptr<Node>, grant: Allocation, dom: LifetimeDomain, }
fn unlink_field(link: ref<write,Option<ptr<Node>>>, replacement: Option<ptr<Node>>) -> unit {
    let old = replace(link, replacement);
    unit
}
struct RootReceipt { node: ptr<Node>, allocation: Allocation, domain: LifetimeDomain, }
struct PairedCustody { receiver: RootReceipt, detached: RootReceipt, }
fn release_receipt(root: RootReceipt) -> unit {
    let RootReceipt { node, allocation, domain } = root;
    let empty = loan_exclusive_read(domain) { |guard| destroy(node, guard) };
    let raw = erase_slot<Node>(empty);
    finalize_domain(domain);
    deallocate(allocation, raw);
    unit
}
fn convey_pair(destination: RootReceipt, isolated: RootReceipt, hint: ptr<Node>) -> PairedCustody {
    let ownership = PairedCustody { receiver: destination, detached: isolated };
    return ownership;
}
fn release_pair(value: PairedCustody) -> unit {
    let PairedCustody { receiver, detached } = value;
    release_receipt(detached);
    release_receipt(receiver);
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
                initialize(vacant_src, Node { next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(10) }, stable_src)
            };
            match try_allocate_one<Node>() {
                None => {
                    let empty_src = loan_exclusive_read(life_src) { |e_src| destroy(ptr_src, e_src) };
                    let reclaimed_src = erase_slot<Node>(empty_src);
                    finalize_domain(life_src);
                    deallocate(allocation_src, reclaimed_src);
                    unit
                },
                Some(bundle_A) => {
                    let OneBacking { allocation, raw } = bundle_A;
                    let allocation_A = allocation;
                    let vacant_A = into_slot<Node>(raw);
                    let life_A = lifetime_domain();
                    let ptr_A = loan_read(life_A) { |stable_A|
                        initialize(vacant_A, Node { next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(19) }, stable_A)
                    };
                    match try_allocate_one<Node>() {
                        None => {
                            let empty_A = loan_exclusive_read(life_A) { |e_A| destroy(ptr_A, e_A) };
                            let reclaimed_A = erase_slot<Node>(empty_A);
                            finalize_domain(life_A);
                            deallocate(allocation_A, reclaimed_A);
                            let empty_src = loan_exclusive_read(life_src) { |e_src| destroy(ptr_src, e_src) };
                            let reclaimed_src = erase_slot<Node>(empty_src);
                            finalize_domain(life_src);
                            deallocate(allocation_src, reclaimed_src);
                            unit
                        },
                        Some(bundle_B) => {
                            let OneBacking { allocation, raw } = bundle_B;
                            let allocation_B = allocation;
                            let vacant_B = into_slot<Node>(raw);
                            let life_B = lifetime_domain();
                            let ptr_B = loan_read(life_B) { |stable_B|
                                initialize(vacant_B, Node { next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(28) }, stable_B)
                            };
                            match try_allocate_one<Node>() {
                                None => {
                                    let empty_B = loan_exclusive_read(life_B) { |e_B| destroy(ptr_B, e_B) };
                                    let reclaimed_B = erase_slot<Node>(empty_B);
                                    finalize_domain(life_B);
                                    deallocate(allocation_B, reclaimed_B);
                                    let empty_A = loan_exclusive_read(life_A) { |e_A| destroy(ptr_A, e_A) };
                                    let reclaimed_A = erase_slot<Node>(empty_A);
                                    finalize_domain(life_A);
                                    deallocate(allocation_A, reclaimed_A);
                                    let empty_src = loan_exclusive_read(life_src) { |e_src| destroy(ptr_src, e_src) };
                                    let reclaimed_src = erase_slot<Node>(empty_src);
                                    finalize_domain(life_src);
                                    deallocate(allocation_src, reclaimed_src);
                                    unit
                                },
                                Some(bundle_C) => {
                                    let OneBacking { allocation, raw } = bundle_C;
                                    let allocation_C = allocation;
                                    let vacant_C = into_slot<Node>(raw);
                                    let life_C = lifetime_domain();
                                    let ptr_C = loan_read(life_C) { |stable_C|
                                        initialize(vacant_C, Node { next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(37) }, stable_C)
                                    };
                                    match try_allocate_one<Node>() {
                                        None => {
                                            let empty_C = loan_exclusive_read(life_C) { |e_C| destroy(ptr_C, e_C) };
                                            let reclaimed_C = erase_slot<Node>(empty_C);
                                            finalize_domain(life_C);
                                            deallocate(allocation_C, reclaimed_C);
                                            let empty_B = loan_exclusive_read(life_B) { |e_B| destroy(ptr_B, e_B) };
                                            let reclaimed_B = erase_slot<Node>(empty_B);
                                            finalize_domain(life_B);
                                            deallocate(allocation_B, reclaimed_B);
                                            let empty_A = loan_exclusive_read(life_A) { |e_A| destroy(ptr_A, e_A) };
                                            let reclaimed_A = erase_slot<Node>(empty_A);
                                            finalize_domain(life_A);
                                            deallocate(allocation_A, reclaimed_A);
                                            let empty_src = loan_exclusive_read(life_src) { |e_src| destroy(ptr_src, e_src) };
                                            let reclaimed_src = erase_slot<Node>(empty_src);
                                            finalize_domain(life_src);
                                            deallocate(allocation_src, reclaimed_src);
                                            unit
                                        },
                                        Some(bundle_dst) => {
                                            let OneBacking { allocation, raw } = bundle_dst;
                                            let allocation_dst = allocation;
                                            let vacant_dst = into_slot<Node>(raw);
                                            let life_dst = lifetime_domain();
                                            let ptr_dst = loan_read(life_dst) { |stable_dst|
                                                initialize(vacant_dst, Node { next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(46) }, stable_dst)
                                            };
                                            let old_link_0 = loan_read(life_src) { |scope_0|
                                                let accessor_0 = ref_from_ptr(write, ptr_src, scope_0);
                                                replace(accessor_0@child, Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            let old_link_1 = loan_read(life_A) { |scope_1|
                                                let accessor_1 = ref_from_ptr(write, ptr_A, scope_1);
                                                replace(accessor_1@prev, Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let old_link_2 = loan_read(life_A) { |scope_2|
                                                let accessor_2 = ref_from_ptr(write, ptr_A, scope_2);
                                                replace(accessor_2@next, Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            let old_link_3 = loan_read(life_B) { |scope_3|
                                                let accessor_3 = ref_from_ptr(write, ptr_B, scope_3);
                                                replace(accessor_3@prev, Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            let old_link_4 = loan_read(life_B) { |scope_4|
                                                let accessor_4 = ref_from_ptr(write, ptr_B, scope_4);
                                                replace(accessor_4@next, Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let old_link_5 = loan_read(life_C) { |scope_5|
                                                let accessor_5 = ref_from_ptr(write, ptr_C, scope_5);
                                                replace(accessor_5@prev, Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            let carried_B = DetachedHolder { node: ptr_B, grant: allocation_B, dom: life_B };
                                            let DetachedHolder { node, grant, dom } = carried_B;
                                            let ptr_B_m = node;
                                            let allocation_B_m = grant;
                                            let life_B_m = dom;
                                            let cleared_previous = loan_read(life_B_m) { |first_stable|
                                                let first_root = ref_from_ptr(write, ptr_B_m, first_stable);
                                                unlink_field(first_root@prev, Option<ptr<Node>>::None)
                                            };
                                            let cleared_next = loan_read(life_B_m) { |second_stable|
                                                let second_root = ref_from_ptr(write, ptr_B_m, second_stable);
                                                unlink_field(second_root@next, Option<ptr<Node>>::None)
                                            };
                                            let swapped_next = loan_read(life_A) { |stable_A_detach|
                                                let direct_A = ref_from_ptr(write, ptr_A, stable_A_detach);
                                                replace(direct_A@next, Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let swapped_prev = loan_read(life_C) { |stable_C_detach|
                                                let direct_C = ref_from_ptr(write, ptr_C, stable_C_detach);
                                                replace(direct_C@prev, Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            // All four original detach scopes ended. R-owned whole-value composition.
                                            let dst_receipt = RootReceipt { node: ptr_dst, allocation: allocation_dst, domain: life_dst };
                                            let b_receipt = RootReceipt { node: ptr_B_m, allocation: allocation_B_m, domain: life_B_m };
                                            let returned_pair = convey_pair(dst_receipt, b_receipt, ptr_B);
                                            // Original donor A,C,src are freed before B or dst.
                                            let empty_A = loan_exclusive_read(life_A) { |e_A| destroy(ptr_A, e_A) };
                                            let reclaimed_A = erase_slot<Node>(empty_A);
                                            finalize_domain(life_A);
                                            deallocate(allocation_A, reclaimed_A);
                                            let empty_C = loan_exclusive_read(life_C) { |e_C| destroy(ptr_C, e_C) };
                                            let reclaimed_C = erase_slot<Node>(empty_C);
                                            finalize_domain(life_C);
                                            deallocate(allocation_C, reclaimed_C);
                                            let empty_src = loan_exclusive_read(life_src) { |e_src| destroy(ptr_src, e_src) };
                                            let reclaimed_src = erase_slot<Node>(empty_src);
                                            finalize_domain(life_src);
                                            deallocate(allocation_src, reclaimed_src);
                                            // Only the returned whole pair is consumed at the matched late terminal.
                                            release_pair(returned_pair);
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
