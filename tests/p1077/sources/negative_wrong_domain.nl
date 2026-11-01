struct ByteLanes { hot: u8, cold: u8 }
fn swap_lane(slot: ref<write,u8>, incoming: u8) -> u8 {
    replace(slot, incoming)
}
fn relay_lane(slot: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(slot, incoming);
    let remembered = displaced;
    return remembered;
}
struct ChainItem { next: Option<ptr<ChainItem>>, mark: u8 }
fn extract_item(edge: ref<write,Option<ptr<ChainItem>>>, item: ptr<ChainItem>,
                backing: Allocation, epoch: LifetimeDomain) -> LiveTail {
    let prior = replace(edge, Option<ptr<ChainItem>>::None);
    let result = LiveTail{owned_ptr: item, owned_allocation: backing, owned_domain: epoch};
    return result;
}
fn dispose_item(item: ptr<ChainItem>, backing: Allocation, epoch: LifetimeDomain) -> unit {
    let hole = loan_exclusive_read(epoch) { |ending| destroy(item, ending) };
    let raw = erase_slot<ChainItem>(hole);
    finalize_domain(epoch);
    deallocate(backing, raw);
    unit
}
fn main() -> unit {
    match try_allocate_one<ChainItem>() {
        None => { unit },
        Some(first_bundle) => {
            let OneBacking { allocation, raw } = first_bundle;
            let first_backing = allocation;
            let first_slot = into_slot<ChainItem>(raw);
            let first_epoch = lifetime_domain();
            let first = loan_read(first_epoch) { |scope|
                initialize(first_slot, ChainItem{next: Option<ptr<ChainItem>>::None, mark: u8(41)}, scope)
            };
            match try_allocate_one<ChainItem>() {
                None => {
                    let hole = loan_exclusive_read(first_epoch) { |ending| destroy(first, ending) };
                    let raw = erase_slot<ChainItem>(hole);
                    finalize_domain(first_epoch);
                    deallocate(first_backing, raw);
                    unit
                },
                Some(second_bundle) => {
                    let OneBacking { allocation, raw } = second_bundle;
                    let second_backing = allocation;
                    let second_slot = into_slot<ChainItem>(raw);
                    let second_epoch = lifetime_domain();
                    let second = loan_read(second_epoch) { |scope|
                        initialize(second_slot, ChainItem{next: Option<ptr<ChainItem>>::None, mark: u8(199)}, scope)
                    };
                    let empty = loan_read(first_epoch) { |scope|
                        let head = ref_from_ptr(write, first, scope);
                        replace(head@next, Option<ptr<ChainItem>>::Some(second))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } =
                        loan_read(first_epoch) { |scope|
                            let head = ref_from_ptr(write, first, scope);
                            extract_item(head@next, second, second_backing, first_epoch)
                        };
                    loan_read(owned_domain) { |scope|
                        let readable = ref_from_ptr(read, owned_ptr, scope);
                        unit
                    };
                    dispose_item(owned_ptr, owned_allocation, owned_domain);
                    let hole = loan_exclusive_read(first_epoch) { |ending| destroy(first, ending) };
                    let raw = erase_slot<ChainItem>(hole);
                    finalize_domain(first_epoch);
                    deallocate(first_backing, raw);
                    unit
                },
            }
        },
    };
    let primary = ByteLanes{hot: u8(31), cold: u8(149)};
    let reserve = ByteLanes{cold: u8(72), hot: u8(203)};
    let old_a = loan_write(primary@hot) { |slot| swap_lane(slot, u8(88)) };
    let old_b = loan_write(reserve@cold) { |slot| relay_lane(slot, old_a) };
    let batch_result = loan_write(reserve@hot) { |slot|
        let original = replace(slot, old_b);
        let intermediate = replace(slot, old_a);
        let forwarded = intermediate;
        forwarded
    };
    let final_old = loan_write(primary@cold) { |slot| replace(slot, batch_result) };
    let observed_a = old_a;
    let observed_b = old_b;
    let observed_batch = batch_result;
    let observed_final = final_old;
    let primary_hot = primary@hot;
    let primary_cold = primary@cold;
    let reserve_hot = reserve@hot;
    let reserve_cold = reserve@cold;
    return unit;
}
