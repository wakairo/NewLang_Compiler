struct BridgeNote { part: u8, tail: u8 }

fn idle_mark() -> u8 {
    return u8(18);
}

fn main() -> u8 {
    let one = BridgeNote { tail: u8(24), part: u8(67) };
    let two = BridgeNote { part: u8(32), tail: u8(91) };
    if (usize(5) == usize(6)) {
        u8(1)
    } else {
        u8(2)
    };
    let observed = one@part;
    let separate = two@tail;
    return observed;
}
