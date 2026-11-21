fn step(input: usize) -> usize {
    let quotient = input / usize(5);
    let residue = quotient % usize(7);
    residue
}

fn main() -> unit {
    let exact = step(usize(61));
    let bad = usize(4) - exact;
    unit
}
