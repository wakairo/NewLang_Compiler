struct CompassCell { mark: u8, twin: u8 }
fn main() -> unit {
    let original = CompassCell{mark: u8(66), twin: u8(192)};
    let result = loop (cursor = usize(2)) {
        if (cursor < usize(12)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let copied = result;
    let old = loan_write(original@mark) { |writer| replace(writer, u8(137)) };
    let updated = original@mark;
    let sibling = original@twin;
    let old_copy = old;
    return unit;
}
