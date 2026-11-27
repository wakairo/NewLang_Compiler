fn window_count(octet: u8) -> usize {
    let count = usize(octet);
    count
}

fn main() -> unit {
    let available = window_count(u8(128));
    let exact = available == usize(128);
    return unit;
}
