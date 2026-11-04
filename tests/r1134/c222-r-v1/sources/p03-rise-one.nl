struct Pebble {
    clay: u8,
    glow: u8,
}

fn refresh_chip(target: ref<write,u8>, incoming: u8) -> u8 {
    replace(target, incoming)
}

fn main() -> u8 {
    let original = Pebble { clay: u8(59), glow: u8(184) };
    let other = Pebble { clay: u8(72), glow: u8(216) };
    let old_copy = original@clay;
    let sibling_copy = original@glow;
    let other_clay_copy = other@clay;
    let other_glow_copy = other@glow;
    let old_through_loop = loop (quantity = usize(6)) {
        if (quantity <= usize(6)) {
            let next_quantity = quantity + usize(2);
            continue(next_quantity);
        } else {
            let old_at_change = loan_write(original@clay) { |slot|
                refresh_chip(slot, u8(133))
            };
            let final_quantity = quantity;
            let current_at_break = original@clay;
            let sibling_at_break = original@glow;
            let other_clay_at_break = other@clay;
            let other_glow_at_break = other@glow;
            break old_at_change;
        }
    };
    let old_copy_after = old_copy;
    let current_after_loop = original@clay;
    let sibling_after_loop = original@glow;
    let other_clay_after_loop = other@clay;
    let other_glow_after_loop = other@glow;
    let caller_old = loan_write(original@clay) { |slot|
        refresh_chip(slot, u8(209))
    };
    let current_after_caller = original@clay;
    let sibling_after_caller = original@glow;
    let other_clay_after_caller = other@clay;
    let other_glow_after_caller = other@glow;
    let old_copy_final = old_copy;
    old_through_loop
}
