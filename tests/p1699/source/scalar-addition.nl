fn stripe_bytes(count: usize) -> usize {
    if (count < usize(1024)) {
        let advanced = count + usize(64);
        advanced
    } else {
        count
    }
}

fn main() -> usize {
    let bytes = stripe_bytes(usize(8));
    let observed = bytes;
    observed
}
