struct Quartz { flash: u8, dot: u8, }

fn main() -> unit {
    let vessel = Quartz { dot: u8(173), flash: u8(52) };
    let entry = vessel@flash;
    let entry_copy = entry;
    let break_value = loop (phase = usize(2)) {
        if (phase < usize(8)) {
            let before_selected = vessel@flash;
            let bridge_one = phase;
            let bridge_two = bridge_one;
            let bridge_three = bridge_two;
            let from_call = gleam(bridge_three);
            let replacement = from_call;
            let old = loan_write(vessel@flash) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@flash;
            let sibling = vessel@dot;
            let entry_still = entry_copy;
            continue(phase + usize(3));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@flash;
    let post_sibling = vessel@dot;
    let entry_after = entry_copy;
    unit
}

fn gleam(notch: usize) -> u8 {
    if (notch == usize(5)) {
        u8(124)
    } else {
        u8(76)
    }
}
