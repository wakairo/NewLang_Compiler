fn main() -> unit {
    let answer = loop (tick = usize(6)) {
        if (tick < usize(7)) {
            continue(usize(7));
        } else {
            break u8(199);
        }
    };
    return unit;
}
