struct Cairn { ridge: u8, moss: u8 }

fn main() -> u8 {
    let original = Cairn { ridge: u8(83), moss: u8(19) };
    let finished = loop (phase = usize(2), memory = u8(206)) {
        if (phase < usize(3)) {
            let prewrite = original@moss;
            let sibling_before = original@ridge;
            let selected = if (phase < usize(3)) {
                u8(47)
            } else {
                u8(79)
            };
            let copied = selected;
            let old = loan_write(original@moss) { |writer|
                replace(writer, copied)
            };
            let old_copy = old;
            let current = original@moss;
            let sibling = original@ridge;
            continue(phase + usize(1), old_copy);
        } else {
            break memory;
        }
    };
    let current_final = original@moss;
    let sibling_final = original@ridge;
    finished
}
