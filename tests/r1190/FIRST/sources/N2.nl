struct Petal { shade: u8, stem: u8, }

fn hue(receipt: usize) -> u8 {
    let condition_one = receipt;
    let condition_two = condition_one;
    if (condition_two) {
        u8(69)
    } else {
        u8(114)
    }
}

fn main() -> unit {
    let vessel = Petal { stem: u8(159), shade: u8(43) };
    let entry = vessel@shade;
    let entry_copy = entry;
    let break_value = loop (phase = usize(2)) {
        if (phase < usize(6)) {
            let before_selected = vessel@shade;
            let from_call = hue(phase);
            let replacement = from_call;
            let old = loan_write(vessel@shade) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@shade;
            let sibling = vessel@stem;
            let entry_still = entry_copy;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@shade;
    let post_sibling = vessel@stem;
    let entry_after = entry_copy;
    unit
}
