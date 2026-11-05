struct Kestrel { wing: u8, beak: u8 }

fn main() -> u8 {
    let original = Kestrel { wing: u8(12), beak: u8(93) };
    let finished = loop (phase = usize(1), memory = u8(204)) {
        if (phase < usize(8)) {
            let prewrite = original@wing;
            let sibling_before = original@beak;
            let selected = if (phase < usize(4)) {
                u8(34)
            } else {
                u8(78)
            };
            let copied = selected;
            let old = loan_write(original@wing) { |writer|
                replace(writer, copied)
            };
            let old_copy = old;
            let current = original@wing;
            let sibling = original@beak;
            continue(phase + usize(2), old_copy);
        } else {
            break memory;
        }
    };
    let current_final = original@wing;
    let sibling_final = original@beak;
    finished
}
