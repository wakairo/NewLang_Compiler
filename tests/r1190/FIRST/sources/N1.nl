struct Spindle { hub: u8, tip: u8, }

fn meter(notch: usize) -> u8 {
    if (notch < usize(3)) {
        u8(71)
    } else {
        u8(108)
    }
}

fn main() -> unit {
    let vessel = Spindle { tip: u8(31), hub: u8(151) };
    let entry = vessel@tip;
    let entry_copy = entry;
    let break_value = loop (phase = usize(2)) {
        if (phase < usize(5)) {
            let before_selected = vessel@tip;
            let argument_one = u8(2);
            let argument_two = argument_one;
            let from_call = meter(argument_two);
            let replacement = from_call;
            let old = loan_write(vessel@tip) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@tip;
            let sibling = vessel@hub;
            let entry_still = entry_copy;
            continue(phase + usize(3));
        } else {
            break phase;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@tip;
    let post_sibling = vessel@hub;
    let entry_after = entry_copy;
    unit
}
