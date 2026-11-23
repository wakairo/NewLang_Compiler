fn stripe_bytes(count: usize) -> usize {
    if (count < usize(1024)) {
        count * usize(64)
    } else {
        usize(0)
    }
}

fn main() -> usize {
    let bytes = stripe_bytes(usize(8));
    let observed = bytes;
    observed
}
