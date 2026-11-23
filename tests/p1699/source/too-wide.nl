fn stripe_bytes(count: usize) -> usize {
    if (count < usize(3)) {
        count * usize(18446744073709551615)
    } else {
        usize(0)
    }
}

fn main() -> unit {
    let key = usize(1);
    let input = if (key < usize(2)) { usize(2) } else { usize(3) };
    let bytes = stripe_bytes(input);
    let observed = bytes;
    observed;
    unit
}
