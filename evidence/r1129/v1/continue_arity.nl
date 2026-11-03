struct Mosaic { signal: u8, checkpoint: u8 }

fn exchange(cell: ref<write,u8>, replacement: u8) -> u8 {
    replace(cell, replacement)
}

fn probe(seed: usize) -> u8 {
    let original = Mosaic { checkpoint: u8(37), signal: u8(146) };
    let bystander = Mosaic { signal: u8(89), checkpoint: u8(211) };
    let returned_old = loop (phase = seed) {
        let seen_target = original@checkpoint;
        let seen_sibling = original@signal;
        let seen_other_target = bystander@checkpoint;
        let seen_other_sibling = bystander@signal;
        if (phase == usize(4)) {
            continue(usize(11), usize(19));
        } else {
            if (phase == usize(11)) {
                continue(usize(19));
            } else {
                let prior = loan_write(original@checkpoint) { |w|
                    exchange(w, u8(58))
                };
                break prior;
            }
        }
    };
    let after_target = original@checkpoint;
    let stable_sibling = original@signal;
    let other_target = bystander@checkpoint;
    let other_sibling = bystander@signal;
    let caller_old = loan_write(original@signal) { |w|
        exchange(w, returned_old)
    };
    let transferred_old = loan_write(bystander@checkpoint) { |w|
        exchange(w, after_target)
    };
    let final_target = original@checkpoint;
    let final_sibling = original@signal;
    let final_other = bystander@checkpoint;
    let preserved_other = bystander@signal;
    final_target
}

fn main() -> unit {
    let delayed = probe(usize(4));
    let immediate = probe(usize(19));
    unit
}
