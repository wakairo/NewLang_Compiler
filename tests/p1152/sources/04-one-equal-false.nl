struct ToneCell { tone: u8, trace: u8 }
fn main() -> unit {
    let original = ToneCell{tone: u8(52), trace: u8(164)};
    let result = loop (cursor = usize(7)) {
        if (cursor < usize(10)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let copied = result;
    let decision = copied == usize(10);
    let old = if (decision) {
        loan_write(original@tone) { |yes| replace(yes, u8(97)) }
    } else {
        loan_write(original@tone) { |no| replace(no, u8(218)) }
    };
    let updated = original@tone;
    let sibling = original@trace;
    let old_copy = old;
    return unit;
}
