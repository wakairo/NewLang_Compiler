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
    let decision = copied < usize(15);
    let old = if (decision) {
        loan_write(original@mark) { |yes| yes }
    } else {
        loan_write(original@mark) { |no| no }
    };
    let updated = original@mark;
    let sibling = original@twin;
    let old_copy = old;
    return unit;
}
