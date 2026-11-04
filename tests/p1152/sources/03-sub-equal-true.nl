struct ValueCell { neighbor: u8, target: u8 }
fn main() -> unit {
    let original = ValueCell{neighbor: u8(148), target: u8(76)};
    let result = loop (cursor = usize(23)) {
        if (cursor < usize(8)) {
            break cursor;
        } else {
            continue(cursor - usize(6));
        }
    };
    let copied = result;
    let decision = copied == usize(5);
    let old = if (decision) {
        loan_write(original@target) { |yes| replace(yes, u8(184)) }
    } else {
        loan_write(original@target) { |no| replace(no, u8(39)) }
    };
    let updated = original@target;
    let sibling = original@neighbor;
    let old_copy = old;
    return unit;
}
