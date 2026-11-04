fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(u8(17));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
