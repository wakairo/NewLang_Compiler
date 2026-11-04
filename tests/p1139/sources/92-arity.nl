fn select_tag(amount: usize) -> u8 {
    if (amount < usize(12)) { u8(43) } else { u8(207) }
}
fn main() -> unit {
    let total = loop (cursor = usize(2)) {
        if (cursor < usize(11)) {
            continue(cursor + usize(3), usize(9));
        } else {
            break cursor;
        }
    };
    let copied = total;
    let selected = select_tag(copied);
    let copied_tag = selected;
    return unit;
}
