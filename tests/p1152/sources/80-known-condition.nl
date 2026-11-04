struct CompassCell { mark: u8, twin: u8 }
fn main() -> unit {
    let original = CompassCell{mark: u8(66), twin: u8(192)};
    let known = usize(3);
    let decision = known < usize(9);
    let old = if (decision) {
        loan_write(original@mark) { |yes| replace(yes, u8(137)) }
    } else {
        loan_write(original@mark) { |no| replace(no, u8(23)) }
    };
    let updated = original@mark;
    let sibling = original@twin;
    let old_copy = old;
    return unit;
}
