struct MarkPair { heat: u8, spare: u8 }
fn choose_MarkPair(quantity: usize) -> u8 {
    if (quantity < usize(14)) { u8(119) } else { u8(47) }
}
fn main() -> unit {
    let original = MarkPair{heat: u8(72), spare: u8(203)};
    let amount = loop (cursor = usize(1)) {
        if (cursor < usize(10)) {
            continue(cursor + usize(18446744073709551615));
        } else {
            break cursor;
        }
    };
    let argument = amount;
    let incoming = choose_MarkPair(argument);
    let old = loan_write(original@heat) { |writer| replace(writer, incoming) };
    let updated = original@heat;
    let sibling = original@spare;
    let old_again = old;
    return unit;
}
