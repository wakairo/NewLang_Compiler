fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(31)) {
        if (cursor < usize(31)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
