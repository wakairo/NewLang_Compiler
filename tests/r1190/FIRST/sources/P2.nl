struct Beacon { crest: u8, knot: u8, }

fn main() -> unit {
    let vessel = Beacon { knot: u8(193), crest: u8(41) };
    let entry = vessel@crest;
    let entry_copy = entry;
    let orbit = usize(9);
    let first_seed = orbit;
    let second_seed = first_seed;
    let break_value = loop (orbit = second_seed) {
        if (orbit < usize(12)) {
            let before_selected = vessel@crest;
            let from_call = stamp(orbit);
            let replacement = from_call;
            let old = loan_write(vessel@crest) { |port_ref|
                replace(port_ref, replacement)
            };
            let old_copy = old;
            let current = vessel@crest;
            let sibling = vessel@knot;
            let entry_still = entry_copy;
            continue(orbit + usize(3));
        } else {
            break orbit;
        }
    };
    let break_copy = break_value;
    let post_selected = vessel@crest;
    let post_sibling = vessel@knot;
    let entry_after = entry_copy;
    let seed_after = orbit;
    unit
}

fn stamp(stage: usize) -> u8 {
    if (stage == usize(9)) {
        u8(73)
    } else {
        u8(106)
    }
}
