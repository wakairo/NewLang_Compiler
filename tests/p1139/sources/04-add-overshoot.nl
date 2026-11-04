fn select_tag(amount: usize) -> u8 {
    if (amount < usize(11)) { u8(43) } else { u8(207) }
}
fn main() -> unit {
    let total = loop (cursor = usize(3)) {
        if (cursor < usize(10)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let copied = total;
    let selected = select_tag(copied);
    let copied_tag = selected;
    return unit;
}
