fn choose_MarkPair(quantity: usize) -> u8 {
    if (quantity < usize(14)) { u8(119) } else { u8(47) }
}
fn main() -> unit {
    let amount = loop (cursor = usize(1)) {
        if (cursor < usize(10)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let argument = amount;
    let incoming = choose_MarkPair(argument);
    return unit;
}
