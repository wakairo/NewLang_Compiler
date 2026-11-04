struct SignalPair { steady: u8, signal: u8 }
fn choose_SignalPair(quantity: usize) -> u8 {
    if (quantity < usize(5)) { u8(41) } else { u8(218) }
}
fn main() -> unit {
    let original = SignalPair{steady: u8(91), signal: u8(156)};
    let amount = loop (cursor = usize(22)) {
        if (cursor < usize(7)) {
            break cursor;
        } else {
            continue(cursor - usize(6));
        }
    };
    let argument = amount;
    let incoming = choose_SignalPair(argument);
    let old = loan_write(original@signal) { |writer| replace(writer, incoming) };
    let updated = original@signal;
    let sibling = original@steady;
    let old_again = old;
    return unit;
}
