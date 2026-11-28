fn window_count(octet: u8) -> usize {
    let count = usize(octet);
    count
}

fn main() -> unit {
    let octet = u8(128);
    let available = window_count(octet);
    let exact = available == usize(128);
    let different = usize(127) == available;
    let again = exact;
    return unit;
}
