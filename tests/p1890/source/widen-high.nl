fn select_octet(input: usize) -> u8 {
    if (input < usize(128)) { u8(0) } else { u8(255) }
}

fn main() -> unit {
    let original = usize(128);
    let selected = select_octet(original);
    let observed = usize(selected);
    return unit;
}
