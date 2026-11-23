fn stripe_bytes() -> usize {
    let count = usize(8);
    count * usize(64)
}

fn main() -> unit {
    let bytes = stripe_bytes();
    bytes;
    unit
}
