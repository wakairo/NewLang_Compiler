struct Grove { branch: Option<ptr<Grove>>, flag: u8 }

fn take_prior(place: ref<write,u8>, incoming: u8) -> u8 {
    return replace(place, incoming);
}

struct Dials { cold: u8, hot: u8 }

fn shuttle_prior(place: ref<write,u8>, incoming: u8) -> u8 {
    let old = replace(place, incoming);
    let copied_old = old;
    return copied_old;
}

fn separate_leaf(edge: ref<write,Option<ptr<Grove>>>, p: ptr<Grove>, grant: Allocation, life: LifetimeDomain) -> LiveTail {
    let old_edge = replace(edge, Option<ptr<Grove>>::None);
    return LiveTail { owned_domain: life, owned_ptr: p, owned_allocation: grant };
}

fn finish_leaf(p: ptr<Grove>, grant: Allocation, life: LifetimeDomain) -> unit {
    let empty = loan_exclusive_read(life) { |ending|
        destroy(p, ending)
    };
    let raw = erase_slot<Grove>(empty);
    finalize_domain(life);
    deallocate(grant, raw);
    unit
}

fn main() -> unit {
    match try_allocate_one<Grove>() {
        None => { unit },
        Some(seed_bundle) => {
            let OneBacking { allocation, raw } = seed_bundle;
            let grant_h = allocation;
            let slot_h = into_slot<Grove>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(slot_h, Grove { branch: Option<ptr<Grove>>::None, flag: u8(58) }, stable_h)
            };
            match try_allocate_one<Grove>() {
                None => {
                    let dead_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let freed_h = erase_slot<Grove>(dead_h);
                    finalize_domain(life_h);
                    deallocate(grant_h, freed_h);
                    unit
                },
                Some(fruit_bundle) => {
                    let OneBacking { allocation, raw } = fruit_bundle;
                    let grant_t = allocation;
                    let slot_t = into_slot<Grove>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(slot_t, Grove { branch: Option<ptr<Grove>>::None, flag: u8(219) }, stable_t)
                    };
                    let none_before = loan_read(life_h) { |stable_h|
                        let writer = ref_from_ptr(write, ptr_h, stable_h);
                        replace(writer@branch, Option<ptr<Grove>>::Some(ptr_t))
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(life_h) { |stable_h|
                        let writer = ref_from_ptr(write, ptr_h, stable_h);
                        separate_leaf(writer@branch, ptr_t, grant_t, life_t)
                    };
                    finish_leaf(owned_ptr, owned_allocation, owned_domain);
                    let dead_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let freed_h = erase_slot<Grove>(dead_h);
                    finalize_domain(life_h);
                    deallocate(grant_h, freed_h);
                    unit
                },
            }
        },
    };
    let left = Dials { hot: u8(199), cold: u8(7) };
    let right = Dials { cold: u8(241), hot: u8(18) };
    let old_left_cold = loan_write(left@cold) { |w|
        take_prior(w, u8(85))
    };
    let old_right_hot = loan_write(right@hot) { |w|
        shuttle_prior(w, old_left_cold)
    };
    let old_left_hot = loan_write(left@hot) { |w|
        replace(w, old_right_hot)
    };
    let current_left_cold = left@cold;
    let current_left_hot = left@hot;
    let current_right_cold = right@cold;
    let current_right_hot = right@hot;
    unit
}
