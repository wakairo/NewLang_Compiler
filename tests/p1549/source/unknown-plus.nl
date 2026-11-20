fn padded_count(count: usize) -> usize {
    let quotient = count / usize(7);
    let saved = quotient;
    let padded = saved + usize(1);
    padded
}

fn main() -> unit {
    let proposed = padded_count(usize(20));
    unit
}
