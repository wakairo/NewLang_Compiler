fn select_tag(amount: usize) -> u8 {
    if (amount < usize(3)) { u8(53) } else { u8(181) }
}
fn main() -> unit {
    let total = loop (cursor = usize(17)) {
        if (cursor < usize(6)) {
            break cursor;
        } else {
            continue(cursor - usize(5));
        }
    };
    let copied = total;
    let selected = select_tag(copied);
    let copied_tag = selected;
    return unit;
}
