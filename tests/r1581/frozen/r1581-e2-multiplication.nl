fn portion(input: usize) -> usize {
    let quotient = input / usize(3);
    let residue = quotient % usize(5);
    residue
}

fn main() -> unit {
    let amount = portion(usize(83));
    let product = amount * usize(7);
    let original = product - usize(7);
    unit
}
