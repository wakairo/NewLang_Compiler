struct FlagPair { keep: u8, flag: u8 }
fn choose_FlagPair(quantity: usize) -> u8 {
    if (quantity < usize(10)) { u8(29) } else { u8(231) }
}
fn main() -> unit {
    let original = FlagPair{keep: u8(57), flag: u8(83)};
    let amount = loop (cursor = usize(10)) {
        if (cursor < usize(10)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let argument = amount;
    let incoming = choose_FlagPair(argument);
    let old = loan_write(original@flag) { |writer| replace(writer, incoming) };
    let updated = original@flag;
    let sibling = original@keep;
    let old_again = old;
    return unit;
}
