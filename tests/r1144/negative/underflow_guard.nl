fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(1)) {
        if (cursor > usize(0)) {
            continue(cursor - usize(2));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
