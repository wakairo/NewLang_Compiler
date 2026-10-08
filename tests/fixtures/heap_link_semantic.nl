struct Node { next: Option<ptr<Node>>, payload: u8, }

fn main() -> unit {
match try_allocate_one<Node>() {
    None => { unit },
    Some(bundle) => {
        let OneBacking { allocation, raw } = bundle;
        let vacant = into_slot<Node>(raw);
        let life = lifetime_domain();
        let heap_head = loan_read(life) { |stable|
            initialize(
                vacant,
                Node { next: Option<ptr<Node>>::None, payload: u8(1) },
                stable
            )
        };
        let lexical_tail = Node {
            next: Option<ptr<Node>>::None, payload: u8(2)
        };
        let tail_ptr = loan_read(lexical_tail) { |r| ptr_from_ref(r) };

        let old_none = loan_read(life) { |stable|
            let root_w = ref_from_ptr(write, heap_head, stable);
            replace(root_w@next, Option<ptr<Node>>::Some(tail_ptr))
        };

        let seen = loan_read(life) { |stable|
            let root_r = ref_from_ptr(read, heap_head, stable);
            read(root_r@next)
        };
        match seen {
            None => { unit },
            Some(q) => {
                loan_read_ptr(q) { |tail_read| unit }
            },
        };

        let old_some = loan_read(life) { |stable|
            let root_w2 = ref_from_ptr(write, heap_head, stable);
            replace(root_w2@next, Option<ptr<Node>>::None)
        };

        let empty = loan_exclusive_read(life) { |ending|
            destroy(heap_head, ending)
        };
        let full_raw = erase_slot<Node>(empty);
        finalize_domain(life);
        deallocate(allocation, full_raw);
        unit
    },
}

}
