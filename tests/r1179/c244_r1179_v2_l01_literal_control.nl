struct Orbit { axis: u8, mark: u8 }

fn main() -> unit {
    let original = Orbit { axis: u8(12), mark: u8(93) };
    let finished = loop (phase = usize(1)) {
        if (phase < usize(8)) {
            let prewrite = original@axis;
            let sibling_before = original@mark;
            let old = loan_write(original@axis) { |writer|
                replace(writer, u8(78))
            };
            let old_copy = old;
            let current = original@axis;
            let sibling = original@mark;
            continue(phase + usize(2));
        } else {
            break phase;
        }
    };
    let current_final = original@axis;
    let sibling_final = original@mark;
    unit
}
