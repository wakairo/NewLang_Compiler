struct Lotus { seed: u8, petal: u8 }

fn main() -> u8 {
    let original = Lotus { petal: u8(22), seed: u8(109) };
    let finished = loop (phase = usize(0), memory = u8(203)) {
        if (phase < usize(7)) {
            let prewrite = original@petal;
            let sibling_before = original@seed;
            let selected = if (phase < usize(5)) {
                u8(52)
            } else {
                u8(86)
            };
            let copied = selected;
            let old = loan_write(original@petal) { |writer|
                replace(writer, copied)
            };
            let old_copy = old;
            let current = original@petal;
            let sibling = original@seed;
            continue(phase + usize(3), old_copy);
        } else {
            break memory;
        }
    };
    let current_final = original@petal;
    let sibling_final = original@seed;
    finished
}
