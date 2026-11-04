struct TonePair { tone: u8, stable: u8 }
fn choose_TonePair(quantity: usize) -> u8 {
    if (quantity < usize(12)) { u8(36) } else { u8(214) }
}
fn main() -> unit {
    let original = TonePair{tone: u8(17), stable: u8(65)};
    let amount = loop (cursor = usize(8)) {
        if (cursor < usize(10)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let argument = amount;
    let incoming = choose_TonePair(argument);
    let old = loan_write(original@tone) { |writer| replace(writer, incoming) };
    let updated = original@tone;
    let sibling = original@stable;
    let old_again = old;
    return unit;
}
