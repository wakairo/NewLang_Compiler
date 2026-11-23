fn stripe_bytes(count: usize) -> usize {
    if (count < usize(18446744073709551615)) {
        count * usize(0)
    } else {
        usize(0)
    }
}

fn main() -> unit {
    let key = usize(1);
    let input = if (key < usize(2)) { usize(1023) } else { usize(1024) };
    let bytes = stripe_bytes(input);
    let observed = bytes;
    observed;
    unit
}
