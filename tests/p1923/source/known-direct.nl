fn slots_count(octet: u8) -> usize {
    let widened = usize(octet);
    widened
}

fn main() -> unit {
    let octet = u8(172);
    let count = slots_count(octet);
    let chosen = if (count == usize(172)) { u8(31) } else { u8(71) };
    let saved = chosen;
    return unit;
}
