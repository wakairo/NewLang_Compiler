fn select_octet(input: usize) -> u8 {
    u8(input)
}

fn main() -> unit {
    let original = usize(255);
    let selected = select_octet(original);
    let observed = selected;
    return unit;
}
