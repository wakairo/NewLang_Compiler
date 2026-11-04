struct FlagCell { steady: u8, choice: u8 }
fn main() -> unit {
    let original = FlagCell{steady: u8(41), choice: u8(91)};
    let result = loop (cursor = usize(12)) {
        if (cursor < usize(12)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let copied = result;
    let decision = copied < usize(12);
    let old = if (decision) {
        loan_write(original@choice) { |yes| replace(yes, u8(58)) }
    } else {
        loan_write(original@choice) { |no| replace(no, u8(209)) }
    };
    let updated = original@choice;
    let sibling = original@steady;
    let old_copy = old;
    return unit;
}
