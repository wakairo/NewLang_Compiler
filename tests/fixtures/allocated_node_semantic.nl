struct Node {
    next: Option<ptr<Node>>,
    payload: u8,
}

fn main() -> unit {
match try_allocate_one<Node>() {
    None => {
        unit
    },
    Some(bundle) => {
        let OneBacking { allocation, raw } = bundle;
        let empty = into_slot<Node>(raw);
        let life = lifetime_domain();
        let tail = loan_read(life) { |stable|
            initialize(
                empty,
                Node { next: Option<ptr<Node>>::None, payload: u8(2) },
                stable
            )
        };

        let head = Node {
            next: Option<ptr<Node>>::None,
            payload: u8(1)
        };

        let old_none = loan_write(head@next) { |w|
            replace(w, Option<ptr<Node>>::Some(tail))
        };

        let observed = head@next;
        match observed {
            None => { unit },
            Some(q) => {
                loan_read(life) { |stable|
                    let access = ref_from_ptr(read, q, stable);
                    unit
                }
            },
        };

        let old_some = loan_write(head@next) { |w|
            replace(w, Option<ptr<Node>>::None)
        };

        let empty_again = loan_exclusive_read(life) { |ending|
            destroy(tail, ending)
        };
        let full_raw = erase_slot<Node>(empty_again);
        finalize_domain(life);
        deallocate(allocation, full_raw);
        unit
    },
}

}
