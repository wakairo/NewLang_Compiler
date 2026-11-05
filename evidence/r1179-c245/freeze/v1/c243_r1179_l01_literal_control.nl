struct Orbit { axis: u8, mark: u8 }

fn main() -> u8 {
    let original = Orbit { axis: u8(12), mark: u8(93) };
    let finished = loop (phase = usize(1), memory = u8(204)) {
        if (phase < usize(8)) {
            let prewrite = original@axis;
            let sibling_before = original@mark;
            let old = loan_write(original@axis) { |writer|
                replace(writer, u8(78))
            };
            let old_copy = old;
            let current = original@axis;
            let sibling = original@mark;
            continue(phase + usize(2), old_copy);
        } else {
            break memory;
        }
    };
    let current_final = original@axis;
    let sibling_final = original@mark;
    finished
}
