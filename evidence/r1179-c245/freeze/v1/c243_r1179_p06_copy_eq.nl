struct Nimbus { crest: u8, shade: u8 }

fn main() -> u8 {
    let original = Nimbus { crest: u8(157), shade: u8(39) };
    let finished = loop (phase = usize(2), memory = u8(201)) {
        if (phase < usize(9)) {
            let prewrite = original@shade;
            let sibling_before = original@crest;
            let selected = if (phase == usize(4)) {
                u8(149)
            } else {
                u8(27)
            };
            let copied = selected;
            let copied_again = copied;
            let old = loan_write(original@shade) { |writer|
                replace(writer, copied_again)
            };
            let old_copy = old;
            let current = original@shade;
            let sibling = original@crest;
            continue(phase + usize(2), old_copy);
        } else {
            break memory;
        }
    };
    let current_final = original@shade;
    let sibling_final = original@crest;
    finished
}
